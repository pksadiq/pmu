/* pmu-server.c
 *
 * Copyright (C) 2017 Mohammed Sadiq <sadiq@sadiqpk.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "c37/c37-common.h"
#include "pmu-app.h"

#include "pmu-server.h"

struct _PmuServer
{
  GObject parent_instance;

  GSocketService *service;

  char *admin_ip;
  int port;
};

typedef struct TcpRequest {
  GSocketConnection *socket_connection;
  GBytes *header;
  gsize data_length;
} TcpRequest;

GThread *server_thread = NULL;
PmuServer *default_server = NULL;

G_DEFINE_TYPE (PmuServer, pmu_server, G_TYPE_OBJECT)

enum {
  SERVER_STARTED,
  SERVER_STOPPED,
  DATA_START_REQUEST,
  DATA_STOP_REQUEST,
  N_SIGNALS,
};

static guint signals[N_SIGNALS] = { 0, };

static void
pmu_server_finalize (GObject *object)
{
  PmuServer *self = PMU_SERVER (object);

  g_free (self->admin_ip);

  G_OBJECT_CLASS (pmu_server_parent_class)->finalize (object);
}

static void
pmu_server_class_init (PmuServerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = pmu_server_finalize;

  signals [SERVER_STARTED] =
    g_signal_new ("server-started",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE,
                  0);
}

static void
pmu_server_init (PmuServer *self)
{
  self->service = g_socket_service_new ();
}

static void
tcp_request_free (TcpRequest *request)
{
  g_object_unref (request->socket_connection);
  g_bytes_unref (request->header);
  g_free (request);
}

static void
complete_data_read (GInputStream *stream,
                    GAsyncResult *result,
                    TcpRequest   *request)
{
  g_autoptr(GBytes) bytes = NULL;
  g_autoptr(GError) error = NULL;
  GInputStream *in;
  const guint8 *data;
  const guint8 *header_data;
  gsize real_size;
  gsize size;

  /* To read SYNC and FRAME size bytes from header */
  size = REQUEST_HEADER_SIZE;
  header_data = g_bytes_get_data (request->header, &size);
  size = request->data_length;
  real_size = size;

  bytes = g_input_stream_read_bytes_finish (stream, result, &error);

  if (error != NULL)
    {
      g_warning ("%s", error->message);
      goto out;
    }
  data = g_bytes_get_data (bytes, &size);

  for (int i = 0; i < 4; i++)
    {
      g_print ("%02X ", header_data[i]);
    }

  for (int i = 0; i < real_size - REQUEST_HEADER_SIZE; i++)
    {
      g_print ("%02X ", data[i]);
    }
  g_print ("\n");

  g_print ("Size:%d CRC: %X\n", g_bytes_get_size (bytes), cts_common_calc_crc (data, real_size - 2, header_data));

  if (g_bytes_get_size (bytes) != (real_size - REQUEST_HEADER_SIZE) ||
      !cts_common_check_crc (data, real_size - 2, header_data, real_size - REQUEST_HEADER_SIZE - 2))
    {
      g_print ("CRC check failed\n");
      goto out;
    }
  in = g_io_stream_get_input_stream (G_IO_STREAM (request->socket_connection));

  bytes = g_input_stream_read_bytes(in, REQUEST_HEADER_SIZE, NULL, &error);

  if (error != NULL)
    {
      g_warning ("%s", error->message);
      goto out;
    }

  /* If 4 bytes of data not present, this request isn't interesting
   * for us.
   */
  if (g_bytes_get_size (bytes) < REQUEST_HEADER_SIZE)
    {
      g_print ("Size less than 4 bytes\n");
      goto out;
    }
  data = g_bytes_get_data (bytes, &size);
  if (cts_common_get_type (data) != CTS_TYPE_COMMAND)
    {
      g_print ("Not a command\n");
      goto out;
    }
  /* Jump the 2 SYNC bytes */
  size = cts_common_get_size (data, 2);

  if (size <= REQUEST_HEADER_SIZE)
    {
      g_print ("size is <= 4 bytes\n");
      goto out;
    }

  request->data_length = size;

  g_input_stream_read_bytes_async (in, size - REQUEST_HEADER_SIZE, G_PRIORITY_DEFAULT, NULL,
                                   (GAsyncReadyCallback)complete_data_read,
                                   request);
  return;

 out:
  g_print ("Never here\n");
  tcp_request_free (request);
}

