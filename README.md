nginx-easy-context
==================

Free persistent request contexts. *Free* means that they are not bound to the
Nginx request object and their number is not anyhow restricted. *Persistent*
means that they are not cleared upon internal redirects like the normal Nginx
request context.

Build
-----

From the Nginx source directory, run

```ShellSession
$ ./configure --add-module=/path/to/nginx-easy-context
$ make
```

Test
----

There is a simple test module that creates two free request contexts and one
normal Nginx request context in directive *test_easy_context*. All the contexts
hold Nginx strings with simple text messages.

The module is built from the Nginx source directory.

```ShellSession
$ ./configure --add-module=/path/to/nginx-easy-context --add-module=/path/to/nginx-easy-context/test
$ make
```

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

After internal redirection via *error_page*, the normal request context must be
cleared while the two free request contexts must still be alive.

Test this.

```ShellSession
$ curl 'http://127.0.0.1:8010/'
Request context: ''
Ctx1: 'This is request context'
Ctx2: 'This is context 2'
```

