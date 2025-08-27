
#include <stdio.h> /* printf */
#include <sys/types.h>   /* socket */
#include <sys/socket.h> /* socket */
#include <stdlib.h> /* exit */
#include <arpa/inet.h>  /* INADDR_ANY */
#include <string.h> /* memset */
#include <sys/epoll.h> /* epoll_create1 */
#include <unistd.h> /* close */

#include "http-server.h"
#include "utils.h"

#define LISTEN_BACKLOG (50)
#define BUFF_SIZE (500)

char *status_code_to_string(enum StatusCode code) {
    switch(code) {
        case 200: return "OK"; break;
        case 400: return "Bad Request"; break;
        case 401: return "Unauthorized"; break;
        case 403: return "Fobidden"; break;
        case 404: return "Not Found"; break;
        case 500: return "Internal server error"; break;
    }
}



int create_socket() {
	int opt = 1; 
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		perror("socket creation");
		exit(1);
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
												&opt, sizeof(opt)))
	{
		perror("setsockopt"); 
		exit(EXIT_FAILURE);
	}
	
	return sockfd;
}

void bind_socket(int sockfd, char* addr, int port)  {
	struct sockaddr_in servaddr;
	
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	if(addr) {
		servaddr.sin_addr.s_addr = inet_addr(addr);
	}
	else {
		servaddr.sin_addr.s_addr = INADDR_ANY;
	}
	servaddr.sin_port = htons(port);
	
	if(-1 == bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) {
		perror("bind");
		exit(1);
	}

}

/**
 *  
 *  free_http_headers
 *
 */
void free_http_headers(struct http_headers headers) {
    struct http_header **runner = headers.header_list;

    if(NULL == runner) return;

    while(*runner) {
        struct http_header *current = *runner;
        free(current->key);
        free(current->value);
        free(current);
        ++runner;
    }
    free(headers.header_list);
}

/**
 *
 * free_http_request_params
 *
 */

void free_http_request_params(struct query_params params) {
    struct query_param **param_list_runner = params.param_list;

    if(NULL == param_list_runner) return;

    while(*param_list_runner) {
        struct query_param *current = *param_list_runner;
        free(current->key);
        free(current->value);
        free(current);
        ++param_list_runner;
    }

    free(params.param_list);
}


/**
 *
 *  free_http_response
 *
 *  frees the allocated memory for the response
 *
 **/
void free_http_response(struct http_response response) {
    free(response.protocol);
    free_http_headers(response.headers);
    free(response.body);
}

/**
 *
 *  free_http_request
 *
 *  frees the allocated memory for the request
 *
 */

void free_http_request(struct http_request request) {
    free(request.path);
    free(request.protocol);
    free_http_headers(request.headers);
    free_http_request_params(request.query_params);
    if(request.body) free(request.body);
}


/**
 *
 * send_response
 *
 * */

void send_response(int cfd, struct http_response response) {

    #define RESP_BUFF_SIZE (1024)
    char buffer[RESP_BUFF_SIZE] = { 0 };
    
    /* status line */
    sprintf(buffer, "%s %u %s\r\n",
        response.protocol, response.status_code, status_code_to_string(response.status_code));
    
    /* headers */
    struct http_header **runner = response.headers.header_list;
    while(*runner) {
        char *key = (*runner)->key;
        char *value = (*runner)->value;
        sprintf(buffer + strlen(buffer), "%s: %s\r\n", key, value);
        /*sprintf(buffer, "%s%s: %s\r\n", buffer, key, value);*/
        ++runner;
    }
    /* end response header */
    sprintf(buffer + strlen(buffer), "\r\n");
    /* body */

    sprintf(buffer + strlen(buffer), "%s", response.body);

    write(cfd, buffer, strlen(buffer));
}


/**
 *
 * create_header
 *
 * creates a http_header struct with key and value
 *
 */

struct http_header *create_header(const char *key, const char *value) {
    struct http_header *header = malloc(sizeof(*header));
    header->key = malloc(strlen(key) + 1);
    strcpy(header->key, key);
    header->value = malloc(strlen(value) + 1);
    strcpy(header->value, value);

    return header;
}

/**
 *
 * display_request
 * 
 * for debugging
 */
void display_request(struct http_request request) {
    /* request line */
    if(request.method == GET) { printf("GET\n"); }
    else if(request.method == POST) { printf("POST\n"); }
    else if(request.method == PUT) { printf("PUT\n"); }
    printf("Path: %s\n", request.path);
    printf("Query params:\n");
    
    struct query_param **param_list = request.query_params.param_list;

    if(NULL != param_list) {
         while(*param_list) {
            printf("%s: %s\n", (*param_list)->key, (*param_list)->value);
            param_list++;
        }
    }
    printf("Protocol: %s\n", request.protocol);

    /* headers*/
    printf("Headers:\n");
    struct http_header **header_list = request.headers.header_list;
    while(*header_list) {
        printf("%s: %s\n", (*header_list)->key, (*header_list)->value);
        header_list++;
    }

    /* body */
    if(request.body) {
        printf("Body:\n");
        printf("%s\n", request.body);
    }
}

/**
 *
 * get_header_value
 *
 * headers - http_headers struct containing all the headers
 * key - key to be looked for
 *
 * returns: the value if exists, NULL otherwise
 * */
char *get_header_value(struct http_headers headers, const char *key) {
    struct http_header **runner = headers.header_list;
    
    while(*runner) {
        if(strcmp((*runner)->key, key) == 0) {
            return (*runner)->value;
        }
        ++runner;
    }

    return NULL;
}

