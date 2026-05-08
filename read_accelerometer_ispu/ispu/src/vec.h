/**
 * Implementation of a vector data structure for static memory allocation.
 * 
 * @author Francesco Saccani
 */

#ifndef __TINY_RBF_VEC_H__
#define __TINY_RBF_VEC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "constants.h"

/**
 * Vector data structure using static memory allocation.
 */
typedef struct {
    float data[MAX_NUM_NODES];
    uint32_t size;
} vec_t;

/**
 * Check if the vector is empty
 * 
 * @param vec the vector
 * @return true if the vector is empty, false otherwise
 */
static inline bool is_empty(vec_t *vec) {
    return vec->size == 0;
}

/**
 * Check if the vector is full
 * 
 * @param vec the vector
 * @return true if the vector is full, false otherwise
 */
static inline bool is_full(vec_t *vec) {
    return vec->size == MAX_NUM_NODES;
}

/**
 * Add an item to the end of the vector
 * 
 * @param vec the vector
 * @param item the item to add
 * @return true if the item has been added, false otherwise
 */
static inline bool add_item(vec_t *vec, float item) {
    if (is_full(vec)) {
        return false; // Cannot add, vector is full
    }
    vec->data[vec->size] = item;
    vec->size++;
    return true;
}

/**
 * Remove the item at a specified index
 * 
 * @param vec the vector
 * @param index the index of the item to remove
 * @return true if the item has been removed, false otherwise
 */
static inline bool remove_at(vec_t *vec, uint32_t index) {
    if (index >= vec->size) {
        // Index out of bounds
        return false;
    }
    // Shift elements to the left to overwrite the element at index
    for (uint32_t i = index; i < vec->size - 1; i++) {
        vec->data[i] = vec->data[i + 1];
    }
    vec->size--;
    return true;
}

#ifdef __cplusplus
}
#endif

#endif // __TINY_RBF_VEC_H__
