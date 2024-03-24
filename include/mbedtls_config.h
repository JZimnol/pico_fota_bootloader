/* Workaround for some mbedtls source files using INT_MAX without including limits.h */
#include <limits.h>

#define MBEDTLS_SHA256_SMALLER
#define MBEDTLS_SHA256_C
