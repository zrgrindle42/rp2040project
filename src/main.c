#include "pico/stdlib.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "sensor.h"

static QueueHandle_t xImuQueue;

//look into interrupts for this task when more stuff gets added in
void ImuTask(void* pv)
{
   TickType_t xLastWakeTime = xTaskGetTickCount();
   IMUParsed localimutask;

    while(1){
      
      
      if(IMU_data_exfil(&localimutask))
      {
        xQueueSend(xImuQueue, (void*) &localimutask, 0);
      }
      else
      {
        printf("IMU read error\n");
      }
      
      vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
    }
}

void ImuLoggingTask(void* pv)
{
    IMUParsed imureceived;

    while (1) {
        //blocks until queue is ready
        if (xQueueReceive(xImuQueue, &imureceived, portMAX_DELAY) == pdTRUE) {
            printf("ax %f | ay %f | az %f\n", (float)imureceived.ax_parsed, (float)imureceived.ay_parsed, (float)imureceived.az_parsed);
            printf("--------------------------------\n");
            printf("gx %f | gy %f | gz %f\n", (float)imureceived.gx_parsed, (float)imureceived.gy_parsed, (float)imureceived.gz_parsed);
            
        }

    }
}


int main()
{
     stdio_init_all();
     sleep_ms(2000);
    configure_i2c_peripheral();
    xImuQueue = xQueueCreate(16, sizeof(IMUParsed));
    xTaskCreate(ImuTask,
                "imu task",
                256,
                NULL, 
                1,
                NULL
    );

     xTaskCreate(ImuLoggingTask,
                "logging task",
                512,
                NULL,
                2,
                NULL
    );

    vTaskStartScheduler();

    while(1)
    {
    }
}