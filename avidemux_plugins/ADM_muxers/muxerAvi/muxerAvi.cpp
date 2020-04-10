/**********************************************************************
            \file            muxerAvi
            \brief           Avi openDML muxer
                             -------------------
    TODO: Fill in drops/holes in audio as for video
    copyright            : (C) 2008/2012 by mean
    email                : fixounet@free.fr

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ADM_default.h"
#include "fourcc.h"
#include "muxerAvi.h"
#include "ADM_vidMisc.h"
#include "ADM_codecType.h"


avi_muxer muxerConfig={AVI_MUXER_AUTO};

#if 1
#define aprintf(...) {}
#else
#define aprintf printf
#endif


/**
    \fn     muxerAVI
    \brief  Constructor
*/
muxerAvi::muxerAvi()
{
    audioPackets=NULL;
    videoBuffer=NULL;
    clocks=NULL;
    firstPacketOffset=0;
};
/**
    \fn     muxerAVI
    \brief  Destructor
*/

muxerAvi::~muxerAvi()
{
    printf("[AviMuxer] Destructing\n");
    if(clocks)
    {
        for(int i=0;i<nbAStreams;i++)
            delete clocks[i];
        delete [] clocks;
        clocks=NULL;
    }
}

/**
    \fn open
    \brief Check that the streams are ok, initialize context...
*/

bool muxerAvi::open(const char *file, ADM_videoStream *s,uint32_t nbAudioTrack,ADM_audioStream **a)
{
       uint32_t fcc=s->getFCC();
       if(isH264Compatible(fcc) || isH265Compatible(fcc))
       {
           if(!GUI_YesNo(QT_TRANSLATE_NOOP("avimuxer","Bad Idea"),QT_TRANSLATE_NOOP("avimuxer","Using H264/H265 in AVI is a bad idea, MKV is better for that.\n Do you want to continue anyway ?")))
               return false;
       }
        audioDelay=s->getVideoDelay();
        if(!writter.saveBegin (
             file,
		     s,
             nbAudioTrack,
             a))
        {
            GUI_Error_HIG(QT_TRANSLATE_NOOP("avimuxer","Error"),QT_TRANSLATE_NOOP("avimuxer","Cannot create AVI file"));
            return false;

        }
        vStream=s;
        nbAStreams=nbAudioTrack;
        aStreams=a;
        clocks=new audioClock*[nbAStreams];
        for(int i=0;i<nbAStreams;i++)
            clocks[i]=new audioClock(a[i]->getInfo()->frequency);
        
        return true;
}
/**
        \fn fillAudio
        \brief Put audio datas until targetDts is reached
*/
bool muxerAvi::fillAudio(uint64_t targetDts)
{
// Now send audio until they all have DTS > lastVideoDts+increment
            for(int audioIndex=0;audioIndex<nbAStreams;audioIndex++)
            {
                ADM_audioStream*a=aStreams[audioIndex];
                WAVHeader *info=a->getInfo();
                if(!info) // no more track
                    continue;
                uint32_t fq=info->frequency;
                int nb=0;
                audioClock *clk=clocks[audioIndex];
                aviAudioPacket *aPacket=audioPackets+audioIndex;
                if(true==aPacket->eos) return true;
                while(1)
                {
                    if(false==aPacket->present)
                    {
                        if(!a->getPacket(aPacket->buffer,
                                         &(aPacket->sizeInBytes),
                                         AUDIO_BUFFER_SIZE,
                                         &(aPacket->nbSamples),
                                         &(aPacket->dts)))
                        {
                                ADM_warning("Cannot get audio packet for stream %d\n",audioIndex);
                                aPacket->eos=true;
                                break;
                        }
                            if(aPacket->dts!=ADM_NO_PTS) 
                            {
                                aPacket->dts+=audioDelay;
                                aPacket->dts-=firstPacketOffset;
                            }
                            aprintf("[Audio] Packet size %" PRIu32" sample:%" PRIu32" dts:%" PRIu64" target :%" PRIu64"\n",
                                            aPacket->sizeInBytes,aPacket->nbSamples,aPacket->dts,targetDts);
                            if(aPacket->dts!=ADM_NO_PTS)
                                if( labs((long int)aPacket->dts-(long int)clk->getTimeUs())>32000)
                                {
                                    ADM_warning("[AviMuxer] Audio skew!\n");
                                    clk->setTimeUs(aPacket->dts);
//#warning FIXME add padding
                                }
                            aPacket->present=true;
                    }
                    // We now have a packet stored
                    aprintf("Audio packet dts =%s\n",ADM_us2plain(aPacket->dts));
                    if(aPacket->dts!=ADM_NO_PTS)
                        if(aPacket->dts>targetDts) 
                        {
                            aprintf("In the future..\n");
                            break; // this one is in the future
                        }
                    nb=writter.saveAudioFrame(audioIndex,aPacket->sizeInBytes,aPacket->buffer) ;
                    encoding->pushAudioFrame(aPacket->sizeInBytes);
                    aprintf("writting audio packet\n");
                    clk->advanceBySample(aPacket->nbSamples);
                    aPacket->present=false;
                    //printf("%u vs %u\n",audioDts/1000,(lastVideoDts+videoIncrement)/1000);
                }
            }

            return true;
}
/**
 * 
 * @param in
 * @param off
 */
