/**
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "application.h"
#include "ispu.h"

#include "i2c.h"
#include "tim.h"
#include "gpio.h"
#include "usart.h"

extern TIM_HandleTypeDef htim2;
static volatile uint32_t ispu_timestamp = 0;

#define UART_BUF_SIZE 256

static void read(uint8_t reg, uint8_t *val, uint16_t len);
static void write(uint8_t reg, uint8_t val);
static void write_mul(uint8_t reg, uint8_t *val, uint16_t len);
uint8_t get_type_size(uint8_t type);

static volatile char uart_char;
static volatile uint8_t uart_received;
static volatile char uart_buf[UART_BUF_SIZE + 1];
static volatile uint16_t uart_size;

static volatile uint8_t enable_int;
static volatile uint8_t algo_int;
static volatile uint8_t sleep_int;

static uint32_t exec_time, max_exec_time, min_exec_time;
static uint64_t num_exec_time;
static float avg_exec_time;

static uint16_t print_results;
static uint16_t print_time;

#define GT_X_MG             (0.0f)
#define GT_Y_MG             (0.0f)
#define GT_Z_MG             (1000.0f)

/*
 * Il main ISPU attuale pubblica raw e calibrated in m/s^2.
 * Qui li riconvertiamo in mg per il calcolo del MAE rispetto al ground truth
 * [0, 0, 1000] mg.
 *
 * Se in futuro l'ISPU pubblichera' direttamente mg, sostituisci questa
 * funzione con: return value;
 */
static float output_to_mg(float value_ms2)
{
    return value_ms2 / 0.00980665f;
}

static float compute_mae_mg(float x_mg, float y_mg, float z_mg)
{
    return (fabsf(x_mg - GT_X_MG) +
            fabsf(y_mg - GT_Y_MG) +
            fabsf(z_mg - GT_Z_MG)) / 3.0f;
}

static float compute_error_reduction_pct(float mae_uncomp, float mae_comp)
{
    if (mae_uncomp < 1e-6f) {
        return 0.0f;
    }

    return 100.0f * (mae_uncomp - mae_comp) / mae_uncomp;
}

