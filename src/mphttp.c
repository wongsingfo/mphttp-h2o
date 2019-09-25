//
// Created by Chengke Wong on 2019-09-15.
//

#include <stdio.h>
#include <h2o/mpclient.h>

#include "h2o.h"
#include "h2o/mpclient.h"

h2o_mpclient_t *if1;
h2o_mpclient_t *if2;

static h2o_mpclient_t *on_reschedule(h2o_mpclient_t* mp) {
  if (mp == if1) {
    printf("call on_reschedule(1) at %zums\n", h2o_time_elapsed_nanosec(mp->ctx->loop) / 1000000);
    return if2;
  }
  if (mp == if2) {
    printf("call on_reschedule(2) at %zums\n", h2o_time_elapsed_nanosec(mp->ctx->loop) / 1000000);
    return if1;
  }
  return NULL;
}

static int is_download_complete() {
  if (h2o_time_elapsed_nanosec(if1->ctx->loop) / 1000000 > 10000) {
  }
  if (if1->rangeclient.running || if1->rangeclient.pending) {
    return 0;
  }
  if (if2->rangeclient.running || if2->rangeclient.pending) {
    return 0;
  }
  return 1;
}

int main(int argc, char* argv[]) {
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

  if1 = h2o_mpclient_create("https://10.100.1.2/", &ctx, on_reschedule);
  if2 = h2o_mpclient_create("https://10.100.2.2/", &ctx, on_reschedule);
  h2o_mpclient_fetch(if1,
                     "2M",
                     "./2M.bin",
                     0,
                     1024*1024);
  h2o_mpclient_fetch(if2,
                     "2M",
                     "./2M.bin",
                     1024*1024,
                     2* 1024*1024);

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

  h2o_mpclient_destroy(if1);
  h2o_mpclient_destroy(if2);

  return 0;
}