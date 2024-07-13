
/***********************************************
 * Not for production
 * This web server is intended for development
 * and experimentation.
 *
 * Build and Run
 * gcc main.c & ./a.out
 ***********************************************/

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
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

#define u32 uint32_t
#define i32 int32_t
#define ARRAY_SIZE(array_name) ((i32)(sizeof(array_name) / sizeof(array_name[0])))

#include "http/content_types.c"
#include "http/status_codes.c"
#include "http/headers.c"
#include "http/context.c"
#include "http/send.c"
#include "server.c"
#include "middleware/error_handler.c"
#include "middleware/static_file_handler.c"

//void (*accept_connection)(ServerState*, HttpContext*);

/*void accept_connection(ServerState* state, HttpContext* context) {
	
}*/

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

