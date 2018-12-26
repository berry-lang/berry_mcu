#include "stm32f10x.h"
#include "main.h"
#include "usart1.h"
#include "berry.h"
#include "be_repl.h"
#include "boardlib.h"

static volatile int usart_ready = 0;
static volatile char usart_buf[400];

static const char* get_line(const char *prompt)
{
    printf(prompt);
    fflush(stdout);
    while (!usart_ready);
    usart_ready = 0;
    return (const char*)usart_buf;
}

int main(void)
{
    bvm *vm = be_vm_new(16);

    usart1_config(115200);
    be_loadlibs(vm);
    board_init(vm);
    printf("Berry " BERRY_VERSION "\n");
    printf("On STM32F103RCT6 (build in " __DATE__ ", " __TIME__ ")\n");
    be_repl(vm, get_line);
    be_vm_delete(vm);
    while (1) {
        GPIO_SetBits(GPIOA, GPIO_Pin_8);
        delay(200);
        GPIO_ResetBits(GPIOA, GPIO_Pin_8);
        delay(200);
    }
    return 0;
}

void USART1_IRQHandler(void)                                 
{
    static int pos = 0;
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        char ch = USART1->DR;
        if (ch == '\r' || ch == '\n') {
            if (pos) {
                usart_ready = 1;
                usart_buf[pos++] = '\n';
                usart_buf[pos] = 0;
                pos = 0;
            } /* else: empty line */
        } else {
            usart_buf[pos++] = ch;
        }
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

void HardFault_Handler(void)
{
    printf("\n    !!! Hard Fault. !!!\n");
    while (1);
}
