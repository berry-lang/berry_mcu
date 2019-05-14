#include "boardlib.h"
#include "stm32f10x.h"
#include "delay.h"
#include <math.h>
#include <string.h>

#define SINE_POINTS     32

#ifndef M_PI
#define M_PI            3.141592653589793
#endif

static volatile uint16_t sine_tab[SINE_POINTS];

void gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* GPIOD Periph clock enable */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* Configure PD0 and PD2 in output pushpull mode */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_WriteBit(GPIOA, GPIO_Pin_8, SET);
}

void dac_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;//gpio初始化的结构体
	DAC_InitTypeDef DAC_InitStruct;//DAC初始化结构体
	
	//打开GPIOA时钟，DAC接在PA4上
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	//打开DAC时钟，是DAC不是ADC，刚开始我打开的是ADC的时钟，找了好久才找到原因
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

	//DAC输出时IO要配置为模拟输入，《STM32中文参考手册 》12.2章节最后部分有解释
	//打开DAC功能后，PA4会自动连接到DAC输出，《TM32中文参考手册 》12.2章节最后部分有解释
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	DAC_DeInit();
	DAC_InitStruct.DAC_OutputBuffer = DAC_OutputBuffer_Enable;//不输出缓存
	DAC_InitStruct.DAC_Trigger = DAC_Trigger_None;//不需要触发事件
	DAC_InitStruct.DAC_WaveGeneration = DAC_WaveGeneration_None;//不使用波形发生器

	DAC_Init(DAC_Channel_1, &DAC_InitStruct);
	DAC_Cmd(DAC_Channel_1, ENABLE);
    DAC_Init(DAC_Channel_2, &DAC_InitStruct);
	DAC_Cmd(DAC_Channel_2, ENABLE);

	DAC_SetChannel1Data(DAC_Align_12b_R, 0);//初始化输出0v	
	DAC_SetChannel2Data(DAC_Align_12b_R, 0);//初始化输出0v
}

void board_init(void)
{
    gpio_init();
    dac_init();
}

static void sinewave_tim_config(int period)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);//开时钟
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Prescaler = 0x0;     //不预分频
    TIM_TimeBaseStructure.TIM_ClockDivision = 0x0; //不分频
    TIM_TimeBaseStructure.TIM_Period = (uint16_t)period; //设置输出频率
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //向上计数
    //TIM_DeInit(TIM2);
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    TIM_SelectOutputTrigger(TIM2, TIM_TRGOSource_Update); //设置TIME输出触发为更新模式
}

static void sinewave_dac_config(void)
{
    DAC_InitTypeDef DAC_InitStructure;
    DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;//不产生波形
    DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Disable; //不使能输出缓存
    DAC_InitStructure.DAC_Trigger = DAC_Trigger_T2_TRGO;	  //设置为TIM2更新，即设置TIM2作为触发源
    DAC_Init(DAC_Channel_1, &DAC_InitStructure);//初始化
    DAC_Cmd(DAC_Channel_1, ENABLE);    //使能DAC的通道1
    DAC_DMACmd(DAC_Channel_1, ENABLE); //使能DAC通道1的DMA
}

static void sinewave_dma_config(volatile void *pdata, int size)
{                  
    DMA_InitTypeDef DMA_InitStructure;
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE); //开启DMA2时钟
     
    DMA_StructInit(&DMA_InitStructure); //DMA结构体初始化
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST; //从寄存器读数据
    DMA_InitStructure.DMA_BufferSize = size; //寄存器大小
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; //外设地址不递增
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; //内存地址递增
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; //宽度为半字
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord; //宽度为半字
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh; //优先级非常高
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable; //关闭内存到内存模式
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; //循环发送模式
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&DAC->DHR12R1); //外设地址为DAC通道1的地址
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)pdata; //波形数据表内存地址

    DMA_Init(DMA2_Channel3, &DMA_InitStructure); //初始化
    DMA_Cmd(DMA2_Channel3, ENABLE); //使能DMA通道3
}

