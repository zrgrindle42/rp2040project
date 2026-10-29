# RP2040 FreeRTOS Smartwatch

FreeRTOS firmware for a smartwatch built on the Waveshare RP2040-LCD-1.28, a small RP2040 board with an onboard round display and IMU. It shows live IMU and ADC readings on the round LCD.

running product https://drive.google.com/file/d/1Z2q3aRLOh3xfTAMVfIFaQKTdapT_zn2o/edit the log
                  https://drive.google.com/file/d/120xHSA0CCEXuBB70KdK_qf8ZDDMpgOzX/edit   the watch

## Hardware

| Part | Interface | Notes |
| --- | --- | --- |
| Waveshare RP2040-LCD-1.28 | — | RP2040 MCU with onboard display and IMU |
| GC9A01 1.28" round LCD (240×240) | SPI | Onboard |
| QMI8658C 6-axis IMU | I2C (i2c1, GPIO 6/7) | Onboard accelerometer and gyroscope |
| Analog input | ADC0 (GPIO 26) | Sampled through DMA |
| SHT31 and BMP280 | I2C (i2c0) | External temperature/humidity and pressure sensors, in progress |

## How it works

Three FreeRTOS tasks pass data to each other through queues:

| Task | Priority | What it does |
| --- | --- | --- |
| IMU | 3 (highest) | Every 50 ms, checks the IMU's data-ready flag, reads accelerometer and gyro data over I2C, and queues the parsed values |
| Display and logging | 2 | Every 100 ms, draws the latest readings into a frame buffer, pushes it to the LCD, and logs them over USB serial |
| ADC | 1 | Waits for a notification from the DMA interrupt, averages the finished buffer to one reading, and queues it |

ADC sampling uses DMA ping-pong buffering. Two chained DMA channels fill two 1,024-sample buffers in turn. When one fills, the DMA interrupt notifies the ADC task to average it while the other buffer keeps filling, so the CPU never handles individual samples. The ADC sample rate was lowered after the DMA interrupts were found to be starving the IMU task.

## Status

- **Working:** IMU readout, LCD output, DMA-buffered ADC sampling.
- **In progress:** SHT31 and BMP280 drivers on a second I2C bus.

## Build and flash

Requires the [Pico SDK](https://github.com/raspberrypi/pico-sdk), the Arm GNU toolchain (`arm-none-eabi-gcc`), and CMake. The FreeRTOS kernel is a submodule, so clone with `--recurse-submodules`.

```
git clone --recurse-submodules https://github.com/zrgrindle42/rp2040project
cd rp2040project
mkdir build && cd build
cmake .. -DPICO_SDK_PATH=/path/to/pico-sdk
cmake --build .
```

On Windows, add `-G "MinGW Makefiles"` to the first `cmake` command.

To flash, hold BOOTSEL while plugging the board into USB, then copy `build/rp2040project.uf2` onto the `RPI-RP2` drive. The firmware waits for a USB serial connection before it starts, so open a serial monitor after flashing.

## Credits

The LCD driver, drawing functions, and fonts in `src/lib` come from Waveshare's RP2040-LCD-1.28 demo code. The IMU driver, ADC/DMA code, and FreeRTOS tasks are my own.
