#include "platform_Semaphore.h"

/* 二值信号量初始
 * 初始化为0
*/
int platform_semphore_init(platform_semphr_t* s)
{
	s->Semphore = xSemaphoreCreateBinary(); //xSemaphoreCreateMutex();
	//xSemaphoreGive(s->Semphore);
	return 0;
}

/* 获取一个信号量,无限等待
*/
int platform_semphore_take(platform_semphr_t* s)
{ 
	return xSemaphoreTake(s->Semphore,portMAX_DELAY);
}

/* 获取一个信号量,复制等待时间
*/
int platform_semphore_take_timeout(platform_semphr_t* s,int timeout)
{ 
	return xSemaphoreTake(s->Semphore,timeout);
}


/* 尝试获取一个信号量,不等待
*/
int platform_semphore_trytake(platform_semphr_t* s)
{ 
	return xSemaphoreTake(s->Semphore,0);
}

/* 释放一个信号量
*/
int platform_semphore_give(platform_semphr_t* s)
{ 
	return xSemaphoreGive(s->Semphore);
}


/* 释放一个信号量,从中断里面释放
*/
int platform_semphore_give_from_isr(platform_semphr_t* s)
{
		
	static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	/*释放二值信号量*/
	xSemaphoreGiveFromISR(s->Semphore,&xHigherPriorityTaskWoken);
	/*如果需要的话进行一次任务切换,系统会判断是否需要进行切换*/
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	return pdTRUE;
}


int platform_semphore_destroy(platform_semphr_t* s)
{
	vSemaphoreDelete(s->Semphore);
  return 0;
}
