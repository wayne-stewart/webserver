

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

