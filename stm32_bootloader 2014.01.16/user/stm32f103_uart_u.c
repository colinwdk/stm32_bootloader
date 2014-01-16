/*******************************************************************************
** 文件名: 		stm32f0_uart_u.c
** 版本：  		1.0
** 工作环境: 	RealView MDK-ARM 4.60
** 作者: 		  wdk
** 生成日期: 	2013-11-18
** 功能:		  STM32F0
** 相关文件:	
** 修改日志:
*******************************************************************************/
#include "stm32f10x.h"
#include "misc.h"
#include "stdarg.h"
#include "stm32f103_flash_u.h"
#include "stm32f103_uart_u.h"
#include "stm32f10x_usart.h"
//#include "stdlib.h"

/* 外部变量引用 */
extern int32_t timingDelay_sign;
extern __IO uint32_t TimingDelay;

/* 串口字节接收延时 */
__IO uint32_t UartReTimingDelay = 10;//10ms

uint8_t PackCnt = 0;                 //??????????,??????
uint16_t uart_data_cnt = 0;
uint8_t PackNum = 0;                 //?????
uint8_t TestVal = 0;                 //??????????
uint8_t Tab[266] = {0};                //wdk 2013.12.18 modify 265->266增加返回程序代码段读取的错误代码

uint32_t flash_1K_add = 0;
uint32_t flash_1K_add_temp = 0;
uint32_t flash_code_add = FLASH_ADR_B;//B区程序写

/* xmodem协议阶段完成标志 */
//#pragma pack(1)
uart_state_eu eu_uart_sign = UART_NONE;

/* 串口命令 */
uint8_t uart_cmd = 0;

/* 程序校验区数据 */
flash_ABcode_info_st st_code_info = {0, 0, CRC8_BASE, 0 , 0, 0};

/****************************************************************************
* 名    称：void NVIC_Configuration(void)
* 功    能：中断源配置
* 入口参数：无
* 出口参数：无
* 说    明：
* 调用方法：无 
****************************************************************************/
void NVIC_Configuration(void)
{
  /*  结构声明*/
  NVIC_InitTypeDef NVIC_InitStructure;

  /* Configure the NVIC Preemption Priority Bits */  
  /* Configure one bit for preemption priority */
  /* 优先级组 说明了抢占优先级所用的位数，和子优先级所用的位数   在这里是1， 7 */    
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);	  
  
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;			     	//设置串口1中断
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;	     	//抢占优先级 0
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;				//子优先级为0
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;					//使能
  NVIC_Init(&NVIC_InitStructure);
}

/******************************************************
*uart配置
*******************************************************/
void USART_Configuration(void)
{
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    //USART_ClockInitTypeDef USART_ClockInitStructure;
    
    NVIC_Configuration();   
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;	         	 //USART1 TX
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;    		 //复用推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);		    		 //A端口 

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;	         	 //USART1 RX
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;    //复用开漏输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);		         	 //A端口 

    USART_InitStructure.USART_BaudRate = 9600;					//速率115200bps
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;		//数据位8位
    USART_InitStructure.USART_StopBits = USART_StopBits_1;			//停止位1位
    USART_InitStructure.USART_Parity = USART_Parity_No;				//无校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;   //无硬件流控
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;					  //收发模式
    USART_Init(USART1, &USART_InitStructure);

    /* Enable USART1 Receive and Transmit interrupts */
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);                      //使能接收中断
    //USART_ITConfig(USART1, USART_IT_TXE, ENABLE);						//使能发送缓冲空中断   

    /* Enable the USART1 */
    USART_Cmd(USART1, ENABLE);	
}


/******************************************************
*uart发送字节
*******************************************************/
void UART_send_byte(uint8_t byte) 
{
    USART_SendData(USART1, byte);
	while(USART_GetFlagStatus(USART1, USART_FLAG_TC)==RESET);
}

/******************************************************
*uart发送数据
*******************************************************/
void UART_Send(uint8_t *Buffer, uint32_t Length)
{
    while(Length != 0)
    {
        UART_send_byte(*Buffer);
        Buffer++;
        Length--;
    }
}
    
