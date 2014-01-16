/*******************************************************************************
** 文件名: 		stm32f0_flash_u.h
** 版本：  		1.0
** 工作环境: 	RealView MDK-ARM 4.62
** 作者: 		wdk
** 生成日期: 	2013-11-08
** 功能:		  
** 修改日志：	
*******************************************************************************/

/* 防止重定义-----------------------------------------------------------------*/
#ifndef __STM32F0_FLASH_U_H
#define __STM32F0_FLASH_U_H

/* 包含的头文件---------------------------------------------------------------*/
#include "stm32f10x.h"

#define CRC8_BASE          0x00

#define FLASH_PAGE_SIZE    0x800            //flash一页大小
//#define	FLASH_ADR_A	       0x08000c00		//flash中运行程序的地址 A
//#define	FLASH_ADR_B	       0x08008800		//flash中运行程序的地址 B
//#define FLASH_CODE_INFO    0x08008400       //关于程序A、B区信息的地址

#define	FLASH_ADR_A	       0x08001000		//flash中运行程序的地址 A
#define	FLASH_ADR_B	       0x08040000		//flash中运行程序的地址 B
#define FLASH_CODE_INFO    0x0803f800       //关于程序A、B区信息的地址


#define CODE_INFO_CRC_ADDR (&(((flash_ABcode_info_st*)0)->crc8))

/* 代码改变标志 */
#define CODE_CHANGED 0xAA

typedef unsigned           char u8;
typedef unsigned           int u32;
typedef unsigned short     int u16;
typedef __IO uint16_t vu16;
typedef __IO uint32_t vu32;

/* flash A、B程序的信息 */
typedef struct
{
    uint8_t  code_changed;//0xAA为更新了新的程序 需要搬移B->A
    uint8_t  code_page_count;//代码所占的页数
    uint8_t  code_crc8;//代码段的CRC校验值
    uint32_t code_length;//代码长度
    
    uint32_t code_A_length;//A区代码段的长度，主要是为通过串口读取A区代码准备的  wdk 2013.12.17 add
    
	uint8_t  crc8;//该段的CRC16校验值
}flash_ABcode_info_st;


extern uint8_t flash_page_data[FLASH_PAGE_SIZE];

void FlashWriteStr(uint32_t flash_add, uint16_t len, uint8_t* data);
void FlashInit(void);
uint8_t CRC8_table(uint8_t crc8, uint8_t *p, uint32_t counter);

int32_t boot_init(void);

/******************************************************
flash 读取数据
*******************************************************/
void FlashReadStr( u32 flash_add, u16 len, u8* data );

#endif /* __STM32F0_FLASH_U_H */

