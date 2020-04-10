/**
    \brief VDPAU filters
    \author mean (C) 2010
    This is slow as we copy back and forth data to/from the video cards
    Only 1 filter exposed at the moment : resize...

*/

#include "ADM_default.h"
#ifdef USE_VDPAU
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavcodec/vdpau.h"
}

#include "ADM_coreVideoFilter.h"
#include "ADM_videoFilterCache.h"
#include "DIA_factory.h"
#include "vdpauFilter.h"
#include "vdpauFilter_desc.cpp"




#include "ADM_coreVdpau.h"
//
#define ADM_INVALID_FRAME_NUM 0x80000000
#define ADM_NB_SURFACES 3

/**
    \class vdpauVideoFilter
*/
class vdpauVideoFilter : public  ADM_coreVideoFilterCached
{
protected:
                    ADMColorScalerSimple *scaler;
                    bool                 passThrough;
                    bool                 setupVdpau(void);
                    bool                 cleanupVdpau(void);

                    uint8_t             *tempBuffer;
                    vdpauFilter          configuration;
                    VdpOutputSurface     outputSurface;
                    VdpVideoSurface      input[ADM_NB_SURFACES];
                    uint32_t             frameDesc[ADM_NB_SURFACES];
                    uint32_t             currentIndex;
                    VdpVideoMixer        mixer;
                    bool                 uploadImage(ADMImage *next,uint32_t surfaceIndex,uint32_t frameNumber) ;
                    bool                 setIdentityCSC(void);

public:
        
                             vdpauVideoFilter(ADM_coreVideoFilter *previous,CONFcouple *conf);
                             ~vdpauVideoFilter();

        virtual const char   *getConfiguration(void);                 /// Return  current configuration as a human readable string
        virtual bool         getNextFrame(uint32_t *fn,ADMImage *image);           /// Return the next image
        virtual bool         getCoupledConf(CONFcouple **couples) ;   /// Return the current filter configuration
		virtual void setCoupledConf(CONFcouple *couples);
        virtual bool         configure(void) ;                        /// Start graphical user interface
};

// Add the hook to make it valid plugin
DECLARE_VIDEO_FILTER(   vdpauVideoFilter,   // Class
                        1,0,0,              // Version
                        ADM_UI_GTK+ADM_UI_QT4+ADM_FEATURE_VDPAU,     // We need a display for VDPAU; so no cli...
                        VF_TRANSFORM,            // Category
                        "vdpauResize",            // internal name (must be uniq!)
                        QT_TRANSLATE_NOOP("vdpresize","vdpau: Resize"),            // Display name
                        QT_TRANSLATE_NOOP("vdpresize","vdpau: Resize image using vdpau.") // Description
                    );

/**
    \fn setIdentityCSC
    \brief set the RGB/YUV matrix to identity so that data are still YUV at the end
            Should not work, but it does.
*/
bool vdpauVideoFilter::setIdentityCSC(void)
{
    ADM_info("Setting custom CSC\n");
    const VdpCSCMatrix   matrix={{1.,0,0,0},{0,1.,0,0},{0,0,1.,0}};    
    VdpVideoMixerAttribute attributes_key[]={VDP_VIDEO_MIXER_ATTRIBUTE_CSC_MATRIX};
    const void * attribute_values[] = {&matrix};
    
    VdpStatus st = admVdpau::mixerSetAttributesValue(mixer, 1,attributes_key, (void * const *)attribute_values);
    if(st!=VDP_STATUS_OK)
        ADM_error("Cannot set custom matrix (CSC)\n");
    return true;
}
//

