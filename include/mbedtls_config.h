/* Workaround for some mbedtls source files using INT_MAX without including limits.h */
#include <limits.h>

#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA256_SMALLER
#define MBEDTLS_AES_C
#define MBEDTLS_AES_FEWER_TABLES
