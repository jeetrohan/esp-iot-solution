/*
 * @File: aht20.c
 *
 * @brief: AHT20 driver function definitions
 *
 * @Date: April 28, 2025
 *
 * @Author: Rohan Jeet <jeetrohan92@gmail.com>
 *
 */




#include <stdio.h>
#include "aht20.h"


static const char *s_TAG = "AHT20";


/**
* @brief a function used to handle reinitialization of registers of the device, if not found calibrated when initialized
*
* @param[in] aht20_handle AHT20 device handle
*
*/
static esp_err_t aht20_Start_Init(aht20_handle_t aht20_handle);


/**
* @brief a function used to handle resetting of registers of the device, if not found calibrated when initialized
*
* @param[in] aht20_handle AHT20 device handle
*
* @param[in] addr AHT20 internal register, undocumented in datasheet
*
*/
static esp_err_t aht_JH_Reset_REG(aht20_handle_t aht20_handle, uint8_t addr);

/**
* @brief check crc validity of response received
*
* @param[in] message AHT reading
*
*
* @param[in] num Data bytes to check in message for crc
*
* @return  crc calculated from the provided message
*
*/
static uint8_t calc_CRC8(uint8_t *message, uint8_t Num);



static uint8_t calc_CRC8(uint8_t *message, uint8_t Num)
{
    uint8_t i;
    uint8_t byte;
    uint8_t crc = 0xFF;
    for (byte = 0; byte < Num; byte++) {
        crc ^= (message[byte]);
        for (i = 8; i > 0; --i) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc = (crc << 1);
            }
        }
    }
    return crc;
}

esp_err_t aht20_read_raw(aht20_handle_t aht20_handle, aht20_raw_reading_t *raw_read)
{
    ESP_RETURN_ON_FALSE( (aht20_handle != NULL), ESP_ERR_INVALID_ARG, s_TAG, "empty handle, provide a valid AHT20 handle");

    uint8_t measure_cmd[] = {AHT20_MEASURE_CYC, 0x33, 0x00};

    ESP_RETURN_ON_ERROR (   i2c_master_transmit(aht20_handle->dev_handle, measure_cmd, sizeof(measure_cmd), 50),
                            s_TAG, "unable to set mode for AHT20\n");
    uint8_t read_measurement[7], read_bytes = 6;;
#ifdef CONFIG_AHT20_CHECK_CRC
    read_bytes = 7;
#endif

    bool busy = true;
    while (busy) {
        aht20_busy_status(aht20_handle, &busy);    // wait for measurement
    }
    ESP_RETURN_ON_ERROR (   i2c_master_receive( aht20_handle->dev_handle, read_measurement, read_bytes, 100 ),
                            s_TAG, "unable to read raw measurement");

#ifdef CONFIG_AHT20_CHECK_CRC
    ESP_RETURN_ON_FALSE(   (calc_CRC8(read_measurement, 6) == read_measurement[6]),
                           ESP_ERR_INVALID_CRC,
                           s_TAG, "CRC match failed, invalid response received from AHT20");
#endif

    raw_read->humidity = ( (read_measurement[1] << 16) | (read_measurement[2] << 8) | read_measurement[3] ) >> 4;
    raw_read->temperature = ( (read_measurement[3] << 16) | (read_measurement[4] << 8) | read_measurement[5] ) & 0xfffff;

    return ESP_OK;
}

esp_err_t aht20_read_humiture(aht20_handle_t aht20_handle)
{
    aht20_raw_reading_t raw_read;
    ESP_RETURN_ON_ERROR (  aht20_read_raw (aht20_handle, &raw_read),
                           "", "");

    aht20_handle->humiture.humidity = raw_read.humidity * 100.0 / 1024 / 1024; //Calculated humidity value
    aht20_handle->humiture.temperature = (raw_read.temperature * 200.0 / 1024 / 1024) - 50; //Calculated temperature value

    return ESP_OK;
}

esp_err_t aht20_read_humidity(aht20_handle_t aht20_handle, float_t *humidity)
{
    aht20_raw_reading_t raw_read;
    ESP_RETURN_ON_ERROR (  aht20_read_raw (aht20_handle, &raw_read),
                           "", "");

    *humidity = raw_read.humidity * 100.0 / 1024 / 1024; //Calculated humidity value
    return ESP_OK;
}

esp_err_t aht20_read_temperature(aht20_handle_t aht20_handle, float_t *temperature)
{
    aht20_raw_reading_t raw_read;
    ESP_RETURN_ON_ERROR (  aht20_read_raw (aht20_handle, &raw_read),
                           "", "");

    *temperature = (raw_read.temperature * 200.0 / 1024 / 1024) - 50; //Calculated temperature value

    return ESP_OK;
}


esp_err_t aht20_calibration_status(aht20_handle_t aht20_handle, bool *calibration)
{
    ESP_RETURN_ON_FALSE( (  aht20_handle != NULL), ESP_ERR_INVALID_ARG,
                         s_TAG, "empty handle, initialize AHT20 handle");

    ESP_RETURN_ON_FALSE( (  calibration != NULL), ESP_ERR_INVALID_ARG,
                         s_TAG, "provide a variable to store status value");

    uint8_t read_status;
    ESP_RETURN_ON_ERROR (   i2c_master_receive( aht20_handle->dev_handle, &read_status, sizeof(read_status), 100 ),
                            s_TAG, "unable to read status");

    if ( read_status & BIT3) {
        *calibration = true;
    } else {
        *calibration = false;
    }

    return ESP_OK;
}


