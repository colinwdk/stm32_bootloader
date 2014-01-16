/*******************************************************************************
** �ļ���: 		stm32f0_flash_u.h
** �汾��  		1.0
** ��������: 	RealView MDK-ARM 4.62
** ����: 		wdk
** ��������: 	2013-11-08
** ����:		  
** �޸���־��	
*******************************************************************************/

/* ��ֹ�ض���-----------------------------------------------------------------*/
#ifndef __STM32F0_FLASH_U_H
#define __STM32F0_FLASH_U_H

/* ������ͷ�ļ�---------------------------------------------------------------*/
#include "stm32f10x.h"

#define CRC8_BASE          0x00

#define FLASH_PAGE_SIZE    0x800            //flashһҳ��С
//#define	FLASH_ADR_A	       0x08000c00		//flash�����г���ĵ�ַ A
//#define	FLASH_ADR_B	       0x08008800		//flash�����г���ĵ�ַ B
//#define FLASH_CODE_INFO    0x08008400       //���ڳ���A��B����Ϣ�ĵ�ַ

#define	FLASH_ADR_A	       0x08001000		//flash�����г���ĵ�ַ A
#define	FLASH_ADR_B	       0x08040000		//flash�����г���ĵ�ַ B
#define FLASH_CODE_INFO    0x0803f800       //���ڳ���A��B����Ϣ�ĵ�ַ


#define CODE_INFO_CRC_ADDR (&(((flash_ABcode_info_st*)0)->crc8))

/* ����ı��־ */
#define CODE_CHANGED 0xAA

typedef unsigned           char u8;
typedef unsigned           int u32;
typedef unsigned short     int u16;
typedef __IO uint16_t vu16;
typedef __IO uint32_t vu32;

/* flash A��B�������Ϣ */
typedef struct
{
    uint8_t  code_changed;//0xAAΪ�������µĳ��� ��Ҫ����B->A
    uint8_t  code_page_count;//������ռ��ҳ��
    uint8_t  code_crc8;//����ε�CRCУ��ֵ
    uint32_t code_length;//���볤��
    
    uint32_t code_A_length;//A������εĳ��ȣ���Ҫ��Ϊͨ�����ڶ�ȡA������׼����  wdk 2013.12.17 add
    
	uint8_t  crc8;//�öε�CRC16У��ֵ
}flash_ABcode_info_st;


extern uint8_t flash_page_data[FLASH_PAGE_SIZE];

void FlashWriteStr(uint32_t flash_add, uint16_t len, uint8_t* data);
void FlashInit(void);
uint8_t CRC8_table(uint8_t crc8, uint8_t *p, uint32_t counter);

int32_t boot_init(void);

/******************************************************
flash ��ȡ����
*******************************************************/
void FlashReadStr( u32 flash_add, u16 len, u8* data );

#endif /* __STM32F0_FLASH_U_H */

