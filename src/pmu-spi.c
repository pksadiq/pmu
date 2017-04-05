/* pmu-spi.c
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

#include "c37/c37.h"
#include "pmu-app.h"
#include "pmu-window.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/spi/spidev.h>

#include "pmu-spi.h"

#define DATA_SIZE 6 /* Bytes */

struct _PmuSpi
{
  GObject parent_instance;

  GMainContext   *context;

  int spi_fd;
  uint8_t mode;
  uint8_t bits_per_word;
  uint32_t speed;

  guint update_time; /* in milliseconds */
};

GThread *spi_thread  = NULL;
PmuSpi  *default_spi = NULL;
guchar    buffer[2];

uint8_t tx[DATA_SIZE + 1];
uint8_t rx[DATA_SIZE + 1];

G_DEFINE_TYPE (PmuSpi, pmu_spi, G_TYPE_OBJECT)

enum {
  SPI_DATA_BEGIN,    /* After 0xFFFF test data has been received from FPGA */
  START_SPI,
  STOP_SPI,
  SPI_DATA_RECEIVED,
  N_SIGNALS,
};

static guint signals[N_SIGNALS] = { 0, };

GQueue *spi_data;

G_LOCK_DEFINE (spi_data);

static void
pmu_spi_finalize (GObject *object)
{
  G_OBJECT_CLASS (pmu_spi_parent_class)->finalize (object);
}

static void
pmu_spi_class_init (PmuSpiClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = pmu_spi_finalize;

  signals [START_SPI] =
    g_signal_new ("start-spi",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE,
                  0);

  signals [START_SPI] =
    g_signal_new ("stop-spi",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE,
                  0);

  spi_data = g_queue_new ();
}

static void
pmu_spi_init (PmuSpi *self)
{
}

PmuSpi *
pmu_spi_get_default (void)
{
  return default_spi;
}

GMainContext *
pmu_spi_get_default_context (void)
{
  if (default_spi)
    return default_spi->context;

  return NULL;
}

guint
pmu_spi_default_get_update_time (void)
{
  if (default_spi)
    return default_spi->update_time;

  return 0;
}

gboolean
pmu_spi_default_set_update_time (guint update_time)
{
  if (default_spi == NULL)
    return FALSE;

  default_spi->update_time = update_time;
  return TRUE;
}

static void
pmu_spi_run (void)
{
  int ret;

  while (1)
    {
      memset (tx, 0xFD, 3);  /* 3 byte Debug test data */

      struct spi_ioc_transfer tr =
        {
         .tx_buf = (unsigned long)tx,
         .rx_buf = (unsigned long)rx,
         .len = 3,
         .delay_usecs = 0,
         .speed_hz = default_spi->speed,
         .bits_per_word = default_spi->bits_per_word,
        };


      ret = ioctl(default_spi->spi_fd, SPI_IOC_MESSAGE(1), &tr);

      g_usleep (10);

      if (rx[1] == 0xFF && rx[2] == 0xFF)
        {
          memset (tx, 0xFE, DATA_SIZE + 1);
          memset (rx, 0x00, 3);         /* Clear debug data */

          struct spi_ioc_transfer tr =
            {
             .tx_buf = (unsigned long)tx,
             .rx_buf = (unsigned long)rx,
             .len = DATA_SIZE + 1,
             .delay_usecs = 0,
             .speed_hz = default_spi->speed,
             .bits_per_word = default_spi->bits_per_word,
            };

          ret = ioctl(default_spi->spi_fd, SPI_IOC_MESSAGE(1), &tr);

          g_usleep (4000);
        }
      g_usleep (100);

      GBytes *data = g_bytes_new (rx + 1, DATA_SIZE);
      G_LOCK (spi_data);
      g_queue_push_tail (spi_data, data);
      G_UNLOCK (spi_data);
      g_print ("queue size: %d\n", g_queue_get_length (spi_data));


    } // while loop
}

static void
start_spi_cb (PmuSpi   *self,
              gpointer  user_data)
{
}

static void
stop_spi_cb (PmuSpi   *self,
             gpointer  user_data)
{
  g_autoptr(GError) error = NULL;

  if (spi_data)
    g_queue_free_full (spi_data, (GDestroyNotify)g_bytes_unref);

  default_spi->update_time = 1000;
}

