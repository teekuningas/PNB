#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <stdio.h>

// Simple test framework macros
#define TEST_PASSED 0
#define TEST_FAILED 1

extern int tests_run;
extern int tests_failed;

#define RUN_TEST(test_func) do { \
	printf("Running: %s ... ", #test_func); \
	tests_run++; \
	if (test_func() == TEST_PASSED) { \
		printf("PASSED\n"); \
	} else { \
		printf("FAILED\n"); \
		tests_failed++; \
	} \
} while(0)

#define ASSERT(condition, message) do { \
	if (!(condition)) { \
		printf("\n  Assertion failed: %s\n  %s:%d\n", message, __FILE__, __LINE__); \
		return TEST_FAILED; \
	} \
} while(0)

#define ASSERT_EQ(expected, actual, message) do { \
	if ((expected) != (actual)) { \
		printf("\n  Assertion failed: %s\n  Expected: %d, Got: %d\n  %s:%d\n", \
		       message, (int)(expected), (int)(actual), __FILE__, __LINE__); \
		return TEST_FAILED; \
	} \
} while(0)

#define ASSERT_STR_EQ(expected, actual, message) do { \
	if (strcmp((expected), (actual)) != 0) { \
		printf("\n  Assertion failed: %s\n  Expected: \"%s\", Got: \"%s\"\n  %s:%d\n", \
		       message, (expected), (actual), __FILE__, __LINE__); \
		return TEST_FAILED; \
	} \
} while(0)

#define ASSERT_NOT_NULL(ptr, message) ASSERT((ptr) != NULL, message)
#define ASSERT_TRUE(condition, message) ASSERT((condition), message)

#endif /* TEST_HELPERS_H */
