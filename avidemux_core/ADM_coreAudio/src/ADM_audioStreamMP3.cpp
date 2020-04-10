/**
    \file ADM_audioStream
    \brief Base class

(C) Mean 2008
GPL-v2
*/
#include "ADM_default.h"
#include "ADM_audioStreamMP3.h"
#include "ADM_mp3info.h"
#include "DIA_working.h"
#include "ADM_clock.h"
#include "ADM_vidMisc.h"

#if 1 
#define aprintf(...) {}
#else
#define aprintf printf
#endif
/**
    \fn ADM_audioStreamMP3
    \brief constructor
*/
ADM_audioStreamMP3::ADM_audioStreamMP3(WAVHeader *header,ADM_audioAccess *access,bool createMap) : ADM_audioStreamBuffered(header,access)
{
    // Suppress repeated debug messages
    _msg_counter=0;
    _msg_ratelimit=new ADMCountdown(200);
    _msg_ratelimit->reset();
    // If hinted..., compute the duration ourselves
    if(access->isCBR()==true && access->canSeekOffset()==true)
    {
        // We can compute the duration from the length
        float size=access->getLength();
              size/=header->byterate; // Result is in second
              size*=1000;
              size*=1000; // s->us
              durationInUs=(uint64_t)size;
              return;
    }
    // and built vbr map if needed
    // The 2 conditions below means there is no gap i.e. avi style stream
    // else not needed
    if( access->canSeekTime()==false)
    {
        ADM_assert(access->canSeekOffset()==true);
        if(true==createMap)
        {
            buildTimeMap();
            uint32_t nb=seekPoints.size();
            if(nb)
                durationInUs=seekPoints[nb-1]->timeStamp;
            return;
        }
    }
    // Time based
    durationInUs=access->getDurationInUs();
}

/**
    \fn ADM_audioStream
    \brief destructor
*/
ADM_audioStreamMP3::~ADM_audioStreamMP3()
{
    // Delete our map if needed...
   for(int i=0;i<seekPoints.size();i++)
   {
        delete seekPoints[i];
        seekPoints[i]=NULL;
    }
    if(_msg_ratelimit)
        delete _msg_ratelimit;
    _msg_ratelimit = NULL;
}
/**
    \fn goToTime
    \brief goToTime
*/
bool         ADM_audioStreamMP3::goToTime(uint64_t nbUs)
{
    if(access->canSeekTime()==true)
    {
        if( access->goToTime(nbUs)==true)
        {
           setDts(nbUs);
           limit=start=0;
           refill();
           return 1;
        }
        return 1;
    }
    // If CBR we can use the default way
    if(access->isCBR()==true)
        return ADM_audioStream::goToTime(nbUs);
    // if VBR use our time map
    if(!seekPoints.size())
    {
        ADM_error("VBR MP2/MP3 stream with no time map, cannot seek\n");
        return false;
    }
    if(nbUs<=seekPoints[0]->timeStamp) // too early
    {
            start=limit=0;
            access->setPos(0);
            setDts(0);
            return true;
    }
    // Search the switching point..
    for(int i=0;i<seekPoints.size()-1;i++)
    {
        //printf("[%d]Target %u * %u * %u *\n",i,nbUs,seekPoints[i]->timeStamp,seekPoints[i+1]->timeStamp);
        if(seekPoints[i]->timeStamp<=nbUs && seekPoints[i+1]->timeStamp>=nbUs)
        {
            start=limit=0;
            access->setPos(seekPoints[i]->offset);
            setDts(seekPoints[i]->timeStamp);
            ADM_info("MP3 : Time map : Seek request for %s\n",ADM_us2plain(nbUs));
            ADM_info("MP3 : Sync found at %s\n",ADM_us2plain(seekPoints[i]->timeStamp));
            return true;
        }
    }
    ADM_error("VBR MP2/MP3 request for time outside of time map, cannot seek\n");
    return false;
}
/**
        \fn getPacket
*/
uint8_t ADM_audioStreamMP3::getPacket(uint8_t *buffer,uint32_t *size, uint32_t sizeMax,uint32_t *nbSample,uint64_t *dts)
{
#define ADM_LOOK_AHEAD 4 // Need 4 bytes...
uint8_t data[ADM_LOOK_AHEAD];
MpegAudioInfo info;
uint32_t offset;
int nbSyncBytes=0;
    while(1)
    {
        // Do we have enough ? Refill if needed ?
        if(needBytes(ADM_LOOK_AHEAD)==false) 
        {
            if(_msg_ratelimit->done())
            {
                if(_msg_counter)
                {
                    ADM_warning("MP3: Not enough data to lookup header (message repeated %" PRIu32" times)\n",_msg_counter);
                    _msg_counter=0;
                }else
                {
                    ADM_warning("MP3: Not enough data to lookup header\n");
                }
                _msg_ratelimit->reset();
            }else
            {
                _msg_counter++;
            }
            return 0;
        }
        // Peek
        peek(ADM_LOOK_AHEAD,data);
        if(getMpegFrameInfo(data,ADM_LOOK_AHEAD, &info,NULL,&offset))
        {
            ADM_assert(info.size<=sizeMax);
            if(needBytes(info.size)==true)
            {
                *size=info.size;
                read(*size,buffer);
                *nbSample=info.samples;
                //if(info.samples!=1152) ADM_assert(0);
                *dts=lastDts;
            //    printf("MP3 DTS =%"PRId64" ->",*dts);
                advanceDtsBySample(*nbSample);
                //printf("%"PRId64" , size=%d\n",*dts,*size);
                if(nbSyncBytes)
                        ADM_info("[MP3 Stream] Sync found after %d bytes...\n",nbSyncBytes);
                _msg_counter=0;
                return 1;
            }
            
        }
        //discard one byte
        nbSyncBytes++;
        
        read8();
    }
}
 /**
      \fn buildTimeMap
     \brief compute a map between time<->Offset. It is only used for stream in Offset mode with no gap (avi like)
            In that case, the incoming dts is irrelevant.
            We may have up to one packet error
  
  */
