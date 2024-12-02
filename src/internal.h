
#define u32 uint32_t
#define i32 int32_t
#define ARRAY_SIZE(array_name) ((i32)(sizeof(array_name) / sizeof(array_name[0])))

#if defined(TEST_BUILD)
char global__test_last_log[1024];
#define LOG(msg, ...) snprintf(global__test_last_log, ARRAY_SIZE(global__test_last_log), msg, ##__VA_ARGS__)
#define LASTLOG global__test_last_log
#define CLEAR_LASTLOG() memset(global__test_last_log, 0, ARRAY_SIZE(global__test_last_log))
#else
#define LOG(msg, ...) printf(msg "\n", ##__VA_ARGS__)
#endif

#include "http/content_types.c"
#include "http/status_codes.c"
#include "http/headers.c"
#include "http/context.c"
#include "http/send.c"
#include "server.c"
#include "middleware/error_handler.c"
#include "middleware/static_file_handler.c"

