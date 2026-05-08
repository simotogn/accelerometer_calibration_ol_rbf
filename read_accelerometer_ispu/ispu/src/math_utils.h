/**
 * Implementation of mathematical functions.
 * 
 * @author Francesco Saccani
 */

#ifndef __TINY_RBF_MATH_H__
#define __TINY_RBF_MATH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * Perform element-wise addition of two vectors.
 * 
 * @param a the first vector
 * @param b the second vector
 * @param c the result vector
 * @param n the number of elements in the vectors
 */
static inline void add(const float *a, const float *b, float *c, const uint32_t n) {
    for (uint32_t i = 0; i < n; i++) {
        c[i] = a[i] + b[i];
    }
}

/**
 * Perform element-wise subtraction of two vectors.
 * 
 * @param a the first vector
 * @param b the second vector
 * @param c the result vector
 * @param n the number of elements in the vectors
 */
static inline void sub(const float *a, const float *b, float *c, const uint32_t n) {
    for (uint32_t i = 0; i < n; i++) {
        c[i] = a[i] - b[i];
    }
}

/**
 * Perform the dot product of two vectors.
 * 
 * @param a the first vector
 * @param b the second vector
 * @param n the number of elements in the vectors
 * @return the dot product
 */
static inline float dot(const float *a, const float *b, const uint32_t n) {
    float result = 0.0f;
    for (uint32_t i = 0; i < n; i++) {
        result += a[i] * b[i];
    }
    return result;
}

#ifdef __cplusplus
}
#endif

#endif // __TINY_RBF_MATH_H__
