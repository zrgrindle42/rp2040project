#include "sensor.h"
#include "pico/error.h"

//use mutex for the potential different i2c buses being used between i2c0/i2c1
//add dma interrupts whenever grapyics begin to be added

//rp2040 is using i2c1 bus
enum gpio_function_rp2040;
enum gpio_irq_level; //this has values that see if edges go high/low, or if pullups go high or low
//refer to rasp pi docs hardeware_gpio for enum values

uint8_t imupacket[1] = {0x35};    //page 31 if tge imu docs 

uint8_t imu_buffer[16];
uint8_t config_buffer[2] = {0x02, 0x40};
uint8_t accel_settings[2] = {0x03, 0x22};
uint8_t gyro_settings[2] = {0x04, 0x54};
uint8_t lpf[2] = {0x06, 0x11};
uint8_t start_buffer[2] = {0x08, 0x03};

uint8_t sht_command[2] = {0x24, 0x00};
uint8_t sht_buffer[6];
uint8_t bmp_buffer[16];



//write to ctrl8 register if you ant to do any motion sensor stuff

void configure_i2c_peripheral()
{
i2c_init(i2c1, 100 * 1000); //set up clock and bus used
gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C); // gpio is part of an enum to set the pin as i2c, initialize its enum 
gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
gpio_pull_up(I2C_SDA_PIN); //now we have to have conditions for tis to be pulled down ????
gpio_pull_up(I2C_SCL_PIN);
//now we configure the imu registers here to prepare the byte reads of 
i2c_write_blocking(i2c1, PERIPHERAL_ADDRESS, config_buffer, 2,false );
i2c_write_blocking(i2c1, PERIPHERAL_ADDRESS, accel_settings, 2, false);
i2c_write_blocking(i2c1, PERIPHERAL_ADDRESS, gyro_settings, 2, false);
i2c_write_blocking(i2c1, PERIPHERAL_ADDRESS, lpf, 2, false);
i2c_write_blocking(i2c1, PERIPHERAL_ADDRESS, start_buffer, 2, false);

bmp280_init(&bmp, BMP280_I2C, BMP280_I2C_ADDRESS_1);


sleep_ms(500);

//configure master, already done in i2c init//

//enable inteerupts dma, etc. //DO LATER
}

bool IMU_data_exfil(IMUParsed *parsed_data) // last commit modified a global var, pass this pointer in
{
    
    //once tasks build up, use mutex to keep guard of bus resource during read/write
     
    int write = i2c_write_blocking(i2c1, PERIPHERAL_ADDRESS, imupacket, 1, false); 
    if(write == PICO_ERROR_GENERIC) return false;
    int read = i2c_read_blocking(i2c1, PERIPHERAL_ADDRESS, imu_buffer, sizeof(imu_buffer), false);
    if(read == PICO_ERROR_GENERIC) return false;

    //printf("raw axl%02X axh%02X | ayh%02X| ayl%02X| azl%02X  | azh%02X| %02X| %02X | %02X | %02X | %02X | %02X\n",
         //  imu_buffer[0], imu_buffer[1], imu_buffer[2], imu_buffer[3], imu_buffer[4], imu_buffer[5], imu_buffer[6],
         // imu_buffer[7], imu_buffer[8], imu_buffer[9], imu_buffer[10], imu_buffer[11]);
    int16_t ax = (int16_t)imu_buffer[1] << 8  | imu_buffer[0];
    int16_t ay = (int16_t)imu_buffer[3] << 8  | imu_buffer[2];
    int16_t az = (int16_t)imu_buffer[5] << 8  | imu_buffer[4];
    int16_t gx = (int16_t)imu_buffer[7] << 8  | imu_buffer[6];
    int16_t gy = (int16_t)imu_buffer[9] << 8  | imu_buffer[8];
    int16_t gz = (int16_t)imu_buffer[11] << 8  | imu_buffer[10];
    //printf("raw16 ax=%d ay=%d az=%d gx=%d gy=%d gz=%d\n",
       // imu.ax, imu.ay, imu.az, imu.gx, imu.gy, imu.gz);

    parsed_data->ax_parsed = (float)ax / ACCEL_SCALE;
    parsed_data->ay_parsed = (float)ay / ACCEL_SCALE;
    parsed_data->az_parsed = (float)az / ACCEL_SCALE;
    parsed_data->gx_parsed = (float)gx / GYRO_SCALE; 
    parsed_data->gy_parsed = (float)gy / GYRO_SCALE;
    parsed_data->gz_parsed = (float)gz / GYRO_SCALE;

    return true;

}

