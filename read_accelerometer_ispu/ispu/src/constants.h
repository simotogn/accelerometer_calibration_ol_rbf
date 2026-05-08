/**
 * Definition of constants used in the project
 * 
 * @author Francesco Saccani <francesco.saccani@unipr.it>
 */

#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#ifndef ACTIVATION_THRESHOLD
// Activation threshold for the RBF neurons. Defaults to 0.01.
#define ACTIVATION_THRESHOLD (float)0.2
#endif

#ifndef AVERAGE_WINDOW
// Size of the moving window for the approximated mean and standard deviation.
// Defaults to 100.
#define AVERAGE_WINDOW 100
#endif

#ifndef LEARNING_RATE
// Learning rate used during parameter updates. Defaults to 0.2.
#define LEARNING_RATE (float)0.09
#endif

#ifndef MAX_NUM_NODES
// Maximum number of RBF nodes. Defaults to 10.
#define MAX_NUM_NODES 5
#endif

#ifndef LOOK_BACK_VAL

#define LOOK_BACK_VAL 5
#endif

#ifndef NUM_INPUTS
// Number of input features. Defaults to 3.
#define NUM_INPUTS 3*LOOK_BACK_VAL
#endif

#ifndef NUM_OUTPUTS
// Number of output features. Defaults to 3.
#define NUM_OUTPUTS 3
#endif

#ifndef OVERLAP_FACTOR
// Overlap factor. The radius of a new node is set to the distance to the
// nearest node multiplied by this factor. Defaults to 0.8.
#define OVERLAP_FACTOR (float)2.0
#endif

#ifndef PRUNING_WINDOW
// Number of consecutive iterations that lead to the elimination of a node
// if its activation is below the threshold. Defaults to u32::MAX.
#define PRUNING_WINDOW 490
#endif

#define TRAINING_ITERATIONS 10

#define INFERENCE_ITERATIONS 100

#endif // __CONSTANTS_H__
