/**
 * Implementation of a vector data structure for static memory allocation.
 * 
 * @author Francesco Saccani
 */

#ifndef __TINY_RBF_NODE_LIST_H__
#define __TINY_RBF_NODE_LIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "constants.h"
#include "node.h"

/**
 * Node list data structure.
 */
typedef struct {
    node_t data[MAX_NUM_NODES];
    uint32_t size;
} node_list_t;

/**
 * Check if the node list is empty
 * 
 * @param list the vector
 * @return true if the vector is empty, false otherwise
 */
static inline bool is_node_list_empty(node_list_t *list) {
    return list->size == 0;
}

/**
 * Check if the node list is full
 * 
 * @param list the vector
 * @return true if the vector is full, false otherwise
 */
static inline bool is_node_list_full(node_list_t *list) {
    return list->size == MAX_NUM_NODES;
}

/**
 * Add a node to the end of the node list
 * 
 * @param list the node list
 * @param item the node to add
 * @return true if the node has been added, false otherwise
 */
static inline bool add_node(node_list_t *list, node_t item) {
    if (is_node_list_full(list)) {
        // Cannot add, list is full
        return false;
    }
    list->data[list->size] = item;
    list->size++;
    return true;
}

/**
 * Remove the node at the specified index
 * 
 * @param list the node list
 * @param index the index of the node to remove
 * @return true if the node has been removed, false otherwise
 */
static inline bool remove_node_at(node_list_t *list, uint32_t index) {
    if (index >= list->size) {
        // Index out of bounds
        return false;
    }
    // Shift elements to the left to overwrite the element at index
    for (uint32_t i = index; i < list->size - 1; i++) {
        list->data[i] = list->data[i + 1];
    }
    list->size--;
    return true;
}

#ifdef __cplusplus
}
#endif

#endif // __TINY_RBF_NODE_LIST_H__
