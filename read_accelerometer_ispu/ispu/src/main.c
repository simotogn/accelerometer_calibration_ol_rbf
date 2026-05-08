/**
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#include "peripherals.h"
#include "reg_map.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>


#include "network.h"


void __attribute__ ((signal)) algo_00_init(void);
void __attribute__ ((signal)) algo_00(void);

#define ACC_SENS_MG 0.244f
static uint32_t phase_count = 0;

static volatile uint32_t int_status;

static network_t network;
static node_list_t nodes;
static weight_matrix_t weights;
typedef struct
{
  float Acc[3];   /* Acceleration in X, Y, Z axis in [g] */
  uint8_t TimeStamp;  /* Time stamp for accelerometer sensor output in [us] */
} acc_in;

static uint32_t it = 0;
static uint32_t t = 0;


static bool learning_phase = true;

static float acc_input[NUM_INPUTS];


void metrics(float* acc, float* comp, float* results) {
    float reference[3] = {0.0f, 0.0f, 1000.0f};

    float mae_mems = 0.0f;
    float mae_comp = 0.0f;
    float mcp = 0.0f;

    for (int i = 0; i < 3; i++) {
        mae_mems += (float)fabs(reference[i] - acc[i]);
        mae_comp += (float)fabs(reference[i] - comp[i]);
    }

    mae_mems /= 3;
    mae_comp /= 3;
    mcp = ((mae_mems - mae_comp) / mae_mems) * 100;

    results[0] = mae_mems;
    results[1] = mae_comp;
    results[2] = mcp;
}




void __attribute__ ((signal)) algo_00_init(void)
{
	it = 0;
	t = 0;
	phase_count = 0;
	learning_phase = true;

	network = new_empty_network();
	// initialize the rbf network
    nodes.size = 3;

    /* Node 0 */
    nodes.data[0].center[0]  = -17.65574417f;
    nodes.data[0].center[1]  = -17.65574417f;
    nodes.data[0].center[2]  = -17.65549984f;
    nodes.data[0].center[3]  = -17.65519040f;
    nodes.data[0].center[4]  = -17.65045023f;
    nodes.data[0].center[5]  = 7.85718041f;
    nodes.data[0].center[6]  = 7.85670883f;
    nodes.data[0].center[7]  = 7.85662824f;
    nodes.data[0].center[8]  = 7.85569813f;
    nodes.data[0].center[9]  = 7.85371086f;
    nodes.data[0].center[10] = 1013.39472399f;
    nodes.data[0].center[11] = 1013.66508486f;
    nodes.data[0].center[12] = 1013.66936669f;
    nodes.data[0].center[13] = 1013.67317893f;
    nodes.data[0].center[14] = 1013.67759266f;
    nodes.data[0].radius     = 1523.216477f;
    nodes.data[0].low_activation_count = 0;

    /* Node 1 */
    nodes.data[1].center[0]  = -17.24245f;
    nodes.data[1].center[1]  = -16.96775f;
    nodes.data[1].center[2]  = -8.45335f;
    nodes.data[1].center[3]  = 0.0f;
    nodes.data[1].center[4]  = 0.0f;
    nodes.data[1].center[5]  = 6.25615f;
    nodes.data[1].center[6]  = 6.01200f;
    nodes.data[1].center[7]  = 2.83815f;
    nodes.data[1].center[8]  = 0.0f;
    nodes.data[1].center[9]  = 0.0f;
    nodes.data[1].center[10] = 0.0f;
    nodes.data[1].center[11] = 0.0f;
    nodes.data[1].center[12] = 499.4812f;
    nodes.data[1].center[13] = 998.9624f;
    nodes.data[1].center[14] = 998.4113f;
    nodes.data[1].radius     = 1117.05940436f;
    nodes.data[1].low_activation_count = 0;

    /* Node 2 */
    nodes.data[2].center[0]  = -16.9067f;
    nodes.data[2].center[1]  = 0.0f;
    nodes.data[2].center[2]  = 0.0f;
    nodes.data[2].center[3]  = 0.0f;
    nodes.data[2].center[4]  = 0.0f;
    nodes.data[2].center[5]  = 5.6763f;
    nodes.data[2].center[6]  = 0.0f;
    nodes.data[2].center[7]  = 0.0f;
    nodes.data[2].center[8]  = 0.0f;
    nodes.data[2].center[9]  = 0.0f;
    nodes.data[2].center[10] = 0.0f;
    nodes.data[2].center[11] = 0.0f;
    nodes.data[2].center[12] = 0.0f;
    nodes.data[2].center[13] = 0.0f;
    nodes.data[2].center[14] = 998.9624f;
    nodes.data[2].radius     = 1117.05940436f;
    nodes.data[2].low_activation_count = 0;

    weights.n_cols = 3;
    weights.data[0][0] = 0.39453805f;
    weights.data[0][1] = -1.74144501f;
    weights.data[0][2] = -15.86864615f;

    weights.data[1][0] = -0.68561623f;
    weights.data[1][1] = 4.13329297f;
    weights.data[1][2] = 40.88906404f;

    weights.data[2][0] = -0.63908730f;
    weights.data[2][1] = 0.56219450f;
    weights.data[2][2] = -1.48076217f;

    network.nodes = nodes;
    network.bias_weights[0] = 17.65532400f;
    network.bias_weights[1] = -7.85574496f;
    network.bias_weights[2] = -13.66552755f;
    network.weights = weights;
    network.distance_mean = 0.0f;
    network.distance_std = 0.0f;
}

