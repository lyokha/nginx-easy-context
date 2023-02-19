/*
 * =============================================================================
 *
 *          File:  ngx_easy_context.h
 *
 *   Description:  Free persistent request contexts
 *
 *        Author:  Alexey Radkov
 *       Created:  15.02.2023 12:42
 *
 * =============================================================================
 */

#ifndef NGX_EASY_CONTEXT_H
#define NGX_EASY_CONTEXT_H

#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
    ngx_uint_t  ctx_index;
    ngx_int_t   var_index;
} ngx_http_easy_ctx_handle_t;


ngx_int_t ngx_http_register_easy_ctx(ngx_conf_t *cf,
    ngx_module_t *module, ngx_http_easy_ctx_handle_t *handle);
ngx_int_t ngx_http_set_easy_ctx(ngx_http_request_t *r,
    ngx_http_easy_ctx_handle_t *handle, void *ctx);
void *ngx_http_get_easy_ctx(ngx_http_request_t *r,
    ngx_http_easy_ctx_handle_t *handle);

#endif

