/*******************************************************************************
** �ļ���: 		main.c
** �汾��  		1.0
** ��������: 	RealView MDK-ARM 4.62
** ����: 		  wdk
** ��������: 	2013-11-26
** ����:		  
** �޸���־��	
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f103_flash_u.h"
#include "stm32f103_uart_u.h"

#define APPLICATION_ADDRESS   (uint32_t)FLASH_ADR_A

typedef  void (*pFunction)(void);
pFunction Jump_To_Application;
uint32_t JumpAddress;

/* ���� ----------------------------------------------------------------------*/
__IO uint32_t TimingDelay;
int32_t timingDelay_sign = 1;

/* �����ֽڽ�����ʱ */
extern __IO uint32_t UartReTimingDelay;//10ms

void RCC_Configuration(void);
void Delay(__IO uint32_t nCount);

void BT_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    //K1�����ӿ�����
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15; 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//���߷�ת�ٶ�Ϊ50MHz
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

/****************************************************************************
* ��    �ƣ�int main(void)
* ��    �ܣ�������
* ��ڲ�������
* ���ڲ�������
* ˵    ����
* ���÷������� 
****************************************************************************/ 
int main(void)
{
    uint8_t Tx_Buffer[5] = {0xAA,0x55,0x00,0x00,0xFC};
    ///uint8_t Tx_Buffer1[5] = {0x31,0x32,0x33,0x34,0x35};
    uint8_t data_c = 0;
    int32_t stat = 0;
    uint32_t i = 0;

    RCC_Configuration();   				//ϵͳʱ������
 
    if (SysTick_Config(72000))		    //ʱ�ӽ����ж�ʱ1msһ��  ���ڶ�ʱ 
    { 
        while (1);/* Capture error */ 
    }
    
    /* USART1 ���ó�ʼ�� */
    USART_Configuration();
    //for (i = 0; i < 0xffff; i++);
    UART_Send(Tx_Buffer, 5);//����
    TimingDelay = 10000;//10���ӵĿ����ȴ�ʱ��
    
    while (1)
    {
CRC_ERROR:
        /* ��������Э�����ݴ��� */
        data_c = uart_reply();
        if (1 == data_c)
        {
            break;
        }
        
        /* �ȴ���ʱ�䵽 */
        if ((0 == TimingDelay) && timingDelay_sign)
        {
            break;
        }
    }
    
    /* flash����У�顢���Ƶȹ��� */
    stat = boot_init();
    if (-3 == stat)//AB�������У�鶼����
    {
        /* ��ʼ���������� */
        data_c = 0;
        TimingDelay = 10000;//10���ӵĿ����ȴ�ʱ��
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
* ��    �ƣ�void RCC_Configuration(void)
* ��    �ܣ�ϵͳʱ������Ϊ72MHZ
* ��ڲ�������
* ���ڲ�������
* ˵    ����
* ���÷������� 
****************************************************************************/ 
void RCC_Configuration(void)
{   
  SystemInit();
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |RCC_APB2Periph_AFIO, ENABLE);
  //RCC_APB1PeriphClockCmd( RCC_APB1Periph_USART2,ENABLE);
}


/****************************************************************************
* ��    �ƣ�void Delay(__IO uint32_t nTime)
* ��    �ܣ���ʱ��ʱ���� 1msΪ��λ
* ��ڲ�������
* ���ڲ�������
* ˵    ����
* ���÷������� 
****************************************************************************/  
void Delay(__IO uint32_t nTime)
{ 
  TimingDelay = nTime;

  while(TimingDelay != 0);
}

/****************************************************************************
* ��    �ƣ�void TimingDelay_Decrement(void)
* ��    �ܣ���ȡ���ĳ���
* ��ڲ�������
* ���ڲ�������
* ˵    ����
* ���÷������� 
****************************************************************************/  
void TimingDelay_Decrement(void)
{
  if (TimingDelay != 0x00)
  { 
    TimingDelay--;
  }
}

/*******************************************************************************
  * @��������	SysTick_Handler
  * @����˵��   SysTick�жϷ������жϴ�����
  * @�������   ��
  * @���ز���   ��
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

/******************* (C) COPYRIGHT 2011 �ܶ�STM32 *****END OF FILE****/
