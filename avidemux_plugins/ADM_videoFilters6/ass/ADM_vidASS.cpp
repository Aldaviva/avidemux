/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*
  Initial port from MPlayer by Moonz
  Mplayer version is Copyright (C) 2006 Evgeniy Stepanov <eugeni.stepanov@gmail.com>

*/


#include "ADM_default.h"
#include "ADM_coreVideoFilter.h"
#include "DIA_coreToolkit.h"
#include "DIA_factory.h"

#include "ass_ssa.h"
#include "ass_ssa_desc.cpp"
#include "ADM_coreSubtitles.h"
#include "prefs.h"
extern "C"
{
#ifdef USE_EXTERNAL_LIBASS
#include "ass/ass.h"
#else
#include "ADM_libass/ass.h"
#endif
}

typedef enum
{
    SQUARE_PIXELS,
    STANDARD,
    WIDE,
    UNI,
    CINEMA
}SUBASS_DAR;

/**
    \class subAss
*/
class subAss : public  ADM_coreVideoFilter
{
protected:
        ass_ssa         param;
        ASS_Library     *_ass_lib;
        ASS_Renderer    *_ass_rend;
        ASS_Track       *_ass_track;
        bool            setup(void);
        bool            cleanup(void);
        ADMImage        *src;
        
        bool            mergeOneImage(ASS_Image *img,ADMImage *target);
public:
                            subAss(ADM_coreVideoFilter *previous,CONFcouple *conf);
                            ~subAss();

        virtual const char   *getConfiguration(void);                   /// Return  current configuration as a human readable string
        virtual bool         getNextFrame(uint32_t *fn,ADMImage *image);    /// Return the next image
	 //  virtual FilterInfo  *getInfo(void);                             /// Return picture parameters after this filter
        virtual bool         getCoupledConf(CONFcouple **couples) ;   /// Return the current filter configuration
        virtual void         setCoupledConf(CONFcouple *couples);
        virtual bool         configure(void) ;           /// Start graphical user interface
};

// Add the hook to make it valid plugin
DECLARE_VIDEO_FILTER_PARTIALIZABLE(   subAss,   // Class
                        1,0,1,              // Version
                        ADM_UI_ALL,         // UI
                        VF_SUBTITLE,            // Category
                        "ssa",            // internal name (must be uniq!)
                        QT_TRANSLATE_NOOP("ass","SSA/ASS/SRT"),            // Display name
                        QT_TRANSLATE_NOOP("ass","Hardcode ASS/SSA/SRT subtitles using libass.") // Description
                    );



#ifndef DIR_SEP
# ifdef WIN32
#   define DIR_SEP '\\'
#   define DEFAULT_FONT_DIR "c:"
# else
#   define DIR_SEP '/'
#   define DEFAULT_FONT_DIR "/usr/share/fonts/truetype/"
# endif
#endif
//*****************

/**
    \fn getConfiguration
    \brief Return current setting as a string
*/
const char *subAss::getConfiguration(void)
{
    static char buf[500];
    buf[0]=0;

      sprintf((char *)buf," ASS/SSA Subtitles: ");

      const char *filename = param.subtitleFile.c_str();
      if(filename)
      {
          if(strrchr(filename, DIR_SEP) != NULL && *(strrchr(filename, DIR_SEP) + 1) != 0)
              filename = strrchr(filename, DIR_SEP) + 1;
          strncat(buf, filename, 49-strlen(buf));
          buf[49] = 0;
      }else
      {
        strcat(buf," (no sub)");
      }
      return buf;
}
/**
    \fn ctor
*/
subAss::subAss( ADM_coreVideoFilter *in,CONFcouple *setup) : ADM_coreVideoFilter(in,setup) {
    if (!setup || !ADM_paramLoad(setup, ass_ssa_param, &param)) {
        param.font_scale = 1.;
        param.line_spacing = param.topMargin = param.bottomMargin = 0;
        param.subtitleFile = std::string("");
        param.fontDirectory = std::string(DEFAULT_FONT_DIR);
        param.extractEmbeddedFonts = 1;
        param.displayAspectRatio = 0;
    }
    src = new ADMImageDefault(in->getInfo()->width, in->getInfo()->height);

    /* ASS initialization */
    _ass_lib = NULL;
    _ass_track = NULL;
    _ass_rend = NULL;

    if (param.subtitleFile.size()) 
    {
        if (!this->setup()) 
        {
            GUI_Error_HIG(QT_TRANSLATE_NOOP("ass","Format ?"), QT_TRANSLATE_NOOP("ass","Are you sure this is an ASS/SSA file ?"));
        }
    }
}
/**
    \fn dtor
*/
#define DELETE(x) if(x) {ADM_dealloc(x);x=NULL;}
subAss::~subAss()
{
      if(src) delete src;
      src=NULL;
      cleanup();
}
/**
    \fn getCoupledConf
    \brief Return our current configuration as couple name=value
*/
bool         subAss::getCoupledConf(CONFcouple **couples)
{
    return ADM_paramSave(couples, ass_ssa_param,&param);
}

