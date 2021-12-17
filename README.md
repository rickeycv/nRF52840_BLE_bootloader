# nRF52840_Quectel_bootloader
## Solution's path
nRF5_SDK_17.1.0_ddde560_bootloader\examples\dfu\secure_bootloader\pca10056_s140_ble\ses
## Features
- RX frame is 1024 bytes long
- .bin and .dat file name must be 32 bytes long max (including null character, extension no needed)
- Max .bin file size is 778240 bytes (10 minutes avg. via BG77)
- 3 FLASH pages previous to bootloader area are destinated for main app data and will not be erase by the bootloader
- FLASH memory layout:

|Region|Memory range|
|---|---|
| MBR| 0x00000000 - 0x00000fff|
| SoftDevice | 0x00001000 - 0x00026fff |
| Main app | 0x00027000 - 0x000e4fff |
| App data | 0x000e5000 - 0x000e7fff |
| Bootloader | 0x000e8000 - 0x000fdfff |
| Bootloader settings | 0x000fe000 - 0x000fffff |

## Instructions
- **FW test files** folder includes the FW test files I have been using.
- FOTA.txt includes FW test files names (the max length of names are 32 bytes, including null character, do not provide .bin or .dat extensions).
- **FW test files/ble_app_uart_signed_0x100_SD_enable_dfu_v1_0.zip** is a UART-BLE bridge app, it was modified to launch the bootloader sending
'e' plus enter via UART (this can be done in the SerialPortBridge app, pressing the **Enter DFU** button).
## Next steps
- Check for missing error paths.
