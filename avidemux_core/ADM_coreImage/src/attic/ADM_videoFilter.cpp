/***************************************************************************
                          ADM_genvideo.cpp  -  description
                             -------------------
    begin                : Sun Apr 14 2002
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
//#include "ADM_editor/ADM_edit.hxx"
#include "ADM_videoFilter.h"

uint8_t AVDMGenericVideoStream::getPixel(int32_t x,int32_t y,uint8_t *data)
{
		if(x>(int32_t)_info.width)
  			{
            	x=2*_info.width-x;
       	}
       if(y>(int32_t)_info.height)
  			{
            	y=2*_info.height-y;
       	}
        if(   x<0)
          x=-x;
        if(   y<0)
          y=-y;

          return *(data+x+(y*_info.width));
}

uint32_t   AVDMGenericVideoStream::getPARWidth(void)
{
  if(_in)
  {
    return _in-> getPARWidth();
  }
  return 1;
}
/* If there is a father, retrieve PAR from him else assume it is 1:1 */
uint32_t   AVDMGenericVideoStream::getPARHeight(void)
{
  if(_in)
  {
    return _in-> getPARHeight();
  }
  return 1;
}


uint8_t AVDMGenericVideoStream::getPixelU(int32_t x,int32_t y,uint8_t *data)
{
int32_t w=_info.width>>1;
int32_t h=_info.height>>1;

			x=x>>1;
			y=y>>1;

		if(x>w)
  			{
            	x=2*w-x;
       	}
       if(y>h)
  			{
            	y=2*h-y;
       	}
        if(   x<0)
          x=-x;
        if(   y<0)
          y=-y;

          return *(data+x+(y*w));


}
uint8_t AVDMGenericVideoStream::setPixelU(uint8_t val,int32_t x,int32_t y,uint8_t *data)
{
int32_t w=_info.width>>1;
int32_t h=_info.height>>1;

			x=x>>1;
			y=y>>1;

		if(x>w)
  			{
            	x=2*w-x;
       	}
       if(y>h)
  			{
            	y=2*h-y;
       	}
        if(   x<0)
          x=-x;
        if(   y<0)
          y=-y;

           *(data+x+(y*w))=val;
           return 1;


}
uint8_t AVDMGenericVideoStream::unPackChroma(uint8_t *ssrc,uint8_t *ddst)
{
uint8_t *src,*dst,*srcu,*srcv; //,*dstu,*dstv;
	src=ssrc;
 	dst=ddst;
   	// unpack   luma
				for(uint32_t l=_info.width*_info.height;l>0;l--)
    				{
                   	*dst++=*src++;
                    	dst+=2;
          			}
         	// unpack   chroma

	          srcu=ssrc+_info.width*_info.height;
       		srcv=srcu+((_info.width*_info.height)>>2);
         		dst=ddst+1;

             for(int32_t y=0;y<(int32_t)(_info.height >>1);y++)
           	{
		         		for(int32_t x=0;x<(int32_t)(_info.width );x++)
             			{

                 			*dst=*(dst+3*_info.width)=*srcu;
                    		dst++;
                 			*dst=*(dst+3*_info.width)=*srcv;
                    		dst+=2;
                    		if(x&1) srcu++;
                    		if(x&1) srcv++;

                  	}
                   dst+=_info.width*3;
               }
          return 1;
}

// EOF
