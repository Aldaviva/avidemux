/***************************************************************************
                          ADM_vidAddBorder.cpp  -  description
                             -------------------
    begin                : Sun Aug 11 2002
    copyright            : (C) 2002 by mean
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
#include "ADM_coreVideoFilter.h"
#include "DIA_coreToolkit.h"
#include "ADM_vidAddBorder.h"
#include "DIA_factory.h"
#include "addBorder_desc.cpp"

/**
    \fn getConfiguration
    \brief Return current setting as a string
*/
const char *addBorders::getConfiguration(void)
{
    static char conf[250];
    conf[0]=0;
    snprintf(conf,80,"Add Border : Left:%" PRIu32" Right:%" PRIu32" Top:%" PRIu32" Bottom:%" PRIu32" => %" PRIu32"x%" PRIu32"\n",
                param.left,param.right,param.top,param.bottom,
                info.width,info.height);
    return conf;
}
/**
    \fn ctor
*/
addBorders::addBorders( ADM_coreVideoFilter *in,CONFcouple *setup) : ADM_coreVideoFilter(in,setup)
{
    if(!setup || !ADM_paramLoad(setup,addBorder_param,&param))
    {
        // Default value
        param.left=0;
        param.right=0;
        param.top=0;
        param.bottom=0;

    }
    info.width=in->getInfo()->width+param.left+param.right;
    info.height=in->getInfo()->height+param.top+param.bottom;

}
/**
    \fn dtor
*/
addBorders::~addBorders()
{

}

/**
    \fn getCoupledConf
    \brief Return our current configuration as couple name=value
*/
bool         addBorders::getCoupledConf(CONFcouple **couples)
{
    return ADM_paramSave(couples, addBorder_param,&param);
}

void addBorders::setCoupledConf(CONFcouple *couples)
{
    ADM_paramLoad(couples, addBorder_param, &param);
}
#define Y_BLACK 16
#define UV_BLACK 128
static bool blackenHz(int w,int nbLine,uint8_t *ptr[3],int strides[3])
{
    // y
    uint8_t *p=ptr[0];
    uint32_t s=strides[0];
    for(int y=0;y<nbLine;y++)
    {
        memset(p,Y_BLACK,w);
        p+=s;
    }
    p=ptr[1];
    s=strides[1];
    nbLine/=2;
    w/=2;
    for(int y=0;y<nbLine;y++)
    {
        memset(p,UV_BLACK,w);
        p+=s;
    }
    p=ptr[2];
    s=strides[2];
    for(int y=0;y<nbLine;y++)
    {
        memset(p,UV_BLACK,w);
        p+=s;
    }
    return true;
}

/**
    \fn getNextFrame
*/
bool addBorders::getNextFrame(uint32_t *fn,ADMImage *image)
{
    uint32_t smallWidth=previousFilter->getInfo()->width;
    uint32_t smallHeight=previousFilter->getInfo()->height;

    ADMImageRefWrittable ref(smallWidth,smallHeight);

    image->GetWritePlanes(ref._planes);
    image->GetPitches(ref._planeStride);

    uint32_t offset=param.top*image->GetPitch(PLANAR_Y);
    ref._planes[0]+=param.left+offset;

    offset=(param.top>>1)*image->GetPitch(PLANAR_U);
    ref._planes[1]+=(param.left>>1)+offset;

    offset=(param.top>>1)*image->GetPitch(PLANAR_V);
    ref._planes[2]+=(param.left>>1)+offset;


    if(false==previousFilter->getNextFrame(fn,&ref))
    {
        ADM_warning("FlipFilter : Cannot get frame\n");
        return false;
    }
    // Now do fill

    // Top...
    uint8_t *ptr[3];
    int     stride[3];
    image->GetPitches(stride);
    image->GetWritePlanes(ptr);
    blackenHz(image->_width,param.top,ptr,stride);
    // Left
    blackenHz(param.left,image->_height,ptr,stride);
    // Right
    ptr[0]+=param.left+smallWidth;
    ptr[1]+=(param.left+smallWidth)/2;
    ptr[2]+=(param.left+smallWidth)/2;
    blackenHz(param.right,image->_height,ptr,stride);
    // Bottom
    image->GetPitches(stride);
    image->GetWritePlanes(ptr);
    uint32_t offsetLine=smallHeight+param.top;
    ptr[0]+=offsetLine*stride[0];
    ptr[1]+=(offsetLine/2)*stride[1];
    ptr[2]+=(offsetLine/2)*stride[2];
    blackenHz(image->_width,param.bottom,ptr,stride);
    // Copy info
    image->copyInfo(&ref);
    return true;
}

/**
    \fn configure
*/
bool addBorders::configure(void)
{
        uint32_t width,height;
#define MAKEME(x) uint32_t x=param.x;
        while(1)
        {
          MAKEME(left);
          MAKEME(right);
          MAKEME(top);
          MAKEME(bottom);

          width=previousFilter->getInfo()->width;
          height=previousFilter->getInfo()->height;

#define BORDER_MAX_WIDTH 2160

          diaElemUInteger dleft(&left,QT_TRANSLATE_NOOP("addBorder", "_Left border:"),        0,BORDER_MAX_WIDTH);
          diaElemUInteger dright(&right,QT_TRANSLATE_NOOP("addBorder", "_Right border:"),     0,BORDER_MAX_WIDTH);
          diaElemUInteger dtop(&(top),QT_TRANSLATE_NOOP("addBorder", "_Top border:"),         0,BORDER_MAX_WIDTH);
          diaElemUInteger dbottom(&(bottom),QT_TRANSLATE_NOOP("addBorder", "_Bottom border:"),0,BORDER_MAX_WIDTH);

          diaElem *elems[4]={&dleft,&dright,&dtop,&dbottom};
          if(diaFactoryRun(QT_TRANSLATE_NOOP("addBorder", "Add Borders"),4,elems))
          {
            if((left&1) || (right&1)|| (top&1) || (bottom&1))
            {
              GUI_Error_HIG(QT_TRANSLATE_NOOP("addBorder", "Incorrect parameters"),QT_TRANSLATE_NOOP("addBorder", "All parameters must be even and within range."));
              continue;
            }
            else
            {
  #undef MAKEME
  #define MAKEME(x) param.x=x;
                MAKEME(left);
                MAKEME(right);
                MAKEME(top);
                MAKEME(bottom);
                info.width=width+left+right;
                info.height=height+top+bottom;
                return 1;
            }
          }
          return 0;
      }
}




