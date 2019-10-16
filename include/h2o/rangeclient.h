//
// Created by Chengke Wong on 2019/9/16.
//

#ifndef h2o__rangeclient_h
#define h2o__rangeclient_h

#ifdef __cplusplus
extern "C" {
#endif

#include "h2o/httpclient.h"

#define H2O_RANGECLIENT_NUM_BANDWIDTH_SAMPLES 5

typedef struct st_h2o_bandwidth_sampler_t h2o_bandwidth_sampler_t;
typedef struct st_h2o_rangeclient_t h2o_rangeclient_t;

struct st_h2o_bandwidth_sampler_t {
  size_t last_received;
  uint64_t last_ack_time;
//  int i_sample;
//  size_t samples[H2O_RANGECLIENT_NUM_BANDWIDTH_SAMPLES]; // bytes/s
  int skip_sample;
  size_t bw;
};

struct st_h2o_rangeclient_t {
  h2o_mem_pool_t *mempool;
  void *data;
  h2o_httpclient_ctx_t *ctx;
  h2o_httpclient_connection_pool_t *connpool;
  h2o_httpclient_t *httpclient;
  char *save_to_file;

  h2o_url_t *url_parsed;
  char *buf;
  int fd;
  FILE *logger;
  h2o_header_t *range_header;

  /* bytes range: [begin, end) */
  struct {
    size_t begin;
    /* |end| can be zero, indicating that the file size is unknown */
    size_t end;
    size_t received;
  } range;

  int enable_cancel;
  h2o_timer_t cancel_timer;

  h2o_bandwidth_sampler_t *bw_sampler;

  char is_closed;
//    h2o_timer_t exit_deferred;

  struct {
    void (*on_mostly_complete)(h2o_rangeclient_t*);
    void (*on_complete)(h2o_rangeclient_t*);
    void (*on_get_size)();
  }cb;
};

h2o_rangeclient_t *
h2o_rangeclient_create(h2o_httpclient_connection_pool_t *connpool, void *data, FILE *logger,
                       h2o_httpclient_ctx_t *ctx, h2o_url_t *url_parsed, char *save_to_file,
                       size_t bytes_begin, size_t bytes_end);
void h2o_rangeclient_destroy(h2o_rangeclient_t *client);

uint32_t h2o_rangeclient_get_remaining_time(h2o_rangeclient_t *client); // in ms
size_t h2o_rangeclient_get_bw(h2o_rangeclient_t *client); // bytes / s
uint32_t h2o_rangeclient_get_ping_rtt(h2o_rangeclient_t *client); // in ms

void h2o_rangeclient_adjust_range_end(h2o_rangeclient_t *client, size_t end);

#ifdef __cplusplus
}
#endif

#endif //h2o__rangeclient_h
