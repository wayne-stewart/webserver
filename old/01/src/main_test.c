#include "external.h"
#include "config.h"
#include "internal.h"
#include "test.h"
#include "test/url_validation_tests.c"

#define RED   "\e[1;31m"
#define GREEN "\e[1;32m"
#define RESET "\e[0m"

int main(int argc, char** argv) {
	printf("starting test runner\n");

	int successes = 0;
	int failures = 0;

	for(int i = 0; i < test_count; i++) {
		TestContext* context = &test_contexts[i];
		context->test_func(context);
		if (context->result) {
			printf(GREEN"SUCCESS: %s"RESET"\n", context->name);
			successes += 1;
		} else {
			printf(RED  "FAILED:  %s"RESET"\n", context->name);
			failures += 1;
		}
	}

	printf(GREEN"Successes: %d "RED"Failures %d\n", successes, failures);
}

