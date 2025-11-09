
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
	CONTENT_TYPE_WEBP,	
	CONTENT_TYPE_BINARY,	
	CONTENT_TYPE_WASM,	
	CONTENT_TYPE_JSON,	
	CONTENT_TYPE_PDF,		
	CONTENT_TYPE_OTF,		
	CONTENT_TYPE_TTF,		
	CONTENT_TYPE_WOFF,	
	CONTENT_TYPE_WOFF2,	
	CONTENT_TYPE_WEBA,	
	CONTENT_TYPE_MP3,		
	CONTENT_TYPE_AAC,		
	CONTENT_TYPE_WEBM,	
	CONTENT_TYPE_MP4,

	CONTENT_TYPES_COUNT
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
	{ CONTENT_TYPE_WEBP,	".webp",	"image/webp" },
	{ CONTENT_TYPE_BINARY,	"",			"application/octet-stream" },
	{ CONTENT_TYPE_WASM,	".wasm",	"application/wasm" },
	{ CONTENT_TYPE_JSON,	".json",	"application/json" },
	{ CONTENT_TYPE_PDF,		".pdf",		"application/pdf" },
	{ CONTENT_TYPE_OTF,		".otf",		"font/otf" },
	{ CONTENT_TYPE_TTF,		".ttf",		"font/ttf" },
	{ CONTENT_TYPE_WOFF,	".woff",	"font/woff" },
	{ CONTENT_TYPE_WOFF2,	".woff2",	"font/woff2" },
	{ CONTENT_TYPE_WEBA,	".weba",	"audio/webm" },
	{ CONTENT_TYPE_MP3,		".mp3",		"audio/mpeg" },
	{ CONTENT_TYPE_AAC,		".aac",		"audio/aac" },
	{ CONTENT_TYPE_WEBM,	".webm",	"video/webm" },
	{ CONTENT_TYPE_MP4,		".mp4",		"video/mp4" }
};

HttpContentTypes http_get_content_type_from_file_path(const char* file_path)
{
	i32 path_length = strlen(file_path);
	for(i32 i = 0; i < CONTENT_TYPES_COUNT; i++) {
		i32 file_ext_len = strlen(CONTENT_TYPES[i].file_extension);
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
	for (i32 i = 0; i < CONTENT_TYPES_COUNT; i++) {
		if (CONTENT_TYPES[i].type_id == type_id) {
			return CONTENT_TYPES[i].text;
		}
	}
	return "text/plain";
}