static int setled(bvm *vm)
{
    if (be_isbool(vm, 1)) {
        GPIO_WriteBit(GPIOA, GPIO_Pin_8, !be_tobool(vm, 1));
    }
    be_return_nil(vm);
}

static int setdac(bvm *vm)
{
    if (be_isint(vm, 1)) {
        int ch = be_toint(vm, 1);
        int value = be_toint(vm, 2);
        if (value > -1 || value < 4096) {
            if (ch == 1) {
                DAC_SetChannel1Data(DAC_Align_12b_R, value);
            } else if (ch == 2) {
                DAC_SetChannel2Data(DAC_Align_12b_R, value);
            }
        }
    }
    be_return_nil(vm);
}

static void calc_sin_table(float amp)
{
    int i = 0;
    for (i = 0; i < SINE_POINTS; ++i) {
        float v = sinf((float)(2 * M_PI / SINE_POINTS) * i);
        sine_tab[i] = (uint16_t)(v * amp * 2047 + 2047);
    }
}

static void calc_rect_table(float amp)
{
    int i = 0;
    for (i = 0; i < SINE_POINTS; ++i) {
        int v = i < SINE_POINTS / 2 ? 1 : -1;
        sine_tab[i] = (uint16_t)(v * amp * 2047 + 2047);
    }
}

static void calc_tri_table(float amp)
{
    int i = 0;
    for (i = 0; i < SINE_POINTS; ++i) {
        int v = i < SINE_POINTS / 2 ? i : SINE_POINTS - 1 - i;
        v = v - (SINE_POINTS / 4 - 1);
        sine_tab[i] = (uint16_t)(v * amp / SINE_POINTS * 8192 + 2047);
    }
}

static int play_wave(bvm *vm, void (*genwave)(float))
{
    int freq = be_toint(vm, 1);
    breal amp = be_toreal(vm, 2);
    TIM_Cmd(TIM2, DISABLE);
    genwave(amp);
    sinewave_tim_config(72000000 / SINE_POINTS / freq - 1);
    sinewave_dac_config();
    sinewave_dma_config(sine_tab, SINE_POINTS);
    TIM_Cmd(TIM2, ENABLE);
    be_return_nil(vm);
}

static int l_play_sin(bvm *vm)
{
    return play_wave(vm, calc_sin_table);
}

static int l_play_rect(bvm *vm)
{
    return play_wave(vm, calc_rect_table);
}

static int l_play_tri(bvm *vm)
{
    return play_wave(vm, calc_tri_table);
}

static int l_play_stop(bvm *vm)
{
    TIM_Cmd(TIM2, DISABLE);
    dac_init();
    be_return_nil(vm);
}

static int l_reboot(bvm *vm)
{
    be_writestring("software reboot...\n\n");
    delay_ms(10);
    NVIC_SystemReset();
    be_return_nil(vm);
}

#if !BE_USE_PRECOMPILED_OBJECT
be_native_module_attr_table(attr_table) {
    be_native_module_function("setled", setled),
    be_native_module_function("setdac", setdac),
    be_native_module_function("play_sin", l_play_sin),
    be_native_module_function("play_rect", l_play_rect),
    be_native_module_function("play_tri", l_play_tri),
    be_native_module_function("play_stop", l_play_stop),
    be_native_module_function("reboot", l_reboot)
};

be_define_native_module(board, attr_table);
#else
/* @const_object_info_begin
module board (scope: golbal) {
    setled, func(setled)
    setdac, func(setdac)
    play_sin, func(l_play_sin)
    play_rect, func(l_play_rect)
    play_tri, func(l_play_tri)
    play_stop, func(l_play_stop)
    reboot, func(l_reboot)
}
@const_object_info_end */
#include "../generate/be_fixed_board.h"
#endif
