/***************************************************************************
                          \file  audiofilter_normalize
                          \brief absolute or automatic gain filter
                             -------------------
    
    copyright            : (C) 2002/2009 by mean
    email                : fixounet@free.fr
    
    Compute the ratio so that the maximum is at -3db
    
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
#include "ADM_audioFilter.h"
#include "audiofilter_normalize_param.h"
#include "audiofilter_normalize.h"
#include "audiofilter_dolby.h"

#include "ADM_coreAudio.h"

#if defined (_WIN32) || defined (__HAIKU__)
#define POW10(x)   pow(10,x)
#elif defined(ADM_BSD_FAMILY) || defined(__sun__)
#define POW10(x) powf(10.0,x)
#else
#define POW10(x) exp10f(x)
#endif

#define LINEAR_TO_DB(x) (20.*log10(x))
#define DB_TO_LINEAR(x) (POW10((x/20.)))

static int32_t max_level_x10;

/**
        \fn Ctor
**/

AUDMAudioFilterNormalize::AUDMAudioFilterNormalize(AUDMAudioFilter * instream,GAINparam *param):AUDMAudioFilter (instream)
{
  float db_out;
    // nothing special here...
  switch(param->mode)
  {
    case ADM_NO_GAIN: _ratio=1;_scanned=1;ADM_info("[Gain] Gain of 1.0\n");break; 
    case ADM_GAIN_AUTOMATIC:
                _ratio=1;
                _scanned=0;
                max_level_x10 = param->maxlevel10;
                ADM_info("[Gain] Automatic gain, max level %.1f\n",max_level_x10/10.0);
                break;
    case ADM_GAIN_MANUAL: 
                _scanned=1;
                db_out =  param->gain10/10.0; // Dbout is in 10*DB (!)
                _ratio = DB_TO_LINEAR(db_out);
                ADM_info("[Gain] %f db (p=%d)\n", (float)(param->gain10)/10.,param->gain10);
                ADM_info("[Gain] Linear ratio of : %03.3f\n", _ratio);
                break;
    default:
                ADM_error("Unknown normalize mode\n");
                ADM_assert(0);
                break;
  }
    _previous->rewind();
};
/**
        \fn dtor
*/
AUDMAudioFilterNormalize::~AUDMAudioFilterNormalize()
{
        ADM_info("Destroying normalize audio filter\n");
}
/**
        \fn preprocess
        \brief Search for max value to compute gain
*/
uint8_t AUDMAudioFilterNormalize::preprocess(void)
{


    uint32_t scanned = 0;
    AUD_Status status;
    _ratio = 0;

    float *max=new float[_wavHeader.channels];
    _previous->rewind();
    ADMDolbyContext::DolbySkip(1); 
    ADM_info("Seeking for maximum value, that can take a while\n");

      for(int i=0;i<_wavHeader.channels;i++) max[i]=0;
      while (1)
      {
          int ready=_previous->fill(AUD_PROCESS_BUFFER_SIZE>>2,_incomingBuffer.at(0),&status);
          if(!ready)
          {
            if(status==AUD_END_OF_STREAM) 
            {
              break; 
            }
           else 
            {
              ADM_error("Unknown cause : %d\n",status);
              ADM_assert(0); 
            }
          }
          ADM_assert(!(ready %_wavHeader.channels));
          
          int index=0;
          float current;
          
        //  printf("*\n");
          int sample= ready /_wavHeader.channels;
          for(int j=0;j<sample;j++)
            for(int chan=0;chan<_wavHeader.channels;chan++)
          {
            current=fabs(_incomingBuffer[index++]);
            if(current>max[chan]) max[chan]=current;
          }
          scanned+=ready;
      }
      
      
      

    _previous->rewind();
    float mx=0;
    for(int chan=0;chan<_wavHeader.channels;chan++)
    {
        if(max[chan]>mx) mx=max[chan];
        ADM_info("[Normalize] maximum found for channel %d : %f\n", chan,max[chan]);
    }
    ADM_info("[Normalize] Using : %0.4f as max value \n", mx);
    double db_in, db_out=max_level_x10/10.0;

    if (mx>0.001)
      db_in = LINEAR_TO_DB(mx);
    else
      db_in = -20; // We consider -20 DB to be noise

    printf("--> %2.2f db / %2.2f \n", db_in, db_out);

    // search ratio
    _ratio=1;

    float db_delta=db_out-db_in;
    ADM_info("[Normalize]Gain %f dB\n",db_delta);
    _ratio = DB_TO_LINEAR(db_delta);
    ADM_info("\n Using ratio of : %f\n", _ratio);

    _scanned = 1;
    ADMDolbyContext::DolbySkip(0);
    _previous->rewind();
    delete [] max;
    return 1;
}
/**
        \fn fill
        \brief
*/
uint32_t AUDMAudioFilterNormalize::fill( uint32_t max, float * buffer,AUD_Status *status)
{
    uint32_t rd, i;

    *status=AUD_OK;
    if(!_scanned) preprocess();
    rd = _previous->fill(max, _incomingBuffer.at(0),status);
    if(!rd)
    {
      if(*status==AUD_END_OF_STREAM) return 0;
      ADM_assert(0);
    }
    float tmp;
    for (i = 0; i < rd; i++)
    {
      tmp=_incomingBuffer[i];
      tmp*=_ratio;
      buffer[i]=tmp;
    }
    return rd;
};
//EOF

