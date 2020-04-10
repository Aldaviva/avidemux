/**
    \file dmxPSPacket
    \brief Packet demuxer for mpeg PS
    copyright            : (C) 2005-2008 by mean
    email                : fixounet@free.fr
        
 ***************************************************************************/
/**
    Stream remapping 

            00--   : AC3 audio
            20--   : Subtitles
            40--   : DTS
            60--   : VOBU
            80--   : 
            A0--   : PCM
            C0/E0..: Mpeg2 audio/Mpeg2 video/


*/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "ADM_default.h"

#include "dmxPSPacket.h"
#include "dmx_mpegstartcode.h"

/**
    \fn psPacket
    \brief ctor
*/
psPacket::psPacket(void) 
{

}
/**
    \fn psPacket
    \brief dtor
*/
psPacket::~psPacket()
{
    close();
}
/**
    \fn open
    \brief dtor
*/
bool psPacket::open(const char *filenames,FP_TYPE append)
{
    _file=new fileParser();
    if(!_file->open(filenames,&append))
    {
        printf("[DmxPS] cannot open %s\n",filenames);
        delete _file;
        _file=NULL;
        return false;
    }
    _size=_file->getSize();
    return true;
}
/**
    \fn close
    \brief dtor
*/
bool psPacket::close(void)
{
    if(_file)
    {
        delete _file;
        _file=NULL;
    }
    return true;
}
/**
    \fn getPos
*/
uint64_t    psPacket::getPos(void)
{
    return 0;
}
/**
    \fn setPos
*/

bool    psPacket::setPos(uint64_t pos)
{
    if(!_file->setpos(pos))
    {
        printf("[psPacket] Cannot seek to %" PRIx64"\n", pos);
        return false;
    }
    return true;
}

/**
    \fn getPacket
*/      
bool        psPacket::getPacket(uint32_t maxSize, uint8_t *pid, uint32_t *packetSize,uint64_t *opts,uint64_t *odts,uint8_t *buffer,uint64_t *startAt)
{
uint32_t globstream,len;
uint8_t  stream,substream;
uint64_t pts,dts;
        // Resync on our stream
_again2:
        *pid=0;
        if(!_file->sync(&stream)) 
        {
                uint64_t pos;
                _file->getpos(&pos);
                printf("[DmxPS] cannot sync  at %" PRIu64"/%" PRIu64"\n",pos,_size);
                return false;
        }
// Position of this packet just before startcode
        _file->getpos(startAt);
        *startAt-=4;
// Handle out of band stuff        
        if(stream==PACK_START_CODE) 
        {
        		_file->forward(8);
        		goto _again2;
        }
        if( stream==PADDING_CODE ||stream==SYSTEM_START_CODE) 
        {
                        len=_file->read16i();
                        //printf("\tForwarding %lu bytes\n",len);
        		_file->forward(len);
        		goto _again2;
        }
        // Only keep relevant parts
        // i.e. a/v : C0 C9 E0 E9
        // subs 20-29
        // private data 1/2
#define INSIDE(min,max) (stream>=min && stream<max)
        if(!(  INSIDE(0xC0,0xC9) || INSIDE(0xE0,0xE9) || INSIDE(0x20,0x29) || 
                    stream==PRIVATE_STREAM_1 || stream==PRIVATE_STREAM_2
        			)) goto _again2;
        // Ok we got a candidate
        if(!getPacketInfo(stream,&substream,&len,&pts,&dts))   
        {
                goto _again2;
        }
        
        //printf("Main Stream :%x substream :%x\n",stream,substream);
        switch(stream)
        {
                case PRIVATE_STREAM_1: globstream=0xFF00+substream; break;
                case PRIVATE_STREAM_2: globstream=0xF000+substream; break;
                default:               globstream=stream;break;
                        
        }
        *pid=globstream; // Keep only LSB 8 BITS
        *opts=pts;
        *odts=dts;
        *packetSize=len;
        if(len>     maxSize)
        {
                printf("[DmxPS] Packet too big %d vs %d\n",len,maxSize);
                goto _again2;
        }
        if(!_file->read32(len,buffer)) return false;
        return true;
       
}
/**

    \fn getPacketInfo
    \brief       Retrieve info about the packet we just met.It is assumed that parser is just after the packet startcode

*/

