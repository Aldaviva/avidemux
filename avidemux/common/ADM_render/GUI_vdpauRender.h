
/***************************************************************************
    Class to handle Xv accelerated renderer
    
    copyright            : (C) 2006 by mean
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
#pragma once
/**
    \fn class XvRender
*/
class vdpauRender: public VideoRenderBase
{
      protected:
                            GUI_WindowInfo info;
                        bool cleanup(void);
                        bool reallocOutputSurface(uint32_t tgtWidth, uint32_t tgtHeight);
      public:
                             vdpauRender( void ) ;
                             ~vdpauRender();
              virtual	bool init( GUI_WindowInfo *window, uint32_t w, uint32_t h, float zoom);
              virtual	bool stop(void);
              virtual   bool displayImage(ADMImage *pic);
              virtual   bool changeZoom(float newzoom);
              virtual   bool refresh(void);
              virtual   bool usingUIRedraw(void) {return false;}; // we can redraw by ourself
              virtual   ADM_HW_IMAGE getPreferedImage(void ) {return ADM_HW_VDPAU;}
                        const char *getName() {return "VDPAU";}
};





