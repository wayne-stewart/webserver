/*
	Server
*/

typedef struct HttpResponse {
	int status_code;
	HttpHeaders headers;
} HttpResponse;

typedef struct HttpRequest {
	const char* method;
	const char* uri;
	const char* http_version;
} HttpRequest;

typedef struct HttpContext {
	Buffer read_buffer;
	Buffer write_buffer;
	HttpRequest request;
	HttpResponse response;
	int client_fd;
} HttpContext;

typedef struct CompiledRegex {
	regex_t request_line;
	regex_t uri_double_dots;
	regex_t uri_valid_chars;
} CompiledRegex;

typedef struct MiddlewareHandler MiddlewareHandler;
typedef struct ServerState ServerState;

typedef struct MiddlewareHandler {
	struct MiddlewareHandler* next;
	void (*run)(ServerState*, HttpContext*, MiddlewareHandler*);
} MiddlewareHandler;

typedef struct ServerState {
	int server_fd;
	CompiledRegex regex;
	MiddlewareHandler* middleware;
} ServerState;

void send_file(HttpContext* context, int file_fd, const char* file_path)
{
	struct stat file_stat;
	fstat(file_fd, &file_stat);
	off_t file_size = file_stat.st_size;
	context->response.status_code = 200;
	context->response.headers.content_length = file_size;
	context->response.headers.content_type = http_get_content_type_from_file_path(file_path);
	http_write_headers(
		&context->write_buffer, 
		context->response.status_code, 
		&context->response.headers);
	Buffer* buffer = &context->write_buffer;
	if (buffer->length > 0) {
		send(context->client_fd, buffer->data, buffer->length, 0);
		buffer->length = 0;
	}
	while((buffer->length = read(file_fd, buffer->data, buffer->size)) > 0) {
		send(context->client_fd, buffer->data, buffer->length, 0);
	}
}

void send_404(HttpContext* context)
{
	context->response.status_code = 404;
	context->response.headers.content_length = 0;
	context->response.headers.content_type = CONTENT_TYPE_PLAIN;
	http_write_headers(
		&context->write_buffer,
		context->response.status_code, 
		&context->response.headers);
	send(context->client_fd, context->write_buffer.data, context->write_buffer.length, 0);
}

void send_302(HttpContext* context, u8* path)
{
	context->response.status_code = 302;
	context->response.headers.location = path;
	http_write_headers(
		&context->write_buffer, 
		context->response.status_code, 
		&context->response.headers);
	send(context->client_fd, context->write_buffer.data, context->write_buffer.length, 0);
}

int read_request(ServerState* state, HttpContext* context)
{
	context->read_buffer.length = recv(context->client_fd, context->read_buffer.data, context->read_buffer.size, 0);
	if (context->read_buffer.length == 0) { perror("failed to read from network stream"); return 0; }
	
	regmatch_t matches[3];
	if (regexec(&state->regex.request_line, context->read_buffer.data, 3, matches, 0) != 0) return 0;

	context->read_buffer.data[matches[1].rm_eo] = '\0'; // null terminator for method
	context->read_buffer.data[matches[2].rm_eo] = '\0'; // null terminator for URI
	context->request.method = context->read_buffer.data + matches[1].rm_so;
	context->request.uri = context->read_buffer.data + matches[2].rm_so;

	// if double dots are found, request validation fails
	if (regexec(&state->regex.uri_double_dots, context->request.uri, 1, matches, 0) == 0) return 0;

	// make sure there are no unexpected characters in the uri
	if (regexec(&state->regex.uri_valid_chars, context->request.uri, 1, matches, 0) != 0) return 0;

	return 1;
}

void Server_Init(ServerState* state) {
	regcomp(&state->regex.request_line, "^([A-Z]+) (/[^ ]*) HTTP/1.1\r\n", REG_EXTENDED);
	regcomp(&state->regex.uri_double_dots, "\\.\\.", REG_EXTENDED);
	regcomp(&state->regex.uri_valid_chars, "^[0-9a-zA-Z/_\\.-]+$", REG_EXTENDED);
}

void Server_Destroy(ServerState* state) {
	if (state->server_fd) {
		close(state->server_fd);
	}
}

void Server_AddMiddleware(ServerState* state, MiddlewareHandler* handler) {
	if (state->middleware) {
		MiddlewareHandler* parent = state->middleware;
		while (parent->next) parent = parent->next;
		parent->next =  handler;
	} else {
		state->middleware = handler;
	}
}

volatile sig_atomic_t server_is_running = 1;

static void Server_SignalCatch(int signo) {
	if (signo == SIGINT) {
		server_is_running = 0;
	}
}

void Server_Run(ServerState* state) {

	/*if (signal(SIGINT, Server_SignalCatch) == SIG_ERR) {
		perror("Unable to set signal handler!");
		return;
	}*/

	struct sockaddr_in server_addr = {0};

	if ((state->server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("create socket failed");
		return;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	if (bind(state->server_fd,
		(struct sockaddr *)&server_addr,
		sizeof(server_addr)) < 0) {
		perror("socket bind failed");
		return;
	}

	if (listen(state->server_fd, 10) < 0) {
		perror("listening to socket failed");
		return;
	}
	printf("listening on port: %d\n", PORT);

	while(server_is_running) {
		struct sockaddr_in client_addr = {0};
		socklen_t client_addr_len = sizeof(client_addr);

		HttpContext context = {0};
		buffer_init(&context.read_buffer);
		buffer_init(&context.write_buffer);

		context.client_fd = accept(state->server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
		if (context.client_fd < 0) { perror("accept failed"); goto CLEANUP; }
		
		if (read_request(state, &context) != 1) goto CLEANUP;
		
		state->middleware->run(state, &context, state->middleware->next);
		
		CLEANUP:
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
}



