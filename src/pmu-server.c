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
  /* line = g_data_input_stream_read_line_finish (G_DATA_INPUT_STREAM (stream), result, &length, NULL); */
  /* if (line == NULL) */
  /*   { */
  /*     /\* http_request_free (request); *\/ */
  /*     g_printerr ("Error reading request lines\n"); */
  /*     return; */
  /*   } */
  /* for (i = 0; i < length; i++) */
  /*   { */
  /*     g_print ("%x ", line[i]); */
  /*   } */

  /* g_usleep (1000 * 1000 * 10); */
  /* g_print ("Request: %s\n", line); */
  /* g_free (line); */
}

static gboolean
data_incoming_cb (GSocketService    *service,
                  GSocketConnection *connection,
                  GObject           *source_object)
{
  g_autoptr(GBytes) data = NULL;
  GInputStream *in;
  gsize size = 5;
  /* GDataInputStream *data_stream; */

  in = g_io_stream_get_input_stream (G_IO_STREAM (connection));

  /* data_stream = g_data_input_stream_new (in); */
  /* g_filter_input_stream_set_close_base_stream (G_FILTER_INPUT_STREAM (data), TRUE); */

  data = g_input_stream_read_bytes(in, size, NULL, NULL);
  guint8 *atom = g_bytes_get_data (data, &size);
  
  g_print ("%zu %2X\n", g_bytes_get_size (data), (int) atom[0], atom[1], atom[2]);
  /* g_data_input_stream_read_line_async (data, 0, NULL, */
	/* 			       (GAsyncReadyCallback)got_http_request_line, NULL); */
  return TRUE;
}

static void
pmu_server_new (int *port)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(GMainContext) server_context = NULL;
  g_autoptr(GMainLoop) server_loop = NULL;

  server_context = g_main_context_new ();
  server_loop = g_main_loop_new(server_context, FALSE);

  g_main_context_push_thread_default (server_context);

  default_server = g_object_new (PMU_TYPE_SERVER, NULL);
  default_server->port = *port;

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
pmu_server_start_default (int *port)
{
  if (server_thread == NULL)
    {
      g_print ("Port is %d\n", *port);
      server_thread = g_thread_new ("server",
                                    (GThreadFunc)pmu_server_new,
                                    port);
    }
}
