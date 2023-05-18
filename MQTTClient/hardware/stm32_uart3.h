#ifndef __STM32_UART3_H
#define __STM32_UART3_H


void UART3_Lock_Init(void);
void USART3_Write(char *buf,int len);
void USART3_Read(char *c,int timeout);


#endif

