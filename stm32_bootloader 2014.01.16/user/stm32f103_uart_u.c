/*******************************************************************************
** �ļ���: 		stm32f0_uart_u.c
** �汾��  		1.0
** ��������: 	RealView MDK-ARM 4.60
** ����: 		  wdk
** ��������: 	2013-11-18
** ����:		  STM32F0
** ����ļ�:	
** �޸���־:
*******************************************************************************/
#include "stm32f10x.h"
#include "misc.h"
#include "stdarg.h"
#include "stm32f103_flash_u.h"
#include "stm32f103_uart_u.h"
#include "stm32f10x_usart.h"
//#include "stdlib.h"

/* �ⲿ�������� */
extern int32_t timingDelay_sign;
extern __IO uint32_t TimingDelay;

/* �����ֽڽ�����ʱ */
__IO uint32_t UartReTimingDelay = 10;//10ms

uint8_t PackCnt = 0;                 //??????????,??????
uint16_t uart_data_cnt = 0;
uint8_t PackNum = 0;                 //?????
uint8_t TestVal = 0;                 //??????????
uint8_t Tab[266] = {0};                //wdk 2013.12.18 modify 265->266���ӷ��س������ζ�ȡ�Ĵ������

uint32_t flash_1K_add = 0;
uint32_t flash_1K_add_temp = 0;
uint32_t flash_code_add = FLASH_ADR_B;//B������д

/* xmodemЭ��׶���ɱ�־ */
//#pragma pack(1)
uart_state_eu eu_uart_sign = UART_NONE;

/* �������� */
uint8_t uart_cmd = 0;

/* ����У�������� */
flash_ABcode_info_st st_code_info = {0, 0, CRC8_BASE, 0 , 0, 0};

/****************************************************************************
* ��    �ƣ�void NVIC_Configuration(void)
* ��    �ܣ��ж�Դ����
* ��ڲ�������
* ���ڲ�������
* ˵    ����
* ���÷������� 
****************************************************************************/
void NVIC_Configuration(void)
{
  /*  �ṹ����*/
  NVIC_InitTypeDef NVIC_InitStructure;

  /* Configure the NVIC Preemption Priority Bits */  
  /* Configure one bit for preemption priority */
  /* ���ȼ��� ˵������ռ���ȼ����õ�λ�����������ȼ����õ�λ��   ��������1�� 7 */    
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);	  
  
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;			     	//���ô���1�ж�
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;	     	//��ռ���ȼ� 0
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;				//�����ȼ�Ϊ0
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;					//ʹ��
  NVIC_Init(&NVIC_InitStructure);
}

/******************************************************
*uart����
*******************************************************/
void USART_Configuration(void)
{
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    //USART_ClockInitTypeDef USART_ClockInitStructure;
    
    NVIC_Configuration();   
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;	         	 //USART1 TX
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;    		 //�����������
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);		    		 //A�˿� 

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;	         	 //USART1 RX
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;    //���ÿ�©����
    GPIO_Init(GPIOA, &GPIO_InitStructure);		         	 //A�˿� 

    USART_InitStructure.USART_BaudRate = 9600;					//����115200bps
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;		//����λ8λ
    USART_InitStructure.USART_StopBits = USART_StopBits_1;			//ֹͣλ1λ
    USART_InitStructure.USART_Parity = USART_Parity_No;				//��У��λ
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;   //��Ӳ������
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;					  //�շ�ģʽ
    USART_Init(USART1, &USART_InitStructure);

    /* Enable USART1 Receive and Transmit interrupts */
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);                      //ʹ�ܽ����ж�
    //USART_ITConfig(USART1, USART_IT_TXE, ENABLE);						//ʹ�ܷ��ͻ�����ж�   

    /* Enable the USART1 */
    USART_Cmd(USART1, ENABLE);	
}


/******************************************************
*uart�����ֽ�
*******************************************************/
void UART_send_byte(uint8_t byte) 
{
    USART_SendData(USART1, byte);
	while(USART_GetFlagStatus(USART1, USART_FLAG_TC)==RESET);
}

