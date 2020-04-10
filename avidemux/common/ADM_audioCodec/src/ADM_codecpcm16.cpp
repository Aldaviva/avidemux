/***************************************************************************
                          ADM_codecwav.cpp  -  description
                             -------------------
    begin                : Fri May 31 2002
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
#include <math.h>
#include "ADM_audiocodec.h"

ADM_AudiocodecWav::ADM_AudiocodecWav( uint32_t fourcc,const WAVHeader &info ) : ADM_Audiocodec(fourcc,info)
{
    channelMapping[0] = ADM_CH_FRONT_LEFT;
    channelMapping[1] = ADM_CH_FRONT_RIGHT;
    channelMapping[2] = ADM_CH_FRONT_CENTER;
    channelMapping[3] = ADM_CH_REAR_LEFT;
    channelMapping[4] = ADM_CH_REAR_RIGHT;
    channelMapping[5] = ADM_CH_LFE;
    ADM_info("Creating not swapped wav decoder (PCM)\n");
}
ADM_AudiocodecWav::~ADM_AudiocodecWav()
{

}

uint8_t ADM_AudiocodecWav::isCompressed( void )
{
 	return 0;
}

uint8_t ADM_AudiocodecWav::run(uint8_t * inptr, uint32_t nbIn, float * outptr, uint32_t * nbOut)
{
	int16_t *in = (int16_t *) inptr;

	*nbOut=nbIn / 2;
	for (int i = 0; i < *nbOut; i++) {
		*(outptr++) = (float)*in / 32768;
		in++;
	}

	return 1;
}