#define TIME_BETWEEN_UPDATE 1500
#define SAVE_EVERY_N_BLOCKS 3    // One seek point every ~ 60 ms
bool ADM_audioStreamMP3::buildTimeMap(void)
{
uint32_t size;
uint64_t newDts,pos;
DIA_workingBase *work=createWorking(QT_TRANSLATE_NOOP("adm","Building time map"));
    
    ADM_assert(access->canSeekOffset()==true);
    access->setPos(0);
    ADM_info("Starting MP3 time map\n");
    rewind();
    Clock *clk=new Clock();
    clk->reset();
    uint32_t nextUpdate=clk->getElapsedMS()+TIME_BETWEEN_UPDATE;
    int markCounter=SAVE_EVERY_N_BLOCKS;
    while(1)
    {
        // Push where we are...
        
        if(markCounter>SAVE_EVERY_N_BLOCKS)
        {
            MP3_seekPoint *seek=new MP3_seekPoint;
            seek->offset=access->getPos();
            seek->timeStamp=lastDts;
            // Mark this point
            seekPoints.append(seek);
            markCounter=0;
        }
        // Shrink ?
        if(limit>ADM_AUDIOSTREAM_BUFFER_SIZE && start> 10*1024)
        {
            memmove(buffer.at(0), buffer.at(start),limit-start);
            limit-=start;
            start=0;
        }

        if(false==access->getPacket(buffer.at(limit), &size, 2*ADM_AUDIOSTREAM_BUFFER_SIZE-limit,&newDts))
        {
            aprintf("Get packet failed\n");
            break;
        }
        aprintf("Got MP3 packet : size =%d\n",(int)size);
        limit+=size;
        // Start at...
        pos=access->getPos();
        uint32_t now=clk->getElapsedMS();
        if(now>nextUpdate)
        {
            work->update(pos,access->getLength());
            nextUpdate=now+TIME_BETWEEN_UPDATE;
        }
        
        // consume all packets in the buffer we just got
        MpegAudioInfo info;
        uint32_t offset;

        while(1)
        {
            if(limit-start<ADM_LOOK_AHEAD) break;
            if(!getMpegFrameInfo(buffer.at(start),ADM_LOOK_AHEAD, &info,NULL,&offset))
            {
                start++;
                continue;
            }
            // Enough bytes ?
            if(limit-start>=info.size)
            {
                start+=info.size;
                advanceDtsBySample(info.samples);
                markCounter++;
                continue;
            }
            break;
        }

    }
    rewind();
    delete work;
    delete clk;
    access->setPos(0);
    ADM_info("Finishing MP3 time map\n");
    return true;
}
  
  // EOF