/******************************************************
*uart��������
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
*xmodemЭ�鴦��
*******************************************************/
int32_t uart_reply(void)
{
    int32_t i = 0, stat = 0;
    uint8_t data_send[30] = {0};
    uint16_t length = 0;
    uint16_t packet_count = 0, packet_curr = 0;
    uint8_t *p_st_code_info = (uint8_t *)&st_code_info;
    uint32_t read_code_length = 0;//��¼Ҫ��ȡ����εĳ���
    uint32_t read_code_place = FLASH_ADR_A;//��¼Ҫ��ȡ����ε�λ��
    
    data_send[0] = 0xAA;
    switch(eu_uart_sign)
    {
        case UART_CRC://crc����
            data_send[1] = uart_cmd;
            data_send[2] = 0x01;//��������
            ///data_send[3] = 0x00;
            data_send[4] = UART_CRC;
            data_send[5] = CRC8_table((uint8_t)CRC8_BASE, data_send, 5);
            
            eu_uart_sign = UART_NONE;//��ñ�־
            UART_Send(data_send, 6);
        break;
        case UART_PACKET_FINISH://һ�����ݽ������
            switch(uart_cmd)
            {
                case 0x01:
                {
                    length = ((uint16_t)Tab[3]) * 256 + Tab[2] - 4;//��ô������ݳ���
                    packet_count = ((uint16_t)Tab[5]) * 256 + Tab[4];//������
                    packet_curr = ((uint16_t)Tab[7]) * 256 + Tab[6];//��ǰ��
                    
                    if (0 == packet_curr)//��ʾ��һ��
                    {//��ʼ����ر��������¿�ʼ
                        
                        /* ������������룬��Ҫ��Ϊ����ȡA����εĳ��� wdk 2013.12.17 add */
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
                        flash_page_data[flash_1K_add] = Tab[8 + i];//���ݴӵ�8���ֽڿ�ʼ
                        flash_1K_add++;
                        if (flash_1K_add == FLASH_PAGE_SIZE)
                        {
                            FlashWriteStr(flash_code_add, FLASH_PAGE_SIZE, flash_page_data);
                            
                            flash_code_add += FLASH_PAGE_SIZE;
                            flash_1K_add = 0;
                            
                            st_code_info.code_page_count++;
                            st_code_info.code_length += FLASH_PAGE_SIZE;
                 
                            /* ����CRC8 */
                            st_code_info.code_crc8 = CRC8_table(st_code_info.code_crc8, flash_page_data, FLASH_PAGE_SIZE);
                        }
                    }
                    
                    /* ��ʾ���һ������ */
                    if (packet_count == packet_curr)
                    {
                        if (flash_1K_add > 0)
                        {
                            FlashWriteStr(flash_code_add, FLASH_PAGE_SIZE, flash_page_data);
                            
                            ///flash_code_add += FLASH_PAGE_SIZE;
                            st_code_info.code_page_count++;
                            st_code_info.code_length += flash_1K_add;
                            /* ����CRC8 */
                            st_code_info.code_crc8 = CRC8_table(st_code_info.code_crc8, flash_page_data, flash_1K_add);
                            
                            flash_1K_add = 0;
                        }
                        
                        /* дУ�����ݵ�flash */
                        st_code_info.code_changed = CODE_CHANGED;
                        st_code_info.crc8 = CRC8_table((uint8_t)CRC8_BASE, (uint8_t *)&st_code_info, (uint32_t)CODE_INFO_CRC_ADDR);
                        FlashWriteStr(FLASH_CODE_INFO, sizeof(flash_ABcode_info_st), (uint8_t *)&st_code_info);
                        
                        stat = 1;
                    }
                    
                    /* ��ظ��� */
                    data_send[1] = uart_cmd;
                    data_send[2] = 0x01;//��������
                    //data_send[3] = 0x00;
                    data_send[4] = UART_NONE;
                    data_send[5] = CRC8_table((uint8_t)CRC8_BASE, data_send, 5);
                    
                    eu_uart_sign = UART_NONE;//��ñ�־
                    UART_Send(data_send, 6);
                }
                break;
                case 0x02:
                {
                    /* ������������룬��Ҫ��Ϊ����ȡA����εĳ��� wdk 2013.12.17 add */
                    FlashReadStr(FLASH_CODE_INFO, sizeof(flash_ABcode_info_st) , (uint8_t *)&st_code_info);
                        
                    /* ��ظ��� */
                    data_send[1] = uart_cmd;
                    length = sizeof(flash_ABcode_info_st);
                    data_send[2] = length % 256;//��������
                    data_send[3] = length / 256;
                    
                    for (i = 0; i < length; i++)
                    {
                        data_send[4 + i] = *p_st_code_info++;
                    }
                    
                    data_send[4 + length] = CRC8_table((uint8_t)CRC8_BASE, data_send, 4 + length);
                    
                    eu_uart_sign = UART_NONE;//��ñ�־
                    UART_Send(data_send, 4 + length + 1);
                }
                break;
                
                /* B��������ݶ�ȡ */
                case 0x03:
                    read_code_place = FLASH_ADR_B;
                case 0x04:
                {
                    length = ((uint16_t)Tab[3]) * 256 + Tab[2] - 4;//��ô������ݳ���
                    packet_count = ((uint16_t)Tab[5]) * 256 + Tab[4];//������
                    packet_curr = ((uint16_t)Tab[7]) * 256 + Tab[6];//��ǰ��
                    
                    if (packet_count > packet_curr)
                    {
                        read_code_length = 256;//��¼Ҫ��ȡ����εĳ���
                        read_code_place += packet_curr * read_code_length;
                    }
                    else if (packet_count == packet_curr)
                    {
                        read_code_length = st_code_info.code_length % 256;//���һ��Ҫ��ȡ����ĳ���
                        read_code_place += st_code_info.code_length - read_code_length;
                    }
                    else
                    {
                        break;
                    }
                    
                    /* ��ظ��� */
                    Tab[1] = uart_cmd;
                    Tab[2] = (uint16_t)(1 + 4 + read_code_length) % 256;//���ݳ���
                    Tab[3] = (uint16_t)(1 + 4 + read_code_length) / 256;//���ݳ���
                    //data_send[4] = 0;//�������
                    Tab[8] = Tab[7];
                    Tab[7] = Tab[6];
                    Tab[6] = Tab[5];
                    Tab[5] = Tab[4];

                    Tab[4] = 0;//�������
                    //FlashReadStr(FLASH_CODE_INFO, sizeof(flash_ABcode_info_st) , (uint8_t *)&st_code_info);
                    FlashReadStr(read_code_place, read_code_length, (u8 *)(Tab + 9));//Tab[9]
                    Tab[9 + read_code_length] = CRC8_table((uint8_t)CRC8_BASE, Tab, 9 + read_code_length);
                    
                    eu_uart_sign = UART_NONE;//��ñ�־
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
  * �������ƣ�	USART1_IRQHandler
  * ����˵����  ����1���жϴ�����
  * ���������  ��
  * ���ز�����  ��
*******************************************************************************/
void USART1_IRQHandler(void)
{
    uint8_t data_temp;
    uint8_t crc_re = 0, crc_new = 0;
    ///static int8_t cmd_index = 0;
    static uint16_t length = 0;

    //�����ж�
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)	   //�ж϶��Ĵ����Ƿ�ǿ�
	{   
        //eu_uart_sign = UART_NONE;
        /* ��ȡ���� */
        data_temp = USART_ReceiveData(USART1);

        /* �û��Զ������� */
        if (0xaa == data_temp && UartReTimingDelay == 0)
        {
            uart_data_cnt = 0;
        }
        
        UartReTimingDelay = 10;
        
        Tab[uart_data_cnt] = data_temp;

        if (1 == uart_data_cnt)
        {
            uart_cmd = data_temp;//�������
        }
        else if (3 == uart_data_cnt)
        {
            length = ((uint16_t)Tab[3]) * 256 + Tab[2];//������ݳ���
            if (length > 260) //|| length == 0
            {
                eu_uart_sign = UART_LENGTH;
            }
        }
         
        if (eu_uart_sign == UART_NONE)
        {
            // �ж�һ֡�����Ƿ������� 
            if (uart_data_cnt == length + 4)//4(֡ͷ) + ������ + 1(crc) uart_data_cnt�������ʱ�� ֻ��������
            {
                crc_re = Tab[uart_data_cnt];//��ȡCRC
                crc_new = CRC8_table((uint8_t)CRC8_BASE, Tab, uart_data_cnt);
                if (crc_re == crc_new)
                {
                    /* ���ó�ʱ */
                    //timingDelay_sign = 1;//��ʱ�˳��ı�־
                    TimingDelay = 10000;
                    
                    eu_uart_sign = UART_PACKET_FINISH;//һ֡���ݽ������
                }
                else
                {
                    eu_uart_sign = UART_CRC;//CRCУ�����
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
	
    if(USART_GetITStatus(USART1, USART_IT_TXE) != RESET)//�����Ϊ�˱���STM32 USART ��һ���ֽڷ�����ȥ��BUG 
    { 
        USART_ITConfig(USART1, USART_IT_TXE, DISABLE);	//��ֹ�����������жϣ� 
    }	
}