/**
    \fn resetVdpau
*/
bool vdpauVideoFilter::setupVdpau(void)
{
    scaler=NULL;

    info.width=configuration.targetWidth;
    info.height=configuration.targetHeight;
    for(int i=0;i<ADM_NB_SURFACES;i++)             frameDesc[i]=ADM_INVALID_FRAME_NUM;
    currentIndex=0;
    if(!admVdpau::isOperationnal())
    {
        ADM_warning("Vdpau not operationnal\n");
        return false;
    }
    // check if we have something to do
    if(info.width==previousFilter->getInfo()->width &&  info.height==previousFilter->getInfo()->height)
        return false;
    
    if(VDP_STATUS_OK!=admVdpau::outputSurfaceCreate(VDP_RGBA_FORMAT_B8G8R8A8,
                        info.width,info.height,&outputSurface)) 
    {
        ADM_error("Cannot create outputSurface0\n");
        return false;
    }
    for(int i=0;i<ADM_NB_SURFACES;i++)
    {
        if(VDP_STATUS_OK!=admVdpau::surfaceCreate(   previousFilter->getInfo()->width,
                                                    previousFilter->getInfo()->height,input+i)) 
        {
            ADM_error("Cannot create input Surface %d\n",i);
            goto badInit;
        }
    }
    {
    int paddedHeight=(previousFilter->getInfo()->height+15)&~15;
    if(VDP_STATUS_OK!=admVdpau::mixerCreate(previousFilter->getInfo()->width,
                                            paddedHeight,&mixer)) 
    {
        ADM_error("Cannot create mixer\n");
        goto badInit;
    }
    }
    setIdentityCSC();
    tempBuffer=new uint8_t[info.width*info.height*4];
    scaler=new ADMColorScalerSimple( info.width,info.height, ADM_COLOR_RGB32A,ADM_COLOR_YV12);
    ADM_info("VDPAU setup ok\n");
    return true;
badInit:
    cleanupVdpau();
    passThrough=true;
    return false;
}
/**
    \fn cleanupVdpau
*/
bool vdpauVideoFilter::cleanupVdpau(void)
{
    for(int i=0;i<ADM_NB_SURFACES;i++)
        if(input[i]!=VDP_INVALID_HANDLE) admVdpau::surfaceDestroy(input[i]);
    if(outputSurface!=VDP_INVALID_HANDLE)  admVdpau::outputSurfaceDestroy(outputSurface);
    if(mixer!=VDP_INVALID_HANDLE) admVdpau::mixerDestroy(mixer);
    outputSurface=VDP_INVALID_HANDLE;
    for(int i=0;i<ADM_NB_SURFACES;i++)
        input[i]=VDP_INVALID_HANDLE;
    mixer=VDP_INVALID_HANDLE;
    if(tempBuffer) delete [] tempBuffer;
    tempBuffer=NULL;
    if(scaler) delete scaler;
    scaler=NULL;
    return true;
}

/**
    \fn constructor
*/
vdpauVideoFilter::vdpauVideoFilter(ADM_coreVideoFilter *in, CONFcouple *setup)
        : ADM_coreVideoFilterCached(5,in,setup)
{
    for(int i=0;i<ADM_NB_SURFACES;i++)
        input[i]=VDP_INVALID_HANDLE;
    mixer=VDP_INVALID_HANDLE;
    outputSurface=VDP_INVALID_HANDLE;
    if(!setup || !ADM_paramLoad(setup,vdpauFilter_param,&configuration))
    {
        // Default value
        configuration.targetWidth=info.width;
        configuration.targetHeight=info.height;
    }

    myName="vdpau";
    tempBuffer=NULL;
    passThrough=!setupVdpau();
    
    
}
/**
    \fn destructor
*/
vdpauVideoFilter::~vdpauVideoFilter()
{
        cleanupVdpau();
}
/**
    \fn updateInfo
*/
bool vdpauVideoFilter::configure( void) 
{
    
     
     diaElemUInteger  tWidth(&(configuration.targetWidth),QT_TRANSLATE_NOOP("vdpresize","Width :"),16,2048);
     diaElemUInteger  tHeight(&(configuration.targetHeight),QT_TRANSLATE_NOOP("vdpresize","Height :"),16,2048);
     
     diaElem *elems[]={&tWidth,&tHeight};
     
     if(diaFactoryRun(QT_TRANSLATE_NOOP("vdpresize","vdpau"),sizeof(elems)/sizeof(diaElem *),elems))
     {
                info.width=configuration.targetWidth;
                info.height=configuration.targetHeight;
                ADM_info("New dimension : %d x %d\n",info.width,info.height);
                cleanupVdpau();
                passThrough=!setupVdpau();
                return 1;
     }
     return 0;
}
/**
    \fn getCoupledConf
    \brief Return our current configuration as couple name=value
*/
bool         vdpauVideoFilter::getCoupledConf(CONFcouple **couples)
{
    return ADM_paramSave(couples, vdpauFilter_param,&configuration);
}

