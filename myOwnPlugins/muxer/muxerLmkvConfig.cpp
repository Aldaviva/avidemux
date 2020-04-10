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
#if 0
#include "ADM_default.h"
#include "ADM_muxerInternal.h"
#include "fourcc.h"
#include "muxerMp4v2.h"
#include "mp4v2_muxer.h"
#include "DIA_factory.h"
extern mp4v2_muxer muxerConfig;
/**
    \fn mp4v2Configure
*/
bool mp4v2Configure(void)
{
        bool optimize=muxerConfig.optimize;
        bool addItuneMetaData=muxerConfig.add_itunes_metadata;

        //diaElemToggle   wOptimize(&optimize,"Optimize for streaming");
        diaElemToggle   wItunes(&addItuneMetaData,"Add ipod metadata");

        //diaElem *tabs[]={&wOptimize,&wItunes};
        diaElem *tabs[]={&wItunes};
        if( diaFactoryRun(("MP4V2 Settings"),1,tabs))
        {
            //muxerConfig.optimize=optimize;
            muxerConfig.optimize=false;
            muxerConfig.add_itunes_metadata=addItuneMetaData;
            return true;
        }
        return false;
}
#endif
// EOF
