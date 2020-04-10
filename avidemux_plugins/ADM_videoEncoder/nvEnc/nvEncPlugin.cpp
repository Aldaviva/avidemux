/***************************************************************************
                          \fn     jpegPlugin
                          \brief  Plugin for jpeg dummy encoder
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

#include "ADM_default.h"
#include "ADM_nvEnc.h"
#include "nvenc_desc.cpp"
#include "ADM_coreVideoEncoderInternal.h"
extern bool            nvEncConfigure(void);
extern nvencconf       NvEncSettings;

void resetConfigurationData()
{
	nvencconf defaultConf = NVENC_CONF_DEFAULT;

	memcpy(&NvEncSettings, &defaultConf, sizeof(defaultConf));
}

ADM_DECLARE_VIDEO_ENCODER_PREAMBLE(ADM_nvEncEncoder);
ADM_DECLARE_VIDEO_ENCODER_MAIN("NvEnc",
                               "H264 (nvidia)",
                               "Nvidia hw encoder",
                                nvEncConfigure, // No configuration
                                ADM_UI_ALL,
                                1,0,0,
                                nvencconf_param, // conf template
                                &NvEncSettings,NULL,NULL // conf var
);
