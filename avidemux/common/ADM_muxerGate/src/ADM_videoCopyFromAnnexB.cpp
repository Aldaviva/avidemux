/**
    \file ADM_videoCopyFromAnnexB
    \brief Convert from annexB h264 to iso on the fly
    (c) Mean 2010/GPLv2

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
#include "ADM_videoCopy.h"
#include "ADM_edit.hxx"
#include "ADM_coreUtils.h"
#include "ADM_h264_tag.h"
#include "ADM_videoInfoExtractor.h"

using std::string;
extern "C" 
{
    #include "libavcodec/avcodec.h"
}
#include "ADM_h265_tag.h"
extern ADM_Composer *video_body; // Fixme!

//#warning fixme : double definition

//#define ANNEX_B_DEBUG

#if defined(ANNEX_B_DEBUG)
#define aprintf ADM_info
#define check isNalValid
#else
#define aprintf(...) {}
#define check(...) {}
#endif

#ifdef MAX_NALU_PER_CHUNK
#undef MAX_NALU_PER_CHUNK
#define MAX_NALU_PER_CHUNK 20
#endif

static uint32_t readBE32(uint8_t *p)
{
    uint32_t v=(p[0]<<8)+p[1];
    uint32_t u=(p[2]<<8)+p[3];
    return (v<<16)+u;
}

static const char *nalType[13]=
{
    "invalid","nonIdr","invalid","invalid","invalid",
    "idr","sei","sps","pps","au delimiter","invalid","invalid","filler"
};
static bool isNalValid(int nal)
{
    nal&=0x1f;
    switch(nal)
    {
        case 1:case 5:case 6: case 7: case 8: case 9:case 0xc:
                break;
        default:
            ADM_warning("Invalid NAL 0x%x\n",nal);break;
    }
    if(nal<=12) ADM_info("Nal : %s",nalType[nal]);
    return true;
}
/**
 * \fn extractExtraDataH264
 */
bool ADM_videoStreamCopyFromAnnexB::extractExtraDataH264()
{
    myBitstream=new ADMBitstream(ADM_COPY_FROM_ANNEX_B_SIZE);
    myBitstream->data=buffer;

    myExtra=NULL;
    myExtraLen=0;
    // Read First frame, it contains PPS & SPS
    // Built avc-like atom from it.

    // We need the absolute 1st frame to gety SPS PPS
    // Hopefully it will be ok
    ADMCompressedImage img;
    img.data=buffer;
    img.dataLength=0;
    if(false==video_body->getDirectImageForDebug(0,&img))
    {
        ADM_warning("Cannot read first image\n");
        return false;
    }
    myBitstream->len=img.dataLength;
    NALU_descriptor desc[MAX_NALU_PER_CHUNK];
    //mixDump(img.data,img.dataLength);
    int nbNalu=ADM_splitNalu(myBitstream->data,myBitstream->data+myBitstream->len,MAX_NALU_PER_CHUNK,desc);
    // search sps
    uint32_t spsLen=0, ppsLen=0;
    int indexSps,indexPps;

    indexSps=ADM_findNalu(NAL_SPS,nbNalu,desc);
    if(-1==indexSps)
    {
        ADM_error("Cannot find SPS");
        return false;
    }
    indexPps=ADM_findNalu(NAL_PPS,nbNalu,desc);
    if(-1==indexPps)
    {
        ADM_error("Cannot find SPS");
        return false;
    }else
    {
        int count=desc[indexPps].size;
        uint8_t *ptr=desc[indexPps].start+count-1;
        while(count > 4)
        {
            if(*ptr) break;
            ptr--;
            count--;
        }
        ADM_info("PPS removed zero filler %d -> %d\n",(int)desc[indexPps].size,(int)count);
        desc[indexPps].size=count;
    }

    spsLen=desc[indexSps].size;
    ppsLen=desc[indexPps].size;

    ADM_info("Copy from annexB: Found sps=%d, pps=%d.\n",(int)spsLen,(int)ppsLen);
    // Build extraData
    myExtraLen=5+1+2+1+spsLen+1+2+1+ppsLen;
    myExtra=new uint8_t[myExtraLen];
    uint8_t *ptr=myExtra;
    uint8_t *sps=desc[indexSps].start;
    *ptr++=1;           // AVC version
    *ptr++=sps[0];        // Profile
    *ptr++=sps[1];        // Profile
    *ptr++=sps[2];        // Level
    *ptr++=0xff;        // Nal size minus 1

    *ptr++=0xe1;        // SPS
    *ptr++=(1+spsLen)>>8;
    *ptr++=(1+spsLen)&0xff;
    *ptr++=desc[indexSps].nalu;
    memcpy(ptr,desc[indexSps].start,spsLen);
    ptr+=spsLen;

    *ptr++=0x1;         // PPS
    *ptr++=(1+ppsLen)>>8;
    *ptr++=(1+ppsLen)&0xff;
    *ptr++=desc[indexPps].nalu;
    memcpy(ptr,desc[indexPps].start,ppsLen);
    ptr+=ppsLen;

    ADM_info("generated %d bytes of extradata.\n",(int)myExtraLen);
    mixDump(myExtra, myExtraLen);
    return true;
}

