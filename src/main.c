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
static QueueHandle_t xADCQueue;
uint16_t screen_frame_buffer[240 * 240];
char ax_buffer[16];
char ay_buffer[16];
char az_buffer[16];
char gx_buffer[16];
char gy_buffer[16];
char gz_buffer[16];

char adc_buffer[64];
TaskHandle_t adc_task_handle = NULL;

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
      
      vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));
    }
}

void ImuDisplayandLoggingTask(void* pv)
{
    IMUParsed imureceived;
    voltageData voltage;
    LCD_1IN28_Init(0); //reset chip select, data lines
    DEV_SET_PWM(100);   //backligh on
   
    //printf("STEP 2 INIT LCD\n");

    Paint_NewImage((UBYTE*)screen_frame_buffer, 240, 240, 0, WHITE);
    //printf("STEP 3 new image\n");
    Paint_SetScale(65); //LAST MADE CHANGE research on this tomorrow get a better grasp of the boot
    if((UBYTE*) screen_frame_buffer == NULL)
    {
        printf("hung\n");
        exit(0);
    } 
   // printf("STEP 4 scale \n"); //print pointer if getting continuous issues
    Paint_Clear(BRRED); //clears in ram
    //LCD_1IN28_Clear(WHITE); // too expensive rn
    
  
  

    while (1) {
        

       // printf("STEP 5  CLEAR\n");
               Paint_SetRotate(ROTATE_0);
              // printf("STEP 6 rotate\n");
             //Paint_SelectImage((UBYTE*)screen_frame_buffer);

              Paint_Clear(GREEN); //clears in ram
              //printf("STEP 7\n");

        //blocks until queue is ready
        if (xQueueReceive(xImuQueue, &imureceived, 0 ) == pdTRUE) {
            
           
           
            
            //Paint_DrawCircle(120,120,20, GRAY, 3, DRAW_FILL_FULL);
             
            //move to snprintf as project grows, sprintf is too heavy
            snprintf(ax_buffer,sizeof(ax_buffer), "ax: %.2f", imureceived.ax_parsed);
            Paint_DrawString_EN(90, 65, ax_buffer, &Font16, BLACK, GREEN);

            snprintf(ay_buffer,sizeof(ay_buffer), "ay: %.2f", imureceived.ay_parsed);
            Paint_DrawString_EN(90, 90, ay_buffer, &Font16, BLACK, GREEN);

            snprintf(az_buffer,sizeof(az_buffer), "az: %.2f", imureceived.az_parsed);
            Paint_DrawString_EN(90, 105, az_buffer, &Font16, BLACK, GREEN);

            snprintf(gx_buffer,sizeof(gx_buffer), "gx: %.2f", imureceived.gx_parsed);
            Paint_DrawString_EN(90, 130, gx_buffer, &Font16, BLACK, GREEN);
            //printf("gxbuffer: %.2f, %.2f,%.2f,%.2f\n",gx_buffer[0], gx_buffer[1], gx_buffer[2], gx_buffer[3]);

            snprintf(gy_buffer,sizeof(gy_buffer), "gy: %.2f", imureceived.gy_parsed);
            Paint_DrawString_EN(90, 155, gy_buffer, &Font16, BLACK, GREEN);

            snprintf(gz_buffer,sizeof(gz_buffer), "gz: %.2f", imureceived.gz_parsed);
            Paint_DrawString_EN(90, 180, gz_buffer, &Font16, BLACK, GREEN);

          

            //log for proof of data being received for debug
            printf("ax %f | ay %f | az %f\n", (float)imureceived.ax_parsed, (float)imureceived.ay_parsed, (float)imureceived.az_parsed);
            printf("--------------------------------\n");
            printf("gx %f | gy %f | gz %f\n", (float)imureceived.gx_parsed, (float)imureceived.gy_parsed, (float)imureceived.gz_parsed);
           // printf("step 8 after the print of the imu\n");
        }

        if(xQueueReceive(xADCQueue, &voltage, 0) == pdTRUE)
        {
            //printf("in the xqueuereceive sprintf\n");
            snprintf(adc_buffer,sizeof(adc_buffer), "v: %.2f", voltage.meanReceived);
            //printf("after sprintf\n");
            Paint_DrawString_EN(90, 195, adc_buffer, &Font16, BLACK, GREEN);
            
            //vTaskDelay(pdMS_TO_TICKS(100));

            printf("voltage %.2f\n", voltage.meanReceived);
            //printf("voltage %.2f, %.2f, %.2f\n", adc_buffer[0], adc_buffer[1], adc_buffer[2]);
        }
          LCD_1IN28_Display(screen_frame_buffer); // bug fix to get voltage to display too

          vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void AdcParseTask(void* pv)
{
   const uint16_t* loc_buff;
   int sumOfAll = 0;
   voltageData voltage;


   
    while(1)
    {
     // printf("made it indie the ad voltage task\n");
        
      
      
      if(ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0)
        {
            //handle the event
            if(which_buffer == 1)
            {
               printf("which_buffer: %d\n", which_buffer);
               printf("buffer b %f\n" , dma.dma_buffer_b);

                loc_buff = dma.dma_buffer_b;

                 
                for(int i = 0; i <ADC_BUFFER_SIZE; i++)
                {
                    sumOfAll += loc_buff[i];
                }

                // printf("sumofall %f\n" , sumOfAll);
                voltage.meanReceived = sumOfAll / (float)ADC_BUFFER_SIZE;
                xQueueSend(xADCQueue, (void*) &voltage, 0);
                  sumOfAll = 0;
                
            }
            else if(which_buffer == 0)
            {
                loc_buff = dma.dma_buffer_a;
                printf("which_buffer: %d\n", which_buffer);
                printf("buffer a %f\n" , dma.dma_buffer_a);
                for(int i = 0; i < ADC_BUFFER_SIZE; i++)
                {
                    sumOfAll += loc_buff[i];
                }
                
                voltage.meanReceived  = sumOfAll / (float)ADC_BUFFER_SIZE;
                xQueueSend(xADCQueue, (void*) &voltage, 0);
                sumOfAll = 0;
              
            }
            //convert these numbers to get a parseable voltage number

        }
       // printf("made it to end of the adc voltage PARSE task\n");
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
    init_adc();
    
    
    xImuQueue = xQueueCreate(16, sizeof(IMUParsed));
    xADCQueue =  xQueueCreate(16, sizeof(voltageData));
    
    xTaskCreate(ImuTask,
                "imu task",
                256,
                NULL, 
                3,
                NULL
    );

    xTaskCreate(AdcParseTask,
                "adc parsing task",
                724,
                NULL, 
                1,// do not need to read the voltage that much... interrupt essentially just puts it ina. cmd queue
                &adc_task_handle
    );


    
    //maybe make a stack overflow hook method if nthis ever stops working

     xTaskCreate(ImuDisplayandLoggingTask,
                "logging task",
                1024 * 3, // takes a lot of memory to display
                NULL,
                2,
                NULL
    );

    vTaskStartScheduler();

    while(1)
    {
    }
}