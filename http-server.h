
#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

enum Method {
    GET,
    POST,
    PUT,
    HEAD
};

enum StatusCode {
    OK = 200,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    SERVER_ERROR = 500
};

struct http_server {
    int sockfd;
};

struct http_header {
    char *key;
    char *value;
};

struct query_param {
    char *key;
    char *value;
};

struct http_headers {
    struct http_header **header_list;
};

struct query_params {
    struct query_param **param_list;
};


struct http_request {
    enum Method method;
    char *path;
    char *protocol;
    struct query_params query_params;
    struct http_headers headers;
    char *body;
    int clientfd;
};

struct http_response {
    enum StatusCode status_code;
    char *protocol;
    struct http_headers headers;
    char *body;
};


void free_http_response(struct http_response response);
void free_http_request(struct http_request request);
void send_response(int cfd, struct http_response response);
struct http_header *create_header(const char *key, const char *value);
void display_request(struct http_request request);
char *get_header_value(struct http_headers headers, const char *key);
char *get_query_param(struct query_params params, const char *key);
struct http_request get_request(struct http_server server);
void close_server(struct http_server server);
struct http_server init_server(char *address, int port);

#endif