/******************************************************
*xmodem协议处理
*******************************************************/
int32_t uart_reply(void)
{
    int32_t i = 0, stat = 0;
    uint8_t data_send[30] = {0};
    uint16_t length = 0;
    uint16_t packet_count = 0, packet_curr = 0;
    uint8_t *p_st_code_info = (uint8_t *)&st_code_info;
    uint32_t read_code_length = 0;//记录要读取代码段的长度
    uint32_t read_code_place = FLASH_ADR_A;//记录要读取代码段的位置
    
    data_send[0] = 0xAA;
    switch(eu_uart_sign)
    {
        case UART_CRC://crc错误
            data_send[1] = uart_cmd;
            data_send[2] = 0x01;//长度数据
            ///data_send[3] = 0x00;
            data_send[4] = UART_CRC;
            data_send[5] = CRC8_table((uint8_t)CRC8_BASE, data_send, 5);
            
            eu_uart_sign = UART_NONE;//清该标志
            UART_Send(data_send, 6);
        break;
        case UART_PACKET_FINISH://一包数据接收完成
            switch(uart_cmd)
            {
                case 0x01:
                {
                    length = ((uint16_t)Tab[3]) * 256 + Tab[2] - 4;//获得代码数据长度
                    packet_count = ((uint16_t)Tab[5]) * 256 + Tab[4];//包总数
                    packet_curr = ((uint16_t)Tab[7]) * 256 + Tab[6];//当前包
                    
                    if (0 == packet_curr)//表示第一包
                    {//初始化相关变量，重新开始
                        
                        /* 这里添加这句代码，主要是为了提取A代码段的长度 wdk 2013.12.17 add */
                        FlashReadStr(FLASH_CODE_INFO, sizeof(flash_ABcode_info_st) , (uint8_t *)&st_code_info);
                    
                        flash_1K_add = 0;
                        flash_code_add = FLASH_ADR_B;
                        st_code_info.code_changed = 0;
                        st_code_info.code_page_count = 0;
                        st_code_info.code_length = 0;
                        st_code_info.code_crc8 = CRC8_BASE;
                    }
                    
                    for (i = 0; i < length; i++)
                    {
                        flash_page_data[flash_1K_add] = Tab[8 + i];//数据从第8个字节开始
                        flash_1K_add++;
                        if (flash_1K_add == FLASH_PAGE_SIZE)
                        {
                            FlashWriteStr(flash_code_add, FLASH_PAGE_SIZE, flash_page_data);
                            
                            flash_code_add += FLASH_PAGE_SIZE;
                            flash_1K_add = 0;
                            
                            st_code_info.code_page_count++;
                            st_code_info.code_length += FLASH_PAGE_SIZE;
                 
                            /* 计算CRC8 */
                            st_code_info.code_crc8 = CRC8_table(st_code_info.code_crc8, flash_page_data, FLASH_PAGE_SIZE);
                        }
                    }
                    
                    /* 表示最后一包数据 */
                    if (packet_count == packet_curr)
                    {
                        if (flash_1K_add > 0)
                        {
                            FlashWriteStr(flash_code_add, FLASH_PAGE_SIZE, flash_page_data);
                            
                            ///flash_code_add += FLASH_PAGE_SIZE;
                            st_code_info.code_page_count++;
                            st_code_info.code_length += flash_1K_add;
                            /* 计算CRC8 */
                            st_code_info.code_crc8 = CRC8_table(st_code_info.code_crc8, flash_page_data, flash_1K_add);
                            
                            flash_1K_add = 0;
                        }
                        
                        /* 写校验数据到flash */
                        st_code_info.code_changed = CODE_CHANGED;
                        st_code_info.crc8 = CRC8_table((uint8_t)CRC8_BASE, (uint8_t *)&st_code_info, (uint32_t)CODE_INFO_CRC_ADDR);
                        FlashWriteStr(FLASH_CODE_INFO, sizeof(flash_ABcode_info_st), (uint8_t *)&st_code_info);
                        
                        stat = 1;
                    }
                    
                    /* 组回复包 */
                    data_send[1] = uart_cmd;
                    data_send[2] = 0x01;//长度数据
                    //data_send[3] = 0x00;
                    data_send[4] = UART_NONE;
                    data_send[5] = CRC8_table((uint8_t)CRC8_BASE, data_send, 5);
                    
                    eu_uart_sign = UART_NONE;//清该标志
                    UART_Send(data_send, 6);
                }
                break;
                case 0x02:
                {
                    /* 这里添加这句代码，主要是为了提取A代码段的长度 wdk 2013.12.17 add */
                    FlashReadStr(FLASH_CODE_INFO, sizeof(flash_ABcode_info_st) , (uint8_t *)&st_code_info);
                        
                    /* 组回复包 */
                    data_send[1] = uart_cmd;
                    length = sizeof(flash_ABcode_info_st);
                    data_send[2] = length % 256;//长度数据
                    data_send[3] = length / 256;
                    
                    for (i = 0; i < length; i++)
                    {
                        data_send[4 + i] = *p_st_code_info++;
                    }
                    
                    data_send[4 + length] = CRC8_table((uint8_t)CRC8_BASE, data_send, 4 + length);
                    
                    eu_uart_sign = UART_NONE;//清该标志
                    UART_Send(data_send, 4 + length + 1);
                }
                break;
                
                /* B代码段数据读取 */
                case 0x03:
                    read_code_place = FLASH_ADR_B;
                case 0x04:
                {
                    length = ((uint16_t)Tab[3]) * 256 + Tab[2] - 4;//获得代码数据长度
                    packet_count = ((uint16_t)Tab[5]) * 256 + Tab[4];//包总数
                    packet_curr = ((uint16_t)Tab[7]) * 256 + Tab[6];//当前包
                    
                    if (packet_count > packet_curr)
                    {
                        read_code_length = 256;//记录要读取代码段的长度
                        read_code_place += packet_curr * read_code_length;
                    }
                    else if (packet_count == packet_curr)
                    {
                        read_code_length = st_code_info.code_length % 256;//最后一段要读取代码的长度
                        read_code_place += st_code_info.code_length - read_code_length;
                    }
                    else
                    {
                        break;
                    }
                    
                    /* 组回复包 */
                    Tab[1] = uart_cmd;
                    Tab[2] = (uint16_t)(1 + 4 + read_code_length) % 256;//数据长度
                    Tab[3] = (uint16_t)(1 + 4 + read_code_length) / 256;//数据长度
                    //data_send[4] = 0;//错误代码
                    Tab[8] = Tab[7];
                    Tab[7] = Tab[6];
                    Tab[6] = Tab[5];
                    Tab[5] = Tab[4];

                    Tab[4] = 0;//错误代码
                    //FlashReadStr(FLASH_CODE_INFO, sizeof(flash_ABcode_info_st) , (uint8_t *)&st_code_info);
                    FlashReadStr(read_code_place, read_code_length, (u8 *)(Tab + 9));//Tab[9]
                    Tab[9 + read_code_length] = CRC8_table((uint8_t)CRC8_BASE, Tab, 9 + read_code_length);
                    
                    eu_uart_sign = UART_NONE;//清该标志
                    UART_Send(Tab, 10 + read_code_length);
                }
                break;
            }
            break;
        //---------------------------------------------------------------------------------
        case UART_JUMP_APP:
        break;
        case UART_RESET:
            NVIC_SystemReset();
        break;
        default:break;
    }
    
    return stat;
}


