

typedef struct HttpResponse {
	i32 status_code;
	HttpHeaders headers;
} HttpResponse;

typedef struct HttpRequest {
	const char* method;
	const char* uri;
	const char* http_version;
} HttpRequest;

typedef enum HttpContextState {
	HttpContextState_Created,
	HttpContextState_EOHFound,
	HttpContextState_ReadingDone,
	HttpContextState_Sending,
	HttpContextState_Complete
} HttpContextState;

typedef struct HttpContext {
	Buffer read_buffer;
	Buffer write_buffer;
	HttpRequest request;
	HttpResponse response;
	i32 client_fd;
	HttpContextState state;
} HttpContext;