bool temp_exfil(SHT_31 *sht) //make this the third task, temp chip heats up everytime it is polled, poll as little as possible
{
    if(xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE) // block until key is ready
    {
        int write = i2c_write_blocking(i2c0, SHT_PERIPHERAL, sht_command, 2, false); 
        if(write == PICO_ERROR_GENERIC) return false;
        
        sleep_ms(30);//give temp sensor time to config, according to documentation
        
        int read = i2c_read_blocking(i2c0, SHT_PERIPHERAL, sht_buffer, sizeof(sht_buffer), false);
        if(read == PICO_ERROR_GENERIC) return false;
        
        xSemaphoreGive(i2c_mutex);
    }
     
        int16_t raw_temp = (sht_buffer[1] << 8) | sht_buffer[0];      //buffer parsing logic
        int16_t raw_humidity = (sht_buffer[3] << 8) | sht_buffer[4];

        float humidity = 100f *((float)raw_humidity / 65535.0f);
        float temp = -49.0f + 315.0f * ((float)raw_temp / 65535.0f);

        sht->parsed_temperature = temp;
        sht->parsed_humidity = humidity;

     return true;
}

//bmp280_handle_t bmp;
//bmp280_sensors_data_t bmpdata;
//bmp280_init(&bmp, BMP280_I2C, BMP280_I2C_ADDRESS_1);
//bmp280_get_all(&bmp, &bmp_data);

bool baro_exfil(bmp280_handle_t *bmp) // moight have to add the struct needed to calc evertything
{
    bmp280_handle_t bmp;
    bmp280_sensors_data_t bmpdata;
    if(xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE) // block until key is ready
    {
        int write = i2c_write_blocking(i2c0, BMP280_I2C_ADDRESS_1, BMP280_REGISTER_ADDRESS_PRESSURE_MSB, 2, false); 
        if(write == PICO_ERROR_GENERIC) return false;
        
        sleep_ms(30);//give temp sensor time to config, according to documentation
        
        int read = i2c_read_blocking(i2c0, SHT_PERIPHERAL, bmp_buffer , sizeof(sht_buffer), false);
        if(read == PICO_ERROR_GENERIC) return false;
        
        xSemaphoreGive(i2c_mutex);
    }

    bmp280_get_all(&bmp, &bmp_data);

    return true;

}
//more thna likely incorrect. see how data flows in to call the parsing methods



//new methods:
//parse the barometer humidity and temperature data, and add to the queue for the logging task to print out.
// what we will need; two wueues for exclusicvity, mutex for the separate buses, and then we can add a task for the barometer data exfiltration and logging.
//as well as one for temperature data exfiltration and logging.
//may need interrupt for the barometer data exfiltration task to ensure we are getting accurate data, and not just the most recent data when the task is scheduled to run.









//registers to know
//IC_DATA_CMD :controls start or stop conditions. if start/stop bit is 1, STOP bit is issued, 0 for start  
//0-7 is data, 8 is the cmd(set this to 0 for write, 1 for read), 9 is the start stop 
//handkled by reafd/write() method



//IC_CON: WRITE TO BIT 6 to disable slave mode and write to bit 0 with 1 to ensure master mode


//imu registers to known 
//CTRL2 0x03 accelerometer output data page 31 for bit details
//CTRL3 0x04 gyroscope output.     32
//CTRL7 0x08 enable sensors
//CTRL8 0x09 motion detction control

//look page 27 in imu docs for all individual accelerometer data page 38 for bit breakdown




