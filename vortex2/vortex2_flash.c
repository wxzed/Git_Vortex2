#include "vortex2_flash.h"

uint8_t test_data[64];
void vortex2_flash_write_bytes(uint32_t address, const uint8_t * src, uint32_t num_bytes)
{
  nrf_nvmc_write_bytes(address,src,num_bytes);
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