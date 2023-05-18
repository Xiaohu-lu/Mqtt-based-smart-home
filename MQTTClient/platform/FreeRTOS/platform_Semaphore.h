#ifndef _PLATFORM_SEMAPHORE_H
#define _PLATFORM_SEMAPHORE_H
#include "FreeRTOS.h"
#include "semphr.h"


typedef struct platform_semphr{
	SemaphoreHandle_t Semphore;
}platform_semphr_t;


int platform_semphore_init(platform_semphr_t* s);
int platform_semphore_take(platform_semphr_t* s);
int platform_semphore_take_timeout(platform_semphr_t* s,int timeout);
int platform_semphore_trytake(platform_semphr_t* s);
int platform_semphore_give(platform_semphr_t* s);
int platform_semphore_give_from_isr(platform_semphr_t* s);
int platform_semphore_destroy(platform_semphr_t* s);
#endif

