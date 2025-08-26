#include <sys/types.h> /* recv */
#include <sys/socket.h> /* recv */
#include <string.h> /* strlen */
#include <stdlib.h> /* malloc */
#include <unistd.h> /* read */
#include <stdio.h> /* perror */
#include "utils.h"

#define BUFSIZE (1000)

char *read_until(int sock, const char *delim, int include_delim)
{
	char *ret_buf = NULL;
	char buffer[BUFSIZE];
	int read_bytes = 0;
	int found = 0;
	int counter = 0;
	
	while(!found)
	{
		
		int n = recv(sock, (buffer + read_bytes), 1, 0);
		if(-1 == n)
		{
			perror("read_until");
			
			return NULL;
		}
		if(buffer[read_bytes] == delim[counter])
		{
			++counter;
			if(strlen(delim) == counter)
			{
				found = 1;
			}
		}
		else
		{
			counter = 0;
		}
		++read_bytes;	
	}
	buffer[read_bytes] = 0;
	
	if(!include_delim)
	{
		buffer[read_bytes - strlen(delim)] = 0;
	}
	
	if(strlen(buffer) && 1 == found)
	{
		ret_buf = (char*)malloc(strlen(buffer) + 1);
		strcpy(ret_buf, buffer);	
		return ret_buf;
	}
	
	return NULL;
}

char *read_all(int sock, char *buffer, size_t length)
{
	int read_bytes = 0;
	
	if(NULL == buffer)
	{
		return NULL;
	}
	
	while(read_bytes < length)
	{
		int n = read(sock, (buffer + read_bytes), (length - read_bytes));
		if(-1 == n)
		{
			perror("read_all");
			
			return NULL;
		}
		read_bytes += n;
	}
	
	
	return buffer;
}

/* 
 * Split a delim-delimited string to tokens
 *
 * str - the string to split
 * delim - the delimiter string
 *
 * user needs to free the returned pointer using free_string_array. 
 * if you want to keep the strings only, use free_string_array_leave_strings
 *
 * ISSUES
 * - don't send an empty delim
 * - limited to 100 sub-tokens
 *  
 *
 * */
char** split(const char* str, const char* delim) {
    int delim_length = strlen(delim);
    const char *delim_runner, *last = str, *str_runner;
    char *token, *tokens[100];
    char **ret;
    int i = 0;
    int index = 0;
    int token_length = 0;

    while(*str) {
        for(delim_runner = delim, str_runner = str; *delim_runner && *delim_runner == *str_runner; ++str_runner, ++delim_runner) {}

        if('\0' == *delim_runner) { /* found delim */
            token_length = str - last;
            if(token_length > 0) {
                token = (char*)malloc(token_length + 1);
                memcpy(token, last, token_length);
                token[token_length] = '\0';
                tokens[index++] = token;
            }
            last = str_runner;
            str = str_runner;
            continue;
        }
        ++str;
    }

    if(str > last) {
        token_length = str - last;
        token = (char*)malloc(token_length + 1);
        memcpy(token, last, token_length);
        token[token_length] = '\0';
        tokens[index++] = token;
    }

    ret = (char**) malloc(sizeof(char*) * (index + 1));
    for(i = 0; i < index; ++i) {
        ret[i] = tokens[i];
    }
    ret[i] = NULL;

    return ret;
}


void free_string_array_leave_strings(char** arr) {
    free(arr);
}

void free_string_array(char** arr) {
    char** tmp = arr;
    while(*tmp) {
        free(*tmp);
        ++tmp;
    }
    free(arr);
}


