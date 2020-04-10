/***************************************************************************
    copyright            : (C) 2007 by mean
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
#include "ADM_colorspace.h"
#include "ADM_image.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
}

#include "ADM_rgb.h" 
#include "ADM_colorspace.h"

#ifdef ADM_CPU_X86
		#define ADD(x,y) if( CpuCaps::has##x()) flags|=SWS_CPU_CAPS_##y;
#define FLAGS()		ADD(MMX,MMX);				ADD(3DNOW,3DNOW);		ADD(MMXEXT,MMX2);
#else
#ifdef ADM_CPU_ALTIVEC
#define FLAGS() flags|=SWS_CPU_CAPS_ALTIVEC;
#else
#define FLAGS()
#endif
#endif

#define CONTEXT (SwsContext *)context

/**
    \fn swapRGB
*/
static void swapRGB32(uint32_t w,uint32_t h, uint8_t *to)
{
  uint32_t l=w*h;
    uint8_t *d=(uint8_t *)to;
    while(l--)
    {
        uint8_t s=d[0];
        d[0]=d[2];
        d[2]=s;
        d+=4;
        
    }
}
/**
    \fn swapRGB
*/
static void swapRGB24(uint32_t w,uint32_t h, uint8_t *to)
{
    uint32_t l=w*h;
    uint8_t *d=(uint8_t *)to;
    return;

    while(l--)
    {
        uint8_t s=d[0];
        d[0]=d[2];
        d[2]=s;
        d+=3;
    }
}
/**
    \fn ADMColor2LAVColor
    \brief Convert ADM colorspace type swscale/lavcodec colorspace name

*/
static AVPixelFormat ADMColor2LAVColor(ADM_colorspace fromColor_)
{
  ADM_colorspace fromColor=fromColor_;
  int intColor=(int)fromColor;
  intColor&=ADM_COLOR_MASK;
  fromColor=(ADM_colorspace)intColor;
  switch(fromColor)
  {
    case ADM_COLOR_YUV444: return AV_PIX_FMT_YUV444P;
    case ADM_COLOR_YUV411: return AV_PIX_FMT_YUV411P;
    case ADM_COLOR_YUV422: return AV_PIX_FMT_YUYV422;
    case ADM_COLOR_UYVY422: return AV_PIX_FMT_UYVY422;
    case ADM_COLOR_YV12: return AV_PIX_FMT_YUV420P;
    case ADM_COLOR_YUV422P: return AV_PIX_FMT_YUV422P;
    case ADM_COLOR_RGB555: return AV_PIX_FMT_RGB555LE;
    case ADM_COLOR_BGR555: return AV_PIX_FMT_BGR555LE;
    case ADM_COLOR_RGB32A: return AV_PIX_FMT_RGBA;
    case ADM_COLOR_BGR32A: return AV_PIX_FMT_RGBA; // Faster that way...AV_PIX_FMT_BGR32;
    case ADM_COLOR_RGB24: return AV_PIX_FMT_RGB24;
    case ADM_COLOR_BGR24: return AV_PIX_FMT_BGR24;
    case ADM_COLOR_YV12_10BITS: return AV_PIX_FMT_YUV420P10LE;
    case ADM_COLOR_NV12_10BITS:  return AV_PIX_FMT_P010LE;
    case ADM_COLOR_YUV444_10BITS: return AV_PIX_FMT_YUV444P10LE;
    case ADM_COLOR_YUV422_10BITS: return AV_PIX_FMT_YUV422P10LE;
    case ADM_COLOR_Y8: return AV_PIX_FMT_GRAY8;
    default : ADM_assert(0); 
  }
  return AV_PIX_FMT_YUV420P;
}
/**
      \fn getStrideAndPointers
      \param dst=1 -> destination, =0 source
      \brief Fill in strides etc.. needed by libswscale
*/
uint8_t ADMColorScalerFull::getStrideAndPointers(bool dst,
        uint8_t  *from,ADM_colorspace fromColor,
        uint8_t **srcData,int *srcStride)
{
    uint32_t width,height;
    if(!dst)
    {
        width=srcWidth;
        height=srcHeight;
    }else
    {
        width=dstWidth;
        height=dstHeight;
    }
  switch(fromColor)
  {
    case ADM_COLOR_RGB555: 
            srcData[0]=from;
            srcData[1]=NULL;
            srcData[2]=NULL;
            srcStride[0]=width*2;
            srcStride[1]=0;
            srcStride[2]=0;
            break;
    case ADM_COLOR_RGB24:
    case ADM_COLOR_BGR24:
            srcData[0]=from;
            srcData[1]=NULL;
            srcData[2]=NULL;
            srcStride[0]=width*3;
            srcStride[1]=0;
            srcStride[2]=0;
            break;
    case  ADM_COLOR_YV12:
            srcData[0]=from;
            srcData[1]=from+width*height;
            srcData[2]=from+((5*width*height)>>2);
            srcStride[0]=width;
            srcStride[1]=width>>1;
            srcStride[2]=width>>1;
            break;
    case  ADM_COLOR_YUV422:
    case  ADM_COLOR_UYVY422:        
            srcData[0]=from;
            srcData[1]=NULL;
            srcData[2]=NULL;
            srcStride[0]=width*2;
            srcStride[1]=0;
            srcStride[2]=0;
            break;            
    case  ADM_COLOR_YUV422P:
            srcData[0]=from;
            srcData[1]=from+width*height;
            srcData[2]=from+((3*width*height)>>1);
            srcStride[0]=width;
            srcStride[1]=width>>1;
            srcStride[2]=width>>1;
            break;
    case ADM_COLOR_RGB32A:
    case ADM_COLOR_BGR32A:
            srcData[0]=from;
            srcData[1]=NULL;
            srcData[2]=NULL;
            srcStride[0]=width*4;
            srcStride[1]=0;
            srcStride[2]=0;
            break;
    
    default:
        ADM_assert(0);
  }
  return 1;
}
/**
    \fn  convert
    \brief Do the color conversion
  @param from Source image
  @param to Target image
*/
#define swap16(x) x=((x>>8)&0xff)+(x<<8)
bool ADMColorScalerFull::convert(uint8_t  *from, uint8_t *to)
{
  uint8_t *srcData[3];
  uint8_t *dstData[3];
  int srcStride[3];
  int dstStride[3];
  
  getStrideAndPointers(false,from,fromColor,srcData,srcStride);
  getStrideAndPointers(true,to,toColor,dstData,dstStride);
 
  
  sws_scale(CONTEXT,srcData,srcStride,0,srcHeight,dstData,dstStride);
  if(toColor==ADM_COLOR_BGR32A)
  {
     swapRGB32(dstWidth,dstHeight,to);
  }
  if(toColor==ADM_COLOR_BGR24)
  {
     swapRGB24(dstWidth,dstHeight,to);
  }
  
  return true;
  
}
/**
    \fn convertPlanes
    \brief Same as convert but the 3 planes are given separately
*/
bool            ADMColorScalerFull::convertPlanes(int  sourceStride[3],int destStride[3],     
                                  uint8_t   *sourceData[3], uint8_t *destData[3])
{
    int xs[4]={(int)sourceStride[0],(int)sourceStride[1],(int)sourceStride[2],0};
    int xd[4]={(int)destStride[0],(int)destStride[1],(int)destStride[2],0};
    uint8_t *src[4]={NULL,NULL,NULL,NULL};
    uint8_t *dst[4]={NULL,NULL,NULL,NULL};
     for(int i=0;i<3;i++)
        {
            src[i]=sourceData[i];
            dst[i]=destData[i];
        }
     sws_scale(CONTEXT,src,xs,0,srcHeight,dst,xd);
     return true;
}
/**
    \fn convertPlanes
    \brief Same as convert but the 3 planes are given separately
*/
bool            ADMColorScalerFull::convertImage(ADMImage *sourceImage, ADMImage *destImage)
{
    int xs[4];
    int xd[4];
    uint8_t *src[4];
    uint8_t *dst[4];
    sourceImage->GetPitches(xs);
    destImage->GetPitches(xd);
    xs[3]=sourceImage->GetPitch(PLANAR_ALPHA);
    xd[3]=destImage->GetPitch(PLANAR_ALPHA);
    
    destImage->GetWritePlanes(dst);
    sourceImage->GetReadPlanes(src);
    
    src[3]=sourceImage->GetReadPtr(PLANAR_ALPHA);
    dst[3]=destImage->GetWritePtr(PLANAR_ALPHA);
    
    sws_scale(CONTEXT,src,xs,0,srcHeight,dst,xd);
    return true;
}

