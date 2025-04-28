/*
 * @File: aht20_test_app.c
 *
 * @brief: AHT20 driver unity test app
 *
 * @Date: April 20, 2025
 *
 * Copyright 2025 Rohan Jeet <jeetrohan92@gmail.com>
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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "unity.h"
#include "esp_system.h"
#include "aht20.h"


// I2C config
#define I2C_MASTER_SCL_IO       22
#define I2C_MASTER_SDA_IO       21
#define I2C_MASTER_NUM          I2C_NUM_0
#define I2C_MASTER_FREQ_HZ      400000

// Global handles
i2c_master_bus_handle_t my_i2c_bus_handle = NULL;
aht20_handle_t aht20_handle = NULL;

void i2c_master_init(void)
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
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &my_i2c_bus_handle));
    printf("I2C bus handle acquired\n");
}

void aht20_init_test()
{
    i2c_master_init();
    aht20_handle = aht20_create(my_i2c_bus_handle, AHT20_ADDRESS_LOW);

    printf("Initializing AHT20 sensor\n");
    while (aht20_init(aht20_handle) != ESP_OK) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    printf("AHT20 initialized\n");
}

void aht20_deinit_test(void)
{
    aht20_remove(&aht20_handle);
}

void aht20_read_test(void)
{
    vTaskDelay(400 / portTICK_PERIOD_MS);

    for (int i = 0; i < 5; i++) {
        if (aht20_read_humiture(aht20_handle) == ESP_OK) {
            printf("Temperature = %.2f°C, Humidity = %.3f%%\n",
                   aht20_handle->humiture.temperature,
                   aht20_handle->humiture.humidity);
        } else {
            printf("Failed to read data from AHT20 sensor\n");
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


TEST_CASE("AHT20 Init", "[aht20][init]")
{
    TEST_ASSERT_NULL(aht20_handle);
    aht20_init_test();
}

TEST_CASE("AHT20 Read", "[aht20][read]")
{
    TEST_ASSERT_NOT_NULL(aht20_handle);

    aht20_read_test();
}

TEST_CASE("AHT20 Deinit", "[aht20][deinit]")
{
    TEST_ASSERT_NOT_NULL(aht20_handle);

    aht20_deinit_test();
    TEST_ASSERT_NULL(aht20_handle);

    printf("AHT20 deinitialized\n");
}

void app_main(void)
{
    printf("\n=== AHT20 Sensor Test Menu ===\n");
    unity_run_menu();  // Run test selection menu in flash monitor
}
