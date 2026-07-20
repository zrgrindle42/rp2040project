#include "sensor.h"
#include "pico/error.h"

//use mutex for the potential different i2c buses being used between i2c0/i2c1
//add dma interrupts whenever grapyics begin to be added

//rp2040 is using i2c1 bus
enum gpio_function_rp2040;
enum gpio_irq_level; //this has values that see if edges go high/low, or if pullups go high or low
//refer to rasp pi docs hardeware_gpio for enum values

uint8_t imupacket[1] = {0x35};    //page 31 if tge imu docs 

uint8_t rx_buffer[16];
uint8_t config_buffer[2] = {0x02, 0x40};
uint8_t accel_settings[2] = {0x03, 0x22};
uint8_t gyro_settings[2] = {0x04, 0x54};
uint8_t lpf[2] = {0x06, 0x11}; // was 0x11
uint8_t fifo[2] = {0x14, 0x00};
uint8_t start_buffer[2] = {0x08, 0x83};

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
i2c_write_blocking(i2c1, PERIPHERAL_ADDRESS, fifo, 2, false);
sleep_ms(500);

//configure master, already done in i2c init//

//enable inteerupts dma, etc. //DO LATER
}

bool IMU_data_exfil(IMUParsed *parsed_data) // last commit modified a global var, pass this pointer in
{
    
    //once tasks build up, use mutex to keep guard of bus resource during read/write
     uint8_t statusint_reg[1] = {0x2D};
    uint8_t statusint;

    int w1 = i2c_write_blocking(i2c1, PERIPHERAL_ADDRESS, statusint_reg, 1, true);
    if (w1 != 1) return false;
    int r1 = i2c_read_blocking(i2c1, PERIPHERAL_ADDRESS, &statusint, 1, false);
    if (r1 != 1) return false;

    if (!(statusint & 0x01)) {
        return false;  // data not available yet, skip this cycle
    }

    uint8_t status0_reg[1] = {0x2E};
    uint8_t status0;
    int w2 = i2c_write_blocking(i2c1, PERIPHERAL_ADDRESS, status0_reg, 1, true);
    if (w2 != 1) return false;
    int r2 = i2c_read_blocking(i2c1, PERIPHERAL_ADDRESS, &status0, 1, false);
    if (r2 != 1) return false;
    // this read just locked the data — now burst-read immediately
    int write = i2c_write_blocking(i2c1, PERIPHERAL_ADDRESS, imupacket, 1, true); 
    if(write == PICO_ERROR_GENERIC) return false;
    int read = i2c_read_blocking(i2c1, PERIPHERAL_ADDRESS, rx_buffer, sizeof(rx_buffer), false);
    if(read == PICO_ERROR_GENERIC) return false;

    //printf("raw axl%02X axh%02X | ayh%02X| ayl%02X| azl%02X  | azh%02X| %02X| %02X | %02X | %02X | %02X | %02X\n",
         //  rx_buffer[0], rx_buffer[1], rx_buffer[2], rx_buffer[3], rx_buffer[4], rx_buffer[5], rx_buffer[6],
         // rx_buffer[7], rx_buffer[8], rx_buffer[9], rx_buffer[10], rx_buffer[11]);
    int16_t ax = (int16_t)rx_buffer[1] << 8  | rx_buffer[0];
    int16_t ay = (int16_t)rx_buffer[3] << 8  | rx_buffer[2];
    int16_t az = (int16_t)rx_buffer[5] << 8  | rx_buffer[4];
    int16_t gx = (int16_t)rx_buffer[7] << 8  | rx_buffer[6];
    int16_t gy = (int16_t)rx_buffer[9] << 8  | rx_buffer[8];
    int16_t gz = (int16_t)rx_buffer[11] << 8  | rx_buffer[10];
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

//FIND PLACE FOR THIS LATER // handled by config files in lib import
void configure_spi()
{
    spi_init(spi1, 40000 * 1000);
    gpio_set_function(LCD_CLK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(LCD_MOSI_PIN, GPIO_FUNC_SPI);

    gpio_init(LCD_DC_PIN); gpio_set_dir(LCD_DC_PIN, GPIO_OUT);
    gpio_init(LCD_CS_PIN); gpio_set_dir(LCD_CS_PIN, GPIO_OUT );
    gpio_init(LCD_CLK_PIN); gpio_set_dir(LCD_CLK_PIN, GPIO_OUT);
    gpio_init(LCD_RST_PIN); gpio_set_dir(LCD_RST_PIN, GPIO_OUT );
   // gpio_put(LCD_CS_PIN, 1); // keep it not selected for now


   
}
//gpio init gpio set function 0 =gpio_in 1 = gpioout
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