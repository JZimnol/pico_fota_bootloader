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

#ifndef PICO_FOTA_BOOTLOADER
#define PICO_FOTA_BOOTLOADER

#include "pico/stdlib.h"

#define PFB_VALUE_AS_U32(Data) (uint32_t) & (Data)
#define PFB_VALUE_WITH_XIP_OFFSET_AS_U32(Data) \
    (PFB_VALUE_AS_U32(Data) - (XIP_BASE))

/**
 * Some random values tbh.
 */
#define PFB_SLOT_IS_VALID_MAGIC 0xabcdef12
#define PFB_SLOT_IS_INVALID_MAGIC 0x00000000
#define PFB_HAS_NEW_FIRMWARE_MAGIC 0x12345678
#define PFB_NO_NEW_FIRMWARE_MAGIC 0x00000000

/**
 * Mark the download slot as valid, i.e. download slot contains proper binary
 * content and the partitions can be swapped. MUST be called before the next
 * reboot, otherwise data from the download slot will be lost.
 */
void pfb_mark_download_slot_as_valid(void);

/**
 * Mark the download slot as invalid, i.e. download slot no longer contains
 * proper binary content and the partitions MUST NOT be swapped.
 */
void pfb_mark_download_slot_as_invalid(void);

/**
 * Returns the information if the executed app comes from the download slot.
 * NOTE: this function will return true only if the firmware update has been
 *       executed during the previous reboot.
 *
 * @return true if the partitions were swapped during the previous reboot,
 *         false otherwise.
 */
bool pfb_is_after_firmware_update(void);

/**
 * Writes data into the download partition and checks, if the length of the data
 * is 256 bytes alligned.
 *
 * @param src          Pointer to the source buffer.
 * @param offset_bytes Offset which should be applied to the beginning of the
 *                     download slot partition.
 * @param len_bytes    Number of bytes that should be written into flash. Should
 *                     be a multiple of 256.
 *
 * @return 1 when @p len_bytes is not multiple of 256 or when ( @p offset_bytes
 *         + @p len_bytes ) exceeds download slot size,
 *         0 otherwise.
 */
int pfb_write_to_flash_aligned_256_bytes(uint8_t *src,
                                         size_t offset_bytes,
                                         size_t len_bytes);

/**
 * Initializes the download slot, i.e. erases the download partition. MUST be
 * called before writing data into the flash.
 */
void pfb_initialize_download_slot(void);

/**
 * Performs the firmware update. Reboots the Pico and checks if the partitions
 * should be swapped.
 */
void pfb_perform_update(void);

void _pfb_notify_pico_has_new_firmware(void);
void _pfb_notify_pico_has_no_new_firmware(void);

#endif // PICO_FOTA_BOOTLOADER
