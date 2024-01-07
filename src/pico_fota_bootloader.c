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

#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"

#include "pico_fota_bootloader.h"

extern uint32_t __FLASH_INFO_START_ADDRESS;
extern uint32_t __FLASH_INFO_APP_HEADER_ADDRESS;
extern uint32_t __FLASH_INFO_DOWNLOAD_HEADER_ADDRESS;
extern uint32_t __FLASH_INFO_DOWNLOAD_VALID_ADDRESS;
extern uint32_t __FLASH_INFO_FIRMWARE_SWAPPED_ADDRESS;
extern uint32_t __FLASH_APP_START_ADDRESS;
extern uint32_t __FLASH_DOWNLOAD_SLOT_START_ADDRESS;
extern uint32_t __FLASH_SWAP_SPACE_LENGTH;

static inline void erase_flash_info_partition_isr_unsafe(void) {
    flash_range_erase(PFB_VALUE_WITH_XIP_OFFSET_AS_U32(
                              __FLASH_INFO_START_ADDRESS),
                      FLASH_SECTOR_SIZE);
}

static void write_4_bytes_to_flash_isr_unsafe(uint32_t starting_address,
                                              uint32_t data) {
    uint8_t data_arr_u8[FLASH_PAGE_SIZE] = {};
    uint32_t *data_ptr_u32 = (uint32_t *) data_arr_u8;
    data_ptr_u32[0] = data;
    flash_range_program(starting_address, data_arr_u8, FLASH_PAGE_SIZE);
}

static inline void restore_image_headers_to_flash_isr_unsafe(void) {
    write_4_bytes_to_flash_isr_unsafe(
            PFB_VALUE_WITH_XIP_OFFSET_AS_U32(__FLASH_INFO_APP_HEADER_ADDRESS),
            PFB_VALUE_AS_U32(__FLASH_APP_START_ADDRESS));
    write_4_bytes_to_flash_isr_unsafe(
            PFB_VALUE_WITH_XIP_OFFSET_AS_U32(
                    __FLASH_INFO_DOWNLOAD_HEADER_ADDRESS),
            PFB_VALUE_AS_U32(__FLASH_DOWNLOAD_SLOT_START_ADDRESS));
}

static inline void mark_download_slot_isr_unsafe(uint32_t type) {
    write_4_bytes_to_flash_isr_unsafe(
            PFB_VALUE_WITH_XIP_OFFSET_AS_U32(
                    __FLASH_INFO_DOWNLOAD_VALID_ADDRESS),
            type);
}

static inline void notify_pico_about_firmware_isr_unsafe(uint32_t type) {
    write_4_bytes_to_flash_isr_unsafe(
            PFB_VALUE_WITH_XIP_OFFSET_AS_U32(
                    __FLASH_INFO_FIRMWARE_SWAPPED_ADDRESS),
            type);
}

static void mark_download_slot(uint32_t type) {
    uint32_t saved_firmware_state = __FLASH_INFO_FIRMWARE_SWAPPED_ADDRESS;
    uint32_t saved_interrupts = save_and_disable_interrupts();
    erase_flash_info_partition_isr_unsafe();
    restore_image_headers_to_flash_isr_unsafe();
    notify_pico_about_firmware_isr_unsafe(saved_firmware_state);
    mark_download_slot_isr_unsafe(type);
    restore_interrupts(saved_interrupts);
}

static void notify_pico_about_firmware(uint32_t type) {
    uint32_t saved_slot_state = __FLASH_INFO_DOWNLOAD_VALID_ADDRESS;
    uint32_t saved_interrupts = save_and_disable_interrupts();
    erase_flash_info_partition_isr_unsafe();
    restore_image_headers_to_flash_isr_unsafe();
    mark_download_slot_isr_unsafe(saved_slot_state);
    notify_pico_about_firmware_isr_unsafe(type);
    restore_interrupts(saved_interrupts);
}

void pfb_mark_download_slot_as_valid(void) {
    mark_download_slot(PFB_SLOT_IS_VALID_MAGIC);
}

void pfb_mark_download_slot_as_invalid(void) {
    mark_download_slot(PFB_SLOT_IS_INVALID_MAGIC);
}

void _pfb_notify_pico_has_new_firmware(void) {
    notify_pico_about_firmware(PFB_HAS_NEW_FIRMWARE_MAGIC);
}

void _pfb_notify_pico_has_no_new_firmware(void) {
    notify_pico_about_firmware(PFB_NO_NEW_FIRMWARE_MAGIC);
}

bool pfb_is_after_firmware_update(void) {
    return (__FLASH_INFO_FIRMWARE_SWAPPED_ADDRESS
            == PFB_HAS_NEW_FIRMWARE_MAGIC);
}

int pfb_write_to_flash_aligned_256_bytes(uint8_t *src,
                                         size_t offset_bytes,
                                         size_t len_bytes) {
    if (len_bytes % 256
        || offset_bytes + len_bytes
                   > (size_t) PFB_VALUE_AS_U32(__FLASH_SWAP_SPACE_LENGTH)) {
        return 1;
    }

    uint32_t saved_interrupts = save_and_disable_interrupts();
    flash_range_program(PFB_VALUE_WITH_XIP_OFFSET_AS_U32(
                                __FLASH_DOWNLOAD_SLOT_START_ADDRESS)
                                + offset_bytes,
                        src,
                        len_bytes);
    restore_interrupts(saved_interrupts);

    return 0;
}

void pfb_initialize_download_slot(void) {
    const uint32_t SWAP_LEN = PFB_VALUE_AS_U32(__FLASH_SWAP_SPACE_LENGTH);
    uint32_t saved_interrupts = save_and_disable_interrupts();
    flash_range_erase(PFB_VALUE_WITH_XIP_OFFSET_AS_U32(
                              __FLASH_DOWNLOAD_SLOT_START_ADDRESS),
                      SWAP_LEN);
    restore_interrupts(saved_interrupts);
}

void pfb_perform_update(void) {
    watchdog_enable(1, 1);
    while (1)
        ;
}
