#include "usart1.h"
#include <stdarg.h>

static void pin_config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

	/* config USART1 clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

	/* USART1 GPIO config */
    /* Configure USART1 Tx (PA.09) as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);    
    /* Configure USART1 Rx (PA.10) as input floating */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

static void it_config(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x03;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); // 接收中断使能
}

void usart1_config(uint32_t baudrate)
{
	USART_InitTypeDef USART_InitStructure;

	pin_config();
	  
	/* USART1 mode config */
	USART_InitStructure.USART_BaudRate = baudrate;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure); 
    USART_Cmd(USART1, ENABLE);

    it_config();
}

int _write(int fd, char *pBuffer, int size)
{
    int i = size;
    (void)fd;
    while (i--) {
        USART1->DR = (uint16_t)(*pBuffer++ & 0x01FF);
        while (!(USART1->SR & USART_FLAG_TXE));
    }
    
    return size;
}

uint8_t usart_get_buffer(USART_TypeDef* USARTx, char* buffer, int size, uint32_t timeout)
{
    while (size--) {
        while (!(USARTx->SR & USART_FLAG_RXNE) && --timeout) {
            asm("nop");
        }
        if (timeout == 0) {
            return 0;
        }
        *buffer++ = USARTx->DR & 0x00FF;
    }
    return 1;
}
