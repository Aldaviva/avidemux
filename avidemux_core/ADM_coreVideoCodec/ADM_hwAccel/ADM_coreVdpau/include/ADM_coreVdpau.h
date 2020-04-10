/***************************************************************************
    \file             : ADM_coreVdpau.cpp
    \brief            : Wrapper around vdpau functions
    \author           : (C) 2010 by mean fixounet@free.fr
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ADM_CORE_VDPAU_H
#define ADM_CORE_VDPAU_H

#ifdef USE_VDPAU
#include "vdpau/vdpau_x11.h"
#include "vdpau/vdpau.h"

#define FF_VDPAU_STATE_USED_FOR_RENDER 1
#define FF_VDPAU_STATE_USED_FOR_REFERENCE 2

struct ADM_vdpauRenderState {
    VdpVideoSurface surface;
    int state; // Holds FF_VDPAU_STATE_* values.
    int refCount; // Used by how many client MEANX
};

#endif 

#include "ADM_windowInfo.h"
/**
    \class admVdpau
*/
class admVdpau
{
protected:
    static GUI_WindowInfo      myWindowInfo;
#ifdef USE_VDPAU
    static bool         queryYUVPutBitSupport(VdpRGBAFormat rgb,VdpYCbCrFormat yuv);
    static VdpStatus    mixerEnableFeature( VdpVideoMixer mixer,uint32_t nbFeature,VdpVideoMixerFeature *feature,VdpBool *enabledFeature);
    static bool         mixerFeatureSupported(VdpVideoMixerFeature attribute);
#endif
public:
    static bool         init(GUI_WindowInfo *x);
    static bool         isOperationnal(void);
    static bool         cleanup(void);
    /* Surface */
#ifdef USE_VDPAU
    static  void        *getVdpDevice(void);
    static  void        *getProcAddress(void);
    static  VdpGetProcAddress   *getProcAddress2(void)    ;
    static  const char  *getErrorString(VdpStatus er);
    static  bool        mixerIsFeatureEnabled( VdpVideoMixer mixer,VdpVideoMixerFeature feature);
    static  VdpStatus   surfaceCreate(uint32_t width,uint32_t height,VdpVideoSurface *surface);
    static  VdpStatus   surfaceDestroy(VdpVideoSurface surface);
    static  VdpStatus   getDataSurface(VdpVideoSurface surface,uint8_t *planes[3],uint32_t stride[3]);
    static  VdpStatus   surfacePutBits(VdpVideoSurface surface,uint8_t *planes[3],uint32_t stride[3]);
    /* Decoder */
    static  VdpStatus   decoderCreate( VdpDevice dev,VdpDecoderProfile profile,    uint32_t          width,    uint32_t          height,    uint32_t          max_references,       VdpDecoder *      decoder);
    static  VdpStatus   decoderDestroy(VdpDecoder decoder);

    static  VdpStatus   decoderRender(
            VdpDecoder                 decoder,
            VdpVideoSurface            target,
            const void                 *info,
            uint32_t                   bitstream_buffer_count,
            VdpBitstreamBuffer const * bitstream_buffers);

    /* Output surface */
    static VdpStatus outputSurfaceCreate(
            VdpRGBAFormat      rgba_format,
            uint32_t           width,
            uint32_t           height,
            VdpOutputSurface * surface);

    static VdpStatus outputSurfaceDestroy(VdpOutputSurface surface);
    static VdpStatus outPutSurfacePutBitsYV12( 
            VdpOutputSurface     surface,
            uint8_t *planes[3],
            uint32_t pitches[3]);
    // Warning only RGBA32 supported!
    static VdpStatus outputSurfaceGetBitsNative(VdpOutputSurface     surface,  
                        uint8_t *buffer, uint32_t w,uint32_t h);
    static VdpStatus outputSurfaceGetBitsNative_FieldWeave(VdpOutputSurface     surface,  
                        uint8_t *buffer, uint32_t w,uint32_t h);


    /* Presentation queue */
    static VdpStatus presentationQueueCreate(VdpPresentationQueue *queue);
    static VdpStatus presentationQueueDestroy(VdpPresentationQueue queue);
    static VdpStatus presentationQueueDisplay(VdpPresentationQueue queue,VdpOutputSurface outputSurface);
    /* Mixer */
    static VdpStatus mixerCreate(uint32_t width,uint32_t height, VdpVideoMixer *mixer,bool deinterlace=false,bool ivtc=false);
    static VdpStatus mixerDestroy(VdpVideoMixer mixer);
    static VdpStatus mixerRender(VdpVideoMixer mixer,VdpVideoSurface sourceSurface,VdpOutputSurface targetOutputSurface, uint32_t targetWidth, uint32_t targetHeight );
    static VdpStatus mixerRenderWithCropping(VdpVideoMixer mixer,VdpVideoSurface sourceSurface,VdpOutputSurface targetOutputSurface, uint32_t targetWidth, uint32_t targetHeight ,
                                            uint32_t sourceWidth, uint32_t sourceHeight);
    static VdpStatus mixerRenderFieldWithPastAndFuture(bool topField,VdpVideoMixer mixer,
                                VdpVideoSurface sourceSurface[3], // Past present future
                                VdpOutputSurface targetOutputSurface, uint32_t targetWidth,     uint32_t targetHeight,uint32_t sourceWidth,     uint32_t sourceHeight );
    static VdpStatus mixerRenderFrameWithPastAndFuture(VdpVideoMixer mixer,
                                VdpVideoSurface sourceSurface[3], // Past present future
                                VdpOutputSurface targetOutputSurface, uint32_t targetWidth,     uint32_t targetHeight,uint32_t sourceWidth,     uint32_t sourceHeight );
    
    static VdpStatus mixerGetAttributesValue(VdpVideoMixer mixer,
                                uint32_t attrCount,
                                const  VdpVideoMixerAttribute *xkeys,
                                void * const* values);
    static VdpStatus mixerSetAttributesValue(VdpVideoMixer mixer,
                                uint32_t attrCount,
                                const  VdpVideoMixerAttribute *xkeys,
                                void * const* values);
    static bool      queryDecoderCapabilities(      VdpDecoderProfile profile, // return true if supported
                                                    int *        max_width,
                                                    int *        max_height);
    static int dimensionRoundUp(int x)
    {
                return (x+15) & ~15;
    }
    static VdpStatus outputSurfaceGetParameters(  VdpOutputSurface surface,    VdpRGBAFormat *  rgba_format,
                            uint32_t *       width,    uint32_t *       height);
    static VdpStatus surfaceGetParameters(VdpVideoSurface surface,VdpChromaType *chomra,uint32_t *w,uint32_t *h);

#endif
};
#endif
