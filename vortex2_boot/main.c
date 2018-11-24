
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "nrf_delay.h"
#include "boards.h"
#include "vortex2_jump.h"
#include "vortex2_flash.h"

#define BOOT0_ADDR    0x2B000
#define BOOT1_ADDR    0x3A000
/**
 * @brief Function for application main entry.
 */
int main(void)
{
    /* Configure board. */
    ret_code_t rc;
    uint8_t readdata[5];
    while (true)
    {
        vortex2_flash_read_bytes(0x2A000,readdata,5);
        if(strncmp(readdata,"boot0",strlen("boot0")) == 0){
          vortex2_app_start(BOOT0_ADDR);
        }else if(strncmp(readdata,"boot1",strlen("boot1")) == 0){
          vortex2_app_start(BOOT1_ADDR);
        }else{
          vortex2_app_start(BOOT0_ADDR);
        }

    }
}

