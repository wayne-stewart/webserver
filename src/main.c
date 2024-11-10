
#include "external.h"

#define PORT 8080
#define WWWROOT "./www"
#define BUFFER_SIZE 4084 /* 4096 - 12 ( 3 u32 integers ) */
#define PATH_BUFFER_SIZE 1024
#define MAX_CONNECTIONS 10

#include "internal.h"

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

