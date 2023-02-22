# vi:filetype=

use Test::Nginx::Socket;

repeat_each(2);
plan tests => repeat_each() * (2 * (blocks()));

no_shuffle();
run_tests();

__DATA__

=== TEST 1: free contexts survive error_page redirection
--- config
        location /main {
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

        location /sub {
            echo_location /sub1;
            echo_location /sub2;
        }

        location /sub1 {
            internal;
            test_easy_context on;
            error_page 502 = /error1;
            return 502;
        }

        location /sub2 {
            internal;
            error_page 502 = /error2;
            return 502;
        }

        location ~ ^/error(\d) {
            internal;
            echo ">>> Subrequest $1";
            echo "Request context: '$test_easy_ctx_req_ctx'";
            echo "Ctx1: '$test_easy_ctx_ctx1'";
            echo "Ctx2: '$test_easy_ctx_ctx2'";
        }
--- request
GET /main
--- response_body
Request context: ''
Ctx1: 'This is request context'
Ctx2: 'This is context 2'
--- error_code: 200

=== TEST 2: free context are not shared in subrequests
--- request
GET /sub
--- response_body
>>> Subrequest 1
Request context: ''
Ctx1: 'This is request context'
Ctx2: 'This is context 2'
>>> Subrequest 2
Request context: ''
Ctx1: ''
Ctx2: ''
--- error_code: 200

