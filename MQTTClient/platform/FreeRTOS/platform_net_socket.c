/*
 * @Author: jiejie
 * @Github: https://github.com/jiejieTop
 * @Date: 2020-01-10 23:45:59
 * @LastEditTime: 2020-04-25 17:50:58
 * @Description: the code belongs to jiejie, please keep the author information and source code according to the license.
 */
#include "mqtt_log.h"
#include "platform_net_socket.h"
#include "at_command.h"

#define TEST_SSID			"W201314H"
#define TEST_PASSWD		"heguobao"

int platform_net_socket_connect(const char *host, const char *port, int proto)
{
	int err;
	char cmd[100];
	while(1)
	{
		/* 1. ���� WIFI ģʽ */
		err = ATSendCmd("AT+CWMODE=1",NULL,0,2000);
		if(err)
		{
			printf("AT+CWMODE=1 err = %d\r\n",err);
		}
		
		/* 2. ����·���� */
		/* 2.1 ����AP 
		   AT+CWJAP="abc","0123456"
		 */
		err = ATSendCmd("AT+CWJAP=\"" TEST_SSID "\",\"" TEST_PASSWD "\"",NULL,0,10000);
		if(err)
		{
			printf("connect AP err = %d\r\n",err);
			continue;
		}
		/* 2.2 �ر�TCP���� */
		err = ATSendCmd("AT+CIPCLOSE",NULL,0,2000);
		
		/* 3. ���ӵ������� */
		if(proto == PLATFORM_NET_PROTO_TCP)
		{
			/* AT+CIPSTART="TCP","192.168.3.116",8080 */
			sprintf(cmd,"AT+CIPSTART=\"TCP\",\"%s\",%s",host,port);
		}			
		else
		{
			sprintf(cmd,"AT+CIPSTART=\"UDP\",\"%s\",%s",host,port);
		}
		err = ATSendCmd(cmd,NULL,0,20000);
		if(err)
		{
			printf("%s err = %d\r\n",cmd,err);
			continue;
		}
		if(!err)
			break;
	}
	return 0;
}

#if 0
int platform_net_socket_recv(int fd, void *buf, size_t len, int flags)
{
    return 0;
}
#endif

/* �����������ص�����,���ض������ֽ��� +IPD,n:xxxxxxxxxx	*/
int platform_net_socket_recv_timeout(int fd, unsigned char *buf, int len, int timeout)
{
	int i = 0;
	int err;
	/*������,ʧ��������*/
	while(i<len)
	{
		err = ATReadData(&buf[i],timeout);
		if(err)
		{
			return 0;
		}
		i++;
	}
	return len;
}

#if 0
int platform_net_socket_write(int fd, void *buf, size_t len)
{
	return 0;
}
#endif

int platform_net_socket_write_timeout(int fd, unsigned char *buf, int len, int timeout)
{
	int err;
	char cmd[20];
	char resp[100];
	/* ����len�ֽ����� AT+CIPSEND=3 */
	sprintf(cmd,"AT+CIPSEND=%d",len);
	err = ATSendCmd(cmd,resp,100,timeout);
	if(err)
	{
		resp[99] = '\0';
		printf("%s err = %d, timeout = %d\r\n",cmd,err,timeout);
		printf("resp : %s\r\n",resp);
		return err;
	}
	err = ATSendData(buf,len,timeout);
	if(err)
	{
		printf("ATSendData err = %d\r\n",err);
		return err;
	}
	return len;
}

int platform_net_socket_close(int fd)
{
	
	return ATSendCmd("AT+CIPCLOSE",NULL,0,2000);
}

#if 0
int platform_net_socket_set_block(int fd)
{
   
	return 0;
}

int platform_net_socket_set_nonblock(int fd)
{
  
	return 0;
}

int platform_net_socket_setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen)
{
	return 0;
}
#endif
