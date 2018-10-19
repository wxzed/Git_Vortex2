
#ifndef VORTEX2_JUMP_H__
#define VORTEX2_JUMP_H__

#include <stdint.h>
#include <stdbool.h>
#include "sdk_errors.h"



void vortex2_app_start(uint32_t start_addr);
void vortex2_jump_to_boot(void);
#endif // NRF_BOOTLOADER_APP_START_H__

/** @} */