void subAss::setCoupledConf(CONFcouple *couples)
{
    ADM_paramLoad(couples, ass_ssa_param, &param);
}

/**
    \fn configure
*/
bool subAss::configure(void)
{

#define PX(x) &(param.x)
#define MKME(x,y) x=(ELEM_TYPE_FLOAT)param.y
  ELEM_TYPE_FLOAT scale,spacing;

    MKME(scale,font_scale);
    MKME(spacing,line_spacing);
    diaElemFile       file(0,param.subtitleFile,QT_TRANSLATE_NOOP("ass","_Subtitle file (ASS/SSA):"), NULL, QT_TRANSLATE_NOOP("ass","Select Subtitle File"));
    diaElemFloat      dSpacing(&spacing,QT_TRANSLATE_NOOP("ass","_Line spacing:"),0.10,10.0);
    diaElemFloat      dScale(&scale,QT_TRANSLATE_NOOP("ass","_Font scale:"),0.10,10.0);
    diaElemUInteger   dTop(PX(topMargin),QT_TRANSLATE_NOOP("ass","_Top margin:"),0,200);
    diaElemUInteger   dBottom(PX(bottomMargin),QT_TRANSLATE_NOOP("ass","Botto_m margin:"),0,200);
    diaMenuEntry      aspect[]={{SQUARE_PIXELS,QT_TRANSLATE_NOOP("ass","Do not adjust")},{STANDARD,"4:3"},{WIDE,"16:9"},{UNI,"18:9"},{CINEMA,"64:27"}};
    diaElemMenu       menuAspect(PX(displayAspectRatio),QT_TRANSLATE_NOOP("ass","Adjust for display aspect ratio:"),5,aspect,"");

    diaElem *elems[6]={&file,&dSpacing,&dScale,&dTop,&dBottom,&menuAspect};
again:
    if(!diaFactoryRun(QT_TRANSLATE_NOOP("ass","ASS"),6,elems))
        return false;
   
    const char *p=param.subtitleFile.c_str();
    int l=strlen(p);
    if(l>3 && !strcasecmp(p+l-4,".srt"))
    {
        if(!GUI_Question(QT_TRANSLATE_NOOP("ass","This is a SRT file. Convert to SSA ?")))
        {
            goto again;
        }
        ADM_subtitle sub;
        if(!sub.load(p))
        {
            GUI_Error_HIG(QT_TRANSLATE_NOOP("ass","Error"), QT_TRANSLATE_NOOP("ass","Cannot load this SRT file."));
            goto again;
        }
        if(false==sub.srt2ssa())
        {
            GUI_Error_HIG(QT_TRANSLATE_NOOP("ass","Error"), QT_TRANSLATE_NOOP("ass","Cannot convert to SSA."));
            goto again;               
        }

        char *newName=(char *)admAlloca(l+1);
        strcpy(newName,p);
        strcpy(newName+l-4,".ssa");
        if(false==sub.saveAsSSA(newName, previousFilter->getInfo()->width, previousFilter->getInfo()->height))
        {
            GUI_Error_HIG(QT_TRANSLATE_NOOP("ass","Error"), QT_TRANSLATE_NOOP("ass","Cannot save converted file."));
            goto again;                              
        }
        param.subtitleFile=std::string(newName);
    }


#undef MKME
#define MKME(x,y) param.y=(float)x
     MKME(scale,font_scale);
     MKME(spacing,line_spacing);
     cleanup();
     setup();
     return true; 
}