/*******************************************************************************
  * 函数名称：	USART1_IRQHandler
  * 函数说明：  串口1的中断处理函数
  * 输入参数：  无
  * 返回参数：  无
*******************************************************************************/
void USART1_IRQHandler(void)
{
    uint8_t data_temp;
    uint8_t crc_re = 0, crc_new = 0;
    ///static int8_t cmd_index = 0;
    static uint16_t length = 0;

    //接收中断
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)	   //判断读寄存器是否非空
	{   
        //eu_uart_sign = UART_NONE;
        /* 提取数据 */
        data_temp = USART_ReceiveData(USART1);

        /* 用户自定义命令 */
        if (0xaa == data_temp && UartReTimingDelay == 0)
        {
            uart_data_cnt = 0;
        }
        
        UartReTimingDelay = 10;
        
        Tab[uart_data_cnt] = data_temp;

        if (1 == uart_data_cnt)
        {
            uart_cmd = data_temp;//获得命令
        }
        else if (3 == uart_data_cnt)
        {
            length = ((uint16_t)Tab[3]) * 256 + Tab[2];//获得数据长度
            if (length > 260) //|| length == 0
            {
                eu_uart_sign = UART_LENGTH;
            }
        }
         
        if (eu_uart_sign == UART_NONE)
        {
            // 判断一帧数据是否接收完成 
            if (uart_data_cnt == length + 4)//4(帧头) + 数据区 + 1(crc) uart_data_cnt到这里的时候 只是索引号
            {
                crc_re = Tab[uart_data_cnt];//获取CRC
                crc_new = CRC8_table((uint8_t)CRC8_BASE, Tab, uart_data_cnt);
                if (crc_re == crc_new)
                {
                    /* 设置超时 */
                    //timingDelay_sign = 1;//超时退出的标志
                    TimingDelay = 10000;
                    
                    eu_uart_sign = UART_PACKET_FINISH;//一帧数据接收完毕
                }
                else
                {
                    eu_uart_sign = UART_CRC;//CRC校验错误
                }
                uart_data_cnt = 0;
            }
            else
            {
                uart_data_cnt++;
                if (uart_data_cnt >= 266)
                {
                    eu_uart_sign = UART_LENGTH;
                    uart_data_cnt = 0;
                }
            }
        }
	}
	
    if(USART_GetITStatus(USART1, USART_IT_TXE) != RESET)//这段是为了避免STM32 USART 第一个字节发不出去的BUG 
    { 
        USART_ITConfig(USART1, USART_IT_TXE, DISABLE);	//禁止发缓冲器空中断， 
    }	
}
