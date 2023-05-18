#include "at_command.h"
#include "at_uart_hal.h"
#include "ring_buffer.h"
#include <string.h>
#include "freertos.h"
#include "platform_Semaphore.h"
#include "driver_usart.h"
#include <mqttclient.h>

#define AT_CMD_TIMEOUT	1000
#define AT_RESP_LEN			100

static platform_semphr_t at_ret_semphore;
static platform_semphr_t at_packet_semphore;


static ring_buffer g_packet_buffer;

static int   g_at_status;
static char  g_at_resp[AT_RESP_LEN];
//static char  g_at_packet[AT_RESP_LEN];
//static int   g_at_packet_len;


/* Set status
 * 0	-	ok
 * -1	-	err
 * -2	-	timeout
 */
void SetATStatus(int status)
{
	g_at_status = status;
}

/* Get status
 * 0	-	ok
 * -1	-	err
 * -2	-	timeout
 */
int GetATStatus(void)
{
	return g_at_status;
}


int ATInit(void)
{
	/* 初始化二值信号量
	 * 初始值为0
	 */
	platform_semphore_init(&at_ret_semphore);
	platform_semphore_init(&at_packet_semphore);
	ring_buffer_init(&g_packet_buffer);
	return 0;
}

/* AT发送数据
 * buf ：要发送的数据
 * len ：要发送的数据长度
 * timeout ：超时时间
 */
int ATSendData(unsigned char *buf,int len,int timeout)
{
	int ret;
	int err;
	/*发送AT命令*/
	HAL_AT_Send((char*)buf,len);
	/* 等待结果
	 * 1 ：成功得到信号量
	 * 0 ：超时返回
	 */
	ret = platform_semphore_take_timeout(&at_ret_semphore,timeout);
	if (ret)
	{
		/* 判断返回值 */
		/* 存储resp */
		err = GetATStatus();
		return err;
	}
	else
	{
		return AT_TIMEOUT;
	}
}

/* AT读取数据包
 * c ：读取1字节数据
 * timeout ：超时时间
 */
int ATReadData(unsigned char *c,int timerout)
{
	int ret;
	do{
		if(0 == ring_buffer_read((unsigned char*)c,&g_packet_buffer))/*读数据包环形缓冲区读到返回成功*/
			return AT_OK;
		else/*从数据包中还没新的1字节数据,超时等待*/
		{
			ret = platform_semphore_take_timeout(&at_packet_semphore,timerout);
			if(0 == ret)/*PDPASS成功1*/
				return AT_TIMEOUT;
		}
	}while(ret == 1);
	return 0;
}



/* 发送AT命令、并等待接收响应
	 buf = "AT+CIPMODE=1"
	 len = strlen(buf)
*/
int ATSendCmd(char *buf,char *resp,int resp_len,int timeout)
{
	int ret;
	int err;
	/*发生AT命令*/
	HAL_AT_Send(buf,strlen(buf));
	HAL_AT_Send("\r\n",2);
	
	/* 等待结果
	 * 1 ：成功得到信号量
	 * 0 ：超时返回
	 */
	ret = platform_semphore_take_timeout(&at_ret_semphore,timeout);
	if(ret)
	{
		/*判断返回值*/
		/*存储resp*/
		err = GetATStatus();
		if(!err && resp)
		{
			memcpy(resp,g_at_resp,resp_len > AT_RESP_LEN ? AT_RESP_LEN : resp_len);
		}
		return err;
	}
	else/*超时*/
	{
		return AT_TIMEOUT;
	}
}

/* 判断是否接收到特殊字符"+IPD,"
 * 成功 返回1
 * 失败 返回0
 */
static int GetSpecialATString(char *buf)
{
	if(strstr(buf,"+IPD,"))
		return 1;
	else
		return 0;
}


/* 处理接收到服务器的数据
 * +IPD,n:xxxxxxx
 */
static void ProcessSpecialATString(char *buf)
{
	int i = 0;
	int len = 0;
	/* +IPD,78:xxxxxxx*/
	{
		/*解析出长度*/
		i = 0;
		while(1)
		{
			HAL_AT_Secv(&buf[i],(int)portMAX_DELAY);
			if(buf[i] == ':')
			{
				break;
			}
			else
			{
				len = len * 10 + (buf[i] - '0');
			}
			i++;
		}
		
		/*读取len数据*/
		i = 0;
		while(i<len)
		{
			HAL_AT_Secv(&buf[i],(int)portMAX_DELAY);
			if(i<AT_RESP_LEN)
			{
				/*把数据写入环形缓冲区*/
				ring_buffer_write(buf[i],&g_packet_buffer);
				/*释放信号量,通知ATReadData读数据包任何可以读取一个字节数据*/
				platform_semphore_give(&at_packet_semphore);
			}
			i++;
		}
	
	}
}