void vdpauVideoFilter::setCoupledConf(CONFcouple *couples)
{
    ADM_paramLoad(couples, vdpauFilter_param, &configuration);
}

/**
    \fn getConfiguration
    \brief Return current setting as a string
*/
const char *vdpauVideoFilter::getConfiguration(void)
{
    static char conf[80];
    char conf2[80];
    conf2[0]=0;
    sprintf(conf,"vdpau:");
    if(1) //configuration.resizeToggle)
    {
        sprintf(conf2,"%d x %d -> %d x %d",
                        previousFilter->getInfo()->width, 
                        previousFilter->getInfo()->height,
                        info.width,info.height);
        strcat(conf,conf2);
    }
    conf[79]=0;
    return conf;
}
/**
    \fn uploadImage
    \brief upload an image to a vdpau surface
*/
bool vdpauVideoFilter::uploadImage(ADMImage *next,uint32_t surfaceIndex,uint32_t frameNumber) 
{
    if(!next) // empty image
    {
        frameDesc[surfaceIndex%ADM_NB_SURFACES]=ADM_INVALID_FRAME_NUM;
        ADM_warning("No image to upload\n");
        return false;
    }
  // Blit our image to surface
    int      ipitches[3];
    uint32_t pitches[3];
    uint8_t *planes[3];
    next->GetPitches(ipitches);
    next->GetReadPlanes(planes);

    for(int i=0;i<3;i++) pitches[i]=(uint32_t)ipitches[i];
    
    // Put out stuff in input...
    //printf("Uploading image to surface %d\n",surfaceIndex%ADM_NB_SURFACES);

    if(VDP_STATUS_OK!=admVdpau::surfacePutBits( 
            input[surfaceIndex%ADM_NB_SURFACES],
            planes,pitches))
    {
        ADM_warning("[Vdpau] video surface : Cannot putbits\n");
        return false;
    }
    frameDesc[surfaceIndex%ADM_NB_SURFACES]=frameNumber;
    return true;
}
/**
    \fn getConfiguration
    \brief Return current setting as a string
*/
bool vdpauVideoFilter::getNextFrame(uint32_t *fn,ADMImage *image)
{
     bool r=false;
     if(passThrough) return previousFilter->getNextFrame(fn,image);
    // regular image, in fact we get the next image here
    VdpVideoSurface tmpSurface=VDP_INVALID_HANDLE;
    ADMImage *next= vidCache->getImageAs(ADM_HW_VDPAU,nextFrame);
    if(!next)
    {
        ADM_warning("vdpauResize : cannot get image\n");
        return false;
    }
    image->Pts=next->Pts;
    if(next->refType==ADM_HW_VDPAU)
    {
        
        struct ADM_vdpauRenderState *rndr = (struct ADM_vdpauRenderState *)next->refDescriptor.refHwImage;
        tmpSurface=rndr->surface;
        printf("image is already vdpau %d\n",(int)tmpSurface);
    }else
    {
        //printf("Uploading image to vdpau\n");
        if(false==uploadImage(next,0,nextFrame)) 
        {
            vidCache->unlockAll();
            return false;
        }
        tmpSurface=input[0];
    }
    
    // Call mixer...
    if(VDP_STATUS_OK!=admVdpau::mixerRenderWithCropping( 
                mixer,
                tmpSurface,
                outputSurface, 
                info.width,info.height, // target
                previousFilter->getInfo()->width,previousFilter->getInfo()->height))

    {
        ADM_warning("[Vdpau] Cannot mixerRender\n");
        vidCache->unlockAll();
        return false;
    }
    // Now get our image back from surface...
    if(VDP_STATUS_OK!=admVdpau::outputSurfaceGetBitsNative(outputSurface,tempBuffer, info.width,info.height))
    {
        ADM_warning("[Vdpau] Cannot copy back data from output surface\n");
        vidCache->unlockAll();
        return false;
    }
    r=image->convertFromYUV444(tempBuffer);
   
    nextFrame++;
    currentIndex++;
    vidCache->unlockAll();
    
    return r;
}
#else // USE_VDPAU
static void dumy_func2(void)
{
    return;
}
#endif 
//****************
// EOF