/**
    \fn cleanup
*/
bool subAss::cleanup(void)
{
        if(_ass_rend)
        {
              ass_renderer_done(_ass_rend);
              _ass_rend = NULL;
         }

        if(_ass_track)
        {
              ass_free_track(_ass_track);
              _ass_track = NULL;
        }
        if(_ass_lib)
        {
              ass_library_done(_ass_lib);
              _ass_lib = NULL;
        }
        return true;
}
/**
    \fn setup
*/
bool subAss::setup(void)
{
bool use_margins = ( param.topMargin | param.bottomMargin ) != 0;

        // update outpur image size
        memcpy(&info,previousFilter->getInfo(),sizeof(info));
        uint32_t origHeight=info.height;
        info.height += param.topMargin + param.bottomMargin;
        
        bool warn;
        if(!prefs->get(DEFAULT_WARN_FOR_FONTS,&warn))
            warn=true;
        if(warn)
        {
            GUI_Info_HIG(ADM_LOG_INFO,QT_TRANSLATE_NOOP("ass","Fonts"), QT_TRANSLATE_NOOP("ass","Preparing the fonts can take a few minutes the first time.\nThis message will not be displayed again."));
            prefs->set(DEFAULT_WARN_FOR_FONTS,false);
        }
        
        _ass_lib=ass_library_init();
        ADM_assert(_ass_lib);

/*        ass_set_fonts_dir(_ass_lib, (const char*)param.fontDirectory);*/
/*        ass_set_extract_fonts(_ass_lib, param.extractEmbeddedFonts);*/
        ass_set_style_overrides(_ass_lib, NULL);
        _ass_rend = ass_renderer_init(_ass_lib);

        ADM_assert(_ass_rend);

        ass_set_frame_size(_ass_rend, info.width, info.height);
        ass_set_margins(_ass_rend, param.topMargin, param.bottomMargin, 0, 0);
        ass_set_use_margins(_ass_rend, use_margins);
        ass_set_font_scale(_ass_rend, param.font_scale);
        ass_set_line_spacing(_ass_rend, param.line_spacing);
//ASS_Renderer *priv, const char *default_font, const char *default_family, int fc, const char *config,                   int update);
        int fc=0;
#ifdef USE_FONTCONFIG
        fc=1;
#endif
        ass_set_fonts(_ass_rend, NULL, "Sans",fc,NULL,true);
        double par=1.;
        if(param.displayAspectRatio)
        {
            par = (double)info.width / (double)origHeight;
            par *= (double)info.height / (double)origHeight;
            switch(param.displayAspectRatio)
            {
                case 1:
                    par*=3.;
                    par/=4.;
                    break;
                case 2:
                    par*=9.;
                    par/=16.;
                    break;
                case 3:
                    par/=2.;
                    break;
                case 4:
                    par*=27.;
                    par/=64.;
                    break;
            }
        }
        ass_set_pixel_aspect(_ass_rend, par);
       _ass_track = ass_read_file(_ass_lib, (char *)param.subtitleFile.c_str(), NULL);
        if(!_ass_track)
          GUI_Error_HIG(QT_TRANSLATE_NOOP("ass","SSA Error"),QT_TRANSLATE_NOOP("ass","ass_read_file() failed for %s"),param.subtitleFile.c_str());
        return 1;
}

//*******************************************
#define _r(c)  ((c)>>24)
#define _g(c)  (((c)>>16)&0xFF)
#define _b(c)  (((c)>>8)&0xFF)
#define _a(c)  ((c)&0xFF)
#define rgba2y(c)  ( (( 263*_r(c)  + 516*_g(c) + 100*_b(c)) >> 10) + 16  )
#define rgba2u(c)  ( (( 450*_r(c) - 376*_g(c) -  73*_b(c)) >> 10) + 128 )
#define rgba2v(c)  ( ((-152*_r(c) - 298*_g(c) + 450*_b(c)) >> 10) + 128 )
/**
    \fn nextFrame
*/
static bool blacken(ADMImage *src, uint32_t lineStart, uint32_t howto)
{
        for(int i=0;i<3;i++)
            {
                uint32_t w=src->_width;
                uint32_t h=src->_height;
                uint8_t filler=16;
                uint32_t count=howto;
                uint32_t lineOffset=lineStart;
                if(i) {w>>=1;h>>=1;filler=128;count>>=1;lineOffset>>=1;}
                ADM_PLANE plane=(ADM_PLANE)i;

                uint8_t *dy=src->GetWritePtr(plane);
                uint32_t dpitch=src->GetPitch(plane);

                dy+=dpitch*lineOffset;

                for(int y=0;y<count;y++)
                {
                    memset(dy,filler,w);
                    dy+=dpitch;
                 }
            }
            return true;
}
/**
 * \fn mergeOneImage
 * \brief merge one partial sub into the final image
 * @param img
 * @param target
 * @return 
 */