/**
 * 
 * @param ptr
 * @param naluType
 * @param size
 * @param data
 * @return 
 */
static uint8_t *writeNaluH265(uint8_t *ptr, NALU_descriptor *d)
{
    *ptr++=(d->nalu>>1)&0x3f;  //  VPS, SPS, PPS, SEI.  0x20 0x21 0x22
    *ptr++=0x00; // 1 NALU
    *ptr++=0x01;
    *ptr++=(d->size+1)>>8;
    *ptr++=(d->size+1)&0xff;
    *ptr++=d->nalu;
    memcpy(ptr,d->start,d->size);
    return ptr+d->size;
}

/**
 * \fn extractExtraDataH265
 * We need to pack VPS/PPS/SPS MP4 style
 * 
 * order should be  VPS, SPS, PPS, SEI. 
 */
bool ADM_videoStreamCopyFromAnnexB::extractExtraDataH265()
{
    myBitstream=new ADMBitstream(ADM_COPY_FROM_ANNEX_B_SIZE);
    myBitstream->data=buffer;

    myExtra=NULL;
    myExtraLen=0;
    // Read First frame, it contains PPS & SPS
  
    ADMCompressedImage img;
    img.data=buffer;
    img.dataLength=0;
    if(false==video_body->getDirectImageForDebug(0,&img))
    {
        ADM_warning("Cannot read first image\n");
        return false;
    }
    mixDump(img.data, 48);
    myBitstream->len=img.dataLength;
    NALU_descriptor desc[MAX_NALU_PER_CHUNK];

    int nbNalu=ADM_splitNalu(myBitstream->data,myBitstream->data+myBitstream->len, MAX_NALU_PER_CHUNK,desc);
    
    // The list of NALU we are interested in...
    NALU_descriptor *vpsNalu,*ppsNalu,*spsNalu;
    
#define LoadNalu(x,y)     x=ADM_findNaluH265(NAL_H265_##y,nbNalu,desc); \
    if(!x) \
    { \
        ADM_error("Cannot find "#y); \
        return false; \
    }
    
#define NUMBER_OF_NALU_IN_EXTRADATA 3 // SEI ?    
    LoadNalu(vpsNalu,VPS);
    LoadNalu(ppsNalu,PPS);
    LoadNalu(spsNalu,SPS);
    
    // Build extraData
    myExtraLen=vpsNalu->size+ppsNalu->size+spsNalu->size+(7)*NUMBER_OF_NALU_IN_EXTRADATA+32;
    myExtra=new uint8_t[myExtraLen];
    uint8_t *ptr=myExtra;
    
    
    //8.3.3.1
//2    
    *ptr++=0x01;           // HEVC version
    uint8_t *s=spsNalu->start+2;
    *ptr++=*s++;       // 2: general_profile_space,  1: general_tier_flag,  5: general_profile_idc;
//4    
    *ptr++=*s++;       //general_profile_compatibility_flags
    *ptr++=*s++;       
    *ptr++=*s++;       
    *ptr++=*s++;       
 //6   
    *ptr++=0xb0;       //general_constraint_indicator_flags
    *ptr++=00;
    *ptr++=00;
    *ptr++=00;
    *ptr++=00;
    *ptr++=00;
    
//4    
    *ptr++=0x99;  // general_level_idc
    *ptr++=0xf0;  // reserver + min_spatial_segmentation_idc
    *ptr++=0x00; //  min_spatial_segmentation_idc, continued;   
    *ptr++=0xfc; //  parallelismType +111111
    
    *ptr++=0xfd; //  chromaFormat +111111
    *ptr++=0xfa; //   bitDepthLumaMinus8;+111111
    *ptr++=0xfa; //   bitDepthChromaMinus8;+111111
    *ptr++=0; // Avg framerate
    *ptr++=0; // Avg framerate
 
    *ptr++=0x47; //2: constantFrameRate 3numTemporalLayers 1: temporalIdNested 2: lengthSizeMinusOne;
     
    *ptr++=NUMBER_OF_NALU_IN_EXTRADATA; // num elem // SEI MISSING!!!!!# FIXME
    
    
    ptr=writeNaluH265(ptr,vpsNalu); 
    ptr=writeNaluH265(ptr,spsNalu);
    ptr=writeNaluH265(ptr,ppsNalu);
    
    myExtraLen=(int)(ptr-myExtra);
    ADM_info("generated %d bytes of extradata.\n",(int)myExtraLen);
    mixDump(myExtra, myExtraLen);
    return true;
}
/**
    \fn ctor
*/
ADM_videoStreamCopyFromAnnexB::ADM_videoStreamCopyFromAnnexB(uint64_t startTime,uint64_t endTime):
      ADM_videoStreamCopy(startTime,endTime)  
{
    ADM_info("AnnexB to iso filter\n");
    _init=false;
    h265=false;
    aviInfo info;
    video_body->getVideoInfo(&info);
    if(isH264Compatible(info.fcc) )
    {
        if(!extractExtraDataH264())
            ADM_warning("H264: Extract MP4 header from annexB failed\n");
        else
            _init=true;
    }    
    else
    if(isH265Compatible(info.fcc))
    {
        h265=true;
        if(!extractExtraDataH265())
            ADM_warning("H265: Extract MP4 header from annexB failed\n");        
        else
            _init=true;
    }
    else
    {
        ADM_warning("Dont know how to process that\n");
    }
    
    rewind();

}
/**

    \fn dtor

*/

ADM_videoStreamCopyFromAnnexB::~ADM_videoStreamCopyFromAnnexB()
{
    ADM_info("Destroying AnnexB to iso filtet\n");
    delete myBitstream;
    myBitstream=NULL;
}

#define START_CODE_LEN 5

static void parseNalu(uint8_t *head, uint8_t *tail)
{
    printf("**** Parsing NALU : %d****",(int)(tail-head));
    while(head<tail)
    {
        int32_t size=readBE32(head);
            printf("[%02x] size=%d\n",head[START_CODE_LEN],size);
        head+=size+START_CODE_LEN-1;
    }
}
/**
    \fn getPacket
*/

bool    ADM_videoStreamCopyFromAnnexB::getPacket(ADMBitstream *out)
{
    aprintf("-------%d--------\n",(int)currentFrame);
    if(false==ADM_videoStreamCopy::getPacket(myBitstream)) return false;
    
    int size;
    if(h265)
        size=ADM_convertFromAnnexBToMP4H265(myBitstream->data,myBitstream->len,out->data,out->bufferSize);
    else
        size=ADM_convertFromAnnexBToMP4(myBitstream->data,myBitstream->len,out->data,out->bufferSize);
    out->len=size;
    out->dts=myBitstream->dts;
    out->pts=myBitstream->pts;
    out->flags=myBitstream->flags;
    //compactNalus(out);
#ifdef ANNEX_B_DEBUG
    parseNalu(out->data,out->data+out->len);
#endif
    return true;
}
/**
    \fn getExtraData
*/
bool    ADM_videoStreamCopyFromAnnexB::getExtraData(uint32_t *extraLen, uint8_t **extraData) 
{
    *extraData=myExtra;
    *extraLen=myExtraLen;
    return true;
}

//---------------------


/**
    \fn ctor
*/
ADM_videoStreamCopyToAnnexB::ADM_videoStreamCopyToAnnexB(uint64_t startTime,uint64_t endTime):
      ADM_videoStreamCopy(startTime,endTime)  
{
    uint32_t extraLen;
    uint8_t  *extraData;
    
    ADM_info("Iso to AnnexB  filter\n");
    myBitstream=new ADMBitstream(ADM_COPY_FROM_ANNEX_B_SIZE);
    myBitstream->data=buffer;
    
    myExtra=NULL;
    myExtraLen=0;
    
    AVCodec *codec=avcodec_find_decoder(AV_CODEC_ID_H264);
    ADM_assert(codec);
    AVCodecContext *context = avcodec_alloc_context3(codec);
    ADM_assert(context);
    
    aviInfo info;
    video_body->getVideoInfo(&info);
    context->width = info.width;
    context->height = info.height;
    context->pix_fmt = AV_PIX_FMT_YUV420P;
    
    video_body->getExtraHeaderData(&extraLen,&extraData);
    // duplicate extraData with malloc scheme, it will be freed by the bitstream filter
    context->extradata=(uint8_t*)av_malloc(extraLen);
    memcpy( context->extradata,extraData,extraLen);
    context->extradata_size=extraLen;
    codecContext=(void *)context;
    
// #warning  Ok, should we open the codec by itself ?

    
    AVBitStreamFilterContext *bsf;
    bsf = av_bitstream_filter_init("h264_mp4toannexb");
    ADM_assert(bsf);
    bsfContext=bsf;
    ADM_info("Copy to annexB initialized\n");

}
/**
 * 
 */
ADM_videoStreamCopyToAnnexB::  ~ADM_videoStreamCopyToAnnexB()
{
    ADM_info("Destroying iso to AnnexB filtet\n");
    delete myBitstream;
    myBitstream=NULL;
   
    if(codecContext)
    {
        AVCodecContext *context=(AVCodecContext *)codecContext;
        codecContext=NULL;
        avcodec_close(context);
    }
     
    
    if(bsfContext)
    {
        AVBitStreamFilterContext *bsf=(AVBitStreamFilterContext *)bsfContext;
        av_bitstream_filter_close(bsf);
        bsfContext=NULL;
    }
    
}
/**
 * 
 * @param out
 * @return 
 */
bool ADM_videoStreamCopyToAnnexB::getPacket(ADMBitstream *out)
{
    AVPacket pktOut;
    bool keyFrame=false;

    aprintf("-------%d--------\n",(int)currentFrame);
    if(false==ADM_videoStreamCopy::getPacket(myBitstream)) return false;
    
    // filter!
    AVBitStreamFilterContext *bsf=(AVBitStreamFilterContext *)bsfContext;
    AVCodecContext *context=(AVCodecContext *)codecContext;
    if(myBitstream->flags & AVI_KEY_FRAME)
        keyFrame=true;
    int ret= av_bitstream_filter_filter(bsf, context, NULL,
                                         &pktOut.data, &pktOut.size,
                                         myBitstream->data, myBitstream->len,
                                         keyFrame);

    if(ret<0)
    {
        ADM_warning("Error while converting to annex B %d\n",ret);
        return false;
    }
    memcpy(out->data,pktOut.data,pktOut.size);
    out->len=pktOut.size;
    
    if(ret>0)
    {
         av_freep(&(pktOut.data));
    }

    
    
    out->dts=myBitstream->dts;
    out->pts=myBitstream->pts;
    out->flags=myBitstream->flags;
    return true;
}
/**
 * 
 * @param out
 * @return 
 */
bool ADM_videoStreamCopyToAnnexB::getExtraData(uint32_t *extraLen, uint8_t **extraData) 
{
    *extraLen=0;
    *extraData=NULL;
    return true; // no extra data in annex b
}
        

// EOF
