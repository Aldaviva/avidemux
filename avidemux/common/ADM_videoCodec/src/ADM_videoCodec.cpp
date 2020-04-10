/***************************************************************************
        \file ADM_videoCodec
        \brief Search and instantiate video coder
        \author mean fixounet@free.fr (C) 2010

    see here : http://www.webartz.com/fourcc/

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
#include "ADM_codec.h"
#include "ADM_ffmp43.h"
#include "fourcc.h"
#include "DIA_coreToolkit.h"

#if defined(USE_VPX)
#include "ADM_vpx.h"
#endif


extern bool vdpauUsable(void);
extern bool xvbaUsable(void);
extern bool libvaUsable(void);

decoders *tryCreatingVideoDecoder(uint32_t w, uint32_t h, uint32_t fcc,uint32_t extraDataLen,
                    uint8_t *extra, uint32_t bpp);
/**
    \fn getDecoder
    \brief returns the correct decoder for a stream w,h,fcc,extraLen,extraData,bpp
*/
decoders *ADM_getDecoder (uint32_t fcc, uint32_t w, uint32_t h, uint32_t extraLen, 
            uint8_t * extraData,uint32_t bpp)
{
    ADM_info("Searching decoder in plugins\n");
    decoders *fromPlugin=tryCreatingVideoDecoder(w,h,fcc,extraLen,extraData,bpp);
    if(fromPlugin) 
        return fromPlugin;
    ADM_info("Searching decoder in coreVideoCodec(%d x %d, extradataSize:%d)...\n",w,h,extraLen);
    return ADM_coreCodecGetDecoder(fcc,w,h,extraLen,extraData,bpp);
}
//EOF