static gboolean
data_incoming_cb (GSocketService    *service,
                  GSocketConnection *connection,
                  GObject           *source_object)
{
  GBytes *bytes;
  g_autoptr(GError) error = NULL;
  guint16 data_length;
  GInputStream *in;
  const guint8 *data;
  TcpRequest *request;
  gsize size = REQUEST_HEADER_SIZE;

  in = g_io_stream_get_input_stream (G_IO_STREAM (connection));
  bytes = g_input_stream_read_bytes(in, REQUEST_HEADER_SIZE, NULL, &error);

  if (error != NULL)
    {
      g_warning ("%s", error->message);
      return TRUE;
    }

  /* If 4 bytes of data not present, this request isn't interesting
   * for us.
   */
  if (g_bytes_get_size (bytes) < REQUEST_HEADER_SIZE)
    return TRUE;

  data = g_bytes_get_data (bytes, &size);
  if (cts_common_get_type (data) != CTS_TYPE_COMMAND)
    return TRUE;

  /* Jump the 2 SYNC bytes */
  data_length = cts_common_get_size (data, 2);

  if (data_length <= REQUEST_HEADER_SIZE)
    return TRUE;

  request = g_new0 (TcpRequest, 1);
  request->socket_connection = g_object_ref (connection);
  request->header = bytes;
  request->data_length = data_length;
  g_input_stream_read_bytes_async (in, data_length - REQUEST_HEADER_SIZE, G_PRIORITY_DEFAULT, NULL,
                                   (GAsyncReadyCallback)complete_data_read,
                                   request);
  /* g_bytes_unref (bytes); */

  /* if (cts_common_get_type (data) == CTS_TYPE_COMMAND) */

  return TRUE;
}

PmuServer *
pmu_server_get_default (void)
{
  return default_server;
}

static void
server_started_cb (PmuServer *self,
                   PmuWindow *window)
{
  g_print ("Server started\n");
}

static void
pmu_server_new (PmuWindow *window)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(GMainContext) server_context = NULL;
  g_autoptr(GMainLoop) server_loop = NULL;

  server_context = g_main_context_new ();
  server_loop = g_main_loop_new(server_context, FALSE);

  g_main_context_push_thread_default (server_context);

  default_server = g_object_new (PMU_TYPE_SERVER, NULL);
  default_server->port = 4000;

  if (!g_socket_listener_add_inet_port (G_SOCKET_LISTENER (default_server->service),
                                        default_server->port,
                                        G_OBJECT (default_server),
                                        &error))
    {
      g_print ("Here\n");
      g_prefix_error (&error, "Unable to listen to port %d: ", default_server->port);
      return;
    }
  g_signal_connect (default_server->service, "incoming",
                    G_CALLBACK (data_incoming_cb), NULL);

  g_signal_connect (default_server, "server-started",
                    G_CALLBACK (server_started_cb), window);

  g_signal_emit_by_name (default_server, "server-started");

  g_main_loop_run(server_loop);

  g_main_context_pop_thread_default (server_context);
}

void
pmu_server_start (PmuWindow *window)
{
  g_autoptr(GError) error = NULL;

  if (server_thread == NULL)
    {
      server_thread = g_thread_try_new ("server",
                                        (GThreadFunc)pmu_server_new,
                                        window,
                                        &error);
      if (error != NULL)
        g_warning ("Cannot create server thread. Error: %s", error->message);
    }
  else
    g_signal_emit_by_name (default_server, "server-started");
}
