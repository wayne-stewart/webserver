#include "external.h"
#include "config.h"
#include "internal.h"
#include "test.h"
#include "test/url_validation_tests.c"

int main(int argc, char** argv) {
	printf("starting test runner\n");
	for(int i = 0; i < test_count; i++) {
		TestContext* context = &test_contexts[i];
		context->test_func(context);
		if (context->result) {
			printf("\e[1;32mSUCCESS: %s\e[0m\n", context->name);
		} else {
			printf("\e[1:31mFAILED:  %s\e[0m\n", context->name);
		}
	}
}

