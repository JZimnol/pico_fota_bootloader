# Raspberry Pi Pico W FOTA Bootloader

This bootloader allows you to perform `Firmware Over The Air` updates with the
Raspberry Pi Pico W board. It contains all required linker scripts that will
adapt your application to the new application memory layout.

The memory layout is as follows:

```
+-------------------------------------------+  <-- __FLASH_START_ADDRESS (0x10000000)
|              Bootloader (36k)             |
+-------------------------------------------+  <-- __FLASH_INFO_APP_HEADER_ADDRESS
|           App Header (256 bytes)          |
+-------------------------------------------+  <-- __FLASH_INFO_DOWNLOAD_HEADER_ADDRESS
|        Download Header (256 bytes)        |
+-------------------------------------------+  <-- __FLASH_INFO_DOWNLOAD_VALID_ADDRESS
|         Download Valid (256 bytes)        |
+-------------------------------------------+  <-- __FLASH_INFO_FIRMWARE_SWAPPED_ADDRESS
|        Firmware Swapped (256 bytes)       |
+-------------------------------------------+
|            Padding (3072 bytes)           |
+-------------------------------------------+  <-- __FLASH_APP_START_ADDRESS
|      Flash Application Slot (1004k)       |
+-------------------------------------------+  <-- __FLASH_DOWNLOAD_SLOT_START_ADDRESS
|        Flash Download Slot (1004k)        |
+-------------------------------------------+
```

## File structure - example

Assume the following file structure:
```
your_project/
├── CMakeLists.txt
├── main.c
├── pico_fota_bootloader/
│   ├── CMakeLists.txt
│   ├── include/
│   │   └── pico_fota_bootloader.h
│   ├── linker_common/
│   │   ├── application.ld
│   │   ├── bootloader.ld
│   │   └── linker_definitions.ld
│   ├── main.c
│   └── src/
│       └── pico_fota_bootloader.c
└── pico_sdk_import.cmake

```

The following files should have the following contents:

### your_project/CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)
...
pico_sdk_init()

add_subdirectory(pico_fota_bootloader)

add_executable(your_app
               main.c)
target_link_libraries(your_app
                      pico_stdlib
                      pico_fota_bootloader_lib)
target_link_options(your_app PRIVATE "-L${CMAKE_CURRENT_SOURCE_DIR}/pico_fota_bootloader/linker_common")
pico_set_linker_script(your_app ${CMAKE_CURRENT_SOURCE_DIR}/pico_fota_bootloader/linker_common/application.ld)
# rest of the file if needed...
```

### your_project/main.c
```c
#include "pico_fota_bootloader.h"
...
int main() {
    ...
    if (pfb_is_after_firmware_update()) {
        // handle new firmare info if needed
    }
    ...

    // initialize download slot before writing into it
    pfb_initialize_download_slot();
    ...

    // acquire the data (e.g. from the web) and write it into the download slot
    // using chunks of N*256 bytes
    for (int i = 0; i < size; i++) {
        (void) pfb_write_to_flash_aligned_256_bytes(src, offset_bytes, len_bytes);
    }
    ...

    // after downloading the binary, mark the download slot as valid - the
    // firmware will be swapped after a reboot
    pfb_mark_download_slot_as_valid();
    ...

    // when you're ready - reboot and perform the upgrade
    pfb_perform_update();
}
```

## Compiling and running - example

### Compiling

Create the build directory and build the project within it.

```shell
# these commands may vary depending on the OS
mkdir build/
cd build
cmake .. && make -j
```

You should have output similar to:

```
build/
└── your_app
    ├── pico_fota_bootloader
    │   ├── CMakeFiles
    │   ├── cmake_install.cmake
    │   ├── libpico_fota_bootloader_lib.a
    │   ├── Makefile
    │   ├── pico_fota_bootloader.bin
    │   ├── pico_fota_bootloader.dis
    │   ├── pico_fota_bootloader.elf
    │   ├── pico_fota_bootloader.elf.map
    │   ├── pico_fota_bootloader.hex
    │   └── pico_fota_bootloader.uf2
    ├── CMakeFiles
    ├── cmake_install.cmake
    ├── Makefile
    ├── your_app.bin
    ├── your_app.dis
    ├── your_app.elf
    ├── your_app.elf.map
    ├── your_app.hex
    └── your_app.uf2
```

### Running

Set Pico W to the BOOTSEL state (by powering it up with the `BOOTSEL` button
pressed) and copy the `pico_fota_bootloader.uf2` file into it. Right now the
Pico W is flashed with the bootloader but does not have proper application in
the application FLASH memory slot. Then, set Pico W to the BOOTSEL state again
and copy the `your_app.uf2` file. The board should reboot and start `your_app`
application.

**NOTE:** you can also look at the serial output logs to monitor the
application state.

### Performing firmware update

To perform a firmware update (over the air), a `your_app.bin` file should be
sent to or downloaded by the Pico W. Note that while rebuilding the
application, the linker scripts' contents should not be changed or should be
changed carefully to maintain the memory layout backward compatibility.
