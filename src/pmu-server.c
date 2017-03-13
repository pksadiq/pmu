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
#include "pmu-server.h"


struct _PmuServer
{
  GObject parent_instance;

  GSocketService *service;

  char *admin_ip;
  int port;
};

GThread *server_thread = NULL;
PmuServer *default_server = NULL;

G_DEFINE_TYPE (PmuServer, pmu_server, G_TYPE_OBJECT)

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
}

static void
pmu_server_init (PmuServer *self)
{
  self->service = g_socket_service_new ();
}

static void
complete_data_read (GInputStream *stream,
                    GAsyncResult *result,
                    GBytes       *header_bytes)
{
  g_autoptr(GBytes) bytes = NULL;
  g_autoptr(GError) error = NULL;
  const guint8 *data;
  gsize size;

  /* To read SYNC and FRAME size bytes from header */
  size = REQUEST_HEADER_SIZE;
  data = g_bytes_get_data (header_bytes, &size);
  size = pmu_common_get_size (data);
  g_print ("%u\n", size);
  bytes = g_input_stream_read_bytes_finish (stream, result, &error);

  if (error != NULL)
    {
      g_warning ("%s", error->message);
      return;
    }
  data = g_bytes_get_data (bytes, &size);
  g_print ("%u", g_bytes_get_size (bytes));
  g_print ("#%X#\n", data[0]);
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
  if (pmu_common_get_type (data) != CTS_TYPE_COMMAND)
    return TRUE;

  data_length = pmu_common_get_size (data);

  if (data_length <= REQUEST_HEADER_SIZE)
    return TRUE;

  g_input_stream_read_bytes_async (in, data_length - REQUEST_HEADER_SIZE, G_PRIORITY_DEFAULT, NULL,
                                   (GAsyncReadyCallback)complete_data_read,
                                   bytes);
  /* g_bytes_unref (bytes); */

  /* if (cts_common_get_type (data) == CTS_TYPE_COMMAND) */

  return TRUE;
}

static void
pmu_server_new (void)
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

  g_main_loop_run(server_loop);

  g_main_context_pop_thread_default (server_context);
}

void
pmu_server_start_default (void)
{
  g_autoptr(GError) error = NULL;

  if (server_thread == NULL)
    {
      server_thread = g_thread_try_new ("server",
                                        (GThreadFunc)pmu_server_new,
                                        NULL,
                                        &error);
      if (error != NULL)
        g_warning ("Cannot create server thread. Error: %s", error->message);
    }
}
