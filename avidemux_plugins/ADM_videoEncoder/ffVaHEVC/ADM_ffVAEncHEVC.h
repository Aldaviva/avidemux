/***************************************************************************
                          \fn ADM_VideoEncoders
                          \brief Internal handling of video encoders
                             -------------------

    copyright            : (C) 2002/2009 by mean
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
#include "ADM_coreLibVA/ADM_coreLibVA.h"
#include "ADM_coreVideoEncoderFFmpeg.h"
#include "ffVAEnc_HEVC.h"



#define VAENC_HEVC_CONF_DEFAULT \
{ \
    100, \
    2, \
    2500, \
    5000 \
}



/**
        \class ADM_ffVAEncH264Encoder
        \brief

*/
class ADM_ffVAEncHEVC : public ADM_coreVideoEncoderFFmpeg
{
protected:

public:

                           ADM_ffVAEncHEVC(ADM_coreVideoFilter *src, bool globalHeader);
virtual                    ~ADM_ffVAEncHEVC();
virtual        bool        configureContext(void);
virtual        bool        setup(void);
virtual        bool        encode (ADMBitstream * out);
virtual const  char        *getFourcc(void) {return "HEVC";}
virtual        uint64_t    getEncoderDelay(void);
virtual        bool        isDualPass(void) ;

protected:
                AVBufferRef *hwDeviceCtx;
                AVFrame    *swFrame;
                AVFrame    *hwFrame;
                bool       preEncode(void);
                int        encodeWrapper(AVFrame *in, ADMBitstream *out);
};
