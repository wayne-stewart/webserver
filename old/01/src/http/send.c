
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

void send_400(HttpContext* context)
{
	context->response.status_code = 400;
	memset(&context->response.headers, 0, sizeof(context->response.headers));
	context->response.headers.content_length = 0;
	context->response.headers.content_type = CONTENT_TYPE_PLAIN;
	http_write_headers(
		&context->write_buffer,
		context->response.status_code,
		&context->response.headers);
	send(context->client_fd, context->write_buffer.data, context->write_buffer.length, 0);
}

void send_404(HttpContext* context)
{
	context->response.status_code = 404;
	memset(&context->response.headers, 0, sizeof(context->response.headers));
	context->response.headers.content_length = 0;
	context->response.headers.content_type = CONTENT_TYPE_PLAIN;
	http_write_headers(
		&context->write_buffer,
		context->response.status_code, 
		&context->response.headers);
	send(context->client_fd, context->write_buffer.data, context->write_buffer.length, 0);
}

void send_302(HttpContext* context, char* path)
{
	context->response.status_code = 302;
	memset(&context->response.headers, 0, sizeof(context->response.headers));
	context->response.headers.location = path;
	http_write_headers(
		&context->write_buffer, 
		context->response.status_code, 
		&context->response.headers);
	send(context->client_fd, context->write_buffer.data, context->write_buffer.length, 0);
}


