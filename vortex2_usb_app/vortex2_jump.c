#include <stdint.h>
#include <stdio.h>
#include "string.h"
#include "nrf.h"
#include "nrf_peripherals.h"
#include "sdk_config.h"
#include "vortex2_jump.h"
#include "nrf_sdm.h"
#include "vortex2_flash.h"

#define HANDLER_MODE_EXIT 0xFFFFFFF9
#define MBR_SIZE                (0x28000)
#define EXCEPTION_STACK_WORD_COUNT 8

__STATIC_INLINE void jump_to_addr(uint32_t new_msp, uint32_t new_lr, uint32_t addr)
{
    __ASM volatile ("MSR MSP, %[arg]" : : [arg] "r" (new_msp));
    __ASM volatile ("MOV LR,  %[arg]" : : [arg] "r" (new_lr) : "lr");
    __ASM volatile ("BX       %[arg]" : : [arg] "r" (addr));
}

__STATIC_INLINE void app_start(uint32_t vector_table_addr){
    const uint32_t current_isr_num = (__get_IPSR() & IPSR_ISR_Msk);
    const uint32_t new_msp         = *((uint32_t *)(vector_table_addr));                    // The app's Stack Pointer is found as the first word of the vector table.
    const uint32_t reset_handler   = *((uint32_t *)(vector_table_addr + sizeof(uint32_t))); // The app's Reset Handler is found as the second word of the vector table.
    const uint32_t new_lr          = 0xFFFFFFFF;

    __set_CONTROL(0x00000000);   // Set CONTROL to its reset value 0.
    __set_PRIMASK(0x00000000);   // Set PRIMASK to its reset value 0.
    __set_BASEPRI(0x00000000);   // Set BASEPRI to its reset value 0.
    __set_FAULTMASK(0x00000000); // Set FAULTMASK to its reset value 0.
    if (current_isr_num == 0)
    {
        jump_to_addr(new_msp, new_lr, reset_handler); // Jump directly to the App's Reset Handler.
    }
    else
    {
        // The CPU is in Handler mode (interrupt context).
        const uint32_t exception_stack[EXCEPTION_STACK_WORD_COUNT] = // To be copied onto the stack.
        {
            0x00000000,    // New value of R0. Cleared by setting to 0.
            0x00000000,    // New value of R1. Cleared by setting to 0.
            0x00000000,    // New value of R2. Cleared by setting to 0.
            0x00000000,    // New value of R3. Cleared by setting to 0.
            0x00000000,    // New value of R12. Cleared by setting to 0.
            0xFFFFFFFF,    // New value of LR. Cleared by setting to all 1s.
            reset_handler, // New value of PC. The CPU will continue by executing the App's Reset Handler.
            xPSR_T_Msk,    // New value of xPSR (Thumb mode set).
        };
        const uint32_t exception_sp = new_msp - sizeof(exception_stack);

        memcpy((uint32_t *)exception_sp, exception_stack, sizeof(exception_stack)); // 'Push' exception_stack onto the App's stack.

        jump_to_addr(exception_sp, new_lr, HANDLER_MODE_EXIT); // 'Jump' to the special value to exit handler mode. new_lr is superfluous here.
                                                               // exception_stack will be popped from the stack, so the resulting SP will be the new_msp.
                                                               // Execution will continue from the App's Reset Handler.
    }
}

void vortex2_app_start_final(uint32_t start_addr);

void vortex2_app_start(uint32_t start_addr){
    uint32_t err_code;
    NVIC->ICER[0]=0xFFFFFFFF;
    NVIC->ICPR[0]=0xFFFFFFFF;
#if defined(__NRF_NVIC_ISER_COUNT) && __NRF_NVIC_ISER_COUNT == 2
    NVIC->ICER[1]=0xFFFFFFFF;
    NVIC->ICPR[1]=0xFFFFFFFF;
#endif
    sd_softdevice_vector_table_base_set(start_addr);
    vortex2_app_start_final(start_addr);
}

void vortex2_app_start_final(uint32_t start_addr){
    app_start(start_addr);
}

void vortex2_jump_to_boot(void)
{
    uint8_t boot_num[5];
    uint8_t app_status[6];
    uint8_t run_who[4];
    vortex2_flash_read_bytes(VRAM_ADDR,boot_num,5);
    vortex2_flash_read_bytes(VRAM_ADDR+8,app_status,6);
    vortex2_flash_read_bytes(VRAM_ADDR+16,run_who,4);
    vortex2_flash_page_erase(VRAM_ADDR);
    vortex2_flash_write_bytes(VRAM_ADDR,boot_num,5);
    vortex2_flash_write_bytes(VRAM_ADDR+8,app_status,6);
    vortex2_flash_write_bytes(VRAM_ADDR+16,"boot",4);
    __disable_irq();
    NVIC_SystemReset();
}