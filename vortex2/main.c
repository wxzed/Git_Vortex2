
#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"
#include "boards.h"
#define jump(TargetAddr)   (*((void(*)())(TargetAddr)))()

/**
 * @brief Function for application main entry.
 */
void test(){
  printf("hello\r\n");
}
int main(void)
{
    /* Configure board. */
    bsp_board_init(BSP_INIT_LEDS);

    /* Toggle LEDs. */
    while (true)
    {
        for (int i = 0; i < LEDS_NUMBER; i++)
        {
            bsp_board_led_invert(i);
            nrf_delay_ms(500);
        }
    }
}


/**
 *@}
 **/