void __attribute__ ((signal)) algo_00(void)
{
	it = (it + 1) % 500;
	t++;

	/* output della rete: correzione sui 3 assi */
	float compensation_error[NUM_OUTPUTS] = {0.0f, 0.0f, 0.0f};

	/* lettura raw accelerometro */
	int32_t ax_raw = cast_sint32_t(ISPU_ARAW_X);
	int32_t ay_raw = cast_sint32_t(ISPU_ARAW_Y);
	int32_t az_raw = cast_sint32_t(ISPU_ARAW_Z);

	/* conversione in mg: la rete e il target lavorano in mg */
	acc_in data_in;
	data_in.Acc[0] = (float)ax_raw * ACC_SENS_MG;
	data_in.Acc[1] = (float)ay_raw * ACC_SENS_MG;
	data_in.Acc[2] = (float)az_raw * ACC_SENS_MG;
	data_in.TimeStamp = 0;

	/* shift della finestra storica */
	for (int i = 0; i < LOOK_BACK_VAL - 1; i++) {
		acc_input[i] = acc_input[i + 1];
		acc_input[i + LOOK_BACK_VAL] = acc_input[i + LOOK_BACK_VAL + 1];
		acc_input[i + 2 * LOOK_BACK_VAL] = acc_input[i + 2 * LOOK_BACK_VAL + 1];
	}

	/* aggiunta ultimo campione:
	   [x(t-4)...x(t)] [y(t-4)...y(t)] [z(t-4)...z(t)] */
	acc_input[LOOK_BACK_VAL - 1] = data_in.Acc[0];
	acc_input[2 * LOOK_BACK_VAL - 1] = data_in.Acc[1];
	acc_input[3 * LOOK_BACK_VAL - 1] = data_in.Acc[2];

	/* warmup: finche' non riempi la finestra, niente rete */
	if (it < LOOK_BACK_VAL) {
		cast_float(ISPU_DOUT_00) = 0.0f;
		cast_float(ISPU_DOUT_02) = 0.0f;
		cast_float(ISPU_DOUT_04) = 0.0f;
		cast_float(ISPU_DOUT_06) = 0.0f;
		int_status |= 0x1u;
		return;
	}

	if (learning_phase) {
		float y[3];

		y[0] = 0.0f - data_in.Acc[0];
		y[1] = 0.0f - data_in.Acc[1];
		y[2] = 1000.0f - data_in.Acc[2];

		learning(&network, acc_input, y, compensation_error);

		phase_count++;
		if (phase_count >= TRAINING_ITERATIONS) {
			learning_phase = false;
			phase_count = 0;
		}
	} else {
		inference(&network, acc_input, compensation_error);

		phase_count++;
		if (phase_count >= INFERENCE_ITERATIONS) {
			learning_phase = true;
			phase_count = 0;
		}
	}

	/* accelerazione compensata = misura + correzione */
	float acc_comp[3];
	for (int i = 0; i < 3; i++) {
		acc_comp[i] = data_in.Acc[i] + compensation_error[i];
	}

	float metric[3];
	metrics(data_in.Acc, acc_comp, metric);

	/* output:
	   DOUT_00 -> MAE uncompensated
	   DOUT_02 -> MAE compensated
	   DOUT_04 -> error reduction
	   DOUT_06 -> learning flag
	*/
	cast_float(ISPU_DOUT_00) = metric[0];
	cast_float(ISPU_DOUT_02) = metric[1];
	cast_float(ISPU_DOUT_04) = metric[2];
	cast_float(ISPU_DOUT_06) = learning_phase ? 1.0f : 0.0f;
	cast_float(ISPU_DOUT_08) = (float)network.nodes.size;


	int_status |= 0x1u;
}

// For more algorithms implement the corresponding functions: algo_01_init and
// algo_01 for algo 1, algo_02_init and algo_02 for algo 2, etc.

int main(void)
{
	// set boot done flag
	uint8_t status = cast_uint8_t(ISPU_STATUS);
	status = status | 0x04u;
	cast_uint8_t(ISPU_STATUS) = status;

	// enable algorithms interrupt request generation
	cast_uint8_t(ISPU_GLB_CALL_EN) = 0x01u;

	while (true) {
		stop_and_wait_start_pulse;

		// reset status registers and interrupts
		int_status = 0u;
		cast_uint32_t(ISPU_INT_STATUS) = 0u;
		cast_uint8_t(ISPU_INT_PIN) = 0u;

		// get all the algorithms to run in this time slot
		cast_uint32_t(ISPU_CALL_EN) = cast_uint32_t(ISPU_ALGO) << 1;

		// wait for all algorithms execution
		while (cast_uint32_t(ISPU_CALL_EN) != 0u) {
		}

		// get interrupt flags
		uint8_t int_pin = 0u;
		int_pin |= ((int_status & cast_uint32_t(ISPU_INT1_CTRL)) > 0u) ? 0x01u : 0x00u;
		int_pin |= ((int_status & cast_uint32_t(ISPU_INT2_CTRL)) > 0u) ? 0x02u : 0x00u;

		// set status registers and generate interrupts
		cast_uint32_t(ISPU_INT_STATUS) = int_status;
		cast_uint8_t(ISPU_INT_PIN) = int_pin;
	}
}

