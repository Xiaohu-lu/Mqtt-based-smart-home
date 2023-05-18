/*  Copyright (s) 2019 ���ڰ������Ƽ����޹�˾
 *  All rights reserved
 * 
 * �ļ����ƣ�driver_usart.h
 * ժҪ��
 *  
 * �޸���ʷ     �汾��        Author       �޸�����
 *--------------------------------------------------
 * 2020.6.6      v01        ���ʿƼ�      �����ļ�
 *--------------------------------------------------
*/

#ifndef __DRIVER_USART_H
#define __DRIVER_USART_H

#include "stm32f1xx_hal.h"
#include <stdio.h>
/*
 *  ��������void EnableDebugIRQ(void)
 *  �����������
 *  �����������
 *  ����ֵ����
 *  �������ã�ʹ��USART1���ж�
*/
extern void EnableDebugIRQ(void);

/*
 *  ��������void EnableUART3IRQ(void)
 *  �����������
 *  �����������
 *  ����ֵ����
 *  �������ã�ʹ��USART3���ж�
*/
extern void EnableUART3IRQ(void);
/*
 *  ��������void DisableDebugIRQ(void)
 *  �����������
 *  �����������
 *  ����ֵ����
 *  �������ã�ʧ��USART1���ж�
*/
extern void DisableDebugIRQ(void);


int fputc(int ch, FILE *f);
int fgetc(FILE *f);


#endif /* __DRIVER_USART_H */