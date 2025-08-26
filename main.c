
#include <stdio.h>
#include <stdlib.h> /* malloc */
#include <string.h> /* strcpy */
#include "http-server.h"

void answer(int cfd) {
    struct http_response response;

    response.protocol = malloc(sizeof(char*) + 1);
    strcpy(response.protocol, "HTTP/1.1");
    response.status_code = OK;

    struct http_header **headers = malloc(sizeof(*headers) * 3);
    headers[0] = create_header("Content-Length", "10");
    headers[1] = create_header("Content-Type", "text/plain");
    headers[2] = NULL;

    response.headers.header_list = headers;
    response.body = "Hello world!";

    send_response(cfd, response);

    free_http_response(response);
}



int main(int argc, char** argv) {
    
    struct http_server server; 
    if(argc > 2) {
        server = init_server(argv[1], atoi(argv[2]));
    } else if(argc > 1) {
        server = init_server(NULL, atoi(argv[1]));
    } else {
        printf("Usage: ./program [addr] <port>\n");
        exit(1);
    }

    while(1) {
        struct http_request request = get_request(server);
        display_request(request);
        answer(request.clientfd);
        free_http_request(request);
    } 
    close_server(server);
 
    return 0;
}


