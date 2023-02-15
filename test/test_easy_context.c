/*
 * =============================================================================
 *
 *          File:  test_easy_context.c
 *
 *   Description:  Test easy contexts
 *
 *        Author:  Alexey Radkov
 *       Created:  15.02.2023 16:07
 *
 * =============================================================================
 */

#include "ngx_easy_context.h"


typedef struct {
    ngx_http_easy_ctx_handle_t  ctx1;
    ngx_http_easy_ctx_handle_t  ctx2;
} test_easy_ctx_main_conf_t;
#include "ngx_easy_context.h"


typedef struct {
    ngx_flag_t  set_ctx;
} test_easy_ctx_loc_conf_t;


static ngx_int_t test_easy_ctx_add_vars(ngx_conf_t *cf);
static void *test_easy_ctx_create_main_conf(ngx_conf_t *cf);
static void *test_easy_ctx_create_loc_conf(ngx_conf_t *cf);
static char *test_easy_ctx_merge_loc_conf(ngx_conf_t *cf,
    void *parent, void *child);
static ngx_int_t test_easy_ctx_init(ngx_conf_t *cf);
static ngx_int_t test_easy_ctx_rewrite_phase_handler(ngx_http_request_t *r);
static ngx_int_t test_easy_ctx_var(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);


static ngx_command_t  test_easy_context_commands[] = {

    { ngx_string("test_easy_context"),
      NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(test_easy_ctx_loc_conf_t, set_ctx),
      NULL },

      ngx_null_command
};


static ngx_http_variable_t  test_easy_context_vars[] =
{
    { ngx_string("test_easy_ctx_req_ctx"), NULL, test_easy_ctx_var, 0, 0, 0 },
    { ngx_string("test_easy_ctx_ctx1"), NULL, test_easy_ctx_var, 1, 0, 0 },
    { ngx_string("test_easy_ctx_ctx2"), NULL, test_easy_ctx_var, 2, 0, 0 },

    { ngx_null_string, NULL, NULL, 0, 0, 0 }
};


static ngx_http_module_t  test_easy_context_ctx = {
    test_easy_ctx_add_vars,                /* preconfiguration */
    test_easy_ctx_init,                    /* postconfiguration */

    test_easy_ctx_create_main_conf,        /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    test_easy_ctx_create_loc_conf,         /* create location configuration */
    test_easy_ctx_merge_loc_conf           /* merge location configuration */
};


ngx_module_t  test_easy_context = {
    NGX_MODULE_V1,
    &test_easy_context_ctx,                /* module context */
    test_easy_context_commands,            /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t
test_easy_ctx_add_vars(ngx_conf_t *cf)
{
    ngx_http_variable_t  *var, *v;

    for (v = test_easy_context_vars; v->name.len; v++) {
        var = ngx_http_add_variable(cf, &v->name, v->flags);
        if (var == NULL) {
            return NGX_ERROR;
        }

        var->get_handler = v->get_handler;
        var->data = v->data;
    }

    return NGX_OK;
}


static ngx_int_t
test_easy_ctx_init(ngx_conf_t *cf)
{
    ngx_http_core_main_conf_t  *cmcf;
    test_easy_ctx_main_conf_t  *mcf;
    ngx_http_handler_pt        *h;

    mcf = ngx_http_conf_get_module_main_conf(cf, test_easy_context);

    if (ngx_http_register_easy_ctx(cf, &test_easy_context, &mcf->ctx1)
        == NGX_ERROR)
    {
        return NGX_ERROR;
    }

    if (ngx_http_register_easy_ctx(cf, &test_easy_context, &mcf->ctx2)
        == NGX_ERROR)
    {
        return NGX_ERROR;
    }

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_REWRITE_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = test_easy_ctx_rewrite_phase_handler;

    return NGX_OK;
}


static void *
test_easy_ctx_create_main_conf(ngx_conf_t *cf)
{
    test_easy_ctx_main_conf_t  *mcf;

    mcf = ngx_pcalloc(cf->pool, sizeof(test_easy_ctx_main_conf_t));

    return mcf;
}


static void *
test_easy_ctx_create_loc_conf(ngx_conf_t *cf)
{
    test_easy_ctx_loc_conf_t   *lcf;

    lcf = ngx_pcalloc(cf->pool, sizeof(test_easy_ctx_loc_conf_t));

    lcf->set_ctx = NGX_CONF_UNSET;

    return lcf;
}


static char *
test_easy_ctx_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    test_easy_ctx_loc_conf_t   *prev = parent;
    test_easy_ctx_loc_conf_t   *conf = child;

    ngx_conf_merge_value(conf->set_ctx, prev->set_ctx, 0);

    return NGX_CONF_OK;
}


static ngx_int_t
test_easy_ctx_rewrite_phase_handler(ngx_http_request_t *r)
{
    test_easy_ctx_main_conf_t  *mcf;
    test_easy_ctx_loc_conf_t   *lcf;
    ngx_str_t                  *ctx1;
    ngx_str_t                  *ctx2;

    lcf = ngx_http_get_module_loc_conf(r, test_easy_context);

    if (!lcf->set_ctx) {
        return NGX_DECLINED;
    }

    ctx1 = ngx_palloc(r->connection->pool, sizeof(ngx_str_t));
    if (ctx1 == NULL) {
        return NGX_ERROR;
    }

    ctx2 = ngx_palloc(r->connection->pool, sizeof(ngx_str_t));
    if (ctx2 == NULL) {
        return NGX_ERROR;
    }

    ctx1->data = (u_char*) "This is request context";
    ctx1->len = ngx_strlen(ctx1->data);

    ctx2->data = (u_char*) "This is context 2";
    ctx2->len = ngx_strlen(ctx2->data);

    ngx_http_set_ctx(r, ctx1, test_easy_context);

    mcf = ngx_http_get_module_main_conf(r, test_easy_context);

    if (ngx_http_set_easy_ctx(r, &mcf->ctx1, ctx1) == NGX_ERROR) {
        return NGX_ERROR;
    }

    if (ngx_http_set_easy_ctx(r, &mcf->ctx2, ctx2) == NGX_ERROR) {
        return NGX_ERROR;
    }

    return NGX_DECLINED;
}


static ngx_int_t
test_easy_ctx_var(ngx_http_request_t *r, ngx_http_variable_value_t *v,
                  uintptr_t data)
{
    test_easy_ctx_main_conf_t  *mcf;
    ngx_str_t                  *value = NULL;

    mcf = ngx_http_get_module_main_conf(r, test_easy_context);

    switch (data) {
    case 0:
        value = ngx_http_get_module_ctx(r, test_easy_context);
        break;
    case 1:
        value = ngx_http_get_easy_ctx(r, &mcf->ctx1);
        break;
    case 2:
        value = ngx_http_get_easy_ctx(r, &mcf->ctx2);
        break;
    default:
        break;
    }

    if (value == NULL) {
        return NGX_ERROR;
    }

    v->len          = value->len;
    v->data         = value->data;
    v->valid        = 1;
    v->no_cacheable = 0;
    v->not_found    = 0;

    return NGX_OK;
}

