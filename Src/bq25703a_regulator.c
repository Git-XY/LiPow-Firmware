/**
 ******************************************************************************
 * @file           : bq25703a_regulator.c
 * @brief          : Handles battery state information
 ******************************************************************************
 */


#include "bq25703a_regulator.h"
#include "battery.h"
#include "error.h"

extern I2C_HandleTypeDef hi2c1;

/* Private typedef -----------------------------------------------------------*/
struct Regulator {
	uint8_t connected;
	uint8_t status;
	uint32_t input_current;
	uint32_t output_current;
	uint16_t max_charge_voltage;
	uint8_t input_current_limit;
	uint16_t min_input_voltage_limit;
};

/* Private variables ---------------------------------------------------------*/
struct Regulator regulator;

/* The maximum time to wait for the mutex that guards the UART to become
 available. */
#define cmdMAX_MUTEX_WAIT	pdMS_TO_TICKS( 300 )

/* Private function prototypes -----------------------------------------------*/
void I2C_Transfer(uint8_t *pData, uint16_t size);

/**
 * @brief Performs an I2C transfer
 */
void I2C_Transfer(uint8_t *pData, uint16_t size) {

	if ( xSemaphoreTake( xTxMutex_Regulator, cmdMAX_MUTEX_WAIT ) == pdPASS) {
		do
		{
			TickType_t xtimeout_start = xTaskGetTickCount();
			while (HAL_I2C_Master_Transmit_DMA(&hi2c1, (uint16_t)BQ26703A_I2C_ADDRESS, pData, size) != HAL_OK) {
				if (((xTaskGetTickCount()-xtimeout_start)/portTICK_PERIOD_MS) > I2C_TIMEOUT) {
					Set_Error_State(REGULATOR_COMMUNICATION_ERROR);
					break;
				}
			}
		    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY) {
				if (((xTaskGetTickCount()-xtimeout_start)/portTICK_PERIOD_MS) > I2C_TIMEOUT) {
					Set_Error_State(REGULATOR_COMMUNICATION_ERROR);
					break;
				}
		    }
		}
		while(HAL_I2C_GetError(&hi2c1) == HAL_I2C_ERROR_AF);
		xSemaphoreGive(xTxMutex_Regulator);
	}
}

void I2C_Receive(uint8_t *pData, uint16_t size) {
	if ( xSemaphoreTake( xTxMutex_Regulator, cmdMAX_MUTEX_WAIT ) == pdPASS) {
		do
		{
			TickType_t xtimeout_start = xTaskGetTickCount();
			while (HAL_I2C_Master_Receive_DMA(&hi2c1, (uint16_t)BQ26703A_I2C_ADDRESS, pData, size) != HAL_OK) {
				if (((xTaskGetTickCount()-xtimeout_start)/portTICK_PERIOD_MS) > I2C_TIMEOUT) {
					Set_Error_State(REGULATOR_COMMUNICATION_ERROR);
					break;
				}
			}
			while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY) {
				if (((xTaskGetTickCount()-xtimeout_start)/portTICK_PERIOD_MS) > I2C_TIMEOUT) {
					Set_Error_State(REGULATOR_COMMUNICATION_ERROR);
					break;
				}
			}
		}
		while(HAL_I2C_GetError(&hi2c1) == HAL_I2C_ERROR_AF);
		xSemaphoreGive(xTxMutex_Regulator);
	}
}

void vRegulator(void const *pvParameters) {

	TickType_t xDelay = 500 / portTICK_PERIOD_MS;

	/* Get the manufacturer id */
	uint8_t manufacturer_id_address = MANUFACTURER_ID_ADDR;
	I2C_Transfer((uint8_t *) &manufacturer_id_address, 1);
	uint8_t manufacturer_id;
	I2C_Receive((uint8_t *) &manufacturer_id, sizeof(manufacturer_id));

	/* Get the device id */
	uint8_t device_id_address = DEVICE_ID_ADDR;
	I2C_Transfer((uint8_t *) &device_id_address, 1);
	uint8_t device_id;
	I2C_Receive((uint8_t *) &device_id, sizeof(device_id));

	if ( (device_id == BQ26703A_DEVICE_ID) && (manufacturer_id == BQ26703A_MANUFACTURER_ID) ) {
		regulator.connected = CONNECTED;
		Clear_Error_State(REGULATOR_COMMUNICATION_ERROR);
	}
	else {
		regulator.connected = NOT_CONNECTED;
		Set_Error_State(REGULATOR_COMMUNICATION_ERROR);
	}

	for (;;) {



		vTaskDelay(xDelay);
	}
}
