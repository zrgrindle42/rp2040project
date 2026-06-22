#pragma once
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define PERIPHERAL_ADDRESS 0x6B
#define IMU_DATA_REG 0x35
#define IMU_CONFIG 0x22

#define SHT_PERIPHERAL 0x44



#define ACCEL_SCALE 4096.0f
#define GYRO_SCALE 65.536f

#define I2C_SPEED_STANDARD 100000
#define I2C_SDA_PIN 6
#define I2C_SCL_PIN 7
//need to declare pins for the otherr imu bus



typedef struct
{
    float ax_parsed;
    float ay_parsed;
    float az_parsed;
    float gx_parsed;
    float gy_parsed;
    float gz_parsed;
}IMUParsed;

//state machine for the logging, each message will have an id with its state so it can be logged properly
typedef enum
{
    SENSOR_IMU,
    SENSOR_BARO,
    SENSOR_TEMP
}sensor_id;

typedef struct 
{
    float parsed_temperature;
    float parsed_humidity;
}SHT_31;



typedef struct{
    sensor_id id,
        union{
            IMUParsed imu;
            SHT_31 sht;
            uint32_t pressure; //for the bmp280
        }logging_payload;
}sensor_message;





void configure_i2c_peripheral();
bool IMU_data_exfil(IMUParsed *parsed_data);
void ImuTask(void* pv);
void LoggingTask(void* pv);

void BaroTask(void* pv);
void tempTask(void pv);
void baro_exfil(uint32_t pressure);
void temp_exfil(SHT_31 *sht);






