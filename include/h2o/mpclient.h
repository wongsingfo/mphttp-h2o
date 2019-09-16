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

  h2o_httpclient_ctx_t *ctx;
  h2o_httpclient_connection_pool_t *connpool;
};

h2o_mpclient_t* h2o_mpclient_create(char* request_url, h2o_httpclient_ctx_t *_ctx);
int h2o_mpclient_fetch(h2o_mpclient_t* mp, char *request_url, size_t sz_hint, char *save_to_file);

#ifdef __cplusplus
}
#endif

#endif //h2o__mpclient_h
