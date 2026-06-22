#include "pico/stdlib.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "sensor.h"


static QueueHandle_t xLoggingQueue;
SemaphoreHandle_t i2c_mutex = NULL;

//look into interrupts for this task when more stuff gets added in
void ImuTask(void* pv)
{
   TickType_t xLastWakeTime = xTaskGetTickCount();
   sensor_message msg;
   msg.id = SENSOR_IMU;

    while(1){
      if(IMU_data_exfil(&msg.IMUParsed))
      {
        xQueueSend(xLoggingQueue, (void*) &msg.IMUParsed, 0);
      }
      else
      {
        printf("IMU read error\n");
      }
      
      vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
    }
}

void LoggingTask(void* pv)
{
    sensor_message msg;


    while (1) {

        if (xQueueReceive(xLoggingQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            switch(&msg.id)
            {
                case SENSOR_IMU:
                
                    printf("ax %f | ay %f | az %f\n", (float)imureceived.ax_parsed, (float)imureceived.ay_parsed, (float)imureceived.az_parsed);
                    printf("--------------------------------\n");
                    printf("gx %f | gy %f | gz %f\n", (float)imureceived.gx_parsed, (float)imureceived.gy_parsed, (float)imureceived.gz_parsed);
                    printf("--------------------------------\n");
                break;
                

                case SENSOR_TEMP:
                    printf("temp: %f\n", msg.sht.parsed_temperature);
                    printf("--------------------------------\n");
                    printf("Humidity %f\n", msg.sht.parsed_humidity);
                    printf("--------------------------------\n"); 
                break;

            }
        }
        //blocks until queue is ready
        

    }
}

void tempTask(void* pv)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    sensor_message msg;
    msg.id = SENSOR_TEMP;
    
    while(1)
    {
        if(temp_exfil(&msg.sht))
        {
            xQueueSend(xLoggingQueue, (void*) &msg.sht, 0);
        }

        else{
            printf("SHT-31 read error\n");
        }


    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(3000)); // HEAVY DELAY TIME DO TO THE WANT OF ACCURATE TEMP READINGS DUE TO TEMP CHIP HEATUP
    }
}

void baroTask(void* pv)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    sensor_message msg;
    msg.id = SENSOR_BARO;
    
    while(1)
    {
        //if(baroexfil(//?)) //need to put somethong like the bmp struct to fit the parameter but then a variable that reads just the pressure
        
            xQueueSend(xLoggingQueue, (void*) , 0);
        }

        else{
            printf("baro bmp280 read error\n");
        }


    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000)); // HEAVY DELAY TIME DO TO THE WANT OF ACCURATE TEMP READINGS DUE TO TEMP CHIP HEATUP
}





int main()
{
     stdio_init_all();
     sleep_ms(2000);
    i2c_mutex = xSemaphoreCreateMutex(); // need mutex for the sht and bmp since they share the same bus
    
    configure_i2c_peripheral();
    
   
    xloggingQueue = xQueueCreate(16, sizeof(sensor_message));
    
    
    xTaskCreate(ImuTask,
                "imu task",
                256,
                NULL, 
                1,
                NULL
    );

     xTaskCreate(LoggingTask,
                "logging task",
                512,
                NULL,
                4,
                NULL
    );

     xTaskCreate(tempTask,
                "temp task",
                256,
                NULL,
                3,
                NULL
    );

      xTaskCreate(baroTask,
                "temp task",
                256,
                NULL,
                2,
                NULL
    );

    vTaskStartScheduler();

    while(1)
    {
    }
}
