#include "sensor.h"

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
uint8_t lpf[2] = {0x06, 0x11};
uint8_t start_buffer[2] = {0x08, 0x03};

//write to ctrl8 register if you ant to do any motion sensor stuff
IMU imu;
IMUParsed parsedimu;

void configure_i2c_peripheral()
{
i2c_init(i2c1, 100 * 1000); //set up clock and bus used
gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C); // gpio is part of an enum to set the pin as i2c, initialize its enum 
gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
gpio_pull_up(I2C_SDA_PIN); //now we have to have conditions for tis to be pulled down ????
gpio_pull_up(I2C_SCL_PIN);
//now we configure the imu registers here to prepare the byte reads of 
i2c_write_register(PERIPHERAL_ADDRESS, config_buffer, 2,false );
i2c_write_register(PERIPHERAL_ADDRESS, accel_settings, 2, false);
i2c_write_register(PERIPHERAL_ADDRESS, gyro_settings, 2, false);
i2c_write_register(PERIPHERAL_ADDRESS, lpf, 2, false);
i2c_write_register(PERIPHERAL_ADDRESS, start_buffer, 2, false);
sleep_ms(500);

//configure master, already done in i2c init//

//enable inteerupts dma, etc. //DO LATER
}



//look at the imu.rp2040 registers you may need to set for this operation
void i2c_write_register( uint8_t periph_address, const uint8_t *packet, size_t buffer_size, bool stop)
{
    //send register start bit low, do not need to do this wrapped in with the read write methods//
    
    i2c_write_blocking(i2c1, periph_address, packet, buffer_size, stop);//addr is the ddevice to write to, its address, and message is the data to send
}



void i2c_read_register(uint8_t *buffer, uint8_t address)
{
    i2c_read_blocking(i2c1, address, buffer, 16, false);
}

void IMU_data_exfil()
{
    
    //once tasks build up, use mutex to keep guard of bus resource during read/write
     i2c_write_register(PERIPHERAL_ADDRESS, imupacket, 1, false); // reatin control of bus when a read immedaitely follows for more accurate data
     i2c_read_register(rx_buffer, PERIPHERAL_ADDRESS);

    //printf("raw axl%02X axh%02X | ayh%02X| ayl%02X| azl%02X  | azh%02X| %02X| %02X | %02X | %02X | %02X | %02X\n",
         //  rx_buffer[0], rx_buffer[1], rx_buffer[2], rx_buffer[3], rx_buffer[4], rx_buffer[5], rx_buffer[6],
         // rx_buffer[7], rx_buffer[8], rx_buffer[9], rx_buffer[10], rx_buffer[11]);
    imu.ax = (int16_t)rx_buffer[1] << 8  | rx_buffer[0];
    imu.ay = (int16_t)rx_buffer[3] << 8  | rx_buffer[2];
    imu.az = (int16_t)rx_buffer[5] << 8  | rx_buffer[4];
    imu.gx = (int16_t)rx_buffer[7] << 8  | rx_buffer[6];
    imu.gy = (int16_t)rx_buffer[9] << 8  | rx_buffer[8];
    imu.gz = (int16_t)rx_buffer[11] << 8  | rx_buffer[10];
    //printf("raw16 ax=%d ay=%d az=%d gx=%d gy=%d gz=%d\n",
       // imu.ax, imu.ay, imu.az, imu.gx, imu.gy, imu.gz);

    parsedimu.ax_parsed = (float)imu.ax / 4096.0f;
    parsedimu.ay_parsed = (float)imu.ay / 4096.0f;
    parsedimu.az_parsed = (float)imu.az / 4096.0f;
    parsedimu.gx_parsed = (float)imu.gx / 65.536f;
    parsedimu.gy_parsed = (float)imu.gy / 65.536f;
    parsedimu.gz_parsed = (float)imu.gz / 65.536f;

}











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