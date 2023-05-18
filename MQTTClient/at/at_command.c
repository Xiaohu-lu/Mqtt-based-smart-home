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
	/* ��ʼ����ֵ�ź���
	 * ��ʼֵΪ0
	 */
	platform_semphore_init(&at_ret_semphore);
	platform_semphore_init(&at_packet_semphore);
	ring_buffer_init(&g_packet_buffer);
	return 0;
}

/* AT��������
 * buf ��Ҫ���͵�����
 * len ��Ҫ���͵����ݳ���
 * timeout ����ʱʱ��
 */
int ATSendData(unsigned char *buf,int len,int timeout)
{
	int ret;
	int err;
	/*����AT����*/
	HAL_AT_Send((char*)buf,len);
	/* �ȴ����
	 * 1 ���ɹ��õ��ź���
	 * 0 ����ʱ����
	 */
	ret = platform_semphore_take_timeout(&at_ret_semphore,timeout);
	if (ret)
	{
		/* �жϷ���ֵ */
		/* �洢resp */
		err = GetATStatus();
		return err;
	}
	else
	{
		return AT_TIMEOUT;
	}
}

/* AT��ȡ���ݰ�
 * c ����ȡ1�ֽ�����
 * timeout ����ʱʱ��
 */
int ATReadData(unsigned char *c,int timerout)
{
	int ret;
	do{
		if(0 == ring_buffer_read((unsigned char*)c,&g_packet_buffer))/*�����ݰ����λ������������سɹ�*/
			return AT_OK;
		else/*�����ݰ��л�û�µ�1�ֽ�����,��ʱ�ȴ�*/
		{
			ret = platform_semphore_take_timeout(&at_packet_semphore,timerout);
			if(0 == ret)/*PDPASS�ɹ�1*/
				return AT_TIMEOUT;
		}
	}while(ret == 1);
	return 0;
}



/* ����AT������ȴ�������Ӧ
	 buf = "AT+CIPMODE=1"
	 len = strlen(buf)
*/
int ATSendCmd(char *buf,char *resp,int resp_len,int timeout)
{
	int ret;
	int err;
	/*����AT����*/
	HAL_AT_Send(buf,strlen(buf));
	HAL_AT_Send("\r\n",2);
	
	/* �ȴ����
	 * 1 ���ɹ��õ��ź���
	 * 0 ����ʱ����
	 */
	ret = platform_semphore_take_timeout(&at_ret_semphore,timeout);
	if(ret)
	{
		/*�жϷ���ֵ*/
		/*�洢resp*/
		err = GetATStatus();
		if(!err && resp)
		{
			memcpy(resp,g_at_resp,resp_len > AT_RESP_LEN ? AT_RESP_LEN : resp_len);
		}
		return err;
	}
	else/*��ʱ*/
	{
		return AT_TIMEOUT;
	}
}

/* �ж��Ƿ���յ������ַ�"+IPD,"
 * �ɹ� ����1
 * ʧ�� ����0
 */
static int GetSpecialATString(char *buf)
{
	if(strstr(buf,"+IPD,"))
		return 1;
	else
		return 0;
}


/* ������յ�������������
 * +IPD,n:xxxxxxx
 */
static void ProcessSpecialATString(char *buf)
{
	int i = 0;
	int len = 0;
	/* +IPD,78:xxxxxxx*/
	{
		/*����������*/
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
		
		/*��ȡlen����*/
		i = 0;
		while(i<len)
		{
			HAL_AT_Secv(&buf[i],(int)portMAX_DELAY);
			if(i<AT_RESP_LEN)
			{
				/*������д�뻷�λ�����*/
				ring_buffer_write(buf[i],&g_packet_buffer);
				/*�ͷ��ź���,֪ͨATReadData�����ݰ��κο��Զ�ȡһ���ֽ�����*/
				platform_semphore_give(&at_packet_semphore);
			}
			i++;
		}
	
	}
}

#if 0
/* �����������ص����ݰ�"+IPD,len:xxxxx"
 * buf,��ȡ�����ݰ���ŵ�λ��
 * len,Ҫ��ȡ�ĳ���
 * resp_len,ʵ�ʷ������ݵĳ���
 * timeout,��ʱʱ��
 */
int ATReadPacket(char *buf,int len,int *resp_len,int timeout)
{
	int ret;
	/*��ȡat_packet�ź����ȴ����յ����ݰ�*/
	ret = platform_semphore_take_timeout(&at_packet_semphore,timeout);
	if(ret)/*���յ����ݰ�*/
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

/* ����AT������Ӧ,������
*/
void ATRecvParser(void *params)
{
	char buf[AT_RESP_LEN];
	int i = 0;
	while(1)
	{
		/*��ȡESP8266ģ�鷵�ص����ݣ�������ʽ*/
		HAL_AT_Secv(&buf[i],(int)portMAX_DELAY);
		buf[i+1] = '\0';
		
		/* ������� */
		/* 1.  ��ʱ����
		 * 1.1 �յ�"\r\n"
		 * 1.2 �յ������ַ���"+IPD,"
		 */
		if(i && (buf[i-1] == '\r') && (buf[i] == '\n'))
		{
			/*�õ��˻��з�*/
					
			/* 2. ��ô����*/
			if(strstr(buf,"OK\r\n"))/*���յ�OK*/
			{
				/*��¼����*/
				memcpy(g_at_resp,buf,i);
				SetATStatus(AT_OK);
				/*�ͷ�at_ret�ź���,����ATSend,ATSendData����*/
				platform_semphore_give(&at_ret_semphore);
				i = 0;
			}
			else if(strstr(buf,"ERROR\r\n"))/*���յ�ERROR*/
			{
				/*��¼����*/
				memcpy(g_at_resp,buf,i);
				SetATStatus(AT_ERR);
				/*�ͷ�at_ret�ź���,����ATSend����*/
				platform_semphore_give(&at_ret_semphore);
				i = 0;
			}
			i=0;
		}	
		else if(GetSpecialATString(buf))/*���յ�"IPD,"*/
		{
			ProcessSpecialATString(buf);
			i = 0;
		}		
		else/*û���յ�"\r\n"�������½���*/
		{
			i++;
		}
		if(i >= AT_RESP_LEN)/*���ݳ��ȳ�����buf*/
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

