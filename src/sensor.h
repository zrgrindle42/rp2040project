#pragma once
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include "hardware/dma.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "LCD_1in28.h"
#include "GUI_Paint.h"
#include "DEV_Config.h"

#define PERIPHERAL_ADDRESS 0x6B
#define IMU_DATA_REG 0x35
#define IMU_CONFIG 0x22
#define ACCEL_SCALE 4096.0f
#define GYRO_SCALE 65.536f
#define I2C_SPEED_STANDARD 100000
#define I2C_SDA_PIN 6
#define I2C_SCL_PIN 7

#define ADC_BUFFER_SIZE 1024

// #define LCD_DC_PIN 8
// #define LCD_CS_PIN 9
// #define LCD_CLK_PIN 10
// #define LCD_MOSI_PIN 11
// #define LCD_RST_PIN 12
// #define LCD_BL_PIN 25

extern volatile uint8_t which_buffer;

extern TaskHandle_t adc_task_handle;

typedef struct
{
    uint16_t dma_buffer_a[ADC_BUFFER_SIZE];
    uint16_t dma_buffer_b[ADC_BUFFER_SIZE];
}DMA;

extern DMA dma;

typedef struct
{
    float ax_parsed;
    float ay_parsed;
    float az_parsed;
    float gx_parsed;
    float gy_parsed;
    float gz_parsed;
}IMUParsed;

typedef struct{
    float meanReceived; //for voltage
}voltageData;

void configure_i2c_peripheral();
bool IMU_data_exfil(IMUParsed *parsed_data);
void ImuTask(void* pv);
void ImuDisplayandLoggingTask(void* pv);

void init_adc();
void AdcParseTask(void* pv);
void BufferTriggerADC();

void configure_spi();