static uint8_t read_output_float(const struct mems_conf_output *out, float *dst)
{
    if (out->type != MEMS_CONF_OUTPUT_TYPE_FLOAT || out->len != 1) {
        return 0;
    }

    uint8_t raw[4];
    read(out->reg_addr, raw, 4);
    memcpy(dst, raw, 4);
    return 1;
}
void application(void)
{
	HAL_Delay(1000);

	/* Tengo il timer attivo se ispu_timestamp o altre misure temporali
	   usano TIM2 nella callback EXTI */
	HAL_TIM_Base_Start(&htim2);

	uint8_t who_am_i;
	uint32_t start = HAL_GetTick();

	do {
		if (HAL_GetTick() - start > 1000) { // retry for 1.0 s
			while (1) {
				printf("Error: sensor not recognized (%02x)\n", who_am_i);
				HAL_Delay(1000);
			}
		}
		write(0x01, 0x00); // set default registers access
		read(0x0F, &who_am_i, 1);
	} while (who_am_i != 0x22);

	// software reset
	uint8_t tmp;
	read(0x12, &tmp, 1);
	write(0x12, tmp | 0x01);
	do {
		read(0x12, &tmp, 1);
	} while ((tmp & 0x01) != 0);

	print_results = 1;

	uint8_t valid_sensors[ISPU_CONF_SENSORS_NUM];

	for (uint32_t i = 0; i < ISPU_CONF_SENSORS_NUM; i++) {
		// check sensor name(s) validity
		uint32_t name_list_len = ispu_conf_name_lists[i].len;
		const char *const *name_list = ispu_conf_name_lists[i].list;

		valid_sensors[i] = 1;
		for (uint32_t j = 0; j < name_list_len; j++) {
			if (strcmp(name_list[j], "ISM330IS") != 0 &&
			    strcmp(name_list[j], "LSM6DSO16IS") != 0) {
				valid_sensors[i] = 0;
				break;
			}
		}
		if (!valid_sensors[i])
			continue;

		// load configuration
		uint32_t conf_len = ispu_conf_confs[i].len;
		const struct mems_conf_op *conf = ispu_conf_confs[i].list;

		uint32_t mem_i = 0;
		uint8_t *mem_buf = malloc(conf_len);

		if (mem_buf) { // fast loading
			uint8_t ispu_page = 0;
			for (uint32_t j = 0; j < conf_len; j++) {
				if (conf[j].type == MEMS_CONF_OP_TYPE_WRITE) {
					if (conf[j].address == 0x01) {
						if (conf[j].data & 0x80)
							ispu_page = 1;
						else
							ispu_page = 0;
					}
					uint8_t is_mem_write = ispu_page && conf[j].address == 0x0B;

					if (is_mem_write) {
						mem_buf[mem_i++] = conf[j].data;
					} else {
						if (mem_i > 0) {
							write_mul(0x0B, mem_buf, mem_i);
							mem_i = 0;
						}
						write(conf[j].address, conf[j].data);
					}
				} else if (conf[j].type == MEMS_CONF_OP_TYPE_DELAY) {
					HAL_Delay(conf[j].data);
				}
			}
			free(mem_buf);
		} else { // fallback to slow loading
			for (uint32_t j = 0; j < conf_len; j++) {
				if (conf[j].type == MEMS_CONF_OP_TYPE_WRITE)
					write(conf[j].address, conf[j].data);
				else if (conf[j].type == MEMS_CONF_OP_TYPE_DELAY)
					HAL_Delay(conf[j].data);
			}
		}

		// Header personalizzato
		printf("\r\n");
		printf("############################################################\r\n");
		printf("# RBF Online Accelerometer Calibration on ISPU            #\r\n");
		printf("# -------------------------------------------------------- #\r\n");
		printf("# Program RAM used : 18.25 KiB                            #\r\n");
		printf("# Data RAM used    :  1.79 KiB                            #\r\n");
		printf("# Outputs          : MAE uncomp / MAE comp / reduction    #\r\n");
		printf("############################################################\r\n");
		printf("\r\n");
		printf("mode\tMAE_uncomp_mg\tMAE_comp_mg\terror_reduction_pct\trbf_neurons\r\n");
		printf("\r\n");	}

	HAL_UART_Receive_IT(&huart2, (uint8_t *)&uart_char, 1);
	enable_int = 1;

	while (1) {
		// handle commands received from uart
		if (uart_received) {
			if (sscanf((char *)uart_buf, "res%hu", &print_results) > 0) {
				if (print_results)
					printf("Enabled results print.\n");
				else
					printf("Disabled results print.\n");
			}

			if (sscanf((char *)uart_buf, "time%hu", &print_time) > 0) {
				if (print_time)
					printf("Enabled execution time print.\n");
				else
					printf("Disabled execution time print.\n");
			}

			uint8_t reg, val;

			if (sscanf((char *)uart_buf, "write%02hhX%02hhX", &reg, &val) == 2) {
				write(reg, val);
				printf("wrote 0x%02hhX = 0x%02hhX\n", reg, val);
			}

			if (sscanf((char *)uart_buf, "read%02hhX", &reg) == 1) {
				read(reg, &val, 1);
				printf("read 0x%02hhX = 0x%02hhX\n", reg, val);
			}

			if (sscanf((char *)uart_buf, "write_ispu%02hhX%02hhX", &reg, &val) == 2) {
				write(0x01, 0x80);
				write(reg, val);
				write(0x01, 0x00);
				printf("wrote 0x%02hhX = 0x%02hhX\n", reg, val);
			}

			if (sscanf((char *)uart_buf, "read_ispu%02hhX", &reg) == 1) {
				write(0x01, 0x80);
				read(reg, &val, 1);
				write(0x01, 0x00);
				printf("read 0x%02hhX = 0x%02hhX\n", reg, val);
			}

			uart_size = 0;
			uart_received = 0;
		}

		if (algo_int) {
			algo_int = 0;

			/* clear interrupt if latched */
			uint8_t ispu_int_status[4];
			read(0x1A, ispu_int_status, 4);

			if (print_results) {
				for (uint32_t i = 0; i < ISPU_CONF_SENSORS_NUM; i++) {
					if (!valid_sensors[i])
						continue;

					uint32_t output_list_len = ispu_conf_output_lists[i].len;
					const struct mems_conf_output *output_list = ispu_conf_output_lists[i].list;

					/*
					 * Formato atteso:
					 * 0 -> MAE uncompensated (float)
					 * 1 -> MAE compensated   (float)
					 * 2 -> Error reduction   (float)
					 * 3 -> Learning flag     (float)
					 * 4 -> RBF neurons       (float)
					 */
					if (output_list_len < 5) {
						printf("Error: expected at least 5 outputs, found %lu\r\n", output_list_len);
						continue;
					}

					float mae_uncomp = 0.0f;
					float mae_comp   = 0.0f;
					float err_red    = 0.0f;
					float learn_flag = 0.0f;
					float n_neurons  = 0.0f;

					write(0x01, 0x80);

					for (uint32_t j = 0; j < 5; j++) {
						if (output_list[j].type != MEMS_CONF_OUTPUT_TYPE_FLOAT || output_list[j].len != 1) {
							write(0x01, 0x00);
							printf("Error: output %lu must be float scalar.\r\n", j);
							goto next_sensor;
						}

						uint8_t raw[4];
						read(output_list[j].reg_addr, raw, 4);

						switch (j) {
						case 0: memcpy(&mae_uncomp, raw, 4); break;
						case 1: memcpy(&mae_comp,   raw, 4); break;
						case 2: memcpy(&err_red,    raw, 4); break;
						case 3: memcpy(&learn_flag, raw, 4); break;
						case 4: memcpy(&n_neurons,  raw, 4); break;
						}
					}

					write(0x01, 0x00);

					if (learn_flag > 0.5f) {
						printf("      \t\t  LEARNING:  MAE Uncompensated [mg]: %.2f\tMAE Compensated [mg]: %.2f\tError Reduction [%%]: %.2f\tRBF Neurons: %.0f\r\n",
						       mae_uncomp, mae_comp, err_red, n_neurons);
					} else {
						printf("    INFERENCE:  MAE Uncompensated [mg]: %.2f\tMAE Compensated [mg]: %.2f\tError Reduction [%%]: %.2f\tRBF Neurons: %.0f\r\n",
						       mae_uncomp, mae_comp, err_red, n_neurons);
					}

		next_sensor:
					;
				}
			}
		}
		if (sleep_int) {
			sleep_int = 0;

			if (print_time)
				printf("%lu\t%lu\t%lu\t%f\n", exec_time, min_exec_time, max_exec_time, avg_exec_time);
		}
	}
}


