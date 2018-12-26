# berry_mcu
The Berry language for STM32F103.

## Build

1. Install the [GNU Arm Embedded Toolchain](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm).
2. Connect to the MCU using ST Link V2.
3. Enter the following commands:
   ``` bash
   git clone --recursive https://github.com/gztss/berry_mcu.git
   cd berry_mcu
   make
   make download # download to MCU
   ```
