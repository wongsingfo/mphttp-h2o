//
// Created by Chengke Wong on 2019-09-15.
//

#include <stdio.h>
#include <h2o/mpclient.h>

#include "h2o.h"
#include "h2o/mpclient.h"

// TODO: put 3 into a constant
h2o_mpclient_t *interface[3];

// return a busy client
static h2o_mpclient_t *on_reschedule(h2o_mpclient_t *mp) {
  h2o_mpclient_t *rv = NULL;

  for (int i = 0; i < 3; i++) {
    if (interface[i] == mp) {
      printf("call on_reschedule(%d) at %zu ms\n",
             i,
             h2o_time_elapsed_nanosec(mp->ctx->loop) / 1000000);
      continue;
    }

    if (rv == NULL ||
        h2o_mpclient_get_remaining(interface[i]) >
        h2o_mpclient_get_remaining(rv)) {
      rv = interface[i];
    }
  }
  return rv;
}

static void on_get_size() {
  h2o_mpclient_reschedule(interface[1]);
  h2o_mpclient_reschedule(interface[2]);
}

static int is_download_complete() {
  for (int i = 0; i < 3; i++) {
    if (interface[i]->rangeclient.running ||
        interface[i]->rangeclient.pending) {
      return 0;
    }
  }
  return 1;
}

static void usage(const char *progname)
{
  fprintf(stderr,
          "%s Usage: [-o <file>] [-t <path>] <host0> <host1> <host2>\n",
          progname);
}

int main(int argc, char* argv[]) {
  int opt;
  char *save_to_file = NULL;
  char *path_of_url = NULL;
  int num_cdn = 0;
  char *cdn[3];

  while ((opt = getopt(argc, argv, "-o:t:")) != -1) {
    switch (opt) {
      case 1:
        cdn[num_cdn] = optarg;
        num_cdn += 1;
        break;
      case 't':
        path_of_url = optarg;
        break;
      case 'o':
        save_to_file = optarg;
        break;
      default:
        usage(argv[0]);
        exit(0);
        break;
    }
  }

  if (num_cdn != 3 || !path_of_url || !save_to_file) {
    usage(argv[0]);
    exit(0);
  }

  const uint64_t timeout = 10 * 1000; /* ms */
  h2o_multithread_receiver_t getaddr_receiver;
  h2o_httpclient_ctx_t ctx = {
    NULL, /* loop */
    &getaddr_receiver,
    timeout,                                 /* io_timeout */
    timeout,                                 /* connect_timeout */
    timeout,                                 /* first_byte_timeout */
    NULL,                                    /* websocket_timeout */
    0,                                       /* keepalive_timeout */
    65535                                    /* max_buffer_size */
  };
  ctx.http2.ratio = 100;

  /* setup SSL */
  SSL_load_error_strings();
  SSL_library_init();
  OpenSSL_add_all_algorithms();

  /* setup context */
#if H2O_USE_LIBUV
  ctx.loop = uv_loop_new();
#else
  ctx.loop = h2o_evloop_create();
#endif
  h2o_multithread_queue_t *queue;
  queue = h2o_multithread_create_queue(ctx.loop);
  h2o_multithread_register_receiver(queue, ctx.getaddr_receiver, h2o_hostinfo_getaddr_receiver);

  for (int i = 0; i < 3; i++) {
    interface[i] =
      h2o_mpclient_create(cdn[i],
                          &ctx,
                          on_reschedule,
                          on_get_size);
  }
  h2o_mpclient_fetch(interface[0],
                     path_of_url,
                     save_to_file,
                     0,
                     0);

  int rv;
  while (! is_download_complete()) {
#if H2O_USE_LIBUV
    uv_run(ctx.loop, UV_RUN_ONCE);
#else
    if ((rv = h2o_evloop_run(ctx.loop, 1000)) < 0) {
      if (errno != EINTR) {
        return 0;
      }
    }
#endif
  }

  for (int i = 0; i < 3; i++) {
    h2o_mpclient_destroy(interface[i]);
  }

  return 0;
}