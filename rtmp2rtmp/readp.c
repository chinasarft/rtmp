#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "librtmp/rtmp_sys.h"
#include "librtmp/log.h"
#include "librtmp/rtmp.h"

#define true 1
#define false 0
typedef int bool;

int InitSockets()
{
#ifdef WIN32
    WORD version;
    WSADATA wsaData;
    version = MAKEWORD(1, 1);
    return (WSAStartup(version, &wsaData) == 0);
#endif
}
void phex(unsigned char * p, int l){
	int i;
	int j;
	if( p == NULL){
		printf("NULL\n");
		return ;
	}
	j = l > 60 ? 60 : l;
	for(i= 0; i < j; i++){
		printf("%02x", p[i]);
	}
	printf("\n");
}
 
void CleanupSockets()
{
#ifdef WIN32
    WSACleanup();
#endif
}
 
RTMP * connect_push(char * pushurl){
         RTMP *rtmp=NULL;                           
 
         /* set log level */
         //RTMP_LogLevel loglvl=RTMP_LOGDEBUG;
         //RTMP_LogSetLevel(loglvl);
                  
         rtmp=RTMP_Alloc();
         RTMP_Init(rtmp);
         //set connection timeout,default 30s
         rtmp->Link.timeout=5;                      
         if(!RTMP_SetupURL(rtmp,pushurl))
         {
                   RTMP_Log(RTMP_LOGERROR,"SetupURL Err\n");
                   RTMP_Free(rtmp);
                   CleanupSockets();
                   return NULL;
         }
        
         //if unable,the AMF command would be 'play' instead of 'publish'
         RTMP_EnableWrite(rtmp);     
        
         if (!RTMP_Connect(rtmp,NULL)){
                   RTMP_Log(RTMP_LOGERROR,"Connect Err\n");
                   RTMP_Free(rtmp);
                   CleanupSockets();
                   return NULL;
         }
        
         if (!RTMP_ConnectStream(rtmp,0)){
                   RTMP_Log(RTMP_LOGERROR,"ConnectStream Err\n");
                   RTMP_Close(rtmp);
                   RTMP_Free(rtmp);
                   CleanupSockets();
                   return NULL;
         }
		 return rtmp;
}
 
int push(RTMP *rtmp, RTMPPacket *packet){
         RTMP_LogPrintf("Start to send data ...\n");
        
         //start_time=RTMP_GetTime();
         if (!RTMP_IsConnected(rtmp)){
                  RTMP_Log(RTMP_LOGERROR,"rtmp is not connect\n");
				  return -1;
         }
         if (!RTMP_SendPacket(rtmp,packet,0)){
                  RTMP_Log(RTMP_LOGERROR,"Send Error\n");
				  return -1;
         }
 
         return 0;
}
int main(int argc, char* argv[])
{
	RTMPPacket pkt;
	int pushflag;
	if(argc != 2 && argc != 3){
			printf("need rtmpurl [pushurl]\n");
			exit(1);
	}
	if(argc == 3)
		pushflag = 1;
	else
		pushflag = 0;
    InitSockets();
     
    //is live stream ?
    bool bLiveStream=true;              
     
    /* set log level */
    //RTMP_LogLevel loglvl=RTMP_LOGDEBUG;
    //RTMP_LogSetLevel(loglvl);
 
    RTMP *pushrtmp=NULL;

    RTMP *rtmp=RTMP_Alloc();
    RTMP_Init(rtmp);
    //set connection timeout,default 30s
    rtmp->Link.timeout=10;   
    // HKS's live URL
    if(!RTMP_SetupURL(rtmp, argv[1]))
    {
        RTMP_Log(RTMP_LOGERROR,"SetupURL Err\n");
        RTMP_Free(rtmp);
        CleanupSockets();
        return -1;
    }
    if (bLiveStream){
        rtmp->Link.lFlags|=RTMP_LF_LIVE;
    }
     
    //1hour
    RTMP_SetBufferMS(rtmp, 3600*1000);      
     
    if(!RTMP_Connect(rtmp,NULL)){
        RTMP_Log(RTMP_LOGERROR,"Connect Err\n");
        RTMP_Free(rtmp);
        CleanupSockets();
        return -1;
    }
 
    if(!RTMP_ConnectStream(rtmp,0)){
        RTMP_Log(RTMP_LOGERROR,"ConnectStream Err\n");
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        CleanupSockets();
        return -1;
    }

	if(pushflag){
		pushrtmp=connect_push(argv[2]);
		if(pushrtmp == NULL){
			printf("connect push fail\n");
			exit(2);
		}
	}else{
		pushrtmp = NULL;
	}
 
    RTMPPacket_Reset(&pkt);
    while( RTMP_ReadPacket(rtmp,&pkt) ){
		if (!RTMPPacket_IsReady(&pkt) ){              
              continue;
        }
        printf("%5d %5d %5d %2d:",pkt.m_nBytesRead, pkt.m_nBodySize, pkt.m_nChannel, pkt.m_packetType);
		phex((unsigned char *)pkt.m_body, pkt.m_nBodySize);
		if(pushflag){
			int r = push(pushrtmp, &pkt);
			if(r){
					printf("push fail pkt fail\n");
			}
		}
		RTMPPacket_Free(&pkt);
        RTMPPacket_Reset(&pkt);
    }
 
 
    if(pushrtmp){
        RTMP_Close(pushrtmp);
        RTMP_Free(pushrtmp);
        pushrtmp=NULL;
    }   
    if(rtmp){
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        CleanupSockets();
        rtmp=NULL;
    }   
    return 0;
}
