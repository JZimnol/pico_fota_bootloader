/*
 * Copyright (c) 2024 Jakub Zimnol
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
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
*/

__FLASH_START_ADDRESS = 0x10000000;
__BOOTLOADER_LENGTH = 36k;

__FLASH_INFO_START_ADDRESS = __FLASH_START_ADDRESS + __BOOTLOADER_LENGTH;
__FLASH_INFO_LENGTH = 4k;

__FLASH_INFO_APP_HEADER_ADDRESS = __FLASH_INFO_START_ADDRESS;
__FLASH_INFO_DOWNLOAD_HEADER_ADDRESS = __FLASH_INFO_APP_HEADER_ADDRESS + 256;
__FLASH_INFO_DOWNLOAD_VALID_ADDRESS = __FLASH_INFO_DOWNLOAD_HEADER_ADDRESS + 256;
__FLASH_INFO_FIRMWARE_SWAPPED_ADDRESS = __FLASH_INFO_DOWNLOAD_VALID_ADDRESS + 256;

__FLASH_APP_START_ADDRESS = __FLASH_INFO_START_ADDRESS + __FLASH_INFO_LENGTH;

/* (2048k - __BOOTLOADER_LENGTH - __FLASH_INFO_LENGTH) / 2 */
__FLASH_SWAP_SPACE_LENGTH = 1004k;

/*
(max binary size) == (.text .rodata .big_const .binary_info) + (possible .data)
so
(.text .rodata .big_const .binary_info) == (max binary size) - (possible .data)

We can declare the (possible .data) as the whole 256k, but that's not likely to
happen. Assume 128k max, but can be easily lowered depending on the application.
*/
__FLASH_SLOT_LENGTH = __FLASH_SWAP_SPACE_LENGTH - 128k;
__FLASH_DOWNLOAD_SLOT_START_ADDRESS = __FLASH_APP_START_ADDRESS + __FLASH_SWAP_SPACE_LENGTH;

ASSERT((__FLASH_SWAP_SPACE_LENGTH%4k) == 0, "__FLASH_SWAP_SPACE_LENGTH should be multiple of 4k")
ASSERT(2048k >= __BOOTLOADER_LENGTH + __FLASH_INFO_LENGTH + 2*__FLASH_SWAP_SPACE_LENGTH,
      "Flash partitions defined incorrectly");
