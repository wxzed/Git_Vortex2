#ifndef VORTEX2_FLASH_H__
#define VORTEX2_FLASH_H__
#include "nrf_nvmc.h"

void begin_vortex2_flash_test(void);
void vortex2_flash_write_bytes(uint32_t address, const uint8_t * src, uint32_t num_bytes);
void vortex2_flash_page_erase(uint32_t address);
#endif
