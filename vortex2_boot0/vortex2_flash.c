#include "vortex2_flash.h"

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
static void vortex2_flash_write_words(uint32_t address, const uint32_t * src, uint32_t num_words)
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

void vortex2_updata_vram_messge(uint8_t *data)
{
    uint8_t boot_num[5];
    uint8_t run_who[4];
    vortex2_flash_read_bytes(VRAM_ADDR,boot_num,5);
    vortex2_flash_read_bytes(VRAM_ADDR+16,run_who,4);
    nrf_nvmc_page_erase(VRAM_ADDR);
    vortex2_flash_write_bytes(VRAM_ADDR,boot_num,5);
    vortex2_flash_write_bytes(VRAM_ADDR+8, data, strlen(data));/*write to flash application true or false*/
    vortex2_flash_write_bytes(VRAM_ADDR+16, run_who, 4);/*write to flash application true or false*/
}