static int clipWindow(int original, int offset,int targetSize)
{
    int r=  original;
    int tail=offset+original;
    if(tail > targetSize)
    {
        r=targetSize-offset;
    }
    return r;
}

bool subAss::mergeOneImage(ASS_Image *img,ADMImage *target)
{
    uint8_t y, u, v, opacity;
    uint32_t  j, k, l, val;
    int32_t orig_u, orig_v,klong,newu,newv;
    uint8_t orig_y;
    uint8_t *bitmap, *ydata, *udata, *vdata;

    
    
    //  printf("Image is %d x %d \n",img->w, img->h);
    y = rgba2y(img->color);
    u = rgba2u(img->color);
    v = rgba2v(img->color);

    opacity = 255 - _a(img->color);

    uint8_t *planes[3];
    int      pitches[3];

    target->GetPitches(pitches);
    target->GetWritePlanes(planes);

    uint32_t x=img->dst_x;
    ydata = planes[0]+pitches[0]*(img->dst_y)+x;

    x>>=1;
    udata = planes[1]+pitches[1]*(img->dst_y/2)+x;
    vdata = planes[2]+pitches[2]*(img->dst_y/2)+x;
   
    
    
    bitmap = img->bitmap;    
    
    int topH,topW;
    
    // -- clip height --
    topH=clipWindow(img->h, img->dst_y, target->_height);
    if(topH<0)
    {
        ADM_warning("Subtitle outside of video-h\n");
        return false;
    }
    //-- clip width--
    topW=clipWindow(img->w, img->dst_x,target->_width);    
    if(topW<0)
    {
        ADM_warning("Subtitle outside of video-w\n");
        return false;
    }
    
    
    // now go
    for(int i = 0; i < topH; ++i)
    {
            for(j = 0; j < topW; ++j)
            {
                    k = *(bitmap+j) * opacity / 255; // Alpha Channel
                    orig_y = *(ydata+j);
                    *(ydata+j) = (k*y + (255-k)*orig_y) / 255;
            }

            bitmap += img->stride;
            ydata += pitches[0];
    }
    // Now do u & v
    bitmap = img->bitmap;

    newu=u-128;
    newv=v-128;

    for(int i = 0; i < (topH-1); i += 2)
    {
            for(j = 0, l = 0; j < (topW-1); j += 2, ++l)
            {
                    val = 0;
                    val += *(bitmap + j);
                    val += *(bitmap + j + 1);
                    val += *(bitmap + img->stride + j);
                    val += *(bitmap + img->stride + j + 1);
                    val >>= 2;

                    k = val * opacity / 255;
                    orig_u = *(udata+l);
                    orig_v = *(vdata+l);

                    orig_u=( k*u+(255-k)*orig_u)/255;
                    orig_v=( k*v+(255-k)*orig_v)/255;
                    *(udata+l) = orig_u;
                    *(vdata+l) = orig_v;
            }

            bitmap += img->stride << 1;
            udata += pitches[1];
            vdata += pitches[2];
    }
    return true;
}

/**
    \fn getNextFrame
*/
bool subAss::getNextFrame(uint32_t *fn,ADMImage *image)
{
    if(!previousFilter->getNextFrame(fn,src))
    {
        ADM_info("[blackenBorder] Cannot get previous image\n");
        return false;
    }

        
        


       /* copy source to image */

        src->copyTo(image,0,param.topMargin);

        /* Add black border if needed */
        if(param.topMargin)
            blacken(image, 0, param.topMargin);
        if(param.bottomMargin)
            blacken(image, src->_height+param.topMargin, param.bottomMargin);

        image->copyInfo(src); // pts etc..
        // Do we have something to render ?
        if(!_ass_rend || !_ass_track || !_ass_lib)
        {
          printf("[Ass] No sub to render\n");
          return true;
        }

        int changed=0;
        int64_t now=previousFilter->getAbsoluteStartTime()+src->Pts;
        now/=1000; // Ass works in ms
        ASS_Image *img = ass_render_frame(_ass_rend, _ass_track, now,&changed);
        //printf("Time is now %d ms\n",now);

        while(img) 
        {
            mergeOneImage(img,image);
            img = img->next;
        }
        return true;
}

/************************************************/


