//
// Created by Chengke Wong on 2019/9/16.
//

#ifndef h2o__rangeclient_h
#define h2o__rangeclient_h

#ifdef __cplusplus
extern "C" {
#endif

#include "h2o/httpclient.h"

typedef struct st_h2o_rangeclient_t h2o_rangeclient_t;

struct st_h2o_rangeclient_t {
  h2o_mem_pool_t *mempool;
  h2o_httpclient_ctx_t *ctx;
  h2o_httpclient_connection_pool_t *connpool;
  h2o_httpclient_t *httpclient;

  h2o_url_t *url_parsed;
  char *buf;
  FILE *file;
  h2o_header_t *range_header;

  /* bytes range: [begin, end) */
  struct {
    size_t begin;
    /* |end| can be zero, indicating that the file size is unknown */
    size_t end;
  } range;

  char is_closed;
//    h2o_timer_t exit_deferred;
};

h2o_rangeclient_t *h2o_rangeclient_create(h2o_httpclient_connection_pool_t *connpool,
                                          h2o_httpclient_ctx_t *ctx,
                                          h2o_url_t *url_parsed, char *save_to_file,
                                          size_t bytes_begin, size_t bytes_end);

#ifdef __cplusplus
}
#endif

#endif //h2o__rangeclient_h
