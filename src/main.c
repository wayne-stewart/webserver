#define GNUSOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#define u8	uint8_t
#define s16 int16_t
#define s32 int32_t
#define s64 int64_t

#define PORT 80
#define ACCEPT_BACKLOG 20
#define MAX_CONNECTIONS 100
#define EVENT_QUEUE_SIZE ((MAX_CONNECTIONS * 2) + 10)
#define HTTP_BUFFER_SIZE 4096

typedef struct {
	int socket_fd;
	int keepalive_ts;
} SocketConnection;

typedef enum HTTP_VERB {
	HTTP_VERB_NOT_SET,
	HTTP_VERB_GET,
	HTTP_VERB_POST,
	HTTP_VERB_HEAD,
} HTTP_VERB;

typedef enum HTTP_CONTENT_TYPE {
	HTTP_CONTENT_TYPE_NOT_SET,
	HTTP_CONTENT_TYPE_TEXT,
	HTTP_CONTENT_TYPE_CSS,
	HTTP_CONTENT_TYPE_JS,
	HTTP_CONTENT_TYPE_HTML,
} HTTP_CONTENT_TYPE;

typedef struct {
	s64 length;
	u8* data;
} String;

typedef struct {
	s64 length;
	s64 size;
	u8* data;
} Buffer;

typedef struct {
	HTTP_CONTENT_TYPE content_type;
	s64 content_length;
	s32 keep_alive;
} HttpHeaders;

typedef struct {
	HttpHeaders headers;
	Buffer buffer;
	HTTP_VERB verb;
	String path;
	String query_string;
	String http_version;
	String content;
} HttpRequest;

typedef struct {
	s32 status_code;
	HttpHeaders headers;
	Buffer buffer;
} HttpResponse;

typedef struct {
	int start_ts;
	int end_ts;
	int keepalive_ts;
	int bytes_in;
	int bytes_out;
} HttpMetrics;

typedef struct HttpContext {
	int client_socket;
	HttpMetrics metrics;
	HttpRequest request;
	HttpResponse response;
} HttpContext;

typedef struct AsyncContext {
	int kqueue;
	int size;
	int count;
	struct kevent changed[EVENT_QUEUE_SIZE];
} AsyncContext;

typedef struct ServerContext {
	int http_port;
	int listen_socket;
	AsyncContext async_context;
	HttpContext http_contexts[MAX_CONNECTIONS];
} ServerContext;

#define ARRAY_SIZE(x) ((sizeof(x)) / (sizeof((x)[0])))
#define LOG(str) printf(str "\n")
#define LOGf(fmt, ...) printf(fmt "\n", __VA_ARGS__)
#define DEBUG(str) printf(str "\n")
#define DEBUGf(fmt, ...) printf(fmt "\n", __VA_ARGS__)
#define ASSERT(condition, str) if (condition) { \
	printf(str "\n"); \
	exit(EXIT_FAILURE); \
}
#define ASSERTf(condition, fmt, ...) if (condition) { \
	printf(fmt "\n", __VA_ARGS__); \
	exit(EXIT_FAILURE); \
}

void server_bind(ServerContext *ctx);
void server_listen(ServerContext *ctx);
void server_loop(ServerContext *ctx);
void server_socket_monitor(ServerContext *ctx, int client_fd);
void server_socket_unmonitor(ServerContext *ctx, int client_fd);
void server_timer(ServerContext *ctx, int timer_id, int millis);
void server_timer_handler(ServerContext *ctx, int timer_id);
void server_listen_handler(ServerContext *ctx);
void server_read_handler(ServerContext *ctx, int client_fd);
void server_write_handler(ServerContext *ctx, int client_fd);
HttpContext* server_find_http_context(ServerContext *ctx, int client_fd);
HttpContext* server_create_http_context(ServerContext *ctx);
void server_remove_http_context(ServerContext *ctx, int client_fd);
void http_parse_request_header(HttpContext *ctx);

int main(void) {

	ServerContext server_context = { 
		.http_port = PORT,
		.async_context = { .size = EVENT_QUEUE_SIZE }
	};

	server_bind(&server_context);

	server_listen(&server_context);

	server_context.async_context.kqueue = kqueue();
	ASSERT(server_context.async_context.kqueue == -1, "kqueue()");
	
	server_timer(&server_context, 1, 5000);
	server_socket_monitor(&server_context, server_context.listen_socket);

	server_loop(&server_context);

	exit(EXIT_SUCCESS);
}

/*
void __fatal_error_args(const char *fmt, va_list args) {
	fprintf(stderr, fmt, args);
}
void fatal_error(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	__fatal_error_args(fmt, args);
	va_end(args);
	exit(EXIT_FAILURE);
}
*/

