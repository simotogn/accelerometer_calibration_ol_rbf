/**
 * Utility print functions.
 * 
 * @author Francesco Saccani <francesco.saccani@unipr.it>
 */

#ifndef __TINY_RBF_PRINT_UTILS_H__
#define __TINY_RBF_PRINT_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "constants.h"
#include "network.h"
#include "node.h"
#include "vec.h"
#include "weight_matrix.h"

/**
 * Print an array of floats.
 * 
 * @param array the array
 * @param size the size of the array
 */
void print_array(const float *array, uint32_t size) {
    printf("[ ");
    for (uint32_t i = 0; i < size; i++) {
        printf("%f ", array[i]);
    }
    printf("]");
}

/**
 * Print a vector.
 * 
 * @param vec the vector
 */
void print_vec(const vec_t *vec) {
    printf("[ ");
    for (uint32_t i = 0; i < vec->size; i++) {
        printf("%f ", vec->data[i]);
    }
    printf("]");
}

/**
 * Print the center and radius of a node.
 * 
 * @param node the node
 */
void print_node(const node_t *node) {
    printf("Center: ");
    print_array(node->center, NUM_INPUTS);
    printf(" - Radius: %f", node->radius);
}

/**
 * Print the weight matrix.
 * 
 * @param matrix the weight matrix
 */
void print_weight_matrix(const weight_matrix_t *matrix) {
    printf("Weights: [\n");
    for (uint32_t i = 0; i < NUM_OUTPUTS; i++) {
        printf("\t");
        print_array(matrix->data[i], matrix->n_cols);
        printf("\n");
    }
    printf("]");
}

/**
 * Print the nodes, weights and biases of a network.
 * 
 * @param network the network
 */
void print_network(const network_t *network) {
    printf("Nodes:\n");
    for (uint32_t i = 0; i < network->nodes.size; i++) {
        printf("\tNode %d: ", i);
        print_node(&network->nodes.data[i]);
        printf("\n");
    }
    printf("Biases: ");
    print_array(network->bias_weights, NUM_OUTPUTS);
    printf("\n");
    print_weight_matrix(&network->weights);
    printf("\n");
}

#ifdef __cplusplus
}
#endif

#endif // __TINY_RBF_PRINT_UTILS_H__
