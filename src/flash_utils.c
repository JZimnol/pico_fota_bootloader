#include <assert.h>
#include <string.h>

#include <hardware/flash.h>
#include <hardware/sync.h>
#include <pico/stdlib.h>

#include <pico_fota_bootloader/core.h>
#include "linker_definitions.h"

// buffer for one sector global to avoid stack usage
uint8_t g_data_arr_u8[FLASH_SECTOR_SIZE]; 

static flash_executor_t g_flash_exec = flash_exec_bare;
void flash_util_set_executor(flash_executor_t exec) {
    g_flash_exec = exec ? exec : flash_exec_bare;
}

void flash_exec_bare(void (*fn)(void *), void *param) {
    uint32_t saved = save_and_disable_interrupts();
    fn(param);
    restore_interrupts(saved);
}

/**
 * @brief This is the flash_range_erase() function wrapped in a flash executor. It is used by functions to erase flash after that function has made sure it is safe to do so.
 * @param param Pointer to an array of two uintptr_t values: [0] = offset, [1] = count
 * @note This function is intended to be called by a flash executor, which ensures that it is safe to call flash_range_erase() in the current context. It should not be called directly from application code.
 */
void __noinline __not_in_flash_func(call_flash_range_erase)(void *param) {
    const uintptr_t *p = (const uintptr_t*)param;    
    uint32_t offset = (uint32_t)p[0];
    size_t count = (size_t)p[1];
    flash_range_erase(offset, count);
}

/**
 * @brief This is the flash_range_program() function wrapped in a flash executor. It is used by functions to program flash after that function has made sure it is safe to do so.
 * @param param Pointer to an array of three uintptr_t values: [0] = offset, [1] = data pointer, [2] = size
 * @note This function is intended to be called by a flash executor, which ensures that it
 */
void __noinline __not_in_flash_func(call_flash_range_program)(void *param) {
    const uintptr_t *p = (const uintptr_t*)param;    
    uint32_t offset = (uint32_t)p[0];
    const uint8_t *data = (const uint8_t *)p[1];
    size_t size = (size_t)p[2];

    flash_range_program(offset, data, size);
}
/**
 *  @brief If data is changed compared to data in flash at address offset compleat sector will be read, byte changed and written bak to flash
 *  @param offset position in flash where data will be written with XIP offset
 *  @param data data to write
 *  @param length length of data to write (should be less than FLASH_SECTOR_SIZE - offset in sector)
 */
void write_flash_update(uint32_t offset_with_xip, const uint8_t *data, size_t length)
{
    // first determine where in flash we will write and what we need to cach to only write the data at the offset
    // write can only chage ones to zeros, and errase only zeros to one. Also erase goes per sector (4k) and writes per page (256 bytes)
    // if length < page_size check if page is changed, then read sector to buffer, erase block, change bytes in buffer and write it back
    // to flash.

    assert(offset_with_xip > XIP_BASE);                                 // Offset should be with XIP_BASE
    assert(length < FLASH_SECTOR_SIZE - (offset_with_xip-XIP_BASE));    // length should be less than FLASH_SECTOR_SIZE and fit in sector

    const uint8_t *data_in_flash = (const uint8_t *)offset_with_xip;   
    for(uint i = 0; i < length; i++) {
        if(data_in_flash[i] != data[i]) goto needs_update;
    }
    // if we get here, data is the same as in flash, no need to update reduces wear on flash and time to update
    return;
needs_update:;
    //uint16_t page_offset = offset / FLASH_PAGE_SIZE;
    uint32_t offset_in_sector = offset_with_xip % FLASH_SECTOR_SIZE;
    uint32_t sector_base_with_xip = offset_with_xip - offset_in_sector;
    uint32_t sector_base_no_xip      = sector_base_with_xip - XIP_BASE;

    
    memcpy(g_data_arr_u8, (const void*)sector_base_with_xip, FLASH_SECTOR_SIZE);
    // update bytes in buffer
    for(uint i = 0; i <  length; i++)
    {
        g_data_arr_u8[i + offset_in_sector] = data[i];
    }
    
    // if runing on multi core with FreeRTOS running threats (after vTaskStartScheduler()) flash_save_execute() should be used
    // otherwise flash calls should be surrounded with save_and_disable_interrupts()/restore_interrupts() for correct flash writes
    uintptr_t params_errase[] = { sector_base_no_xip, FLASH_SECTOR_SIZE};  
    uintptr_t params_write[] = { sector_base_no_xip,  (uintptr_t)g_data_arr_u8, FLASH_SECTOR_SIZE};

    g_flash_exec((void (*)(void *)) call_flash_range_erase,
                 (void *)(uintptr_t) params_errase);
    g_flash_exec((void (*)(void *))  call_flash_range_program,
                 (void *)(uintptr_t) params_write);
}

/// @brief write data to flash at offset, length should be less than FLASH_SECTOR_SIZE and length sould be multiple of FLASH_PAGE_SIZE
/// @param offset 
/// @param data 
/// @param length 
void write_flash_pages(uint32_t offset, uint8_t data[], size_t length)
{  
    uintptr_t params_write[] = { offset, (uintptr_t)data, length};
    g_flash_exec((void (*)(void *))  call_flash_range_program,
                           (void *)(uintptr_t) params_write);
 }

/// @brief erases flash sectors starting at sector_offset for length bytes (length should be multiple of FLASH_SECTOR_SIZE)
/// @param sector_offset 
/// @param length 
void erase_flash_sectors(uint32_t sector_offset, size_t length)
{ 
    uintptr_t params_errase[] = { sector_offset, length};  
    g_flash_exec((void (*)(void *)) call_flash_range_erase,
                 (void *)(uintptr_t) params_errase); 
}