/*
	Static File Handler
*/

/* try to open file to serve. if path is a directory
 * try to open the default file index.html.
 * return value is a file descriptor.
 * return -1 if file cannot be found.
 * return -2 if a 302 should be sent with appended / */
int middleware_static_file_try_open(HttpContext* context, char* path_buffer, i32 path_buffer_length) {
		
	i32 uri_length = strlen(context->request.uri);
	i32 path_length = 0;

	// if the final character is a slash try to open
	// the default index.html file in the directory
	// we know from the regex uri validation that the first
	// character will always be a slash
	if (context->request.uri[uri_length - 1] == '/') {
		path_length = snprintf(path_buffer, path_buffer_length, "%s%sindex.html", WWWROOT, context->request.uri);
	} else {
		path_length = snprintf(path_buffer, path_buffer_length, "%s%s", WWWROOT, context->request.uri);
	}

	// path was greater than the buffer so return error
	if (path_length == path_buffer_length) return 0;

	// tolower all characters in the path
	// all served files should be lower case
	//for(i32 i = 0; path_buffer[i] != '\0'; i++) {
	//	path_buffer[i] = tolower(path_buffer[i]);
	//}

	// path could not be found as a file or dir so return error
	struct stat stat_buffer;
	if (lstat(path_buffer, &stat_buffer) != 0) return -1;

	// path found, open for reading and return the file descriptor
	if (S_ISREG(stat_buffer.st_mode)) {
		return open(path_buffer, O_RDONLY);
	}
	
	// path is a directory but it did not end in a '/'
	// send back a code to inform the the caller a 302
	// should be generated with the / at the end.
	if (S_ISDIR(stat_buffer.st_mode)) {
		return -2;
	}

	// no path could be found so return error
	return -1;
}

void middleware_static_file_serve(HttpContext* context)
{
	char path_buffer[PATH_BUFFER_SIZE] = {0};
	i32 path_length = ARRAY_SIZE(path_buffer);
	i32 path_length_minus_2 = ARRAY_SIZE(path_buffer) - 2;
	i32 path_length_minus_1 = ARRAY_SIZE(path_buffer) - 1;

	// -2 on the path buffer to make sure there is room for / and a trailing 0
	i32 file_fd = middleware_static_file_try_open(context, path_buffer, path_length_minus_2);
	
	if (file_fd == -1) { 
		send_404(context);
		return;
	}

	if (file_fd == -2) {
		memset(path_buffer, 0, path_length);
		snprintf(path_buffer, path_length_minus_1, "%s/", context->request.uri);
		send_302(context, path_buffer);
		return;
	}
	
	send_file(context, file_fd, path_buffer);

	close(file_fd);
}

/*
	StaticFileHandler
	Run the next middleware in the chain if it exists.
	If no middleware handled the request ( status_code == 0 ),
	then try to look for a file to send as the response.

	TODO: try caching the file so we don't have to stream from disk
		  as often.
*/
void middleware_static_file_handler(ServerState* state, HttpContext* context, MiddlewareHandler* next) {
	if (next) {
		next->run(state, context, next->next);
	}
	if (context->response.status_code == 0) {
		middleware_static_file_serve(context);		
	}
}

