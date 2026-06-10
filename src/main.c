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

    //gpio_init(25);
    //gpio_set_dir(25, GPIO_OUT);
   TickType_t xLastWakeTime = xTaskGetTickCount();

    while(1){
      IMU_data_exfil();
      
      //debug to see what we are sending
        //printf("RP2040 alivefromimutask \n");
      xQueueSend(xImuQueue, (void*) &parsedimu, sizeof(parsedimu));
      vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
      //uxTaskGetStackHighWaterMark(NULL); //debug this to see if we are getting close to stack overflow
    }
}

void ImuLoggingTask(void* pv)
{
    IMUParsed imureceived; //debug this line to see what goes through the queue
      TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1) {
        //printf("RP2040 alive \n");
        
        if (xQueueReceive(xImuQueue, &imureceived, portMAX_DELAY) == pdTRUE) {
            printf("ax %f | ay %f | az %f\n", (float)imureceived.ax_parsed, (float)imureceived.ay_parsed, (float)imureceived.az_parsed);
            printf("gx %f | gy %f | gz %f\n", (float)imureceived.gx_parsed, (float)imureceived.gy_parsed, (float)imureceived.gz_parsed);
            
        }
        // vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
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