void server_bind(ServerContext *ctx) {
	struct addrinfo hints = {0};
	struct addrinfo *myaddr;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	char s_port[20] = {0};
	snprintf(s_port, ARRAY_SIZE(s_port), "%d", ctx->http_port);
	int status = getaddrinfo(NULL, s_port, &hints, &myaddr);
	ASSERTf(status != 0, "getaddrinfo: %s", gai_strerror(status));
	ctx->listen_socket = socket(
		myaddr->ai_family, 
		myaddr->ai_socktype, 
		myaddr->ai_protocol);
	ASSERTf(ctx->listen_socket == -1, "server socket: %s", strerror(errno));
	int flags = fcntl(ctx->listen_socket, F_GETFL, 0);
	flags = flags & ~O_NONBLOCK & ~FD_CLOEXEC;
	fcntl(ctx->listen_socket, F_SETFL, flags);
	int bind_result = bind(
		ctx->listen_socket,
		myaddr->ai_addr,
		myaddr->ai_addrlen);
	ASSERTf(bind_result == -1, "bind: %s", strerror(errno));
	int yes = 1;
	int opt_result = setsockopt(
		ctx->listen_socket, 
		SOL_SOCKET, 
		SO_REUSEADDR,
		&yes,
		sizeof yes);
	ASSERTf(opt_result == -1, "set re-use socket: %s", strerror(errno));
}

void server_listen(ServerContext *ctx) {
	int listen_result = listen(ctx->listen_socket, ACCEPT_BACKLOG);
	ASSERTf(listen_result == -1, "listen: %s", strerror(errno));
}

void server_loop(ServerContext *ctx) {
	AsyncContext *actx = &ctx->async_context;

	for(;;) {
		int change_count = kevent(actx->kqueue, 0, 0, actx->changed, actx->size, 0);

		ASSERT(change_count < 0, "kevent()");

		for(int i = 0; i < change_count; i++) {
			struct kevent *event = &actx->changed[i];
			if (event->flags & EV_EOF) {
				LOGf("Event Id: %lu, Disconnected", event->ident);
				server_socket_unmonitor(ctx, event->ident);
			}
			else if (event->flags & EV_ERROR) {
				LOGf("Event Id: %lu Error: %s", event->ident, strerror(event->data));
				server_socket_unmonitor(ctx, event->ident);
				close(event->ident);
			}
			else if (event->ident == 1 && event->filter == EVFILT_TIMER) {
				server_timer_handler(ctx, event->ident);
			}
			else if ((int)event->ident == ctx->listen_socket && event->filter == EVFILT_READ) {
				server_listen_handler(ctx);
			}
			else if (event->filter == EVFILT_READ) {
				server_read_handler(ctx, event->ident);
			}
			else if (event->filter == EVFILT_WRITE) {
				server_write_handler(ctx, event->ident);
			}
		}
	}

	close(actx->kqueue);
}

void server_socket_monitor(ServerContext *ctx, int socket_fd) {
	struct kevent evset[1] = {0};
	EV_SET(&evset[0], socket_fd, EVFILT_READ, EV_ADD, 0, 0, 0);
	//EV_SET(&evset[1], socket_fd, EVFILT_WRITE, EV_ADD, 0, 0, 0);
	kevent(ctx->async_context.kqueue, evset, 1, 0, 0, 0);
}

void server_socket_unmonitor(ServerContext *ctx, int socket_fd) {
	struct kevent evset[2] = {0};
	EV_SET(&evset[0], socket_fd, EVFILT_READ, EV_DELETE, 0, 0, 0);
	EV_SET(&evset[1], socket_fd, EVFILT_WRITE, EV_DELETE, 0, 0, 0);
	kevent(ctx->async_context.kqueue, evset, 2, 0, 0, 0);
}

void server_timer(ServerContext *ctx, int timer_id, int millis) {
	struct kevent evset[1] = {0};
	EV_SET(&evset[0], timer_id, EVFILT_TIMER, EV_ADD, 0, millis, 0);
	kevent(ctx->async_context.kqueue, evset, 1, 0, 0, 0);
}

void server_timer_handler(ServerContext *ctx, int timer_id) {
	(void)ctx;
	(void)timer_id;
	// pid < 0 ( IN PARENT PROCESS ) indicates error
	// pid == 0 Lets us know we are in the child process
	// pid > 0 ( IN PARENT PROCESS ) is id of child process
	pid_t pid;
	if ((pid = fork()) < 0) { 
		LOG("fork() failed");
	}
	else if (pid == 0) {
		execlp("date", "date", (char*)0);
	}
}

void server_listen_handler(ServerContext *ctx) {
	struct sockaddr_in addr = {0};
	socklen_t addr_size = sizeof(addr);
	int client_fd = accept(ctx->listen_socket, (struct sockaddr*)&addr, &addr_size);
	if (client_fd > 0) {
		LOGf("client connected: %d", client_fd);
		int set = 1;
		setsockopt(client_fd, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(int));
		HttpContext *http_context = server_create_http_context(ctx);
		if (http_context) {
			int flags = fcntl(client_fd, F_GETFL, 0);
			fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
			http_context->client_socket = client_fd;
			server_socket_monitor(ctx, client_fd);
		}
		else {
			const char *response = "HTTP/1.1 503 Service Unavailable\r\n\r\n";
			write(client_fd, response, strlen(response));
			close(client_fd);
		}
	}
}

