/*
 * @file: aht20_demo.c
 *
 * @brief: A simple demo to use the AHT20 driver
 *
 * @date: April 20, 2025
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
#include <stdbool.h>
#include <unistd.h>
#include "aht20.h"


//i2c configuration values
#define I2C_MASTER_SCL_IO           (22)    // SCL pin
#define I2C_MASTER_SDA_IO           (21)    // SDA pin
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          (400000)  // I2C frequency

i2c_master_bus_handle_t my_i2c_bus_handle;

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
    printf("requesting i2c bus handle\n");
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &my_i2c_bus_handle));
    printf("i2c bus handle acquired\n");


}


void read_aht20 (void *my_aht20_handle)
{

    aht20_handle_t aht20_handle = (aht20_handle_t) my_aht20_handle; //retrieve the AHT20 device handle copy
    vTaskDelay(400 / portTICK_PERIOD_MS);
    while (1) {
        //read both humidity and temperature at once from device, using AHT20 device handle
        aht20_read_humiture(aht20_handle);
        //access the results stored in AHT20 device object, using the AHT20 device handle
        //other apis require user to explicitly pass variable address to hold data
        printf("tempertature = %.2fC  humidity = %.3f \n", aht20_handle->humiture.temperature, aht20_handle->humiture.humidity);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


void init_aht20()
{
    //create a AHT20 device object and receive a device handle for it
    aht20_handle_t aht20_handle =  aht20_create( my_i2c_bus_handle, AHT20_ADDRESS_LOW ); //addresses in aht.h

    printf("initializing aht20\n");
    //use the previously created AHT20 device handle for initializing the associated device
    while (aht20_init(aht20_handle) != ESP_OK) { // wait until it is initialized
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    printf("aht20 initialized\n");

    //creating a task to read from AHT20, passing the AHT20 device handle copy
    xTaskCreate(read_aht20, "aht20_tester", 2500, aht20_handle, 5, NULL);
}

void app_main(void)
{
    i2c_master_init(); //initialize i2c master
    init_aht20();    // user defined function for aht20 initialization
}
