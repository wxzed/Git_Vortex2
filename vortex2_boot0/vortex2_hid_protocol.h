
#ifndef VORTEX2_HID_PROTOCLO_H__
#define VORTEX2_HID PROTOCOL_H__

#include <stdint.h>
#include <stdbool.h>
#include "sdk_errors.h"

#define writeflash          1
#define readflash           0
#define boot0_flash         0
#define boot1_flash         1
#define app_flash           2
#define softdevice_flash    3

#define idle_status         0
#define write_status        1

typedef struct protocol_data
{
    uint8_t head0;
    uint8_t head1;
    uint8_t len;
    uint8_t cs;
    uint8_t ctr0;
    uint8_t ctr1;
    uint32_t addr;
    uint8_t flash_data[54];
}protocol_data_t;

#endif

/** @} */
