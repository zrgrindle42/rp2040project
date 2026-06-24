#include "pico/stdlib.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "sensor.h"
#include "LCD_1in28.h"
#include "GUI_Paint.h"
#include "DEV_Config.h"

static QueueHandle_t xImuQueue;
uint16_t screen_frame_buffer[240 * 240];
char ax_buffer[8];
char ay_buffer[8];
char az_buffer[8];
char gx_buffer[8];
char gy_buffer[8];
char gz_buffer[8];

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

void ImuDisplayandLoggingTask(void* pv)
{
    IMUParsed imureceived;
    LCD_1IN28_Init(0); //reset chip select, data lines
    DEV_SET_PWM(100);   //backligh on
   
    printf("STEP 2 INIT LCD\n");

    Paint_NewImage((UBYTE*)screen_frame_buffer, 240, 240, 0, WHITE);
    printf("STEP 3 new image\n");
    Paint_SetScale(65); //LAST MADE CHANGE research on this tomorrow get a better grasp of the boot
    if((UBYTE*) screen_frame_buffer == NULL)
    {
        printf("hung\n");
        exit(0);
    } 
    printf("STEP 4 scale \n"); //print pointer if getting continuous issues
    Paint_Clear(BRRED); //clears in ram
    //LCD_1IN28_Clear(WHITE); // too expensive rn
    
   

    while (1) {
        
        //blocks until queue is ready
        if (xQueueReceive(xImuQueue, &imureceived, portMAX_DELAY) == pdTRUE) {
            
           
             printf("STEP 5  CLEAR\n");
               Paint_SetRotate(ROTATE_0);
               printf("STEP 6 rotate\n");
             //Paint_SelectImage((UBYTE*)screen_frame_buffer);

              Paint_Clear(GREEN); //clears in ram
              printf("STEP 7\n");

            
            //Paint_DrawCircle(120,120,20, GRAY, 3, DRAW_FILL_FULL);
             
            //move to snprintf as project grows, sprintf is too heavy
            sprintf(ax_buffer, "ax: %.2f", imureceived.ax_parsed);
            Paint_DrawString_EN(90, 65, ax_buffer, &Font16, BLACK, GREEN);

            sprintf(ay_buffer, "ay: %.2f", imureceived.ay_parsed);
            Paint_DrawString_EN(90, 90, ay_buffer, &Font16, BLACK, GREEN);

            sprintf(az_buffer, "az: %.2f", imureceived.az_parsed);
            Paint_DrawString_EN(90, 105, az_buffer, &Font16, BLACK, GREEN);

            sprintf(gx_buffer, "gx: %.2f", imureceived.gx_parsed);
            Paint_DrawString_EN(90, 130, gx_buffer, &Font16, BLACK, GREEN);

            sprintf(gy_buffer, "gy: %.2f", imureceived.gy_parsed);
            Paint_DrawString_EN(90, 155, gy_buffer, &Font16, BLACK, GREEN);

            sprintf(gz_buffer, "gz: %.2f", imureceived.gz_parsed);
            Paint_DrawString_EN(90, 180, gz_buffer, &Font16, BLACK, GREEN);

            LCD_1IN28_Display(screen_frame_buffer);

            //log for proof of data being received for debug
            printf("ax %f | ay %f | az %f\n", (float)imureceived.ax_parsed, (float)imureceived.ay_parsed, (float)imureceived.az_parsed);
            printf("--------------------------------\n");
            printf("gx %f | gy %f | gz %f\n", (float)imureceived.gx_parsed, (float)imureceived.gy_parsed, (float)imureceived.gz_parsed);
            
        }

    }
}


int main()
{
    stdio_init_all();
     
    while(!stdio_usb_connected())
    {
        sleep_ms(100);
    }
   printf("init\n");
    sleep_ms(2000);
    configure_i2c_peripheral();
     printf("CHECKPOINT 2: configure the i2c\n");
    DEV_Module_Init(); 
    printf("CHECKPOINT 1: CONIFGURED SPI\n");
    
    
    xImuQueue = xQueueCreate(16, sizeof(IMUParsed));
    
    xTaskCreate(ImuTask,
                "imu task",
                256,
                NULL, 
                1,
                NULL
    );

    
    //maybe make a stack overflow hook method if nthis ever stops working

     xTaskCreate(ImuDisplayandLoggingTask,
                "logging task",
                1024 * 3,
                NULL,
                2,
                NULL
    );

    vTaskStartScheduler();

    while(1)
    {
    }
}