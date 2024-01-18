# Copyright (c) 2024 Jakub Zimnol
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

cmake_minimum_required(VERSION 3.13)

# Define the pico_fota_bootloader_lib library
add_library(pico_fota_bootloader_lib STATIC
            src/pico_fota_bootloader.c)
target_include_directories(pico_fota_bootloader_lib PUBLIC
                           ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(pico_fota_bootloader_lib PUBLIC
                      hardware_watchdog
                      pico_stdlib
                      hardware_flash)
target_link_options(pico_fota_bootloader_lib PRIVATE
                    "-T${CMAKE_CURRENT_SOURCE_DIR}/linker_common/linker_definitions.ld")

# Create bootloader binary
add_executable(pico_fota_bootloader
               main.c)

target_link_libraries(pico_fota_bootloader
                      hardware_structs
                      hardware_resets
                      pico_fota_bootloader_lib
                      cmsis_core)

function(target_compile_link_options Name Option)
    target_compile_options(${Name} PRIVATE ${Option})
    target_link_options(${Name} PRIVATE ${Option})
endfunction()

target_compile_link_options(pico_fota_bootloader "-Os")
target_compile_link_options(pico_fota_bootloader "-ffunction-sections")
target_compile_link_options(pico_fota_bootloader "-fdata-sections")
target_compile_link_options(pico_fota_bootloader "-L${CMAKE_CURRENT_SOURCE_DIR}/linker_common")
target_link_options(pico_fota_bootloader PRIVATE "LINKER:--gc-sections")

pico_set_linker_script(pico_fota_bootloader ${CMAKE_CURRENT_SOURCE_DIR}/linker_common/bootloader.ld)

pico_enable_stdio_usb(pico_fota_bootloader 1)
pico_enable_stdio_uart(pico_fota_bootloader 0)
pico_add_extra_outputs(pico_fota_bootloader)
