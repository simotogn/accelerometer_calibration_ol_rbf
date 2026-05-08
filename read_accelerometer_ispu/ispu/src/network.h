/**
 * Implementation of the TinyRBF network.
 * 
 * @author Francesco Saccani <francesco.saccani@unipr.it>
 */

#ifndef __TINY_RBF_H__
#define __TINY_RBF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>

#include "constants.h"
#include "node_list.h"
#include "vec.h"
#include "weight_matrix.h"

/**
 * The TinyRBF network.
 */
typedef struct {
    // The hidden nodes in the network.
    node_list_t nodes;
    // The weights for the bias node.
    float bias_weights[NUM_OUTPUTS];
    /// The weights of the output layer.
    weight_matrix_t weights;

    // Distance threshold. If the distance between the input vector and the
    // nearest RBF node is above this threshold and the prediction error is
    // above the tolerance, a new node is added to the network. Defaults to 1.0.
    float distance_threshold;
    // Prediction error threshold. If the prediction error is above this
    // threshold and the distance to the nearest RBF node is above the
    // distance threshold, a new node is added to the network. Defaults to 0.01.
    float error_threshold;

    // Approximated mean of the distance to the nearest RBF node.
    float distance_mean;
    // Approximated standard deviation of the distance to the nearest RBF node.
    float distance_std;
} network_t;

/**
 * Create a new empty network.
 */
network_t new_empty_network() {
    network_t network = {
        .nodes = {
            .size = 0,
        },
        .bias_weights = {0},
        .weights = {
            .n_cols = 0
        },
        .distance_threshold = 9.5,
        .error_threshold = 5.0,
        .distance_mean = 0.0,
        .distance_std = 0.0,
    };
    return network;
}

/**
 * Update the distance statistics of the network
 *
 * @param network the network
 * @param distance the distance to the nearest RBF node
 */
void _update_distance_stats(network_t *network, float distance) {
    network->distance_mean -= network->distance_mean / AVERAGE_WINDOW;
    network->distance_mean += distance / AVERAGE_WINDOW;
    network->distance_std -= network->distance_std / AVERAGE_WINDOW;
    network->distance_std += (distance - network->distance_mean)
        * (distance - network->distance_mean)
        / AVERAGE_WINDOW;
}

/**
 * Perform the forward pass of the network.
 * 
 * @param network the network
 * @param input the input vector; must have the size of NUM_INPUTS
 * @param output the output vector; must have the size of NUM_OUTPUTS
 */
void inference(network_t *network, const float *input, float *output) {
    // Compute the activations
    vec_t activations = {
        .data = {0},
        .size = network->nodes.size
    };
    for (uint32_t i = 0; i < network->nodes.size; i++) {
        activations.data[i] = forward_node(&network->nodes.data[i], input);
    }

    // Compute the distance to the nearest RBF node and update the statistics
    if (!is_node_list_empty(&network->nodes)) {
        float distance;
        for (uint32_t i = 0; i < network->nodes.size; i++) {
            float dist_vec[NUM_INPUTS];
            sub(input, network->nodes.data[i].center, dist_vec, NUM_INPUTS);
            float dist = sqrtf(dot(dist_vec, dist_vec, NUM_INPUTS));
            if (i == 0 || dist < distance) {
                distance = dist;
            }
        }
        _update_distance_stats(network, distance);
    }
    
    // Compute the outputs
    matrix_vector_mul(&network->weights, &activations, output);
    add(output, network->bias_weights, output, NUM_OUTPUTS);
}

/**
 * Perform the learning pass of the network
 * 
 * @param network the network
 * @param input the input vector; must have the size of NUM_INPUTS
 * @param target the target vector; must have the size of NUM_OUTPUTS
 * @param output the output vector; must have the size of NUM_OUTPUTS
 */
