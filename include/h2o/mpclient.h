//
// Created by Chengke Wong on 2019/9/16.
//

#ifndef h2o__mpclient_h
#define h2o__mpclient_h

#ifdef __cplusplus
extern "C" {
#endif

#include "h2o/rangeclient.h"

typedef struct st_h2o_mpclient_t h2o_mpclient_t;
struct st_h2o_mpclient_t {
  struct {
    h2o_rangeclient_t *pending;
    h2o_rangeclient_t *running;
  } rangeclient;

  char *url_prefix;
  h2o_httpclient_ctx_t *ctx;
  h2o_httpclient_connection_pool_t *connpool;

  h2o_mpclient_t* (*on_reschedule)(h2o_mpclient_t*);

  FILE* data_log; // owned
};

// TODO: typedef callback
h2o_mpclient_t* h2o_mpclient_create(char *request_url, h2o_httpclient_ctx_t *_ctx,
                                    h2o_mpclient_t *(*on_reschedule)(h2o_mpclient_t *));
int h2o_mpclient_fetch(h2o_mpclient_t *mp, char *request_path, char *save_to_file, size_t begin, size_t end);
void h2o_mpclient_reschedule(h2o_mpclient_t *mp1, h2o_mpclient_t *mp2);

#ifdef __cplusplus
}
#endif

#endif //h2o__mpclient_h