uint8_t psPacket::getPacketInfo(uint8_t stream,uint8_t *substream,uint32_t *olen,uint64_t *opts,uint64_t *odts)
{

//uint32_t un ,deux;
uint64_t size=0;
uint8_t c,d;
uint8_t align=0;
                        
                *substream=0xff;
                *opts=ADM_NO_PTS;
                *odts=ADM_NO_PTS;
                
                                        
                size=_file->read16i();
                if((stream==PADDING_CODE) || 
                	 (stream==PRIVATE_STREAM_2)
                        ||(stream==SYSTEM_START_CODE) //?
                        ) // special case, no header
                        {
                                
                                *olen=size;      
                                if(PRIVATE_STREAM_2==stream)
                                {
                                    *substream=_file->read8i()+0x60;
                                    (*olen)--;
                                }
                                return 1;
                        }
                                
                        //      remove padding if any                                           
        
                while((c=_file->read8i()) == 0xff) 
                {
                        size--;
                }
//----------------------------------------------------------------------------
//-------------------------------MPEG-2 PES packet style----------------------
//----------------------------------------------------------------------------
                if(((c&0xC0)==0x80))
                {
                        uint32_t ptsdts,len;
                        //printf("\n mpeg2 type \n");
                        //_muxTypeMpeg2=1;
                        // c= copyright and stuff       
                        //printf(" %x align\n",c);      
                        if(c & 4) align=1;      
                        c=_file->read8i();     // PTS/DTS
                        //printf("%x ptsdts\n",c
                        ptsdts=c>>6;
                        // header len
                        len=_file->read8i();
                        size-=3;  

                        switch(ptsdts)
                        {
                                case 2: // PTS=1 DTS=0
                                        if(len>=5)
                                        {
                                                uint64_t pts1,pts2,pts0;
                                                //      printf("\n PTS10\n");
                                                        pts0=_file->read8i();  
                                                        pts1=_file->read16i(); 
                                                        pts2=_file->read16i();                 
                                                        len-=5;
                                                        size-=5;
                                                        *opts=(pts1>>1)<<15;
                                                        *opts+=pts2>>1;
                                                        *opts+=(((pts0&6)>>1)<<30);
                                        }
                                        break;
                                case 3: // PTS=1 DTS=1
                                                #define PTS11_ADV 10 // nut monkey
                                                if(len>=PTS11_ADV)
                                                {
                                                        uint32_t skip=PTS11_ADV;
                                                        uint64_t pts1,pts2,dts,pts0;
                                                                //      printf("\n PTS10\n");
                                                                pts0=_file->read8i();  
                                                                pts1=_file->read16i(); 
                                                                pts2=_file->read16i(); 
                                                                                        
                                                                *opts=(pts1>>1)<<15;
                                                                *opts+=pts2>>1;
                                                                *opts+=(((pts0&6)>>1)<<30);
                                                                pts0=_file->read8i();  
                                                                pts1=_file->read16i(); 
                                                                pts2=_file->read16i();                 
                                                                dts=(pts1>>1)<<15;
                                                                dts+=pts2>>1;
                                                                dts+=(((pts0&6)>>1)<<30);
                                                                len-=skip;
                                                                size-=skip;
                                                                *odts=dts;
                                                                        //printf("DTS: %lx\n",dts);                
                                                   }
                                                   break;               
                                case 1:
                                                return 0;//ADM_assert(0); // forbidden !
                                                break;
                                case 0: 
                                                // printf("\n PTS00\n");
                                                break; // no pts nor dts
                                                                                
                                                            
                        }  
// Extension bit        
// >stealthdave<                                

                        // Skip remaining headers if any
                        if(len) 
                        {
                                _file->forward(len);
                                size=size-len;
                        }
                                
                if(stream==PRIVATE_STREAM_1)
                {
                        if(size>5)
                        {
                        // read sub id
                               *substream=_file->read8i();
  //                    printf("\n Subid : %x",*subid);
                                switch(*substream)
                                {
                                // DTS
                                        case 0x88:case 0x89:case 0x8A:case 0x8B:
                                        
                                                *substream=*substream-0x48;
                                                break;

                                //AC3
                                        case 0x80:case 0x81:case 0x82:case 0x83:
                                        case 0x84:case 0x85:case 0x86:case 0x87:
                                                *substream=*substream-0x80;
                                                break;
                                // PCM
                                        case 0xA0:case 0xA1:case 0xa2:case 0xa3:
                                        case 0xA4:case 0xA5:case 0xa6:case 0xa7:
                                                // we have an additionnal header
                                                // of 3 bytes
                                                _file->forward(3);
                                                size-=3;
                                                break;
                                // Subs
                                case 0x20:case 0x21:case 0x22:case 0x23:
                                case 0x24:case 0x25:case 0x26:case 0x27:
                                                break;
                             
                                default:
                                                doNoComplainAnyMore++;
                                                if(doNoComplainAnyMore<10)
                                                    printf("[DmxPS]Unkown substream %x\n",*substream);
                                                *substream=0xff;
                                }
                                // skip audio header (if not sub)
                                if(*substream>0x26 || *substream<0x20)
                                {
                                        _file->forward(3);
                                        size-=3;
                                }
                                size--;
                        }
                }
               //    printf(" pid %x size : %x len %x\n",sid,size,len);
                *olen=size;
                return 1;
        }
//----------------------------------------------------------------------------------------------                
//-------------------------------MPEG-1 PES packet style----------------------                                  
//----------------------------------------------------------------------------------------------                                        
           if(0) //_muxTypeMpeg2)
                {
                        printf("[DmxPS]*** packet type 1 inside type 2 ?????*****\n");
                        return 0; // mmmm                       
                }
          // now look at  STD buffer size if present
          // 01xxxxxxxxx
          if ((c>>6) == 1) 
          {       // 01
                        size-=2;
                        _file->read8i();                       // skip one byte
                        c=_file->read8i();   // then another
           }                       
           // PTS/DTS
           switch(c>>4)
           {
                case 2:
                {
                        // 0010 xxxx PTS only
                        uint64_t pts1,pts2,pts0;
                                        size -= 4;
                                        pts0=(c>>1) &7;
                                        pts1=_file->read16i()>>1;
                                        pts2=_file->read16i()>>1;
                                        *opts=pts2+(pts1<<15)+(pts0<<30);
                                        break;
                  }
                  case 3:
                  {               // 0011 xxxx
                        uint64_t pts1,pts2,pts0;
                                        size -= 9;
                                                                        
                                        pts0=(c>>1) &7;
                                        pts1=_file->read16i()>>1;
                                        pts2=_file->read16i()>>1;
                                        *opts=pts2+(pts1<<15)+(pts0<<30);
                                        _file->forward(5);
                   }                                                               
                   break;
                   
                case 1:
                        // 0001 xxx             
                        // PTSDTS=01 not allowed                        
                                return 0;
                                break; 
                }
                                                                

                if(!align)      
                        size--;         
        *olen=size;
        return 1;
}
//************************************************************************************

