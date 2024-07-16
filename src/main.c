
#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>

#define PORT 8080
#define WWWROOT "./www"
#define BUFFER_SIZE 4084 /* 4096 - 12 ( 3 u32 integers ) */
#define PATH_BUFFER_SIZE 1024
#define MAX_CONNECTIONS 10

#define u32 uint32_t
#define i32 int32_t
#define ARRAY_SIZE(array_name) ((i32)(sizeof(array_name) / sizeof(array_name[0])))
#define LOG(msg, ...) printf(msg "\n", ##__VA_ARGS__)

#include "http/content_types.c"
#include "http/status_codes.c"
#include "http/headers.c"
#include "http/context.c"
#include "http/send.c"
#include "server.c"
#include "middleware/error_handler.c"
#include "middleware/static_file_handler.c"

int main() //int argc, char **argv)
{
	ServerState state = {0};
	server_init(&state);

	MiddlewareHandler error_middleware= { .run = middleware_error_handler };
	server_add_middleware(&state, &error_middleware);

	MiddlewareHandler static_file_middleware = { .run = middleware_static_file_handler };
	server_add_middleware(&state, &static_file_middleware);

	server_run(&state);
	
	server_destroy(&state);
	
	return 0;
}

