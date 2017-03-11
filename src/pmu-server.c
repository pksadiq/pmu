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
got_http_request (GInputStream *stream,
                   GAsyncResult *result,
                   gpointer     *request)
{
}

static gboolean
data_incoming_cb (GSocketService    *service,
                  GSocketConnection *connection,
                  GObject           *source_object)
{
  g_autoptr(GBytes) bytes = NULL;
  GInputStream *in;
  guint8 *data;
  gsize size = 2;

  in = g_io_stream_get_input_stream (G_IO_STREAM (connection));

  bytes = g_input_stream_read_bytes(in, size, NULL, NULL);
  /* data = g_bytes_get_data (bytes, &size); */
  /* g_free (bytes); */

  /* bytes = g_input_stream_read_bytes(in, size, NULL, NULL); */
  /* data = g_bytes_get_data (bytes, &size); */
  /* g_print ("%2X\n", *data); */
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
