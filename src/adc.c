//includes here

#include "sensor.h"


#define ADC_MAX_VAL 4095.0f


int dma_chan_a;
int dma_chan_b;
//declare channel variables


volatile uint8_t which_buffer = 0; //0 = a 1 = b

DMA dma;

//configure the hardware
//initialize ad cpin, set gpio, configure teh dam haneel,etc.
void init_adc()
{
    adc_init();
    adc_gpio_init(26); //using pin 26;
    adc_select_input(0);

    adc_fifo_setup(true, true, 1, false, false);
    adc_set_clkdiv(96000.0f); // the rate at which the buffer fills, before at 960 and was starving imu task of cycles due to the interrupt

    dma_chan_a = dma_claim_unused_channel(true);
    dma_chan_b = dma_claim_unused_channel(true);

    dma_channel_config cfg_a = dma_channel_get_default_config(dma_chan_a);//
    dma_channel_config cfg_b = dma_channel_get_default_config(dma_chan_b);//the var type is  a struct that wraps a 32 bit int
    //each bit has its own job to do

    channel_config_set_transfer_data_size(&cfg_a, DMA_SIZE_16); //sets the size of each dma bus transfer
    channel_config_set_read_increment(&cfg_a, false); //set or net set a channel read increment
    channel_config_set_write_increment(&cfg_a, true);  
    channel_config_set_dreq(&cfg_a, DREQ_ADC); //select a transfer request signa; in channel config object
    //this limits the amount of data beiung sent at one time to match how quickly the adc gets data
    channel_config_set_chain_to(&cfg_a, dma_chan_b); //sets the channel change 

   

    
    

     channel_config_set_transfer_data_size(&cfg_b, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg_b, false);
    channel_config_set_write_increment(&cfg_b, true);     //understand what all this means and comment on each line
    channel_config_set_dreq(&cfg_b, DREQ_ADC);
    channel_config_set_chain_to(&cfg_b, dma_chan_a);

    dma_channel_configure(dma_chan_a, &cfg_a, dma.dma_buffer_a, &adc_hw-> fifo, 1024, false);
    dma_channel_configure(dma_chan_b, &cfg_b, dma.dma_buffer_b, &adc_hw-> fifo, 1024, false);
    
     dma_channel_set_irq0_enabled(dma_chan_a, true);
    dma_channel_set_irq0_enabled(dma_chan_b, true);

    irq_set_exclusive_handler(DMA_IRQ_0, BufferTriggerADC);
    irq_set_enabled(DMA_IRQ_0, true);


    dma_channel_start(dma_chan_a);
    adc_run(true);


    


    
    
    
    
    
    //channel_config_set_transfer_data_size(&cfg_, DMA_SIZE_16);


}  


 
//using the task notififcation method instead of a binary semaphore

void BufferTriggerADC()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
   
    if(dma_channel_get_irq0_status(dma_chan_a)) //if a is full
    {
        dma_channel_acknowledge_irq0(dma_chan_a); //clear the dma channel
        which_buffer = 0; // set the flag
        dma_channel_set_write_addr(dma_chan_a, dma.dma_buffer_a, false);
        vTaskNotifyGiveFromISR(adc_task_handle, &xHigherPriorityTaskWoken);

    }

     if(dma_channel_get_irq0_status(dma_chan_b))
    {
        dma_channel_acknowledge_irq0(dma_chan_b); //clear the dma channel
        which_buffer = 1; // set the flag
        dma_channel_set_write_addr(dma_chan_b, dma.dma_buffer_b, false);
        vTaskNotifyGiveFromISR(adc_task_handle, &xHigherPriorityTaskWoken);

    }

   portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

}


//in task have logic for processing the which buffer variabke
//ulTaskNotify take to take the isr signal, it blocks until notifies by the isr
