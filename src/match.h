#ifndef MATCH_H
#define MATCH_H MATCH_H

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef double score_t;
#define SCORE_MAX INFINITY
#define SCORE_MIN -INFINITY

#define MATCH_MAX_LEN 1024

int has_match_fuzzy(const char *needle, const char *haystack);
int has_match_linear(const char *needle, const char *haystack);
int has_exact_linear(const char *s1, const char *s2);
score_t match_positions(const char *needle, const char *haystack, size_t *positions);
score_t match(const char *needle, const char *haystack);

#ifdef __cplusplus
}
#endif

#endif