/**
    \fn  ADMColorScaler
    \brief Constructor
  @param w width
  @param h height
  @param from colorspace to convert from
  @param to colorspace to concert to
*/

ADMColorScalerFull::ADMColorScalerFull(ADMColorScaler_algo algo,
            int sw, int sh,
            int dw, int dh,
            ADM_colorspace from,ADM_colorspace to)
{
   context=NULL;
   reset(algo,sw,sh,dw,dh,from,to);

}
/**
    \fn  ~ADMColorScaler
    \brief Destructor
*/
ADMColorScalerFull::~ADMColorScalerFull()
{
  if(context)
  {
     sws_freeContext(CONTEXT);
     context=NULL;
  }
}
/**
    \fn reset
*/
bool  ADMColorScalerFull::reset(ADMColorScaler_algo algo, int sw, int sh, int dw,int dh,ADM_colorspace from,ADM_colorspace to)
{
    if(context) sws_freeContext(CONTEXT);
    context=NULL;
    this->algo=algo;
    int flags;
    switch(algo)
    {
#define SETAL(x) case ADM_CS_##x: flags=SWS_##x;break;

    SETAL(BILINEAR);
    SETAL(FAST_BILINEAR);
    SETAL(BICUBIC);
    SETAL(LANCZOS);
    SETAL(BICUBLIN);
    SETAL(GAUSS);
    SETAL(SINC);
    SETAL(SPLINE);
    default: ADM_assert(0);
    }
#if 0 // this is gone, we need to patch av_get_cpu_flags directly now
    {
        FLAGS();
    }
#endif
  
    srcWidth=sw;
    srcHeight=sh;

    dstWidth=dw;
    dstHeight=dh;

    fromColor=from;
    toColor=to;
    AVPixelFormat lavFrom=ADMColor2LAVColor(fromColor );
    AVPixelFormat lavTo=ADMColor2LAVColor(toColor );
    
    context=(void *)sws_getContext(
                      srcWidth,srcHeight,
                      lavFrom ,
                      dstWidth,dstHeight,
                      lavTo,
                      flags, NULL, NULL,NULL);
    return true;
}
//------------------------------
bool            ADMColorScalerSimple::changeWidthHeight(int newWidth, int newHeight)
{
    if(newWidth==srcWidth && newHeight==srcHeight) return true; // no change
    
     return reset(algo, newWidth,newHeight, newWidth,newHeight,fromColor,toColor);

}

/**
    \fn convertColorSpace
*/
bool ADMColorScalerFull::convertImage(ADMImage *img, uint8_t *to)
{
    uint8_t *srcPlanes[3];
    uint8_t *dstPlanes[3];
    int srcPitch[3];
    int dstPitch[3];
    int      idstPitch[3];
    img->GetPitches(srcPitch);
    img->GetReadPlanes(srcPlanes);
    getStrideAndPointers(true,to,toColor, dstPlanes,idstPitch);
    dstPitch[0]=idstPitch[0];
    dstPitch[1]=idstPitch[1];
    dstPitch[2]=idstPitch[2];
    if(false==convertPlanes(srcPitch,dstPitch,srcPlanes,dstPlanes)) return false;

    if(toColor==ADM_COLOR_BGR32A)
    {
             swapRGB32(dstWidth,dstHeight,to);
    }   
    return true;
}
//EOF