static void read(uint8_t reg, uint8_t *val, uint16_t len)
{
	HAL_I2C_Mem_Read(&hi2c1, 0xD4, reg, I2C_MEMADD_SIZE_8BIT, val, len, HAL_MAX_DELAY);
}

static void write(uint8_t reg, uint8_t val)
{
	HAL_I2C_Mem_Write(&hi2c1, 0xD4, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, HAL_MAX_DELAY);
}

static void write_mul(uint8_t reg, uint8_t *val, uint16_t len)
{
	HAL_I2C_Mem_Write(&hi2c1, 0xD4, reg, I2C_MEMADD_SIZE_8BIT, val, len, HAL_MAX_DELAY);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (uart_char == '\n') {
		uart_buf[uart_size] = '\0';
		uart_received = 1;
	} else if (uart_char == '*') {
		uart_size = 0;
	} else if (uart_char != '\r') {
		if (uart_size >= UART_BUF_SIZE)
			uart_size = 0;
		uart_buf[uart_size++] = uart_char;
	}

	HAL_UART_Receive_IT(&huart2, (uint8_t *)&uart_char, 1);
}

int _write(int fd, const void *buf, size_t count)
{
	uint8_t status = HAL_UART_Transmit(&huart2, (uint8_t *)buf, count, HAL_MAX_DELAY);

	return (status == HAL_OK ? count : 0);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	GPIO_TypeDef *int2_port;
	uint16_t int2_pin;

	if (enable_int) {
		switch (GPIO_Pin) {
		case INT1_Pin:
		    ispu_timestamp = __HAL_TIM_GET_COUNTER(&htim2);  // ← cattura subito
			algo_int = 1;
			break;
		//case DIL24_INT2_Pin:
		case IKS4A1_LSM6DSO16IS_INT2_Pin:
			int2_port = IKS4A1_LSM6DSO16IS_INT2_GPIO_Port;
			int2_pin  = IKS4A1_LSM6DSO16IS_INT2_Pin;
			if (HAL_GPIO_ReadPin(int2_port, int2_pin) == GPIO_PIN_RESET) {
				__HAL_TIM_SET_COUNTER(&htim5, 0);
				HAL_TIM_Base_Start(&htim5);
			} else if (HAL_GPIO_ReadPin(int2_port, int2_pin) == GPIO_PIN_SET) {
				HAL_TIM_Base_Stop(&htim5);
				exec_time = __HAL_TIM_GET_COUNTER(&htim5);
				if (num_exec_time++ == 1) {
					avg_exec_time = exec_time;
					min_exec_time = UINT32_MAX;
					max_exec_time = 0;
				}
				avg_exec_time += (float)(exec_time - avg_exec_time) / (float)num_exec_time;
				if (exec_time < min_exec_time)
					min_exec_time = exec_time;
				if (exec_time > max_exec_time)
					max_exec_time = exec_time;
				sleep_int = 1;
			}
		}
	}
}

uint8_t get_type_size(uint8_t type)
{
	const struct {
		uint8_t id;
		uint8_t size;
	} types[] = {
		{ MEMS_CONF_OUTPUT_TYPE_UINT8_T,  1 },
		{ MEMS_CONF_OUTPUT_TYPE_INT8_T,   1 },
		{ MEMS_CONF_OUTPUT_TYPE_CHAR,     1 },
		{ MEMS_CONF_OUTPUT_TYPE_UINT16_T, 2 },
		{ MEMS_CONF_OUTPUT_TYPE_INT16_T,  2 },
		{ MEMS_CONF_OUTPUT_TYPE_UINT32_T, 4 },
		{ MEMS_CONF_OUTPUT_TYPE_INT32_T,  4 },
		{ MEMS_CONF_OUTPUT_TYPE_UINT64_T, 8 },
		{ MEMS_CONF_OUTPUT_TYPE_INT64_T,  8 },
		{ MEMS_CONF_OUTPUT_TYPE_HALF,     2 },
		{ MEMS_CONF_OUTPUT_TYPE_FLOAT,    4 },
		{ MEMS_CONF_OUTPUT_TYPE_DOUBLE,   8 }
	};

	for (uint8_t i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
		if (type == types[i].id)
			return types[i].size;
	}

	return 0;
}

