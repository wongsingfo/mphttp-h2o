//
// Created by Chengke Wong on 2019/9/16.
//

#include "h2o/rangeclient.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "h2o/memory.h"

static h2o_iovec_t range_str = (h2o_iovec_t) {H2O_STRLIT("range")};

static int fd_can_write(int fd) {
  int val;
  if ((val = fcntl(fd, F_GETFL, 0)) < 0) return 0;
  switch (val & O_ACCMODE) {
    case O_RDONLY:
      return 0;
    case O_WRONLY:
    case O_RDWR:
      return 1;
    default:
      return 0;
  }
}

static int on_body(h2o_httpclient_t *httpclient, const char *errstr) {
  if (errstr && errstr != h2o_httpclient_error_is_eos) {
    h2o_error_printf("on_body failed");
    return -1;
  }
  h2o_rangeclient_t *client = httpclient->data;
  h2o_buffer_t *buf = *httpclient->buf;
  fwrite(buf->bytes, 1, buf->size, client->file);
  h2o_buffer_consume(&buf, buf->size);

  if (errstr == h2o_httpclient_error_is_eos) {
    printf("done!\n");
    fclose(client->file);
    client->is_closed = 1;
  }
  return 0;
}

static void print_status_line(int version, int status, h2o_iovec_t msg)
{
  printf("HTTP/%d", (version >> 8));
  if ((version & 0xff) != 0) {
    printf(".%d", version & 0xff);
  }
  printf(" %d", status);
  if (msg.len != 0) {
    printf(" %.*s\n", (int)msg.len, msg.base);
  } else {
    printf("\n");
  }
}

static int parse_content_range(size_t *filezs, h2o_header_t *headers, size_t num_headers) {
  int i;
  h2o_iovec_t content_range = (h2o_iovec_t) {H2O_STRLIT("content-range")};


  for (i = 0; i < num_headers; i++) {
    const char *name = headers[i].orig_name;
    if (name == NULL)
      name = headers[i].name->base;
    if (h2o_memis(name, headers[i].name->len, content_range.base, content_range.len)) {
      break;
    }
  }
  char buf[32];
  if (i == num_headers) {
    h2o_error_printf("response header does not contain content-range\n");
    return -1;
  }
  if (headers[i].value.len >= 32) {
    h2o_error_printf("header value is too long\n");
    return -1;
  }

  sprintf(buf, "%.*s", (int) headers[i].value.len, headers[i].value.base);

  if (sscanf(buf, "bytes %*zu-%*zu/%zu", filezs) < 1) {
    h2o_error_printf("can not read file size from headers\n");
    return -1;
  }
  return 0;
}

static h2o_httpclient_body_cb
on_head(h2o_httpclient_t *httpclient, const char *errstr, int version, int status, h2o_iovec_t msg,
        h2o_header_t *headers, size_t num_headers, int header_requires_dup) {
  if (errstr) {
    h2o_error_printf("on_head failed");
    return NULL;
  }
  if (status != 206) {
    h2o_error_printf("warning: status is %d\n", status);
  }
  h2o_rangeclient_t *client = httpclient->data;
//  print_status_line(version, status, msg);

  size_t filezs;

  if (client->range.end == 0) {
    if (parse_content_range(&filezs, headers, num_headers) < 0) {
      return NULL;
    }
    client->range.end = filezs;
    // TODO: should add a callback here?
  }

  return on_body;
}

static h2o_httpclient_head_cb on_connect(h2o_httpclient_t *httpclient, const char *errstr, h2o_iovec_t *method,
                                         h2o_url_t *url, const h2o_header_t **headers, size_t *num_headers,
                                         h2o_iovec_t *body, h2o_httpclient_proceed_req_cb *proceed_req_cb,
                                         h2o_httpclient_properties_t *props, h2o_url_t *origin) {
  if (errstr) {
    h2o_error_printf("on_connect failed");
    return NULL;
  }
  *method = (h2o_iovec_t) {H2O_STRLIT("GET")};
  h2o_rangeclient_t *client = httpclient->data;
  *url = *client->url_parsed;

  size_t buf_len;
  if (client->range.end > 0) {
    buf_len = snprintf(client->buf, 64, "bytes=%zu-%zu",
                       client->range.begin, client->range.end - 1);
  } else {
    buf_len = snprintf(client->buf, 64, "bytes=%zu-",
                       client->range.begin);
  }
  assert(buf_len < 64);
  h2o_header_t *range = client->range_header;
  range->name = &range_str;
  range->value = h2o_iovec_init(client->buf, buf_len);
  *headers = range;
  *num_headers = 1;

  *body = h2o_iovec_init(NULL, 0);
  *proceed_req_cb = NULL;

  return on_head;
}

h2o_rangeclient_t *h2o_rangeclient_create(h2o_httpclient_connection_pool_t *connpool,
                                          h2o_httpclient_ctx_t *ctx,
                                          h2o_url_t *url_parsed, char *save_to_file,
                                          size_t bytes_begin, size_t bytes_end) {
  assert(connpool != NULL);

  h2o_rangeclient_t *client = h2o_mem_alloc(sizeof(h2o_rangeclient_t));
  client->mempool = h2o_mem_alloc(sizeof(h2o_mem_pool_t));
  h2o_mem_init_pool(client->mempool);
  client->ctx = ctx;
  int default_permissions =
    S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  int fd;
  if ((fd = open(save_to_file,
                 O_WRONLY | O_CREAT | O_NONBLOCK | O_NOCTTY,
                 default_permissions) < 0)) {
    h2o_fatal("open() failed");
  }
  close(fd);

  client->file = fopen(save_to_file, "rb+");
  client->url_parsed = h2o_mem_alloc_pool(client->mempool, h2o_url_t, 1);
  h2o_url_copy(client->mempool, client->url_parsed, url_parsed);
  client->range.begin = bytes_begin;
  client->range.end = bytes_end;
  client->buf = h2o_mem_alloc_pool(client->mempool, char, 64);
  client->range_header = h2o_mem_alloc_pool(client->mempool, h2o_header_t, 1);
  client->is_closed = 0;

  if (fseeko(client->file, bytes_begin, SEEK_SET) < 0) {
    h2o_fatal("fseeko() failed");
  }
  h2o_httpclient_connect(&client->httpclient,
                         client->mempool,
                         client,
                         client->ctx,
                         connpool,
                         client->url_parsed,
                         on_connect);
  return client;
}

void h2o_rangeclient_destroy(h2o_rangeclient_t *client) {
  h2o_mem_clear_pool(client->mempool);
  free(client->mempool);
  fclose(client->file);
  free(client);
}
