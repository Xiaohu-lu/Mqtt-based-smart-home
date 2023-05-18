#include "driver_usart.h"
#include "stm32_uart3.h"
#include "ring_buffer.h"
#include "platform_Semaphore.h"
static ring_buffer uart3_buffer;
static platform_semphr_t uart_recv_semphore;

extern UART_HandleTypeDef huart3;

void USART3_IRQHandler(void)
{
	/* 如果发生的是RX中断
	 * 把数据读出来，存入环形buffer
	 */
	uint32_t isrflags = READ_REG(huart3.Instance->SR);
	uint32_t crlits = READ_REG(huart3.Instance->CR1);
	char c;
	if(((isrflags & USART_SR_RXNE) != RESET) && ((crlits & USART_CR1_RXNEIE) != RESET))
	{
		c = huart3.Instance->DR;
		ring_buffer_write(c,&uart3_buffer);
		/*释放二值信号量*/
		platform_semphore_give_from_isr(&uart_recv_semphore);
		return;
	}
}

void UART3_Lock_Init(void)
{
	/*初始化环形缓冲区*/
	ring_buffer_init(&uart3_buffer);
	/*初始化二值信号量,初始化信号量为0*/
	platform_semphore_init(&uart_recv_semphore);
}


void USART3_Write(char *buf,int len)
{
	int i = 0;
	while(i<len)
	{
		while((huart3.Instance->SR & USART_SR_TXE) == 0);
		huart3.Instance->DR = buf[i];
		i++;
	}
}

void USART3_Read(char *c,int timeout)
{
	while(1)
	{
		if(0 == ring_buffer_read((unsigned char*)c,&uart3_buffer))
			return;
		else/*阻塞*/
		{
			platform_semphore_take_timeout(&uart_recv_semphore,timeout);
		}
	}
}
