/***************************************************************************
                          ADM_vidAddBorder.h  -  description
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
#pragma once
#include "blackenBorder.h"
class blackenBorders : public  ADM_coreVideoFilter
{
protected:
        blackenBorder       param;
public:
                            blackenBorders(ADM_coreVideoFilter *previous,CONFcouple *conf);
                            ~blackenBorders();

        virtual const char   *getConfiguration(void);                   /// Return  current configuration as a human readable string
        virtual bool         getNextFrame(uint32_t *fn,ADMImage *image);    /// Return the next image
	 //  virtual FilterInfo  *getInfo(void);                             /// Return picture parameters after this filter
        virtual bool         getCoupledConf(CONFcouple **couples) ;   /// Return the current filter configuration
        virtual void         setCoupledConf(CONFcouple *couples);
        virtual bool         configure(void) ;           /// Start graphical user interface
};


DECLARE_VIDEO_FILTER_PARTIALIZABLE(   blackenBorders,   // Class
                        1,0,0,              // Version
                        ADM_UI_TYPE_BUILD,         // UI
                        VF_TRANSFORM,            // Category
                        "blackenBorder",            // internal name (must be uniq!)
                        QT_TRANSLATE_NOOP("blacken","Blacken Borders"),            // Display name
                        QT_TRANSLATE_NOOP("blacken","Remove noisy edge by turning them to black.") // Description
                    );

// EOF

