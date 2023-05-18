#ifndef _AT_COMMAND_H
#define _AT_COMMAND_H

#define AT_OK        0
#define AT_ERR      -1
#define AT_TIMEOUT  -2

void SetATStatus(int status);
int GetATStatus(void);
int ATInit(void);
int ATSendData(unsigned char *buf,int len,int timeout);
int ATReadData(unsigned char *c,int timerout);
int ATSendCmd(char *buf,char *resp,int resp_len,int timeout);
static int GetSpecialATString(char *buf);
static void ProcessSpecialATString(char *buf);
//int ATReadPacket(char *buf,int len,int *resp_len,int timeout);
void ATRecvParser(void *params);
void Task_ATTest(void *Param);
void MQTT_Client_Task(void *Param);
#endif