esp_err_t aht20_busy_status(aht20_handle_t aht20_handle, bool *busy)
{
    ESP_RETURN_ON_FALSE( (  aht20_handle != NULL), ESP_ERR_INVALID_ARG,
                         s_TAG, "empty handle, initialize AHT20 handle");

    ESP_RETURN_ON_FALSE( (  busy != NULL), ESP_ERR_INVALID_ARG,
                         s_TAG, "provide a variable to store status value");
    uint8_t read_status;
    ESP_RETURN_ON_ERROR (   i2c_master_receive( aht20_handle->dev_handle, &read_status, sizeof(read_status), 100 ),
                            s_TAG, "unable to read status");

    if ( read_status & BIT7) {
        *busy = true;
    } else {
        *busy = false;
    }

    return ESP_OK;
}


static esp_err_t aht_JH_Reset_REG(aht20_handle_t aht20_handle, uint8_t addr)
{

    uint8_t reset_cmd[] = {addr, 0x00, 0x00}, read_bytes[3];

    ESP_RETURN_ON_ERROR (   i2c_master_transmit_receive(aht20_handle->dev_handle, reset_cmd, sizeof(reset_cmd), read_bytes, sizeof(read_bytes), 80),
                            s_TAG, "unable to reset, check log");

    vTaskDelay(10 / portTICK_PERIOD_MS);
    reset_cmd[0] = 0xB0 | addr;
    reset_cmd[1] = read_bytes[1];
    reset_cmd[2] = read_bytes[2];
    ESP_RETURN_ON_ERROR ( i2c_master_transmit(aht20_handle->dev_handle, reset_cmd, sizeof(reset_cmd), 50), s_TAG, "unable to reset, check log");
    return ESP_OK;
}

static esp_err_t aht20_Start_Init(aht20_handle_t aht20_handle)
{
    ESP_RETURN_ON_ERROR (aht_JH_Reset_REG(aht20_handle, 0x1b), "", "");
    ESP_RETURN_ON_ERROR (aht_JH_Reset_REG(aht20_handle, 0x1c), "", "");
    ESP_RETURN_ON_ERROR (aht_JH_Reset_REG(aht20_handle, 0x1e), "", "");

    return ESP_OK;
}

esp_err_t aht20_init (aht20_handle_t aht20_handle)
{
    ESP_RETURN_ON_FALSE( (aht20_handle != NULL), ESP_ERR_INVALID_ARG, s_TAG, "empty handle, initialize AHT20 handle");

    vTaskDelay(20 / portTICK_PERIOD_MS); //time for AHT20 SCL to stabilize


    /***********************************************************************************/
    /** // This is undocumented in user manual
        //The first time the power is turned on, read the status word at 0x71, determine whether the status word is 0x18,
        //if it is not 0x18,  reset the registers
    **/
    uint8_t read_status;
    ESP_RETURN_ON_ERROR (   i2c_master_receive( aht20_handle->dev_handle, &read_status, sizeof(read_status), 100 ),
                            s_TAG, "unable to read status");

    if ((read_status & 0x18) != 0x18) {
        ESP_RETURN_ON_ERROR (   aht20_Start_Init( aht20_handle),
                                s_TAG, "reset failed, retry");
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    /***********************************************************************************/

    // initialize AHT20
    uint8_t aht20_init_cmd [] =  { AHT20_INIT_REG, 0x08, 0x00 };
    ESP_RETURN_ON_ERROR (   i2c_master_transmit(aht20_handle->dev_handle, aht20_init_cmd, sizeof(aht20_init_cmd), 50),
                            s_TAG, "unable to initialize AHT20\n");
    ESP_LOGI(s_TAG, "AHT20 initialized\n");

    ESP_LOGI(TAG, "%-15s: %d.%d.%d", CHIP_NAME, AHT20_VER_MAJOR, AHT20_VER_MINOR, AHT20_VER_PATCH);
    ESP_LOGI(TAG, "%-15s: %1.1f - %1.1fV", "SUPPLY_VOLTAGE", SUPPLY_VOLTAGE_MIN, SUPPLY_VOLTAGE_MAX);
    ESP_LOGI(TAG, "%-15s: %.2f - %.2f℃", "TEMPERATURE", TEMPERATURE_MIN, TEMPERATURE_MAX);

    return ESP_OK;

}

aht20_handle_t aht20_create ( i2c_master_bus_handle_t bus_handle, uint8_t aht20_address)
{
    aht20_handle_t my_aht20_handle = malloc(sizeof(aht20_dev_t));

    if (my_aht20_handle == NULL) {
        ESP_LOGE(s_TAG, "unable to allocate memory to initialize aht20 handle");
        return my_aht20_handle;
    }

    i2c_device_config_t aht20_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = aht20_address,
        .scl_speed_hz = CONFIG_AHT20_I2C_CLK_SPEED,
    };
    ESP_LOGI(s_TAG, "adding aht20 as device to bus\n");
    i2c_master_bus_add_device(bus_handle, &aht20_config, &my_aht20_handle->dev_handle );
    ESP_LOGI(s_TAG, "device added to bus\n");

    return my_aht20_handle;
}

void aht20_delete ( aht20_handle_t *aht20ptr)
{
    i2c_master_bus_rm_device( (*aht20ptr)->dev_handle);
    free(*aht20ptr);
    *aht20ptr = NULL; // now AHT20 handle is not a dangling pointer
}
