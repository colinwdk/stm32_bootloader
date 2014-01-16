/*******************************************************************************
** 文件名: 		stm32f0_uart_u.h
** 版本：  		1.0
** 工作环境: 	RealView MDK-ARM 4.60
** 作者: 		wdk
** 生成日期: 	2013-11-18
** 功能:		STM32F0
** 相关文件:	
** 修改日志:
*******************************************************************************/

/* 防止重定义-----------------------------------------------------------------*/
#ifndef __STM32F0_UART_U_H
#define __STM32F0_UART_U_H

#include "stm32f10x.h"

#define XMODEM_SOH 0x01
#define XMODEM_EOT 0x04
#define XMODEM_ACK 0x06
#define XMODEM_NAK 0x15
#define XMODEM_CAN 0x18

//#define 

//文件传输过程中的标志
typedef enum
{
    UART_NONE,
    UART_CRC,
    UART_LENGTH,
    UART_PACKET_FINISH,
    //---------------------
    UART_JUMP_APP,
    UART_RESET,
}uart_state_eu;

extern void USART_Configuration(void);

/******************************************************
*uart发送字节
*******************************************************/
extern void UART_send_byte(uint8_t byte);
extern void UART_Send(uint8_t *Buffer, uint32_t Length);
extern uint8_t UART_Recive(void);

extern int32_t uart_reply(void);

extern void USART_OUT(USART_TypeDef* USARTx, uint8_t *Data,...);

#endif /* __STM32F0_UART_U_H */