/**
 *
 * get_query_param
 *
 * query_params - query_params struct
 * key - key to be looked for
 *
 * returns: the value if exists, NULL otherwise
 * */
char *get_query_param(struct query_params params, const char *key) {
    struct query_param **runner = params.param_list;

    if(runner == NULL) return NULL;
    
    while(*runner) {
        if(strcmp((*runner)->key, key) == 0) {
            return (*runner)->value;
        }
        ++runner;
    }

    return NULL;
}



/**
 *
 * parse_uri
 *
 */

void parse_uri(struct http_request *request, char *uri) {
    char **param_str_list;
    char **params_runner;
    int num_params = 0, index = 0;
    char *uri_runner = uri;
    int length = 0;
    
    /* parse path*/
    while(*uri_runner && *uri_runner != '?') ++uri_runner;
    length = uri_runner - uri;
    request->path = malloc(length + 1);
    memcpy(request->path, uri, length);
    request->path[length] = '\0';

    /* if no query params, return */
    if(!*uri_runner) {
        return;
    }

    uri = ++uri_runner;
    param_str_list = split(uri, "&");
    params_runner = param_str_list;

    while(*params_runner) {
        ++num_params;
        ++params_runner;
    }

    params_runner = param_str_list;

    request->query_params.param_list = malloc(sizeof(struct query_param*) * (num_params + 1)); 
    
    /* handle each query param string */
    while(*params_runner) {
        char *curr = *params_runner;
        char **parts = split(curr, "=");

        request->query_params.param_list[index] = malloc(sizeof(struct query_param));
        request->query_params.param_list[index]->key = parts[0];
        request->query_params.param_list[index]->value = parts[1];
        ++params_runner;
        ++index;

        free_string_array_leave_strings(parts);
    }
    request->query_params.param_list[index] = NULL;

    free_string_array(param_str_list);
}



/**
 *
 * parse_head
 *
 * head - string with all the head of the request (request_line + headers)
 *
 * returns: http_request
 * caller needs to call free_http_request
 *
 */
struct http_request parse_head(const char *head) {
    struct http_request ret = { 0 };
    char **head_lines;
    char *request_line;
    struct http_header *headers[100];
    struct http_header *current_header;
    char **current_parts;
    char **header_lines;
    int index = 0, i = 0;

    head_lines = split(head, "\r\n");
    header_lines = head_lines + 1;
    request_line= head_lines[0];
    
    /* parse request line */
    char** request_line_parts = split(request_line, " ");
    if(strcmp("GET", request_line_parts[0]) == 0) {
        ret.method = GET;
    }
    else if(strcmp("POST", request_line_parts[0]) == 0) {
        ret.method = POST;
    }
    else if(strcmp("PUT", request_line_parts[0]) == 0) {
        ret.method = PUT;
    } else {
        printf("UNKNOWN METHOD");
    }
    free(request_line_parts[0]);
    
    /* parse uri - path, request params */
    char *uri = request_line_parts[1];
    parse_uri(&ret, uri);
    free(request_line_parts[1]);
    
    /* protocol */
    ret.protocol = request_line_parts[2];
    free_string_array_leave_strings(request_line_parts);
    
    /* parse headers */ 
    while(*header_lines) {
        current_header = (struct http_header*) malloc(sizeof(*current_header));
        current_parts = split(*header_lines, ": ");
        current_header->key = current_parts[0];
        current_header->value = current_parts[1];
        headers[index++] = current_header;
        
        free_string_array_leave_strings(current_parts);
        header_lines++;
    }
    
    free_string_array(head_lines);
    
    ret.headers.header_list = malloc(sizeof(struct http_header*) * (index + 1));
    for(i = 0; i < index; ++i) {
        ret.headers.header_list[i] = headers[i];
    }
    ret.headers.header_list[index] = NULL;

    return ret;
}

/* read the request line and headers */
char *read_head(int sock) {
    char *head = read_until(sock, "\r\n\r\n", 0);
    return head;
}


/* after accepting connection, server HTTP request */
struct http_request get_request(struct http_server server) {
    struct sockaddr_in cliaddr;
	socklen_t client_addr_size;	
	int cfd;   
    struct http_request request = { 0 };

	memset(&cliaddr, 0, sizeof(cliaddr));
	
	client_addr_size = sizeof(cliaddr);
    
	cfd = accept(server.sockfd, (struct sockaddr*)&cliaddr, &client_addr_size);
	if(-1 == cfd) {
		perror("accept");
		exit(1);
	}
    
    char *head = read_head(cfd);
    int body_len = 0;
    request = parse_head(head);
    request.clientfd = cfd;
    char *content_length = get_header_value(request.headers, "Content-Length");
    if(content_length) {
        body_len = atoi(content_length);
        request.body = malloc(body_len + 1);
        read_all(cfd, request.body, body_len);
        request.body[body_len] = '\0';
    }

    free(head);
    return request;
}

void close_server(struct http_server server) {
    close(server.sockfd);
}

struct http_server init_server(char *address, int port) {
    int sockfd = create_socket();

    bind_socket(sockfd, address, port);
 
    if(-1 == listen(sockfd, LISTEN_BACKLOG)) {
		perror("listen");
		exit(1);
	}

    struct http_server server;
    server.sockfd = sockfd;
    
    return server;
}


