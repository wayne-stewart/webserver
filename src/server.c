/*
	Server
*/


typedef struct CompiledRegex {
	regex_t request_line;
	regex_t uri_double_dots;
	regex_t uri_valid_chars;
	regex_t end_of_headers;
} CompiledRegex;

typedef struct Middleware Middleware;
typedef struct ServerState ServerState;
typedef struct epoll_event epoll_event;
typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef void (*MiddlewareHandler)(ServerState*, HttpContext*, Middleware*);

typedef struct Middleware {
	struct Middleware* next;
	MiddlewareHandler handler;
} Middleware;


typedef struct ServerState {
	i32 running;
	i32 server_fd;
	i32 epoll_fd;
	epoll_event epoll_events[MAX_CONNECTIONS];
	HttpContext context_pool[MAX_CONNECTIONS];
	CompiledRegex regex;
	Middleware* middleware;
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
			LOG("client closed connection");
			context->response.status_code = 0;
			return 0;
		}
		else if (recv_result == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				
			}
			LOG("recv error: %s", strerror(errno));
			context->response.status_code = 400;
			return 0;
		}
		
		context->read_buffer.length = recv_result;

		//printf("%s\n", context->read_buffer.data);
		
		// check if end of headers are found
		regmatch_t eoh_matches[1];
		if (regexec(&state->regex.end_of_headers, context->read_buffer.data, 1, eoh_matches, 0) == 0) {
			eoh_found = 1;
			//printf("END OF HEADERS: %d %d\n", eoh_matches[0].rm_so, eoh_matches[0].rm_eo);
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
		LOG("failed to parse request line");
		context->response.status_code = 400;
		return 0;
	}

	context->read_buffer.data[rl_matches[1].rm_eo] = '\0'; // null terminator for method
	context->read_buffer.data[rl_matches[2].rm_eo] = '\0'; // null terminator for URI
	context->request.method = context->read_buffer.data + rl_matches[1].rm_so;
	context->request.uri = context->read_buffer.data + rl_matches[2].rm_so;

	// if double dots are found, request validation fails
	if (regexec(&state->regex.uri_double_dots, context->request.uri, 0, 0, 0) == 0) {
		LOG("request line had double dots (..)");
		context->response.status_code = 400;
		return 0;
	}

	// make sure there are no unexpected characters in the uri
	if (regexec(&state->regex.uri_valid_chars, context->request.uri, 0, 0, 0) == REG_NOMATCH) {
		LOG("unexpected chars in request line");
		context->response.status_code = 400;
		return 0;
	}

	// TODO: separate path from query string and validate chars in both seprately

	return 1;
}

void server_init(ServerState* state) {
	regcomp(&state->regex.request_line, "^(GET) (/[^ ]*) HTTP/1.1\r\n", REG_EXTENDED);
	regcomp(&state->regex.uri_double_dots, "\\.\\.", REG_EXTENDED);
	regcomp(&state->regex.uri_valid_chars, "^[0-9a-zA-Z/_\\.-]+$", REG_EXTENDED);
	regcomp(&state->regex.end_of_headers, "\r\n\r\n", REG_EXTENDED);
}

void server_destroy(ServerState* state) {
	if (state->server_fd) {
		close(state->server_fd);
	}
}

void server_add_middleware(ServerState* state, Middleware* middleware) {
	if (state->middleware) {
		Middleware* parent = state->middleware;
		while (parent->next) parent = parent->next;
		parent->next =  middleware;
	} else {
		state->middleware = middleware;
	}
}

