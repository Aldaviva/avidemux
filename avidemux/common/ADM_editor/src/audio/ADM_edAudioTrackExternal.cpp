/***************************************************************************
    \file  ADM_edAudioTrackExternal
    \brief Manage audio track(s) coming from an external file
    \author (c) 2012 Mean, fixounet@free.Fr 
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <string.h>
#include "ADM_cpp.h"
#include "ADM_default.h"
#include <math.h>

#include "fourcc.h"
#include "ADM_edit.hxx"
#include "ADM_edAudioTrackExternal.h"
#include "ADM_audioIdentify.h"
#include "ADM_audioAccessFile.h"
#include "ADM_audioAccessFileAACADTS.h"
#include "ADM_vidMisc.h"

#ifdef _MSC_VER
#define abs(x) _abs64(x)
#endif

#if 0
#define vprintf ADM_info
#else
#define vprintf(...) {}
#endif
/**
    \fn ctor
*/
ADM_edAudioTrackExternal:: ADM_edAudioTrackExternal(const char *file, WAVHeader *hdr,ADM_audioAccess *cess)
:  ADM_edAudioTrack(ADM_EDAUDIO_EXTERNAL,hdr)
{
    ADM_info("Creating edAudio from external file %s\n",file);
    sourceFile=std::string(file);
    codec=NULL;
    vbr=false;
    duration=0;
    size=0;
    internalAudioStream=NULL;
    internalAccess=cess;
}
/**
    \fn dtor
*/
ADM_edAudioTrackExternal::~ADM_edAudioTrackExternal()
{
    ADM_info("Destroying edAudio from external %s \n",sourceFile.c_str());
    if(codec) delete codec;
    codec=NULL;
    if(internalAudioStream) delete internalAudioStream;
    internalAudioStream=NULL;
    if(internalAccess) delete internalAccess;
    internalAccess=NULL;
    
}
/**
    \fn create
    \brief actually create the track, can fail
*/
bool ADM_edAudioTrackExternal::create(uint32_t extraLen, uint8_t *extraData)
{
    ADM_info("Initializing audio track from external %s \n",sourceFile.c_str());
    codec=getAudioCodec(wavHeader.encoding,&wavHeader,extraLen,extraData);
    if(!codec || codec->isDummy())
    {
        ADM_warning("No decoder for %s.\n",getStrFromAudioCodec(wavHeader.encoding));
        return false;
    }
    // Check AAC for SBR
    if(wavHeader.encoding==WAV_AAC)
    {
        uint32_t inlen,max=ADM_EDITOR_PACKET_BUFFER_SIZE;
        notStackAllocator inbuf(max);
        uint8_t *in=inbuf.data;
        uint64_t dts;
        if(false==internalAccess->getPacket(in,&inlen,max,&dts))
        {
            ADM_warning("Cannot get packets.\n");
            return false;
        }

        notStackAllocator outbuf(wavHeader.frequency*wavHeader.channels*sizeof(float));
        float *out=(float *)outbuf.data;
        uint32_t nbOut,fq=0;
        if(codec->run(in,inlen,out,&nbOut))
            fq=codec->getOutputFrequency();
        if(fq && fq!=wavHeader.frequency)
        {
            ADM_warning("Updating sampling frequency from %u to %u\n",wavHeader.frequency,fq);
            wavHeader.frequency=fq;
        }
    }
    size=internalAccess->getLength();
    internalAudioStream=ADM_audioCreateStream(&wavHeader,internalAccess,true);
    return true;
}
/**
    \fn getChannelMapping
*/
CHANNEL_TYPE * ADM_edAudioTrackExternal::getChannelMapping(void )
{
    return codec->channelMapping;
}
/**
    \fn getOutputFrequency
*/          
uint32_t    ADM_edAudioTrackExternal::getOutputFrequency(void)
{
    return codec->getOutputFrequency();
}
/**
    \fn getOutputChannels
*/
uint32_t ADM_edAudioTrackExternal::getOutputChannels(void)
{
    return codec->getOutputChannels();
}
/**
    \fn refillPacketBuffer
*/
bool             ADM_edAudioTrackExternal::refillPacketBuffer(void)
{
   packetBufferSize=0; 
   uint64_t dts;
 
    if(!internalAudioStream->getPacket(packetBuffer,&packetBufferSize,ADM_EDITOR_PACKET_BUFFER_SIZE,
                        &packetBufferSamples,&dts))
    {           
             return false;
    }
    //
    // Ok we have a packet, rescale audio
    //if(dts==ADM_NO_PTS) packetBufferDts=ADM_NO_PTS;
    packetBufferDts=dts; // Could have a small error here..
    vprintf("Refilling buffer dts=%s\n",ADM_us2plain(packetBufferDts));
    return true;
}


