/***************************************************************************
    \file ADM_avsproxy_net.cpp
    \brief Handle the network part of avsproxy demuxer
    \author (C) 2007-2010 by mean  fixounet@free.fr

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <io.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include "ADM_default.h"
#include "ADM_avsproxy_internal.h"
#include "ADM_avsproxy_net.h"

#define MAGGIC 0xDEADBEEF

#define aprintf(...) {}
//#define DEBUG_NET
/**
    \fn bindMe
*/
bool avsNet::bindMe(uint32_t port)
{
 #ifdef _WIN32
 mySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
 #else
 mySocket = socket(PF_INET, SOCK_STREAM, 0);
 #endif
    if(mySocket==-1)
    {
        printf("Socket failed\n");
        return 0;
    }
    struct sockaddr_in  service;
    service.sin_family = AF_INET;
#ifdef DEBUG_NET
    service.sin_addr.s_addr = inet_addr("192.168.0.21");
#else
    service.sin_addr.s_addr = inet_addr("127.0.0.1");
#endif    
    service.sin_port = htons(port);
    
// Set socket to lowdelay, else it will be choppy
    int flag = 1;
    setsockopt( mySocket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag) );


    if(connect(mySocket,(struct sockaddr *)&service,sizeof(service)))
    {
        printf("[avsProxy]Socket connect error %d on port %d\n",errno,port);
        return 0;
    }
    printf("[avsproxy]Connected to avsproxy : port %d, socket %d\n",port,mySocket);
    return 1;
}
/**
    \fn close
*/
bool avsNet::close(void)
{
    if(mySocket)
    {
        int er;
#ifdef _WIN32
		er=shutdown(mySocket,SD_BOTH);
#else
        er=shutdown(mySocket,SHUT_RDWR);
#endif
        if(er) printf("[avsProxy]Error when socket shutdown  %d (socket %d)\n",er,mySocket);
        mySocket=0;
    }
    return 1;
}
/**
    \fn askFor
*/
bool avsNet::command(uint32_t cmd,uint32_t frame,avsNetPacket *in,avsNetPacket *out)
{
   avsNetPacket dummy={0,0,NULL};
   avsNetPacket *in2=in;
    if(!in2) in2=&dummy;
    lock.lock();
    if(!sendData(cmd,frame,in2->size,in2->buffer))
    {
        printf("[avsProxy]Send Cmd %u failed for frame %u\n",cmd,frame);
        lock.unlock();
        return 0;
    }
    // Wait reply
    uint32_t size,reply,outframe;
    if(!receiveData(&reply,&outframe,&(out->size),out->buffer))
    {
        printf("[avsProxy]Rx Cmd %u failed for frame %u\n",cmd,frame);
        return 0;   
    }
  
    // Check!
    ADM_assert(out->size<=out->sizeMax);
    ADM_assert(reply==cmd+1);
    aprintf("[avsProxy]Cmd %u on frame %u succeed\n",cmd,frame);
    lock.unlock();
    return 1;   
}
/**
    \fn rxData
*/
bool avsNet::rxData(uint32_t howmuch, uint8_t *where)
{
uint32_t got=0;
int rx;
    while(got<howmuch)
    {
        rx=recv(mySocket,(char *)where,howmuch-got,0);
        if(rx<0)
        {
          perror("RxData");
          return 0;
        }
        where+=rx;
        got+=rx;
    }
  return 1;
}
/**
    \fn txData
*/
bool avsNet::txData(uint32_t howmuch, uint8_t *where)
{
uint32_t got=0,tx;
    while(got<howmuch)
    {
        tx=send(mySocket,(char *)where,howmuch-got,0);
         if(tx<0)
        {
          perror("TxData");
          return 0;
        }
        where+=tx;
        got+=tx;
    }
  return 1;
}
/**
    \fn receiveData
*/
bool avsNet::receiveData(uint32_t *cmd, uint32_t *frame,uint32_t *payload_size,uint8_t *payload)
{
        SktHeader header;
        memset(&header,0,sizeof(header));
        int rx;


        rx=rxData(sizeof(header),(uint8_t *)&header);
       
        *cmd=header.cmd;
        *payload_size=header.payloadLen;
        *frame=header.frame;
        if(header.magic!=(uint32_t)MAGGIC)
        {
            printf("[avsProxy]Wrong magic %x/%x\n",header.magic,MAGGIC);
            return 0;
        }
        if(header.payloadLen)
        {
            int togo=header.payloadLen;
            return rxData(togo,payload);
        }
        return 1;
}

/**
    \fn sendData
*/
bool avsNet::sendData(uint32_t cmd,uint32_t frame, uint32_t payload_size,uint8_t *payload)
{
        SktHeader header;
        memset(&header,0,sizeof(header));

        header.cmd=cmd;
        header.payloadLen=payload_size;
        header.frame=frame;
        header.magic=(uint32_t)MAGGIC;
        if(!txData(sizeof(header),(uint8_t *)&header))
        {
            printf("Error in senddata: header %d\n",(int)sizeof(header));
            return 0;
        }
        int togo=payload_size;
        int chunk;
        return txData(togo,payload);
}
avsNet::avsNet()
{
    mySocket=0;
}
avsNet::~avsNet()
{
    close();
}
//EOF