static void rescaleVideo(ADMBitstream *in,uint64_t off)
{
      if(in->dts!=ADM_NO_PTS) in->dts-=off;
      if(in->pts!=ADM_NO_PTS) in->pts-=off;
}
/**
 * \fn prefill
 *  \brief load first audio & video packets to get the 1st packet offset
 *        Needed to avoid adding fillers for both audio & video at the beginning
 * @return 
 */
bool muxerAvi::prefill(ADMBitstream *in)
{
    
    uint64_t dts=0;
    if(false==vStream->getPacket(in)) 
    {
        ADM_error("Cannot get first video frame\n");
        return false;
    }
    dts=in->dts;
    for(int audioIndex=0;audioIndex<nbAStreams;audioIndex++)
    {
                ADM_audioStream*a=aStreams[audioIndex];
                audioClock *clk=clocks[audioIndex];
                aviAudioPacket *aPacket=audioPackets+audioIndex;
                if(!a->getPacket(aPacket->buffer,
                                 &(aPacket->sizeInBytes),
                                 AUDIO_BUFFER_SIZE,
                                 &(aPacket->nbSamples),
                                 &(aPacket->dts)))
                {
                        ADM_warning("Cannot get audio packet for stream %d\n",audioIndex);
                        aPacket->eos=true;
                        aPacket->present=false;
                        continue;
                }
                aPacket->present=true;
                if(aPacket->dts!=ADM_NO_PTS) 
                {
                        aPacket->dts+=audioDelay;
                }
                if(dts==ADM_NO_PTS) dts=aPacket->dts;
                if(aPacket->dts!=ADM_NO_PTS && dts!=ADM_NO_PTS)
                        if(aPacket->dts<dts) dts=aPacket->dts;
    }
    ADM_info("Min 1st packet time :%s\n",ADM_us2plain(dts));
    if(dts!=ADM_NO_PTS) firstPacketOffset=dts;
    
    rescaleVideo(in,firstPacketOffset);
    for(int audioIndex=0;audioIndex<nbAStreams;audioIndex++)
    {
         aviAudioPacket *aPacket=audioPackets+audioIndex;
         if(!aPacket->present) continue;
         if(aPacket->dts!=ADM_NO_PTS) aPacket->dts-=firstPacketOffset;
    }
    return true;
}
/**
    \fn save
*/
bool muxerAvi::save(void)
{
    printf("[AviMuxer] Saving\n");
    uint32_t bufSize=vStream->getWidth()*vStream->getHeight()*3;
    bool result=true;
    bool keepGoing=true;
   
    uint64_t rawDts;
    uint64_t lastVideoDts=0;
    int ret;
    int written=0;
   
    audioPackets=new aviAudioPacket[nbAStreams];
    videoBuffer=new uint8_t[bufSize];

    ADM_info("[AviMuxer]avg fps=%u\n",vStream->getAvgFps1000());
    ADMBitstream in(bufSize);
    in.data=videoBuffer;
    uint64_t aviTime=0;
    
    if(in.dts==ADM_NO_PTS) in.dts=0;
    lastVideoDts=in.dts;

    initUI("Saving Avi");
    encoding->setContainer("AVI/OpenDML");
    
    if(false==prefill(&in)) goto abt;
    while(keepGoing)
    {
            aprintf("Current clock=%s\n",ADM_us2plain(aviTime));
            aprintf("Video in dts=%s\n",ADM_us2plain(in.dts));
            if(in.dts>aviTime+videoIncrement)
            {
                aprintf("Too far in the future, writting dummy frame\n");
                writter.saveVideoFrame( 0, 0,videoBuffer); // Insert dummy video frame
                encoding->pushVideoFrame(0,0,in.dts);
            }else
            {
                aprintf("Writting video frame\n");
                if(!writter.saveVideoFrame( in.len, in.flags,videoBuffer))  // Put our real video
                {
                        ADM_warning("[AviMuxer] Error writting video frame\n");
                        result=false;
                        goto abt;
                }
                encoding->pushVideoFrame(in.len,in.out_quantizer,in.dts);
                if(false==vStream->getPacket(&in)) goto abt;
                if(in.dts==ADM_NO_PTS)
                {
                    in.dts=lastVideoDts+videoIncrement;
                }else
                    rescaleVideo(&in,firstPacketOffset);
                lastVideoDts=in.dts;
            }

            fillAudio(aviTime+videoIncrement);    // and matching audio

            if(updateUI()==false)
            {  
                result=false;
                keepGoing=false;
            }
           
            written++;
            aviTime+=videoIncrement;
    }
abt:
    closeUI();
    writter.setEnd();
    delete [] videoBuffer;
    videoBuffer=NULL;
    delete [] audioPackets;
    audioPackets=NULL;
    ADM_info("[AviMuxer] Wrote %d frames, nb audio streams %d\n",written,nbAStreams);
    return result;
}
/**
    \fn close
    \brief Cleanup is done in the dtor
*/
bool muxerAvi::close(void)
{

    ADM_info("[AviMuxer] Closing\n");
    return true;
}
//EOF



