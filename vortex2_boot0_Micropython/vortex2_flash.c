#include "vortex2_flash.h"
#include "nrf_log.h"
#include "nrf_delay.h"
#include "string.h"

uint8_t test_data[64];

void vortex2_flash_write_bytes(uint32_t address, const uint8_t * src, uint32_t num_bytes)
{
  nrf_nvmc_write_bytes(address,src,num_bytes);
}

void vortex2_flash_read_bytes(uint32_t address, uint8_t * src, uint32_t num_bytes)
{
  for(int i = 0; i < num_bytes; i++){
    src[i] = ((uint8_t*)address)[i];
  }
}
void vortex2_flash_write_words(uint32_t address, const uint32_t * src, uint32_t num_words)
{
  nrf_nvmc_write_words(address,src,num_words);
}

void vortex2_flash_page_erase(uint32_t address)
{
  nrf_nvmc_page_erase(address);
}
void begin_vortex2_flash_test()
{
  for(int i=0; i<64;i++){
    test_data[i] = i;
  }
  vortex2_flash_page_erase(0x00040000);
  vortex2_flash_page_erase(0x00041000);
}

void vortex2_updata_vram_messge(uint32_t address,uint8_t *data)
{
    uint8_t boot_num[9];                                        /*VRAM_ADDR*/
    uint8_t app_ok[9];                                          /*VRAM_ADDR+8*/
    uint8_t run_who[9];                                         /*VRAM_ADDR+16*/
    uint8_t version[9];                                         /*VRAM_ADDR+24*/
    memset(boot_num,'\0',9);
    memset(app_ok,'\0',9);
    memset(run_who,'\0',9);
    memset(version,'\0',9);
    vortex2_flash_read_bytes(VRAM_ADDR,   boot_num, 8);
    vortex2_flash_read_bytes(VRAM_ADDR+8, app_ok,   8);
    vortex2_flash_read_bytes(VRAM_ADDR+16,run_who,  8);
    vortex2_flash_read_bytes(VRAM_ADDR+24,version,  8);
    switch(address){
    case VRAM_ADDR:                             /*update run boot0 or boot1*/
        memset(boot_num,0xFF,8);
        memcpy(boot_num,data,strlen(data));
        break;
    case VRAM_ADDR+8:                           /*update application true or false*/
        memset(app_ok,0xFF,8);
        memcpy(app_ok,data,strlen(data));
        break;
    case VRAM_ADDR+16:                          /*update run application or boot*/
        memset(run_who,0xFF,8);
        memcpy(run_who,data,strlen(data));
        break;
    case VRAM_ADDR+24:                          /*update run boot version*/
        memset(version,0xFF,8);
        memcpy(version,data,strlen(data));
        break;
    default:
        break;
    }
    nrf_nvmc_page_erase(VRAM_ADDR);
    vortex2_flash_write_bytes(VRAM_ADDR,    boot_num, 8);
    vortex2_flash_write_bytes(VRAM_ADDR+8,  app_ok,   8);             /*write to flash application true or false*/
    vortex2_flash_write_bytes(VRAM_ADDR+16, run_who,  8);             /*write to flash application true or false*/
    vortex2_flash_write_bytes(VRAM_ADDR+24, version,  8);
}