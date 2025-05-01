/*
 * @File: aht20_test_app.c
 *
 * @brief: AHT20 driver unity test app
 *
 * @Date: May 1, 2025
 *
 * @Author: Rohan Jeet <jeetrohan92@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <inttypes.h>

#include "unity.h"

#include "esp_system.h"
#include "esp_log.h"

#include "aht20.h"



#define TEST_MEMORY_LEAK_THRESHOLD (-400)

#define I2C_MASTER_SCL_IO   CONFIG_I2C_MASTER_SCL   /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO   CONFIG_I2C_MASTER_SDA   /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM      I2C_NUM_0               /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ  100000                  /*!< I2C master clock frequency */

i2c_master_bus_handle_t my_i2c_bus_handle = NULL;
static aht20_handle_t aht20_handle = NULL;

/**
 * @brief i2c master initialization
 */
static void i2c_bus_init(void)
{

    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    printf("Requesting I2C bus handle\n");
    TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, i2c_new_master_bus(&i2c_mst_config, &my_i2c_bus_handle),
                              "I2C master bus handle, failure to acquire");
    printf("I2C bus handle acquired\n");

}

static void i2c_sensor_ath20_init(void)
{
    aht20_handle = aht20_create(my_i2c_bus_handle, AHT20_ADDRESS_LOW);
    TEST_ASSERT_NOT_NULL_MESSAGE(aht20_handle, "AHT20 create returned NULL");

    printf("Initializing AHT20 sensor\n");
    while (aht20_init(aht20_handle) != ESP_OK) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    printf("AHT20 initialized\n");

}

void i2c_sensor_ath20_deinit(void)
{
    aht20_delete(&aht20_handle);
    TEST_ASSERT_EQUAL(ESP_OK, i2c_del_master_bus(my_i2c_bus_handle));
}

TEST_CASE("sensor aht20 test", "[aht20][iot][sensor]")
{

    i2c_bus_init();
    i2c_sensor_ath20_init();

    vTaskDelay(400 / portTICK_PERIOD_MS);

    TEST_ASSERT(ESP_OK == aht20_read_humiture(aht20_handle));

    printf("Temperature = %.2f°C, Humidity = %.3f%%\n",
           aht20_handle->humiture.temperature,
           aht20_handle->humiture.humidity);
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    //to get reaw values create a object of following data type
    aht20_raw_reading_t raw_value;
    aht20_read_raw(aht20_handle, &raw_value);

    printf("raw tempertature = %luC  raw humidity = %lu \n", raw_value.temperature, raw_value.humidity);

    aht20_delete(&aht20_handle);
    TEST_ASSERT_EQUAL(ESP_OK, i2c_del_master_bus(my_i2c_bus_handle));
}

static size_t before_free_8bit;
static size_t before_free_32bit;

static void check_leak(size_t before_free, size_t after_free, const char *type)
{
    ssize_t delta = after_free - before_free;
    printf("MALLOC_CAP_%s: Before %u bytes free, After %u bytes free (delta %d)\n", type, before_free, after_free, delta);
    TEST_ASSERT_MESSAGE(delta >= TEST_MEMORY_LEAK_THRESHOLD, "memory leak");
}

void setUp(void)
{
    before_free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    before_free_32bit = heap_caps_get_free_size(MALLOC_CAP_32BIT);
}

void tearDown(void)
{
    size_t after_free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t after_free_32bit = heap_caps_get_free_size(MALLOC_CAP_32BIT);
    check_leak(before_free_8bit, after_free_8bit, "8BIT");
    check_leak(before_free_32bit, after_free_32bit, "32BIT");
}

void app_main(void)
{
    printf("AHT20 TEST \n");
    unity_run_menu();
}
