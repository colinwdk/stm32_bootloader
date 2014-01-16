/*******************************************************************************
** 文件名: 		stm32f0_flash_u.c
** 版本：  		1.0
** 工作环境: 	RealView MDK-ARM 4.62
** 作者: 		  wdk
** 生成日期: 	2013-11-08
** 功能:		  
** 修改日志：	
*******************************************************************************/

/* 包含头文件 *****************************************************************/
#include "stm32f103_flash_u.h"
#include "stm32f10x_flash.h"
/* 字节合并 */
union union_temp16
{
    unsigned int un_temp16;
    unsigned char  un_temp8[2];		// example 16: 0x0102  8:[0]2 [1]1
}my_unTemp16;


/* 变量声明 */
uint8_t flash_page_data[FLASH_PAGE_SIZE] = {0};

/******************************************************
flash 写入数据
*******************************************************/
void FlashWriteStr(u32 flash_add, u16 len, u8* data)
{
	u16 byteN = 0;
	FLASH_Unlock();
	FLASH_ErasePage(flash_add);//擦除整个页

	while (len)
	{
		my_unTemp16.un_temp8[0] = *(data+byteN);
		my_unTemp16.un_temp8[1] = *(data+byteN+1);		
		FLASH_ProgramHalfWord( flash_add+byteN , my_unTemp16.un_temp16 );

		if (1 == len)
		{
			break;													   
		}
		else
		{
			byteN += 2;
			len -= 2;
		}
	}
	FLASH_Lock();
}

/******************************************************
flash 读取数据
*******************************************************/
void FlashReadStr( u32 flash_add, u16 len, u8* data )
{
	u16 byteN = 0;
	//uint32_t flash_page_addr = flash_add / 0x400;		
	
	while (len)
	{
		my_unTemp16.un_temp16 = *(vu16*)(flash_add+byteN);
		if (1 == len)
		{
			*(data+byteN) = my_unTemp16.un_temp8[0];
			break;			   
		}
		else
		{		
			*(data+byteN) = my_unTemp16.un_temp8[0];
			*(data+byteN+1) = my_unTemp16.un_temp8[1];
			byteN += 2;
			len -= 2;
		}
	}
}

/******************************************************
获取代码段的CRC值
*******************************************************/
uint8_t code_crc8(uint32_t code_area_addr, flash_ABcode_info_st *p_st_code_info)
{
    uint8_t code_crc_temp = CRC8_BASE;
    uint32_t code_length_temp = 0;
    int32_t i = 0;
            
    //1.验证B区代码段
    for (i = 0; i < p_st_code_info->code_page_count - 1; i++)
    {
        FlashReadStr(code_area_addr + i * FLASH_PAGE_SIZE, FLASH_PAGE_SIZE, flash_page_data);
        code_crc_temp = CRC8_table(code_crc_temp, (uint8_t *)flash_page_data, FLASH_PAGE_SIZE);
        code_length_temp += FLASH_PAGE_SIZE;
    }
    
    code_length_temp = p_st_code_info->code_length - code_length_temp;
    FlashReadStr(code_area_addr + i * FLASH_PAGE_SIZE, code_length_temp, flash_page_data);
    code_crc_temp = CRC8_table(code_crc_temp, (uint8_t *)flash_page_data, code_length_temp);
    
    return code_crc_temp;
}

//1.读取程序的信息 FLASH_CODE_INFO
extern   flash_ABcode_info_st st_code_info;
/******************************************************
程序一上电，需要做的事情 
*******************************************************/
int32_t boot_init(void)
{
    //uint32_t code_length_temp = 0;
    int32_t i = 0, stat = 0;
    uint8_t code_crc_temp = 0;

    //2.判断程序更新标志，是否更新程序
    FlashReadStr(FLASH_CODE_INFO, sizeof(flash_ABcode_info_st) , (uint8_t *)&st_code_info);
    code_crc_temp = CRC8_table(CRC8_BASE, (uint8_t *)&st_code_info, (uint32_t)CODE_INFO_CRC_ADDR);
    
    if (code_crc_temp == st_code_info.crc8)//判断crc值
    {
        if (CODE_CHANGED == st_code_info.code_changed)//说明代码需要更新
        {
CHANGED:
            code_crc_temp = code_crc8(FLASH_ADR_B, &st_code_info);//获取B代码段的crc8值
            
            if (code_crc_temp == st_code_info.code_crc8)//判断B代码段的CRC
            {
                //搬运代码
                for (i = 0; i < st_code_info.code_page_count; i++)
                {
                    FlashReadStr(FLASH_ADR_B + i * FLASH_PAGE_SIZE, FLASH_PAGE_SIZE, flash_page_data);
                    FlashWriteStr(FLASH_ADR_A + i * FLASH_PAGE_SIZE, FLASH_PAGE_SIZE, flash_page_data);
                    FlashReadStr(FLASH_ADR_A + i * FLASH_PAGE_SIZE, FLASH_PAGE_SIZE, flash_page_data);
                }
                
                /* 清flash校验段的标志 */
                st_code_info.code_A_length = st_code_info.code_length;//保存A代码段的长度 wdk 2013.12.17 add
                st_code_info.code_changed = 0x00;
                st_code_info.crc8 = CRC8_table(CRC8_BASE, (uint8_t *)&st_code_info, (uint32_t)CODE_INFO_CRC_ADDR);
                FlashWriteStr(FLASH_CODE_INFO, sizeof(flash_ABcode_info_st), (uint8_t *)&st_code_info);
                
                stat = 1;
            }
            else
            {
                if (-3 != stat)
                {
                    stat = -2;
                    code_crc_temp = code_crc8(FLASH_ADR_A, &st_code_info);//获取A代码段的crc8值
                    if (code_crc_temp != st_code_info.code_crc8)//判断A代码段的CRC
                    {
                        stat = -3;
                    }
                }
            }
        }
        else//代码不需要更新，先验证A区代码
        {
//CRC_A:
            //......
            code_crc_temp = code_crc8(FLASH_ADR_A, &st_code_info);//获取A代码段的crc8值
            
            if (code_crc_temp != st_code_info.code_crc8)//判断A代码段的CRC
            {
                stat = -3;
                goto CHANGED;
            }
        }
    }
    else
    {
        stat = -1;
        //goto CRC_A;
    }
    
    return stat;
}

/******************************************************
CRC8校验 
*******************************************************/
uint8_t CRC8Table[]={
  0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
  157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
  35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
  190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
  70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
  219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
  101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
  248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
  140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
  17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
  175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
  50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
  202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
  87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
  233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
  116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53
};

uint8_t CRC8_table(uint8_t crc8, uint8_t *p, uint32_t counter)
{
    for( ; counter > 0; counter--)
    {
        crc8 = CRC8Table[crc8^*p];
        p++;
    }
    return (crc8);
}

