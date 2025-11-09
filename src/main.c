
#include "external.h"
#include "config.h"
#include "internal.h"

int main() //int argc, char **argv)
{
	ServerState state = {0};
	server_init_regex(&state);

	Middleware error_middleware = { .handler = middleware_error_handler };
	server_add_middleware(&state, &error_middleware);

	Middleware static_file_middleware = { .handler = middleware_static_file_handler };
	server_add_middleware(&state, &static_file_middleware);

	server_run(&state);
	
	server_destroy(&state);
	
	return 0;
}