void server_bind(ServerState* state) {
	
	sockaddr_in server_addr = {0};

	if ((state->server_fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0)) == -1) {
		LOG("server_bind: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	if (bind(state->server_fd,
		(sockaddr *)&server_addr,
		sizeof(server_addr)) == -1) {
		LOG("server_bind: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void server_bind_nonblock(ServerState* state) {
	
	sockaddr_in server_addr = {0};

	if ((state->server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)) == -1) {
		LOG("server_bind: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	if (bind(state->server_fd,
		(sockaddr *)&server_addr,
		sizeof(server_addr)) == -1) {
		LOG("server_bind: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void server_listen(ServerState* state) {
	
	if (listen(state->server_fd, 10) == -1) {
		LOG("server_listen: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	LOG("listening on port: %d", PORT);
}

void server_process_request(ServerState* state, i32 client_fd) {
	
	HttpContext context = { .client_fd = client_fd };
	buffer_init(&context.read_buffer);
	buffer_init(&context.write_buffer);

	if (read_request(state, &context)) {
		state->middleware->handler(state, &context, state->middleware->next);
	}
	else if (context.response.status_code == 400) {
		send_400(&context);
	}
	
	if (context.request.uri) {
		LOG("%d %s %s", context.response.status_code, context.request.method, context.request.uri);
	}
	else {
		LOG("Could not parse request line");
	}
	if (context.client_fd) {
		close(context.client_fd);
	}
}

void server_process_request_nonblocking(ServerState* state, i32 client_fd) {
	
	HttpContext* context = 0;
	for(i32 i = 0; i < ARRAY_SIZE(state->context_pool); i++) {
		if (state->context_pool[i].client_fd == client_fd) {
			context = &state->context_pool[i];
			break;
		}
	}
	
	if (context == 0) {
		for(i32 i = 0; i < ARRAY_SIZE(state->context_pool); i++) {
			if (state->context_pool[i].client_fd == 0) {
				context = &state->context_pool[i];
				memset(context, 0, sizeof(*context));
				context->client_fd = client_fd;
				buffer_init(&context->read_buffer);
				buffer_init(&context->write_buffer);
				break;
			}
		}
	}

	if (context == 0) {
		LOG("context_pool full");
		close(client_fd);
		return;
	}

	read_request(state, context);

	if (context->state == HttpContextState_ReadingDone) {
		state->middleware->handler(state, context, state->middleware->next);
	}
	else if (context->state == HttpContextState_Complete) {
		if (context->response.status_code == 400) {
			send_400(context);
		}
	
		if (context->request.uri) {
			LOG("%d %s %s", context->response.status_code, context->request.method, context->request.uri);
		}
		else {
			LOG("Could not parse request line");
		}

		close(context->client_fd);
		context->client_fd = 0;
	}
}

void server_loop(ServerState* state) {

	state->running = 1;

	while(state->running) {
		sockaddr_in client_addr = {0};
		socklen_t client_addr_len = sizeof(client_addr);

		i32 client_fd = accept(state->server_fd, (sockaddr*)&client_addr, &client_addr_len);
		if (client_fd < 0) continue;

		server_process_request(state, client_fd);
	}
}

void server_loop_epoll(ServerState* state) {
	
	// create the epoll file descriptor
	state->epoll_fd = epoll_create1(0);
	if (state->epoll_fd == -1) {
		LOG("epoll_create1: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	// add the listen socket server_fd to the event list watched by epoll
	epoll_event ev = {
		.events = EPOLLIN,
		.data.fd = state->server_fd
	};
	if (epoll_ctl(state->epoll_fd, EPOLL_CTL_ADD, state->server_fd, &ev) == -1) {
		LOG("epoll_ctl, server_fd: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	while(state->running) {

		// wait for a socket file descriptor to be ready to read or written to
		i32 nfds = epoll_wait(state->epoll_fd, state->epoll_events, MAX_CONNECTIONS, -1);
		if (nfds == -1) {
			LOG("epoll_wait: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}

		// loop through all the ready descriptors
		// this includes both the server socket and client sockets
		for (i32 n = 0; n < nfds; ++n) {

			// accept new connections
			if (state->epoll_events[n].data.fd == state->server_fd) {
				
				sockaddr_in client_addr = {0};
				socklen_t client_addr_len = sizeof(client_addr);
				i32 client_fd = accept4(
					state->server_fd, 
					(sockaddr*) &client_addr, 
					&client_addr_len, 
					SOCK_NONBLOCK | SOCK_CLOEXEC);
				if (client_fd == -1) {
					LOG("accept: %s", strerror(errno));
					continue;
				}
				
				// setnonblocking(client_fd)
				epoll_event ev = {
					.events = EPOLLIN | EPOLLET,
					.data.fd = client_fd
				};
				if (epoll_ctl(state->epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
					LOG("epoll_ctl_add, client_fd: %s", strerror(errno));
					continue;
				}
			}

			// process a client connection
			else {
				// read request
				server_process_request_nonblocking(state, state->epoll_events[n].data.fd);
			}
		}
	}
}

void server_run(ServerState* state) {

	server_bind(state);

	server_listen(state);
	
	server_loop(state);
}



