nginx-easy-context
==================

[![Build Status](https://github.com/lyokha/nginx-easy-context/workflows/CI/badge.svg)](https://github.com/lyokha/nginx-easy-context/actions?query=workflow%3ACI)

Free persistent request contexts.

*Free* means that they are not bound to the Nginx request object and their
number is not anyhow restricted. *Persistent* means that they are not cleared
upon internal redirections *unlike* the normal Nginx request context.

Note that whether free contexts are shared between the main request and its
subrequests or not depends on how the subrequests are spawned. Internally, a
free context is bound to a distinct variable which tracks the context value.
Normally, variables are shared between the main request and its subrequests
which means that the contexts are shared too. However, some modules such as
*echo-nginx-module* allocate a new set of variables in subrequests they spawn,
and therefore the contexts in these subrequests branch off the main request.

C API
-----

Module *ngx_easy_context* is tagged as *MISC* in the build config file. It does
not expose a module context or any handlers and directives. Instead, the module
exports a number of C functions to help users build their own modules. In
directory *test/*, there is a simple HTTP module *test_easy_context* aimed at
demonstration of how to use module *ngx_easy_context*.

#### Register a request context handle

```c
ngx_int_t ngx_http_register_easy_ctx(ngx_conf_t *cf,
    ngx_module_t *module, ngx_http_easy_ctx_handle_t *handle);
```

Registering a context handle means creation of a handle for using it when
setting and getting a request context during the lifetime of a request. The
best place for registering a context handle is the module's postconfiguration
handler. The best place for storing a context handle is the module's main
configuration.

###### Example (module *test_easy_context*)

```c
typedef struct {
    ngx_http_easy_ctx_handle_t  ctx1;
    ngx_http_easy_ctx_handle_t  ctx2;
} test_easy_ctx_main_conf_t;

typedef struct {
    ngx_int_t  index;
    ngx_str_t  value;
} test_easy_ctx_ctx_t;

static ngx_http_module_t  test_easy_context_ctx = {
    // ---
    test_easy_ctx_init,                    /* postconfiguration */
    // ---
};

static ngx_int_t
test_easy_ctx_init(ngx_conf_t *cf)
{
    test_easy_ctx_main_conf_t  *mcf;

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

    return NGX_OK;
}
```

#### Set a request context

```c
ngx_int_t ngx_http_set_easy_ctx(ngx_http_request_t *r,
    ngx_http_easy_ctx_handle_t *handle, void *ctx);
```

This function is a direct equivalent to `ngx_http_set_ctx(r, ctx, module)`
except it expects a registered context handle rather than a `module`.

###### Example

```c
    test_easy_ctx_main_conf_t  *mcf;
    test_easy_ctx_ctx_t        *ctx1;

    ctx1 = ngx_pcalloc(r->connection->pool, sizeof(test_easy_ctx_ctx_t));
    if (ctx1 == NULL) {
        return NGX_ERROR;
    }

    ctx1->value.data = (u_char *) "This is request context";
    ctx1->value.len = ngx_strlen(ctx1->data);

    mcf = ngx_http_get_module_main_conf(r, test_easy_context);

    if (ngx_http_set_easy_ctx(r, &mcf->ctx1, ctx1) == NGX_ERROR) {
        return NGX_ERROR;
    }
```

#### Get a request context

```c
void *ngx_http_get_easy_ctx(ngx_http_request_t *r,
    ngx_http_easy_ctx_handle_t *handle);
```

This function is a direct equivalent to `ngx_http_get_module_ctx(r, module)`
except it expects a registered context handle rather than a `module`.

###### Example

```c
    test_easy_ctx_main_conf_t  *mcf;
    test_easy_ctx_ctx_t        *ctx1;

    mcf = ngx_http_get_module_main_conf(r, test_easy_context);

    ctx1 = ngx_http_get_easy_ctx(r, &mcf->ctx1);
```

Build
-----

From the Nginx source directory, run

```ShellSession
$ ./configure --add-module=/path/to/nginx-easy-context
$ make
```

Test
----

In module *test_easy_context* located in directory *test/*, two free request
contexts and one normal Nginx request context are created by running directive
*test_easy_context*. All the contexts hold Nginx strings with simple text
messages that will be sent in the response.

The module is built from the Nginx source directory.

```ShellSession
$ ./configure --add-module=/path/to/nginx-easy-context --add-module=/path/to/nginx-easy-context/test
$ make
```

The order of the two *--add-module* options matters. Module *nginx-easy-context*
must go first.

###### File *nginx.conf*

```nginx
user                    nobody;
worker_processes        4;

events {
    worker_connections  1024;
}

error_log               /tmp/nginx-test-easy-context-error.log warn;

http {
    default_type        application/octet-stream;
    sendfile            on;

    access_log          /tmp/nginx-test-easy-context-access.log;

    server {
        listen          8010;
        server_name     main;

        location / {
            test_easy_context on;
            error_page 502 = /error;
            return 502;
        }

        location /error {
            internal;
            echo "Request context: '$test_easy_ctx_req_ctx'";
            echo "Ctx1: '$test_easy_ctx_ctx1'";
            echo "Ctx2: '$test_easy_ctx_ctx2'";
        }
    }
}
```

After internal redirection via *error_page*, the normal request context must
have been cleared while the two free request contexts must still be alive.

Let's test this.

```ShellSession
$ curl 'http://127.0.0.1:8010/'
Request context: ''
Ctx1: 'This is request context'
Ctx2: 'This is context 2'
```