/**
    \fn create_edAudioExternal
*/
ADM_edAudioTrackExternal *create_edAudioExternal(const char *name)
{
    #define EXTERNAL_PROBE_SIZE (1024*1024)
    // Identify file type
    notStackAllocator dummyBuffer(EXTERNAL_PROBE_SIZE);
    uint8_t *buffer=dummyBuffer.data;
    FILE *f=ADM_fopen(name,"rb");
    if(!f)
    {
        ADM_warning("Cannot open %s\n",name);
        return NULL;
    }
    fread(buffer,1,EXTERNAL_PROBE_SIZE,f);
    fclose(f);
    WAVHeader hdr;
    uint32_t offset;
    if(false==ADM_identifyAudioStream(EXTERNAL_PROBE_SIZE,buffer,hdr,offset))
    {
        ADM_warning("Cannot identify external audio track\n");
        return NULL;
    }
    if(!hdr.channels || hdr.channels > MAX_CHANNELS)
    {
        ADM_error("Number of channels out of bounds, the audio file must have been misidentified!\n");
        return NULL;
    }
    if(hdr.frequency < MIN_SAMPLING_RATE || hdr.frequency > MAX_SAMPLING_RATE)
    {
        ADM_error("Sampling frequency out of bounds, the audio file must have been misidentified!\n");
        return NULL;
    }
    // Try to create an access for the file...
    ADM_audioAccess *access=NULL;
    switch(hdr.encoding)
    {
        case WAV_AAC:
                access=new ADM_audioAccessFileAACADTS(name,offset);
                break;
        case WAV_PCM:
        case WAV_EAC3:
        case WAV_AC3:
        case WAV_MP2:
        case WAV_MP3:
                ADM_info("Found external audio track, encoding =%d offset=%d\n",(int)hdr.encoding,(int)offset);
                // create access
                access=new ADM_audioAccessFile(name,offset);
                break;
        default:
                ADM_warning("Unsupported external audio tracks \n");
                return NULL;
                break;
    }
    if(!access)
    {
        ADM_warning("Cannot initialize access for audio track\n");
        return NULL;
    }
    // create ADM_edAudioTrack
    ADM_edAudioTrackExternal *external=new ADM_edAudioTrackExternal(name, &hdr,access);
    uint32_t extraDataLen=0;
    uint8_t  *extraData=NULL;
    access->getExtraData(&extraDataLen,&extraData);
    ADM_info("Trying to create external audio track with %d bytes of extraData %d\n",extraDataLen);
    if(!external->create(extraDataLen,extraData))
    {
        delete external;
        external=NULL;
        ADM_warning("Failed to create external track from %s\n",name);
        return NULL;
    }
    // done!
    return external;
}

/**
    \fn getPCMPacket
*/
bool         ADM_edAudioTrackExternal::getPCMPacket(float  *dest, uint32_t sizeMax, uint32_t *samples,uint64_t *odts)
{
uint32_t fillerSample=0;   // FIXME : Store & fix the DTS error correctly!!!!
uint32_t outFrequency=getOutputFrequency();
uint32_t outChannels=getOutputChannels();

    if(!outChannels) return false;

 vprintf("[PCMPacketExt]  request TRK %d:%x\n",0,(long int)0);
again:
    *samples=0;
    // Do we already have a packet ?
    if(!packetBufferSize)
    {
        if(!refillPacketBuffer())
        {
            ADM_warning("Cannot refill\n");
            return false;
        }
    }
    // We do now
    vprintf("[PCMPacketExt]  Packet size %d, Got %d samples, time code %08lu  lastDts=%08lu delta =%08ld\n",
                packetBufferSize,packetBufferSamples,packetBufferDts,lastDts,packetBufferDts-lastDts);


    // If lastDts is not initialized....
    if(lastDts==ADM_AUDIO_NO_DTS) setDts(packetBufferDts);
    vprintf("Last Dts=%s\n",ADM_us2plain(lastDts));
    //
    //  The packet is ok, decode it...
    //
    uint32_t nbOut=0; // Nb sample as seen by codec
    vprintf("externalPCM: Sending %d bytes to codec\n",packetBufferSize);
    if(!codec->run(packetBuffer, packetBufferSize, dest, &nbOut))
    {
            packetBufferSize=0; // consume
            ADM_warning("[PCMPacketExt::getPCMPacket] Track %d:%x : codec failed failed\n", 0,0);
            return false;
    }
    packetBufferSize=0; // consume

    // Compute how much decoded sample to compare with what demuxer said
    uint32_t decodedSample=nbOut;
    decodedSample/=outChannels;
    if(!decodedSample) goto again;
#define ADM_MAX_JITTER 5000  // in samples, due to clock accuracy, it can be +er, -er, + er, -er etc etc
    if(labs((int64_t)decodedSample-(int64_t)packetBufferSamples)>ADM_MAX_JITTER)
    {
        ADM_warning("[PCMPacketExt::getPCMPacket] Track %d:%x Demuxer was wrong %d vs %d samples!\n",
                    0,0,packetBufferSamples,decodedSample);
    }
    
    // Update infos
    *samples=(decodedSample);
    *odts=lastDts;
    vprintf("externalPCM: got %d samples, PTS is now %s\n",decodedSample,ADM_us2plain(*odts));
    advanceDtsByCustomSample(decodedSample,outFrequency);
    vprintf("[Composer::getPCMPacket] Track %d:%x Adding %u decoded, Adding %u filler sample,"
        " dts is now %lu\n", 0,(long int)0,  decodedSample,fillerSample,lastDts);
    ADM_assert(sizeMax>=(fillerSample+decodedSample)*outChannels);
    vprintf("[getPCMext] %d samples, dts=%s\n",*samples,ADM_us2plain(*odts));
    return true;
}
/**
    \fn goToTime
*/
bool            ADM_edAudioTrackExternal::goToTime(uint64_t nbUs) 
{
        if(codec)
        {
            ADM_info("Resetting audio track\n");
            codec->resetAfterSeek();
        }
        lastDts=ADM_NO_PTS;
        packetBufferSize=0; // reset
        packetBufferSamples=0;
        return internalAudioStream->goToTime(nbUs);
}
// EOF
