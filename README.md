# nRF52840_Quectel_bootloader
## Solution's path
nRF5_SDK_17.1.0_ddde560_bootloader\examples\dfu\secure_bootloader\pca10056_s140_ble\ses
## Characteristics
RX buffer is 1024 bytes long
.bin and .dat file name must be 32 bytes long max (including null character, extension no needed)
## Instructions
- **FW test files** folder includes the FW test files I have been using.
- FOTA.txt includes FW test files names.
- **FW test files/ble_app_uart_signed_0x100_SD_enable_dfu_v1_0.zip** is a UART-BLE bridge app, it was modified to launch the bootloader sending
'e' plus enter (this can be done in the SerialPortBridge app, pressing the **Enter DFU** button).
## Next steps
- Check for missing error paths.
- Delete project files of adiionals IDEs. 