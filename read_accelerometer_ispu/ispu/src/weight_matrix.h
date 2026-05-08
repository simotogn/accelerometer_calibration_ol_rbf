/**
 * Implementation of the matrix of weights for the TinyRBF network.
 * 
 * @author Francesco Saccani <francesco.saccani@unipr.it>
 */

#ifndef __TINY_RBF_WEIGHT_MATRIX_H__
#define __TINY_RBF_WEIGHT_MATRIX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "constants.h"
#include "math_utils.h"
#include "vec.h"

/**
 * Weight matrix data structure.
 * 
 * The number of rows is fixed to NUM_OUTPUTS, while the number of columns is
 * variable and defined by the field `n_cols`, up to a maximum of MAX_NUM_NODES.
 */
typedef struct {
    // The values of the matrix.
    float data[NUM_OUTPUTS][MAX_NUM_NODES];
    // The number of columns of the matrix.
    uint32_t n_cols;
} weight_matrix_t;

/**
 * Perform the matrix-vector multiplication.
 * 
 * @param matrix the weight matrix
 * @param vector the input vector
 * @param result the output vector
 */
static inline void matrix_vector_mul(weight_matrix_t *matrix, vec_t *vector, float *result) {
    for (uint32_t i = 0; i < NUM_OUTPUTS; i++) {
        result[i] = dot(matrix->data[i], vector->data, matrix->n_cols);
    }
}

/**
 * Add a column to the weight matrix.
 * 
 * @param matrix the weight matrix
 * @param column the column to add
 * @return true if the column has been added, false otherwise
 */
static inline bool add_column(weight_matrix_t *matrix, const float *column) {
    if (matrix->n_cols == MAX_NUM_NODES) {
        // Cannot add, matrix is full
        return false;
    }
    for (uint32_t i = 0; i < NUM_OUTPUTS; i++) {
        matrix->data[i][matrix->n_cols] = column[i];
    }
    matrix->n_cols++;
    return true;
}

/**
 * Remove a column at the given index from the weight matrix.
 * 
 * @param matrix the weight matrix
 * @param index the index of the column to remove
 * @return true if the column has been removed, false otherwise
 */
static inline bool remove_column_at(weight_matrix_t *matrix, uint32_t index) {
    if (index >= matrix->n_cols) {
        // Index out of bounds
        return false;
    }
    for (uint32_t i = 0; i < NUM_OUTPUTS; i++) {
        for (uint32_t j = index; j < matrix->n_cols - 1; j++) {
            matrix->data[i][j] = matrix->data[i][j + 1];
        }
    }
    matrix->n_cols--;
    return true;
}

#ifdef __cplusplus
}
#endif

#endif // __TINY_RBF_WEIGHT_MATRIX_H__
