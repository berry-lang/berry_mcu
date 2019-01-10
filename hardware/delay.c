#include "delay.h"
#include "stm32f10x.h"

#define MAX_RELOAD_TICKS            0xFFFFFFUL
#define CPU_CLOCK                   SystemCoreClock

static void delay_xtick(uint32_t ticks)
{
    SysTick->VAL = 0u;
    SysTick->LOAD = ticks;
    SysTick->CTRL = (1u << 1) | (1u << 0);
    while (!(SysTick->CTRL & (1u << 16)));
    SysTick->CTRL &= ~(1u << 0);
}

void delay_ms(int xms)
{
    uint64_t ticks = (uint64_t)CPU_CLOCK * xms / 8000;
    while (ticks > MAX_RELOAD_TICKS) {
        delay_xtick(MAX_RELOAD_TICKS);
        ticks -= MAX_RELOAD_TICKS;
    }
    delay_xtick(ticks);
}