static gboolean
pmu_spi_setup_device (PmuWindow *window)
{
  int spi_fd;
  int ret;

  spi_fd = open ("/dev/spidev0.0", O_RDWR);

  if (spi_fd == -1)
    {
      g_warning ("Opening SPI device failed\n");
      g_idle_add((GSourceFunc) pmu_window_spi_failed_cb, window);
      return FALSE;
    }

  ret = ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &(default_spi->bits_per_word));
  if (ret == -1)
    {
      g_warning ("Setting %d bits per word for write failed\n",  default_spi->bits_per_word);
      g_idle_add((GSourceFunc) pmu_window_spi_failed_cb, window);
      return FALSE;
    }


  ret = ioctl(spi_fd, SPI_IOC_RD_BITS_PER_WORD, &(default_spi->bits_per_word));
  if (ret == -1)
    {
      g_warning ("Setting %d bits per word for read failed\n",  default_spi->bits_per_word);
      g_idle_add((GSourceFunc) pmu_window_spi_failed_cb, window);
      return FALSE;
    }

  ret = ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &(default_spi->speed));
  if (ret == -1)
    {
      g_warning ("Setting max write speed (%d Hz) failed\n",  default_spi->speed);
      g_idle_add((GSourceFunc) pmu_window_spi_failed_cb, window);
      return FALSE;
    }


  ret = ioctl(spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &(default_spi->speed));
  if (ret == -1)
    {
      g_warning ("Setting max read speed (%d Hz) failed\n",  default_spi->speed);
      g_idle_add((GSourceFunc) pmu_window_spi_failed_cb, window);
      return FALSE;
    }

  ret = ioctl(spi_fd, SPI_IOC_RD_MODE, &(default_spi->mode));
  if (ret == -1)
    {
      g_warning ("Setting max read mode %u failed\n", default_spi->mode);
      g_idle_add((GSourceFunc) pmu_window_spi_failed_cb, window);
      return FALSE;
    }

  ret = ioctl(spi_fd, SPI_IOC_WR_MODE, &(default_spi->mode));
  if (ret == -1)
    {
      g_warning ("Setting max write mode %u failed\n", default_spi->mode);
      g_idle_add((GSourceFunc) pmu_window_spi_failed_cb, window);
      return FALSE;
    }

  default_spi->spi_fd = spi_fd;

  return TRUE;
}

static void
pmu_spi_new (PmuWindow *window)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(GMainContext) spi_context = NULL;
  g_autoptr(GMainLoop) spi_loop = NULL;
  gboolean status;

  spi_context = g_main_context_new ();
  spi_loop = g_main_loop_new(spi_context, FALSE);

  g_main_context_push_thread_default (spi_context);

  default_spi = g_object_new (PMU_TYPE_SPI, NULL);
  default_spi->context = spi_context;
  default_spi->update_time = 5;

  default_spi->bits_per_word = 8;
  default_spi->speed = 100 * 1000; /* Speed in Hz */

  status = pmu_spi_setup_device (window);
  if (!status)
    goto out;

  pmu_spi_run ();

  g_signal_connect (default_spi, "start-spi",
                    G_CALLBACK (start_spi_cb), window);

  g_signal_connect (default_spi, "stop-spi",
                    G_CALLBACK (stop_spi_cb), window);

  g_main_loop_run(spi_loop);

 out:
  g_object_unref (default_spi);
  default_spi = NULL;
  g_main_context_pop_thread_default (spi_context);
  g_main_loop_quit (spi_loop);
  spi_thread = NULL;
}

void
pmu_spi_start_thread (PmuWindow *window)
{
  g_autoptr(GError) error = NULL;

  if (spi_thread == NULL)
    {
      spi_thread = g_thread_try_new ("spi",
                                     (GThreadFunc)pmu_spi_new,
                                     window,
                                     &error);
      if (error != NULL)
        g_warning ("Cannot create spi thread. Error: %s", error->message);
    }
}

gboolean
pmu_spi_start (gpointer user_data)
{
  if (spi_thread == NULL)
    pmu_spi_start_thread (NULL);

  if (default_spi)
    g_signal_emit_by_name (default_spi, "start-spi");

  return G_SOURCE_REMOVE;
}

gboolean
pmu_spi_stop (gpointer user_data)
{
  if (default_spi)
    g_signal_emit_by_name (default_spi, "stop-spi");

  return G_SOURCE_REMOVE;
}
