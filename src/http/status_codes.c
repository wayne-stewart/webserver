
typedef struct HttpStatusCode {
	int status_code;
	const char* status_message;
} HttpStatusCode;

const HttpStatusCode STATUS_CODES[] = {
	{ 200, "Ok" },
	{ 302, "Found" },
	{ 400, "Bad Request" },
	{ 401, "Unauthorized" },
	{ 403, "Forbidden" },
	{ 404, "Not Found" },
	{ 405, "Method Not Allowed" },
	{ 500, "Internal Server Error" },
	{ 503, "Service Unavailable" }
};

const char* http_get_status_code_message(int status_code)
{
	for(i32 i = 0; i < ARRAY_SIZE(STATUS_CODES); i++) {
		if (STATUS_CODES[i].status_code == status_code) {
			return STATUS_CODES[i].status_message;
		}
	}
	return "Message Not Available";
}


