/*
 * =============================================================================
 *
 *          File:  ngx_easy_context.c
 *
 *   Description:  Free persistent request contexts
 *
 *        Author:  Alexey Radkov
 *       Created:  15.02.2023 12:20
 *
 * =============================================================================
 */

#include "ngx_easy_context.h"
#include "ngx_config.h"
#include "ngx_http_variables.h"


static ngx_int_t ngx_http_easy_ctx_handler(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);


ngx_int_t
ngx_http_register_easy_ctx(ngx_conf_t *cf, ngx_module_t *module,
                           ngx_http_easy_ctx_handle_t *handle)
{
    ngx_http_variable_t     *v;
    ngx_str_t               *v_name;
    u_char                  *v_name_buf;
    ngx_int_t                v_index;
    size_t                   len;

    static const ngx_str_t   v_prefix = ngx_string("_easy_ctx_");
    static ngx_uint_t        count = 0;

    v_name = ngx_palloc(cf->pool, sizeof(ngx_str_t));
    if (v_name == NULL) {
        return NGX_ERROR;
    }

    v_name_buf = ngx_pnalloc(cf->pool, v_prefix.len + NGX_INT_T_LEN);
    if (v_name_buf == NULL) {
        return NGX_ERROR;
    }

    len = ngx_sprintf(v_name_buf, "%V%ui", &v_prefix, count++) - v_name_buf;

    v_name->data = v_name_buf;
    v_name->len = len;

    v = ngx_http_add_variable(cf, v_name, NGX_HTTP_VAR_NOHASH);
    if (v == NULL) {
        return NGX_ERROR;
    }

    v->data = (uintptr_t) module->ctx_index;
    v->get_handler = ngx_http_easy_ctx_handler;

    v_index = ngx_http_get_variable_index(cf, v_name);
    if (v_index == NGX_ERROR) {
        return NGX_ERROR;
    }

    handle->ctx_index = module->ctx_index;
    handle->var_index = v_index;

    return NGX_OK;
}


static ngx_int_t
ngx_http_easy_ctx_handler(ngx_http_request_t *r, ngx_http_variable_value_t *v,
                          uintptr_t data)
{
    ngx_uint_t  ctx_index = data;
    uintptr_t   r_ctx;

    r_ctx = (uintptr_t) r->ctx[ctx_index];

    v->len = sizeof(uintptr_t);
    v->data = (u_char *) r_ctx;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}


ngx_int_t
ngx_http_set_easy_ctx(ngx_http_request_t *r,
                      ngx_http_easy_ctx_handle_t *handle, void *ctx)
{
    ngx_http_variable_value_t  *v;
    void                       *cur_ctx;

    cur_ctx = r->ctx[handle->ctx_index];
    r->ctx[handle->ctx_index] = ctx;

    v = ngx_http_get_flushed_variable(r, handle->var_index);

    r->ctx[handle->ctx_index] = cur_ctx;

    return v == NULL ? NGX_ERROR : NGX_OK;
}


void *
ngx_http_get_easy_ctx(ngx_http_request_t *r,
                      ngx_http_easy_ctx_handle_t *handle)
{
    ngx_http_variable_value_t  *v;

    v = ngx_http_get_indexed_variable(r, handle->var_index);
    if (v == NULL || !v->valid) {
        return NULL;
    }

    return (void*) v->data;
}

