
typedef struct Buffer { 
	u8  data[BUFFER_SIZE];
	u32 reserved; 	// always set to 0 to act as buffer data sentinel
	u32 size;		// the size of the data
	u32 length;		// the amount of bytes written into data
} Buffer;

void buffer_init(Buffer* buffer) {
	buffer->size = BUFFER_SIZE;
}

/*
	buffer_writef
	writes the fomatted string into the buffer data
	returns 0 if data was written completely
	returns -1 if no data was written
	returns bytes written if data was partially written
*/
u8 buffer_writef(Buffer* buffer, const u8* format, ...) {
	va_list args;
	va_start(args, format);
	u8* data = buffer->data + buffer->length;
	u32 size = buffer->size - buffer->length;
	if (size == 0) return -1;
	u32 written = vsnprintf(data, size, format, args);
	buffer->length += written;
	if (written > 0 && written < size) return 0;
	va_end(args);
	return written;
}

typedef struct HttpHeaders {
	HttpContentTypes content_type;
	int content_length;
	char* location;
} HttpHeaders;

/*
 * Write headers into the buffer
 * returns 0 if headers don't fit into the buffer
 * returns 1 if headers were written successfully
 */
int http_write_headers(
	Buffer* buffer,
	int status_code,
	HttpHeaders* headers) {

	// status line
	const char* status_message = http_get_status_code_message(status_code);
	buffer_writef(buffer, "HTTP/1.1 %d %s\r\n", status_code, status_message);

	// redirect shortcuts to the end
	if ((status_code == 301 || status_code == 302) && headers->location) {
		buffer_writef(buffer, "Location: %s\r\n", headers->location);
		goto FINAL_NEWLINE;
	}

	// content type
	const char* type_string = http_get_content_type_text(headers->content_type);
	buffer_writef(buffer, "Content-Type: %s\r\n", type_string);
	
	// content length
	buffer_writef(buffer, "Content-Length: %d\r\n", headers->content_length);
	
FINAL_NEWLINE:
	buffer_writef(buffer, "\r\n"); 
}


