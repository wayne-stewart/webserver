#ifndef __TEST_H__
#define __TEST_H__

typedef struct TestContext TestContext;
typedef void (*TestFunction)(TestContext*);
typedef struct TestContext {
	const char* name;
	TestFunction test_func;
	bool result;
	char last_log[1024];
} TestContext;

TestContext* test_contexts = 0;
int test_capacity = 0;
int test_count = 0;

void ADD_TEST(const char* name, TestFunction test_func) {
	if (test_capacity == test_count) {
		if (test_capacity == 0) {
			test_capacity = 1024;
			test_contexts = malloc(test_capacity * sizeof(TestContext));
			if (test_contexts == 0) {
				LOG("Unable to allocate test buffer while adding: %s", name);
				exit(1);
			}
		} else {
			int new_capacity = (int)(test_capacity + 1024);
			TestContext* realloc_result = realloc(test_contexts, new_capacity);
			if (realloc_result == 0) {
				LOG("Unable to reallocate test buffer while adding: %s", name);
				exit(1);
			}
			test_capacity = new_capacity;
			test_contexts = realloc_result;
		}
	}
	TestContext* test_context = &test_contexts[test_count];
	test_context->result = false;
	test_context->name = name;
	test_context->test_func = test_func;
	memset(test_context->last_log, 0, ARRAY_SIZE(test_context->last_log));
	test_count++;
}

#define CAT(x,y) x ## y

#define BEGIN_TEST2(name, line_no) void CAT(TEST_, line_no)(TestContext* test_context); \
__attribute__((constructor)) void CAT(TEST_LOAD_, line_no)() { ADD_TEST(name, CAT(TEST_, line_no)); } \
void CAT(TEST_, line_no)(TestContext* test_context) { \
	CLEAR_LASTLOG(); \

#define BEGIN_TEST(name) BEGIN_TEST2(name, __LINE__)

#define END_TEST }

#define TEST_EXPECT_ONE(arg) test_context->result =  (((arg) == 1) ? 1 : 0)
#define TEST_EXPECT_ZERO(arg) test_context->result = (((arg) == 0) ? 1 : 0)
#define TEST_EXPECT_EQUALS(arg1, arg2) test_context->result = (((arg1) == (arg2)) ? 1 : 0)
#define TEST_EXPECT_STRINGS_EQUAL(arg1, arg2) test_context->result = ((strcmp((arg1), (arg2)) == 0) ? 1 : 0)

#endif