#if 0
/* 读服务器返回的数据包"+IPD,len:xxxxx"
 * buf,读取的数据包存放的位置
 * len,要读取的长度
 * resp_len,实际返回数据的长度
 * timeout,超时时间
 */
int ATReadPacket(char *buf,int len,int *resp_len,int timeout)
{
	int ret;
	/*获取at_packet信号量等待接收到数据包*/
	ret = platform_semphore_take_timeout(&at_packet_semphore,timeout);
	if(ret)/*接收到数据包*/
	{
		*resp_len = len > g_at_packet_len ? g_at_packet_len : len;
		memcpy(buf,g_at_packet,*resp_len);
		return AT_OK;
	}
	else
	{
		return AT_TIMEOUT;
	}
}

#endif

/* 接收AT返回响应,并解析
*/
void ATRecvParser(void *params)
{
	char buf[AT_RESP_LEN];
	int i = 0;
	while(1)
	{
		/*读取ESP8266模块返回的数据：阻塞方式*/
		HAL_AT_Secv(&buf[i],(int)portMAX_DELAY);
		buf[i+1] = '\0';
		
		/* 解析结果 */
		/* 1.  何时解析
		 * 1.1 收到"\r\n"
		 * 1.2 收到特殊字符："+IPD,"
		 */
		if(i && (buf[i-1] == '\r') && (buf[i] == '\n'))
		{
			/*得到了换行符*/
					
			/* 2. 怎么解析*/
			if(strstr(buf,"OK\r\n"))/*接收到OK*/
			{
				/*记录数据*/
				memcpy(g_at_resp,buf,i);
				SetATStatus(AT_OK);
				/*释放at_ret信号量,唤醒ATSend,ATSendData任务*/
				platform_semphore_give(&at_ret_semphore);
				i = 0;
			}
			else if(strstr(buf,"ERROR\r\n"))/*接收到ERROR*/
			{
				/*记录数据*/
				memcpy(g_at_resp,buf,i);
				SetATStatus(AT_ERR);
				/*释放at_ret信号量,唤醒ATSend任务*/
				platform_semphore_give(&at_ret_semphore);
				i = 0;
			}
			i=0;
		}	
		else if(GetSpecialATString(buf))/*接收到"IPD,"*/
		{
			ProcessSpecialATString(buf);
			i = 0;
		}		
		else/*没接收到"\r\n"继续向下接收*/
		{
			i++;
		}
		if(i >= AT_RESP_LEN)/*数据长度超出了buf*/
			i = 0;
	}
}

void Task_ATTest(void *Param)
{
	int ret;
	while(1)
	{
		ret = ATSendCmd("AT",NULL,0,1000);
		printf("ATSendCmd ret = %d\r\n",ret);
	}
}


static void topicl_handler(void *client,message_data_t *msg)
{
	(void) client;
	MQTT_LOG_I("-----------------------------------------------------------------------------------");
  MQTT_LOG_I("%s:%d %s()...\r\ntopic: %s\r\nmessage:%s", __FILE__, __LINE__, __FUNCTION__, msg->topic_name, (char*)msg->message->payload);
  MQTT_LOG_I("-----------------------------------------------------------------------------------");
}

void MQTT_Client_Task(void *Param)
{
	int err;
	char buf[20];
	int cnt = 0;
	mqtt_client_t *client = NULL;
	mqtt_message_t msg;
	memset(&msg,0,sizeof(msg));
	
	mqtt_log_init();
	client = mqtt_lease();
	mqtt_set_port(client,"1883");
	
	mqtt_set_host(client,"47.144.187.247");//iot.100ask.net
	mqtt_set_client_id(client,random_string(10));
	mqtt_set_user_name(client,random_string(10));
	mqtt_set_password(client,random_string(10));
	mqtt_set_clean_session(client,1);
	
	if(0!=mqtt_connect(client))
	{
		printf("mqtt_connect err\r\n");
		vTaskDelete(NULL);
	}
	
	err = mqtt_subscribe(client,"topic1",QOS0,topicl_handler);
	if(err)
	{
		printf("mqtt_subscribe topic1 err\r\n");
	}
	
	err = mqtt_subscribe(client, "topic2", QOS1, NULL);
	if (err)
	{
		printf("mqtt_subscribe topic2 err\r\n");
	}

  err = mqtt_subscribe(client, "topic3", QOS2, NULL);
	if (err)
	{
		printf("mqtt_subscribe topic3 err\r\n");
	}
	
	msg.payload = buf;
	msg.qos = QOS0;
	
	while(1)
	{
		sprintf(buf,"100ask,%d",cnt++);
		msg.payloadlen = strlen(msg.payload);
		mqtt_publish(client,"mcu_test",&msg);
		vTaskDelay(5000);
	}
	
}

