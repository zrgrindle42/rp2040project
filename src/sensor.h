#pragma once
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#define PERIPHERAL_ADDRESS 0x6B
#define IMU_DATA_REG 0x35
#define IMU_CONFIG 0x22
#define I2C_SPEED_STANDARD 100000
#define I2C_SDA_PIN 6
#define I2C_SCL_PIN 7


void configure_i2c_peripheral();
void i2c_write_register( uint8_t periph_address, const uint8_t *packet, size_t buffer_size, bool stop);
void i2c_read_register(uint8_t *buffer, uint8_t address);
void IMU_data_exfil();
void ImuTask(void* pv);
void ImuLoggingTask(void* pv);




typedef struct 
{
    int16_t ax;
    int16_t ay;
    int16_t az;
    int16_t gx;
    int16_t gy;
    int16_t gz;
}IMU;

typedef struct
{
    float ax_parsed;
    float ay_parsed;
    float az_parsed;
    float gx_parsed;
    float gy_parsed;
    float gz_parsed;
}IMUParsed;

extern IMU imu;
extern IMUParsed parsedimu;

