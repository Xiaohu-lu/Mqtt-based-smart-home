#include "stm32_uart3.h"
#include "at_uart_hal.h"

void HAL_AT_Send(char *buf,int len)
{
	USART3_Write(buf,len);
}


void HAL_AT_Secv(char *c,int timeout)
{
	/* �ӻ��λ������еõ�����
	 * ������������
	 */
	USART3_Read(c,timeout);
}

