/* Minimal assertion helpers: every failed CHECK is reported, the exit code says pass/fail. */
#ifndef OSL_TEST_H
#define OSL_TEST_H

#include <stdio.h>

static int osl_test_failures;

#define CHECK(cond)                                                                \
	do {                                                                           \
		if (!(cond)) {                                                             \
			fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
			osl_test_failures++;                                                   \
		}                                                                          \
	} while (0)

#define CHECK_EQ(a, b)                                                                     \
	do {                                                                                   \
		long long _a = (long long)(a), _b = (long long)(b);                                \
		if (_a != _b) {                                                                    \
			fprintf(stderr, "%s:%d: %s == %s failed: %lld != %lld\n", __FILE__, __LINE__, \
				#a, #b, _a, _b);                                                       \
			osl_test_failures++;                                                           \
		}                                                                                  \
	} while (0)

#define TEST_RESULT() (osl_test_failures ? 1 : 0)

#endif
