/***************************************************************************
                          \file MuxerRaw
                             -------------------
    
    copyright            : (C) 2009 by mean
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

#include "ADM_muxer.h"
#include "raw_muxer.h"

extern raw_muxer muxerConfig;

class muxerRaw : public ADM_muxer
{
protected:
                FILE *file;
public:
                muxerRaw();
        virtual ~muxerRaw();
        virtual bool open(const char *file, ADM_videoStream *s,uint32_t nbAudioTrack,ADM_audioStream **a);
        virtual bool save(void) ;
        virtual bool close(void) ;
        virtual bool preferH264AnnexB(void) { return muxerConfig.requestAnnexB; }
};

