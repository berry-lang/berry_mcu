#include "stm32f10x.h"
#include "main.h"
#include "usart1.h"
#include "berry.h"
#include "be_repl.h"
#include "boardlib.h"
#include "delay.h"
#include "shell.h"

#if defined(__GNUC__)
    #define COMPILER  "GCC"
#else
    #define COMPILER  "Unknown Compiler"
#endif

#define FULL_VERSION "Berry " BERRY_VERSION

#define repl_prelude                                            \
    FULL_VERSION " (build in " __DATE__ ", " __TIME__ ")\n"     \
    "[" COMPILER "] on STM32F103RCT6\n"                         \

static volatile int usart_ready = 0;
static volatile char usart_buf[400];

int main(void)
{
    bvm *vm = be_vm_new();
    usart1_config(115200);
    board_init();
    printf(repl_prelude);
    be_repl(vm, shell_readline);
    be_vm_delete(vm);
    while (1) {
        GPIO_SetBits(GPIOA, GPIO_Pin_8);
        delay_ms(200);
        GPIO_ResetBits(GPIOA, GPIO_Pin_8);
        delay_ms(200);
    }
    return 0;
}

void USART1_IRQHandler(void)                                 
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        char ch = USART1->DR;
        shell_addchar(ch);
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

void HardFault_Handler(void)
{
    printf("\n    !!! Hard Fault. !!!\n");
    NVIC_SystemReset();
    while (1);
}
