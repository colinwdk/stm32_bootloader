/*******************************************************************************
** 文件名: 		main.c
** 版本：  		1.0
** 工作环境: 	RealView MDK-ARM 4.62
** 作者: 		  wdk
** 生成日期: 	2013-11-26
** 功能:		  
** 修改日志：	
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f103_flash_u.h"
#include "stm32f103_uart_u.h"

#define APPLICATION_ADDRESS   (uint32_t)FLASH_ADR_A

typedef  void (*pFunction)(void);
pFunction Jump_To_Application;
uint32_t JumpAddress;

/* 变量 ----------------------------------------------------------------------*/
__IO uint32_t TimingDelay;
int32_t timingDelay_sign = 1;

/* 串口字节接收延时 */
extern __IO uint32_t UartReTimingDelay;//10ms

void RCC_Configuration(void);
void Delay(__IO uint32_t nCount);

void BT_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    //K1按键接口配置
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15; 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//口线翻转速度为50MHz
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

/****************************************************************************
* 名    称：int main(void)
* 功    能：主函数
* 入口参数：无
* 出口参数：无
* 说    明：
* 调用方法：无 
****************************************************************************/ 
int main(void)
{
    uint8_t Tx_Buffer[5] = {0xAA,0x55,0x00,0x00,0xFC};
    ///uint8_t Tx_Buffer1[5] = {0x31,0x32,0x33,0x34,0x35};
    uint8_t data_c = 0;
    int32_t stat = 0;
    uint32_t i = 0;

    RCC_Configuration();   				//系统时钟配置
 
    if (SysTick_Config(72000))		    //时钟节拍中断时1ms一次  用于定时 
    { 
        while (1);/* Capture error */ 
    }
    
    /* USART1 配置初始化 */
    USART_Configuration();
    //for (i = 0; i < 0xffff; i++);
    UART_Send(Tx_Buffer, 5);//发送
    TimingDelay = 10000;//10秒钟的开机等待时间
    
    while (1)
    {
CRC_ERROR:
        /* 串口数据协议内容处理 */
        data_c = uart_reply();
        if (1 == data_c)
        {
            break;
        }
        
        /* 等待的时间到 */
        if ((0 == TimingDelay) && timingDelay_sign)
        {
            break;
        }
    }
    
    /* flash内容校验、搬移等工作 */
    stat = boot_init();
    if (-3 == stat)//AB区代码段校验都不对
    {
        /* 初始化各个变量 */
        data_c = 0;
        TimingDelay = 10000;//10秒钟的开机等待时间
        timingDelay_sign = 1;
        goto CRC_ERROR;
    }

    if (((*(__IO uint32_t*)APPLICATION_ADDRESS) & 0x2FFE0000) == 0x20000000)
    { 
      /* Jump to user application */
      JumpAddress = *(__IO uint32_t*) (APPLICATION_ADDRESS + 4);
      Jump_To_Application = (pFunction) JumpAddress;
      /* Initialize user application's Stack Pointer */
      __set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);
      Jump_To_Application();
    }
    
    NVIC_SystemReset();
    while(1)
    {
        
    }
}

/****************************************************************************
* 名    称：void RCC_Configuration(void)
* 功    能：系统时钟配置为72MHZ
* 入口参数：无
* 出口参数：无
* 说    明：
* 调用方法：无 
****************************************************************************/ 
void RCC_Configuration(void)
{   
  SystemInit();
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |RCC_APB2Periph_AFIO, ENABLE);
  //RCC_APB1PeriphClockCmd( RCC_APB1Periph_USART2,ENABLE);
}


/****************************************************************************
* 名    称：void Delay(__IO uint32_t nTime)
* 功    能：定时延时程序 1ms为单位
* 入口参数：无
* 出口参数：无
* 说    明：
* 调用方法：无 
****************************************************************************/  
void Delay(__IO uint32_t nTime)
{ 
  TimingDelay = nTime;

  while(TimingDelay != 0);
}

/****************************************************************************
* 名    称：void TimingDelay_Decrement(void)
* 功    能：获取节拍程序
* 入口参数：无
* 出口参数：无
* 说    明：
* 调用方法：无 
****************************************************************************/  
void TimingDelay_Decrement(void)
{
  if (TimingDelay != 0x00)
  { 
    TimingDelay--;
  }
}

/*******************************************************************************
  * @函数名称	SysTick_Handler
  * @函数说明   SysTick中断发生的中断处理函数
  * @输入参数   无
  * @返回参数   无
  *****************************************************************************/
void SysTick_Handler(void)
{
	//TimingDelay_Decrement();
    if (TimingDelay != 0x00)
    { 
        TimingDelay--;
    }
    
    if (UartReTimingDelay != 0x00)
    {
        UartReTimingDelay--;
    }
}

/******************* (C) COPYRIGHT 2011 奋斗STM32 *****END OF FILE****/
