
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "nrf_delay.h"
#include "boards.h"
#include "vortex2_jump.h"
#include "nrf_fstorage.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_fstorage_sd.h"

#define BOOT0_ADDR    0x30000
#define BOOT1_ADDR    0x56000
/**
 * @brief Function for application main entry.
 */
static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);


NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage) =
{
    /* Set a handler for fstorage events. */
    .evt_handler = fstorage_evt_handler,

    /* These below are the boundaries of the flash space assigned to this instance of fstorage.
     * You must set these manually, even at runtime, before nrf_fstorage_init() is called.
     * The function nrf5_flash_end_addr_get() can be used to retrieve the last address on the
     * last page of flash available to write data. */
    .start_addr = 0x2f000,
    .end_addr   = 0x30000,
};

static uint32_t m_data          = 0xBADC0FFE;
static char     m_hello_world[] = "boot0";

static uint32_t nrf5_flash_end_addr_get()
{
    uint32_t const bootloader_addr = NRF_UICR->NRFFW[0];
    uint32_t const page_sz         = NRF_FICR->CODEPAGESIZE;
    uint32_t const code_sz         = NRF_FICR->CODESIZE;

    return (bootloader_addr != 0xFFFFFFFF ?
            bootloader_addr : (code_sz * page_sz));
}

static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
    if (p_evt->result != NRF_SUCCESS)
    {
        //NRF_LOG_INFO("--> Event received: ERROR while executing an fstorage operation.");
        return;
    }

    switch (p_evt->id)
    {
        case NRF_FSTORAGE_EVT_WRITE_RESULT:
        {
            /*NRF_LOG_INFO("--> Event received: wrote %d bytes at address 0x%x.",
                         p_evt->len, p_evt->addr);*/
        } break;

        case NRF_FSTORAGE_EVT_ERASE_RESULT:
        {
//            NRF_LOG_INFO("--> Event received: erased %d page from address 0x%x.",
//                         p_evt->len, p_evt->addr);
        } break;

        default:
            break;
    }
}

static void power_manage(void)
{
#ifdef SOFTDEVICE_PRESENT
    (void) sd_app_evt_wait();
#else
    __WFE();
#endif
}

void wait_for_flash_ready(nrf_fstorage_t const * p_fstorage)
{
    /* While fstorage is busy, sleep and wait for an event. */
    while (nrf_fstorage_is_busy(p_fstorage))
    {
        power_manage();
    }
}
int main(void)
{
    /* Configure board. */
    uint8_t data[256]={0};
    ret_code_t rc;
    bsp_board_init(BSP_INIT_LEDS);
    nrf_fstorage_api_t * p_fs_api;
    p_fs_api = &nrf_fstorage_sd;
    rc = nrf_fstorage_init(&fstorage, p_fs_api, NULL);
    APP_ERROR_CHECK(rc);
    (void) nrf5_flash_end_addr_get();
    rc = nrf_fstorage_erase(&fstorage, 0x2f000, 1, NULL);
    if(rc != NRF_SUCCESS){
      printf("error\r\n");
    }
    wait_for_flash_ready(&fstorage);
    rc = nrf_fstorage_write(&fstorage, 0x2f000, m_hello_world, 8, NULL);
    APP_ERROR_CHECK(rc);

    wait_for_flash_ready(&fstorage);

    rc = nrf_fstorage_read(&fstorage, 0x2f000, data, 8);
    APP_ERROR_CHECK(rc);
    wait_for_flash_ready(&fstorage);
    while (true)
    {
        for (int i = 0; i < 2; i++)
        {
            bsp_board_led_invert(i);
            nrf_delay_ms(500);
        }
        if(strcmp(data,"boot0") == 0){
            //printf("boot0\r\n");
            vortex2_app_start(BOOT0_ADDR);
        }else{
            //printf("boot1\r\n");
            //vortex2_app_start(BOOT1_ADDR);
        }
    }
}