/**
    \fn psPacket
*/
psPacketLinear::psPacketLinear(uint8_t pid) : psPacket()
{
    oldStartAt=startAt=0xfffffff;
    oldBufferLen=bufferLen=0;
    bufferIndex=0;
    myPid=pid;
    eof=false;
}
/**
    \fn ~psPacket
*/
psPacketLinear::~psPacketLinear() 
{
}
/**
    \fn refill
*/
bool psPacketLinear::refill(void) 
{
// In case a startcode spawns across 2 packets
// we have to keep track of the old one
        oldBufferDts=bufferDts;
        oldBufferPts=bufferPts;
        oldStartAt=startAt;
        oldBufferLen=bufferLen;
        if( false== getPacketOfType(myPid,ADM_PACKET_LINEAR, &bufferLen,&bufferPts,&bufferDts,buffer,&startAt)) 
        {
            printf("[PsPacketLinear] Refill failed for pid :%x\n",myPid);
            bufferIndex=bufferLen=0;
            return false;
        }
        //printf("Refill : At :%"PRIx64" size :%"PRIi32"\n",startAt,bufferLen);
        bufferIndex=0;
        return true;
}
#ifndef PS_PACKET_INLINE
/**
    \fn readi8
*/
uint8_t psPacketLinear::readi8(void)
{
    consumed++;
    if(bufferIndex<bufferLen)
    {
        return buffer[bufferIndex++];
    }
    if(false==refill()) 
    {
        eof=1;
        return 0;
    }
    ADM_assert(bufferLen);
    bufferIndex=1;
    return buffer[0];
    
}
/**
    \fn readi16
*/
uint16_t psPacketLinear::readi16(void)
{
    if(bufferIndex+1<bufferLen)
    {
        uint16_t v=(buffer[bufferIndex]<<8)+buffer[bufferIndex+1];;
        bufferIndex+=2;
        consumed+=2;
        return v;
    }
    return (readi8()<<8)+readi8();
}
/**
    \fn readi32
*/
uint32_t psPacketLinear::readi32(void)
{
    if(bufferIndex+3<bufferLen)
    {
        uint8_t *p=buffer+bufferIndex;
        uint32_t v=(p[0]<<24)+(p[1]<<16)+(p[2]<<8)+p[3];
        bufferIndex+=4;
        consumed+=4;
        return v;
    }
    return (readi16()<<16)+readi16();
}
#endif
/**
    \fn forward
*/
bool psPacketLinear::forward(uint32_t v)
{
next:
 uint32_t delta=bufferLen-bufferIndex;
    if(v>100*1000)
    {
        ADM_assert(0);
    }
    if(v<=delta)
    {
        bufferIndex+=v;
        consumed+=v;
        return true;
    }
    // v>delta
    v-=delta;
    consumed+=delta;
    if(!refill()) return false;
    goto next;
}

