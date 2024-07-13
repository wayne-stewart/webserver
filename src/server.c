/*
	Server
*/


typedef struct CompiledRegex {
	regex_t request_line;
	regex_t uri_double_dots;
	regex_t uri_valid_chars;
	regex_t end_of_headers;
} CompiledRegex;

typedef struct MiddlewareHandler MiddlewareHandler;
typedef struct ServerState ServerState;

typedef struct MiddlewareHandler {
	struct MiddlewareHandler* next;
	void (*run)(ServerState*, HttpContext*, MiddlewareHandler*);
} MiddlewareHandler;

typedef struct ServerState {
	i32 server_fd;
	i32 running;
	CompiledRegex regex;
	MiddlewareHandler* middleware;
} ServerState;

i32 read_request(ServerState* state, HttpContext* context)
{
	i32 eoh_found = 0;
	while(context->read_buffer.length < context->read_buffer.size && eoh_found == 0) {

		i32 recv_result = recv(
			context->client_fd, 
			context->read_buffer.data + context->read_buffer.length, 
			context->read_buffer.size - context->read_buffer.length, 
			0);

		if (recv_result == 0) { 
			printf("client closed connection\n");
			context->response.status_code = 0;
			return 0;
		}
		else if (recv_result == -1) {
			printf("recv error: %s\n", strerror(errno));
			context->response.status_code = 400;
			return 0;
		}
		
		context->read_buffer.length = recv_result;

		//printf("%s\n", context->read_buffer.data);
		
		// check if end of headers are found
		regmatch_t eoh_matches[1];
		if (regexec(&state->regex.end_of_headers, context->read_buffer.data, 1, eoh_matches, 0) == 0) {
			eoh_found = 1;
			printf("END OF HEADERS: %d %d\n", eoh_matches[0].rm_so, eoh_matches[0].rm_eo);
		}
	}

	// if end of headers was not found with what has been read so far
	// the request is invalid
	if (eoh_found == 0) {
		context->response.status_code = 400;
		return 0;
	}

	// parse request line
	regmatch_t rl_matches[3];
	if (regexec(&state->regex.request_line, context->read_buffer.data, 3, rl_matches, 0) == REG_NOMATCH) {
		printf("failed to parse request line\n");
		context->response.status_code = 400;
		return 0;
	}

	context->read_buffer.data[rl_matches[1].rm_eo] = '\0'; // null terminator for method
	context->read_buffer.data[rl_matches[2].rm_eo] = '\0'; // null terminator for URI
	context->request.method = context->read_buffer.data + rl_matches[1].rm_so;
	context->request.uri = context->read_buffer.data + rl_matches[2].rm_so;

	// if double dots are found, request validation fails
	if (regexec(&state->regex.uri_double_dots, context->request.uri, 0, 0, 0) == 0) {
		printf("request line had ..\n");
		context->response.status_code = 400;
		return 0;
	}

	// make sure there are no unexpected characters in the uri
	if (regexec(&state->regex.uri_valid_chars, context->request.uri, 0, 0, 0) == REG_NOMATCH) {
		printf("unexpected chars in request line\n");
		context->response.status_code = 400;
		return 0;
	}

	return 1;
}

void server_init(ServerState* state) {
	regcomp(&state->regex.request_line, "^(GET) ([^ ]*) HTTP/1.1\r\n", REG_EXTENDED);
	regcomp(&state->regex.uri_double_dots, "\\.\\.", REG_EXTENDED);
	regcomp(&state->regex.uri_valid_chars, "^[0-9a-zA-Z/_\\.-]+$", REG_EXTENDED);
	regcomp(&state->regex.end_of_headers, "\r\n\r\n", REG_EXTENDED);
}

void server_destroy(ServerState* state) {
	if (state->server_fd) {
		close(state->server_fd);
	}
}

void server_add_middleware(ServerState* state, MiddlewareHandler* handler) {
	if (state->middleware) {
		MiddlewareHandler* parent = state->middleware;
		while (parent->next) parent = parent->next;
		parent->next =  handler;
	} else {
		state->middleware = handler;
	}
}

i32 server_bind(ServerState* state) {
	
	struct sockaddr_in server_addr = {0};

	if ((state->server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("create socket failed");
		return 1;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	if (bind(state->server_fd,
		(struct sockaddr *)&server_addr,
		sizeof(server_addr)) < 0) {
		perror("socket bind failed");
		return 1;
	}

	return 0;
}

i32 server_listen(ServerState* state) {
	
	if (listen(state->server_fd, 10) < 0) {
		perror("listening to socket failed");
		return 1;
	}
	
	printf("listening on port: %d\n", PORT);
	return 0;
}

void server_process_request(ServerState* state, i32 client_fd) {
	
	HttpContext context = { .client_fd = client_fd };
	buffer_init(&context.read_buffer);
	buffer_init(&context.write_buffer);

	if (read_request(state, &context)) {
		state->middleware->run(state, &context, state->middleware->next);
	}
	else if (context.response.status_code == 400) {
		send_400(&context);
	}
	
	if (context.request.uri) {
		printf("%d %s %s\n", context.response.status_code, context.request.method, context.request.uri);
	}
	else {
		printf("Could not parse request line\n");
	}
	if (context.client_fd) {
		close(context.client_fd);
	}
}

void server_accept_loop(ServerState* state) {

	state->running = 1;

	while(state->running) {
		struct sockaddr_in client_addr = {0};
		socklen_t client_addr_len = sizeof(client_addr);

		i32 client_fd = accept(state->server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
		if (client_fd < 0) continue;

		server_process_request(state, client_fd);
	}
}

void server_run(ServerState* state) {

	if (server_bind(state)) return;

	if (server_listen(state)) return;
	
	server_accept_loop(state);
}



