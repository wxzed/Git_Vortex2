#ifndef VORTEX2_FLASH_H__
#define VORTEX2_FLASH_H__
#include "nrf_nvmc.h"

#define MYNAME                          "boot1"
#define VERSION                         "V0.1"
#define COMPILE                         "Micropython"

#define VRAM_ADDR                         0x2A000
#define BOOT0_ADDR                        0x2B000
#define BOOT1_ADDR                        0x3A000
#define APP_ADDR                          0x49000
#define APP_OFFSET                        0x32000
#define APP_VSTART                        0x7B000

void begin_vortex2_flash_test(void);
void vortex2_flash_write_bytes(uint32_t address, const uint8_t * src, uint32_t num_bytes);
void vortex2_flash_page_erase(uint32_t address);
void vortex2_flash_read_bytes(uint32_t address, uint8_t * src, uint32_t num_bytes);
void vortex2_flash_write_words(uint32_t address, const uint32_t * src, uint32_t num_words);
void vortex2_updata_vram_messge(uint32_t address,uint8_t *data);
#endif