/**
    \fn bool    read(uint32_t len, uint8_t *buffer);
    \brief
*/
bool    psPacketLinear::read(uint32_t len, uint8_t *out)
{
    // Enough already ?
    while(len)
    {
        uint32_t avail=bufferLen-bufferIndex;
        uint32_t chunk=avail;
        if(chunk>len) chunk=len;
#if 0
        printf("len:%ld avail:%ld chunk %ld index:%d size:%d\n",
                len,avail,chunk,bufferIndex,bufferLen);
#endif
        memcpy(out,buffer+bufferIndex,chunk);
        bufferIndex+=chunk;
        len-=chunk;
        out+=chunk;
        consumed+=chunk;
        if(bufferIndex==bufferLen)
        {
            //printf("Refill\n");
            if(false==refill()) return false;
        }
    }
    return true;
}
/**
        \fn getInfo
        \brief Returns info about the current (or previous if it spawns) packet.
            It is expected that the caller will do -4 to the index to get the start of the 
            startCode
*/
bool    psPacketLinear::getInfo(dmxPacketInfo *info)
{
    if(bufferIndex<4)
    {
        info->startAt=this->oldStartAt;
        info->offset=oldBufferLen+bufferIndex;
        info->pts=oldBufferPts;
        info->dts=oldBufferDts;

    }else
    {
        info->startAt=this->startAt;
        info->offset=bufferIndex;
        info->pts=bufferPts;
        info->dts=bufferDts;
    }
    return true;

};
/**
    \fn seek
    \brief Async jump
*/
bool    psPacketLinear::seek(uint64_t packetStart, uint32_t offset)
{
    if(!_file->setpos(packetStart))
    {
        printf("[psPacket] Cannot seek to %" PRIx64"\n",packetStart);
        return 0;
    }
    if(!refill())
    {
        printf("[PsPacketLinear] Seek to %" PRIx64":%" PRIx32" failed\n",packetStart,offset);
        return false;
    }
    ADM_assert(offset<bufferLen);
    bufferIndex=offset;
    
    return true;
}
/**
    \fn getConsumed
    \brief returns the # of bytes consumed since the last call
*/
uint32_t psPacketLinear::getConsumed(void)
{
    uint32_t c=consumed;
    consumed=0;
    return c;
}
/**
    \fn changePid
    \brief change the pid of the stream we read (used when probing all tracks)
*/
bool    psPacketLinear::changePid(uint32_t pid) 
{
    myPid=(pid&0xff);
    bufferLen=bufferIndex=0;
    return true;
}
/* ********************************************************* */
/**
    \fn psPacketLinearTracker
*/
 psPacketLinearTracker::psPacketLinearTracker(uint8_t pid)  : psPacketLinear(pid)
{
    resetStats();
    lastVobuEnd=0;
    nextVobuEnd=0;
    nextVobuPosition=0;
    lastVobuPosition=0;
}
/**
    \fn ~psPacketLinearTracker
*/
psPacketLinearTracker::~psPacketLinearTracker()
{

    
}
/**
        \fn getStat
*/
packetStats    *psPacketLinearTracker::getStat(int index)
{   
    if(index<0 || index>=256) ADM_assert(0);
    return stats+index;
}
/**
    \fn decodeVobuPCI
*/
bool psPacketLinearTracker::decodeVobuPCI(uint32_t size,uint8_t *data)
{
/*
	nv_pck_lbn 	: 32;  // block length
	vobu_cat 	: 16;  // vobu category
	reserved 	: 16;
	vobu_uop_ctl 	: 32;  // user operation
	vobu_s_ptm	: 32;  // vobu start time in 90KHz system clock unit
	vobu_e_ptm	: 32;  // vobu end time
	vobu_sl_e_ptm	: 32;  // sequence end termiation time
	e_eltm		: 32;  // cell elapse time
	vobu_isrc[32]	: 32 * 8;
*/
    if(size!=0x3D3)
        {
            ADM_warning("PCI Data not 0x3D4 but 0x%x\n",size+1);
            return false;
        }
        uint8_t *ptr=data;
        ptr+=4+2+2+4;
#define MK32(x) {x=(ptr[0]<<24)+(ptr[1]<<16)+(ptr[2]<<8)+ptr[3];ptr+=4;}
        uint32_t start;
        uint32_t end;
        uint32_t seqEnd;
        MK32(start);
        MK32(end);
        MK32(seqEnd);
        
        lastVobuPosition=nextVobuPosition;
        lastVobuEnd=nextVobuEnd;

        nextVobuEnd=end;
        _file->getpos(&nextVobuPosition);
#if 0
        ADM_info("At : 0x%"PRIx64", Vobu start : %d end: %d seqEnd:%d\n",nextVobuPosition,start,end,seqEnd);
      
#endif

        _file->forward(size-5*4+2*2);
        return true;
}   
/**
    \fn decodeVobuDSI
*/