void learning(network_t *network, const float *input, const float *target, float *output) {
    // Compute the activations
    vec_t activations = {
        .data = {0},
        .size = network->nodes.size
    };
    for (uint32_t i = 0; i < network->nodes.size; i++) {
        activations.data[i] = forward_node(&network->nodes.data[i], input);
    }
    
    // Compute the outputs
    matrix_vector_mul(&network->weights, &activations, output);
    add(output, network->bias_weights, output, NUM_OUTPUTS);

    // Compute prediction error.
    float error[NUM_OUTPUTS];
    sub(target, output, error, NUM_OUTPUTS);

    if (is_node_list_empty(&network->nodes)) {
        // === ALLOCATION ===
        // If the network has only the bias node, we add a new RBF node.
        add_node(&network->nodes, new_node(input, network->distance_threshold));
        // Then, we add the corresponding weights for the output layer
        add_column(&network->weights, error);
        return;
    }

    // Compute the distance to the nearest RBF node.
    float distance = 0.0;
    for (uint32_t i = 0; i < network->nodes.size; i++) {
        float dist_vec[NUM_INPUTS];
        sub(input, network->nodes.data[i].center, dist_vec, NUM_INPUTS);
        float dist = sqrtf(dot(dist_vec, dist_vec, NUM_INPUTS));
        if (i == 0 || dist < distance) {
            distance = dist;
        }
    }

    // Update distance threshold using approximated mean and standard deviation.
    _update_distance_stats(network, distance);
    network->distance_threshold = network->distance_mean + (float)2.0 * sqrtf(network->distance_std);

    if (sqrtf(dot(error, error, NUM_OUTPUTS)) > network->error_threshold
        && distance > network->distance_threshold
        && !is_node_list_full(&network->nodes)
    ){
        // === ALLOCATION ===
        // If the prediction error is above the tolerance and the distance
        // to the nearest node is above the threshold, we add a new neuron.
        add_node(&network->nodes, new_node(input, OVERLAP_FACTOR * distance));
        // Then, we add the corresponding weights for the output layer.
        add_column(&network->weights, error);

    } else {
        // === PARAMETERS UPDATE ===
        // Otherwise, we update the parameters of the network.
        for (uint32_t i = 0; i < NUM_OUTPUTS; i++) {
            network->bias_weights[i] += LEARNING_RATE * error[i];
        }
        for (uint32_t i = 0; i < network->nodes.size; i++) {
            node_t *node = &network->nodes.data[i];
            // Update the center of the RBF node.
            float weight_column[NUM_OUTPUTS];
            for (uint32_t j = 0; j < NUM_OUTPUTS; j++) {
                weight_column[j] = network->weights.data[j][i];
            }
            float k = LEARNING_RATE
                * ((float)2.0 / powf(node->radius, 2))
                * activations.data[i]
                * dot((const float *)&weight_column, (const float *)&error, NUM_OUTPUTS);
            for (uint32_t j = 0; j < NUM_INPUTS; j++) {
                node->center[j] += k * (input[j] - node->center[j]);
            }
            // Update the corresponding weights for the output layer.
            for (uint32_t j = 0; j < NUM_OUTPUTS; j++) {
                network->weights.data[j][i] += LEARNING_RATE
                    * error[j]
                    * activations.data[i];
            }
        }
    }

    // === PRUNING ===
    // Normalization of the activation values
    float max_activation = activations.data[0];
    for (uint32_t i = 1; i < activations.size; i++) {
        if (activations.data[i] > max_activation) {
            max_activation = activations.data[i];
        }
    }
    for (uint32_t i = 0; i < activations.size; i++) {
        activations.data[i] /= max_activation;
    }
    // Update low activation counters.
    for (uint32_t i = 0; i < activations.size; i++) {
        if (activations.data[i] < ACTIVATION_THRESHOLD) {
            network->nodes.data[i].low_activation_count++;
        } else {
            network->nodes.data[i].low_activation_count = 0;
        }
    }
    // If the activation is below the threshold for a given number
    // of iterations, we remove the node from the network.
    for (uint32_t i = 0; i < network->nodes.size; i++) {
        if (network->nodes.data[i].low_activation_count >= PRUNING_WINDOW) {
            remove_node_at(&network->nodes, i);
            remove_column_at(&network->weights, i);
            i--;
        }
    }
}

#ifdef __cplusplus
}
#endif

#endif // __TINY_RBF_H__
