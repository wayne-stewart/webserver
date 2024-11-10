
#define u32 uint32_t
#define i32 int32_t
#define ARRAY_SIZE(array_name) ((i32)(sizeof(array_name) / sizeof(array_name[0])))
#define LOG(msg, ...) printf(msg "\n", ##__VA_ARGS__)

#include "http/content_types.c"
#include "http/status_codes.c"
#include "http/headers.c"
#include "http/context.c"
#include "http/send.c"
#include "server.c"
#include "middleware/error_handler.c"
#include "middleware/static_file_handler.c"