bool psPacketLinearTracker::decodeVobuDSI(uint32_t size)
{

        if(size!=0x3f9)
        {
            ADM_warning("DSI Data not 0x3fa but 0x%x\n",size+1);
            return false;
        }
#define P32(x) printf(#x" :%d ",_file->read32i());
#define P16(x) printf(#x" :%d ",_file->read16i());

        P32(pck_scr);
        P32(pck_lbn);
        _file->read32i();//end add
        _file->read32i();// ref1
        _file->read32i();// ref2
        _file->read32i();// ref3
        P16(vobid);
        _file->read8i();
        P16(cellid);
        _file->read8i();
        P32(etm);
        printf("\n");
        return true;

}
/**
    \fn getPacketgetPacketOfType
    \brief Keep track of all the packets we have seen so far.
    Usefull to detect the streams present and to look up the PTS/DTS of audio streams for the audio part of the index
*/
bool           psPacketLinearTracker::getPacketOfType(uint8_t pid,uint32_t maxSize, uint32_t *packetSize,uint64_t *pts,uint64_t *dts,uint8_t *buffer,uint64_t *startAt)
{
    uint8_t tmppid;
    while(1)
    {
        if(true!=getPacket(maxSize,&tmppid,packetSize,pts,dts,buffer,startAt))
                return false;
        else
        {
                // Update 
                ADM_assert(tmppid<0x100);
                packetStats *p=stats+tmppid;
                if(tmppid==0x60) // VOBU PCI
                {
                    decodeVobuPCI(*packetSize,buffer);
                    continue;
                }
                
                uint64_t ts=*pts;
                if(ts==ADM_NO_PTS) ts=*dts;
                if(ts!=ADM_NO_PTS)
                {
                    p->startCount=p->count;
                    p->startAt=*startAt;
                    p->startSize=p->size;
                    p->startDts=ts;
                }
                p->count++;
                p->size+=*packetSize;
                if(tmppid==pid) return true;
        }
    }
    return false;
}
/**
    \fn resetStats
*/
bool           psPacketLinearTracker::resetStats(void)
{
    memset(stats,0,sizeof(stats));
    for(int i=0;i<256;i++)
    {
        packetStats *p=stats+i;
        p->startDts=ADM_NO_PTS;
    }
    return true;
}
/**
    \fn findStartCode
    \brief Must check stillOk after calling this
*/
int psPacketLinearTracker::findStartCode(void)
{
#define likely(x) x
#define unlikely(x) x
        unsigned int last=0xfffff;
        unsigned int cur=0xffff;
        int startCode=0;
        while(this->stillOk())
        {
            last=cur;
            cur=this->readi16();
            if(likely(last&0xff)) continue;
            if(unlikely(!last)) // 00 00 , need 01 xx
            {
                if((cur>>8)==1) 
                {
                        startCode=cur&0xff;
                        break;
                }
            }
            if(unlikely(!(last&0xff))) // xx 00 need 00 01
            {
                if(cur==1)
                {
                        startCode=this->readi8();
                        break;
                }
            }
        }
        return startCode;
}
//EOF
