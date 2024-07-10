
typedef enum HttpContentTypes {
	CONTENT_TYPE_PLAIN,
	CONTENT_TYPE_HTML,
	CONTENT_TYPE_CSS,
	CONTENT_TYPE_JS,
	CONTENT_TYPE_JPG,
	CONTENT_TYPE_PNG,
	CONTENT_TYPE_GIF,
	CONTENT_TYPE_ICO,
	CONTENT_TYPE_SVG,
	CONTENT_TYPE_WASM
} HttpContentTypes;

typedef struct HttpContentType {
	HttpContentTypes type_id;
	const char* file_extension;
	const char* text;
} HttpContentType;

const HttpContentType CONTENT_TYPES[] = {
	{ CONTENT_TYPE_PLAIN,	".txt",		"text/plain" },
	{ CONTENT_TYPE_HTML,	".html",	"text/html" },
	{ CONTENT_TYPE_CSS,		".css",		"text/css" },
	{ CONTENT_TYPE_JS,		".js",		"text/javascript" },
	{ CONTENT_TYPE_JPG,		".jpg",		"image/jpeg" },
	{ CONTENT_TYPE_PNG,		".png",		"image/png" },
	{ CONTENT_TYPE_GIF,		".gif",		"image/gif" },
	{ CONTENT_TYPE_ICO,		".ico",		"image/x-icon" },
	{ CONTENT_TYPE_SVG,		".svg",		"image/svg+xml" },
	{ CONTENT_TYPE_WASM,	".wasm",	"application/wasm" }
};

HttpContentTypes http_get_content_type_from_file_path(const char* file_path)
{
	s32 path_length = strlen(file_path);
	for(s32 i = 0; i < ARRAY_SIZE(CONTENT_TYPES); i++) {
		s32 file_ext_len = strlen(CONTENT_TYPES[i].file_extension);
		if (path_length > file_ext_len) {
			if (strcmp(CONTENT_TYPES[i].file_extension, &file_path[path_length - file_ext_len]) == 0) {
				return CONTENT_TYPES[i].type_id;
			}
		}
	}
	return CONTENT_TYPE_PLAIN;
}

const char* http_get_content_type_text(HttpContentTypes type_id)
{
	for (s32 i = 0; i < ARRAY_SIZE(CONTENT_TYPES); i++) {
		if (CONTENT_TYPES[i].type_id == type_id) {
			return CONTENT_TYPES[i].text;
		}
	}
	return "text/plain";
}