void server_read_handler(ServerContext* server_ctx, int client_fd) {
	HttpContext* ctx = server_find_http_context(server_ctx, client_fd);
	if (!ctx) {
		LOG("ERROR: client_fd does not have a matching HttpContext!");
		return;
	}
	int r = 1;
	Buffer* buf = &ctx->request.buffer;
	while (buf->length < buf->size && r > 0) { 
		r = read(client_fd, buf->data + buf->length, buf->size - buf->length);
		if (r == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				DEBUG("READ EAGAIN");
			}
			else {
				DEBUGf("READ FAILURE: %s", strerror(errno));
			}
		}
		else {
			//ctx->activity_ts = 
			buf->length += r;
			DEBUGf("%s", buf->data);
		}
	}
}

#define g_size 1000000
#define g_bufsize 4096
int g_one = g_size;
void server_write_handler(ServerContext *ctx, int client_fd) {
	(void)ctx;
	char buffer[g_bufsize];
	for (int i = 0; i < g_bufsize; i++) { buffer[i] = 'a'; }
	if (g_one == g_size) {
		g_one += snprintf(buffer, g_bufsize, 
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Length: %d\r\n\r\n", g_size);
	}
	int max_iter = 10;
	int iter_c = 0;
	while(g_one > 0) {
		if ((iter_c++) > max_iter) {
			LOG("MAX ITER");
			break;
		}
		int tosend = g_bufsize;
		if (g_one < g_bufsize) {
			tosend = g_one;
		}
		if (g_one < g_size) {
			for (int i = 0; i < g_bufsize; i++) { buffer[i] = 'a'; }
		}
		int sent = write(client_fd, buffer, tosend);
		if (sent > 0) {
			g_one -= sent;
			LOGf("SEND %d with %d remaining", sent, g_one);
		}
		else if (sent == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				LOG("SEND EAGAIN");
				break;
			}
			else {
				LOGf("SEND ERROR: %s", strerror(errno));
				close(client_fd);
				break;
			}
		}
	}
}

HttpContext* server_find_http_context(ServerContext *ctx, int client_fd) {
	for(size_t i = 0; i < ARRAY_SIZE(ctx->http_contexts); i++) {
		if (ctx->http_contexts[i].client_socket == client_fd) {
			return &ctx->http_contexts[i];
		}
	}
	return 0;
}

void buffer_init(Buffer* buf, s64 size) {
	if (buf->data) {
		memset(buf->data, 0, size);
		buf->length = 0;
	}
	else {
		buf->data = calloc(size, sizeof(u8));
		buf->size = size;
	}
}

HttpContext* server_create_http_context(ServerContext *ctx) {
	HttpContext *http_context = server_find_http_context(ctx, 0);
	if (http_context) {
		memset(http_context, 0, sizeof(HttpContext));
		buffer_init(&http_context->request.buffer, HTTP_BUFFER_SIZE);
		buffer_init(&http_context->response.buffer, HTTP_BUFFER_SIZE);
		return http_context;
	}
	return 0;
}

void server_remove_http_context(ServerContext *ctx, int client_fd) {
	HttpContext *http_context = server_find_http_context(ctx, client_fd);
	if (http_context) { 
		http_context->client_socket = 0;
	};
}

s32 index_of_chs(String* str, s64 str_ofs, const char* chs, u8 chs_len) {
	if (chs_len == 1) {
		u8 ch = chs[0];
		for (s64 i = str_ofs; i < str->length; i++) {
			if (ch == str->data[i]) {
				return i;
			}
		}
	}
	for(s64 i = str_ofs; i < str->length; i++) {
		for(u8 j = 0; j < chs_len; j++) {
			if (chs[j] == str->data[i]) {
				return i;
			}
		}
	}
	return -1;
}
bool starts_with_str(String* x, s64 x_ofs, const char* y, u8 y_len) { 
	int len = x->length - x_ofs;
	if (y_len > len) {
		return false;
	}
	for (s64 i = x_ofs; i < x->length && i < y_len; i++) {
		if (x->data[i] != y[i]) {
			return false;
		}
	}
	return true;
}
void http_parse_request_header(HttpContext *ctx) {
	// GET /path_to_resource HTTP/1.1
	Buffer* rbuf = &ctx->request.buffer;
	String str = { .data = rbuf->data, .length = rbuf->length };
	if (starts_with_str(&str, 0, "GET ", 4)) {
		ctx->request.verb = HTTP_VERB_GET;
		rbuf->pos += 4;
	}
}



