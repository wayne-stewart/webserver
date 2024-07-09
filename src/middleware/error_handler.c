
/*
 	Error Handler
*/


void middleware_error_handler(ServerState* state, HttpContext* context, MiddlewareHandler* next) {
	if (next) {
		next->run(state, context, next->next);
		if (context->response.status_code >= 400) {
			// send pretty error page
		}
	}
	if (context->response.status_code == 0) {
		// send an error about no handler found
	}
}


