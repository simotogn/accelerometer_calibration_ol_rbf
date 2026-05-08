/**
 * Implementation of RBF node.
 * 
 * @author Francesco Saccani
 */

#ifndef __TINY_RBF_NODE_H__
#define __TINY_RBF_NODE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <stdint.h>

#include "constants.h"
#include "math_utils.h"

/**
 * Gaussian Radial Basis Function node.
 */
typedef struct {
    // The center of the RBF node.
    float center[NUM_INPUTS];
    // The radius of the RBF node.
    float radius;
    // Number of times the RBF prouced a normalized activation value below a
    // given threshold consecutively
    uint32_t low_activation_count;
} node_t;

/**
 * Create a new RBF node.
 * 
 * @param center the center of the node
 * @param radius the radius of the node
 * @return the new node
 */
static inline node_t new_node(const float *center, const float radius) {
    node_t node;
    for (uint32_t i = 0; i < NUM_INPUTS; i++) {
        node.center[i] = center[i];
    }
    node.radius = radius;
    node.low_activation_count = 0;
    return node;
}

/**
 * Perform the forward pass of the node.
 * 
 * @param node the node
 * @param input the input vector; must have the size of NUM_INPUTS
 * @return the output of the node
 */
static inline float forward_node(const node_t *node, const float *input) {
    float d[NUM_INPUTS];
    sub(input, node->center, d, NUM_INPUTS);
    float dist = dot(d, d, NUM_INPUTS); // norm squared
    return (float) exp((-dist / (2.0f * node->radius * node->radius)));
}

#ifdef __cplusplus
}
#endif

#endif // __TINY_RBF_NODE_H__
