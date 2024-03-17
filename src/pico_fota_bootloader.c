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

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <hardware/flash.h>
#include <hardware/sync.h>
#include <hardware/watchdog.h>

#include <pico_fota_bootloader.h>

#include "../linker_common/linker_definitions.h"

/**
 * Some random values tbh.
 */
#define PFB_SHOULD_SWAP_MAGIC 0xabcdef12
#define PFB_SHOULD_NOT_SWAP_MAGIC 0x00000000

#define PFB_HAS_NEW_FIRMWARE_MAGIC 0x12345678
#define PFB_NO_NEW_FIRMWARE_MAGIC 0x00000000

#define PFB_IS_AFTER_ROLLBACK_MAGIC 0xbeefbeef
#define PFB_IS_NOT_AFTER_ROLLBACK_MAGIC 0x00000000

#define PFB_SHOULD_ROLLBACK_MAGIC 0xdeadead
#define PFB_SHOULD_NOT_ROLLBACK_MAGIC 0x00000000

static inline void erase_flash_info_partition_isr_unsafe(void) {
    flash_range_erase(PFB_ADDR_WITH_XIP_OFFSET_AS_U32(__FLASH_INFO_START),
                      FLASH_SECTOR_SIZE);
}

static void
overwrite_4_bytes_in_flash_isr_unsafe(uint32_t dest_addr_with_xip_offset,
                                      uint32_t data) {
    uint8_t data_arr_u8[FLASH_SECTOR_SIZE] = {};
    uint32_t *data_ptr_u32 = (uint32_t *) data_arr_u8;
    uint32_t erase_start_addr_with_xip_offset =
            PFB_ADDR_WITH_XIP_OFFSET_AS_U32(__FLASH_INFO_START);

    assert(dest_addr_with_xip_offset >= erase_start_addr_with_xip_offset);

    void *flash_info_start_addr =
            (void *) (PFB_ADDR_AS_U32(__FLASH_INFO_START));
    memcpy(data_arr_u8, flash_info_start_addr, FLASH_SECTOR_SIZE);

    size_t array_index =
            (dest_addr_with_xip_offset - erase_start_addr_with_xip_offset)
            / (sizeof(uint32_t));
    data_ptr_u32[array_index] = data;

    erase_flash_info_partition_isr_unsafe();
    flash_range_program(erase_start_addr_with_xip_offset, data_arr_u8,
                        FLASH_SECTOR_SIZE);
}

static void overwrite_4_bytes_in_flash(uint32_t dest_addr, uint32_t data) {
    uint32_t saved_interrupts = save_and_disable_interrupts();
    overwrite_4_bytes_in_flash_isr_unsafe(dest_addr - XIP_BASE, data);
    restore_interrupts(saved_interrupts);
}

static void mark_download_slot(uint32_t magic) {
    uint32_t dest_addr = PFB_ADDR_AS_U32(__FLASH_INFO_IS_DOWNLOAD_SLOT_VALID);

    overwrite_4_bytes_in_flash(dest_addr, magic);
}

static void notify_pico_about_firmware(uint32_t magic) {
    uint32_t dest_addr = PFB_ADDR_AS_U32(__FLASH_INFO_IS_FIRMWARE_SWAPPED);

    overwrite_4_bytes_in_flash(dest_addr, magic);
}

static void mark_if_should_rollback(uint32_t magic) {
    uint32_t dest_addr = PFB_ADDR_AS_U32(__FLASH_INFO_SHOULD_ROLLBACK);

    overwrite_4_bytes_in_flash(dest_addr, magic);
}

static void mark_if_is_after_rollback(uint32_t magic) {
    uint32_t dest_addr = PFB_ADDR_AS_U32(__FLASH_INFO_IS_AFTER_ROLLBACK);

    overwrite_4_bytes_in_flash(dest_addr, magic);
}

void pfb_mark_download_slot_as_valid(void) {
    mark_download_slot(PFB_SHOULD_SWAP_MAGIC);
}

void pfb_mark_download_slot_as_invalid(void) {
    mark_download_slot(PFB_SHOULD_NOT_SWAP_MAGIC);
}

bool pfb_is_after_firmware_update(void) {
    return (__FLASH_INFO_IS_FIRMWARE_SWAPPED == PFB_HAS_NEW_FIRMWARE_MAGIC);
}

int pfb_write_to_flash_aligned_256_bytes(uint8_t *src,
                                         size_t offset_bytes,
                                         size_t len_bytes) {
    if (len_bytes % PFB_ALIGN_SIZE || offset_bytes % PFB_ALIGN_SIZE
        || offset_bytes + len_bytes
                   > (size_t) PFB_ADDR_AS_U32(__FLASH_SWAP_SPACE_LENGTH)) {
        return 1;
    }

    uint32_t dest_address =
            PFB_ADDR_WITH_XIP_OFFSET_AS_U32(__FLASH_DOWNLOAD_SLOT_START)
            + offset_bytes;
    uint32_t saved_interrupts = save_and_disable_interrupts();
    flash_range_program(dest_address, src, len_bytes);
    restore_interrupts(saved_interrupts);

    return 0;
}

void pfb_initialize_download_slot(void) {
    uint32_t erase_len = PFB_ADDR_AS_U32(__FLASH_SWAP_SPACE_LENGTH);
    uint32_t erase_address_with_xip_offset =
            PFB_ADDR_WITH_XIP_OFFSET_AS_U32(__FLASH_DOWNLOAD_SLOT_START);
    assert(erase_len % FLASH_SECTOR_SIZE == 0);

    pfb_firmware_commit();

    uint32_t saved_interrupts = save_and_disable_interrupts();
    flash_range_erase(erase_address_with_xip_offset, erase_len);
    restore_interrupts(saved_interrupts);
}

void pfb_perform_update(void) {
    watchdog_enable(1, 1);
    while (1)
        ;
}

void pfb_firmware_commit(void) {
    mark_if_should_rollback(PFB_SHOULD_NOT_ROLLBACK_MAGIC);
}

bool pfb_is_after_rollback(void) {
    return (__FLASH_INFO_IS_AFTER_ROLLBACK == PFB_IS_AFTER_ROLLBACK_MAGIC);
}

void _pfb_mark_should_rollback(void) {
    mark_if_should_rollback(PFB_SHOULD_ROLLBACK_MAGIC);
}

void _pfb_mark_is_after_rollback(void) {
    mark_if_is_after_rollback(PFB_IS_AFTER_ROLLBACK_MAGIC);
}

void _pfb_mark_is_not_after_rollback(void) {
    mark_if_is_after_rollback(PFB_IS_NOT_AFTER_ROLLBACK_MAGIC);
}

bool _pfb_should_rollback(void) {
    return (__FLASH_INFO_SHOULD_ROLLBACK == PFB_SHOULD_ROLLBACK_MAGIC);
}

bool _pfb_has_firmware_to_swap(void) {
    return (__FLASH_INFO_IS_DOWNLOAD_SLOT_VALID == PFB_SHOULD_SWAP_MAGIC);
}

void _pfb_mark_pico_has_new_firmware(void) {
    notify_pico_about_firmware(PFB_HAS_NEW_FIRMWARE_MAGIC);
}

void _pfb_mark_pico_has_no_new_firmware(void) {
    notify_pico_about_firmware(PFB_NO_NEW_FIRMWARE_MAGIC);
}
