/***************************************************************************
                          \fn ADM_ffMsMpeg4
                          \brief Front end for libavcodec MsMpeg4 aka divx3 encoder
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
#include "ADM_ffMsMp4.h"
#undef ADM_MINIMAL_UI_INTERFACE // we need the full UI
#include "DIA_factory.h"

#if 1
#define aprintf(...) {}
#else
#define aprintf printf
#endif

FFcodecSettings MsMp4Settings=
{

    {
    COMPRESS_CQ, //COMPRESSION_MODE  mode;
    2,              // uint32_t          qz;           /// Quantizer
    1500,           //uint32_t          bitrate;      /// In kb/s 
    700,            //uint32_t          finalsize;    /// In ?
    1500,           //uint32_t          avg_bitrate;  /// avg_bitrate is in kb/s!!
    ADM_ENC_CAP_CBR+ADM_ENC_CAP_CQ+ADM_ENC_CAP_2PASS+ADM_ENC_CAP_2PASS_BR+ADM_ENC_CAP_GLOBAL+ADM_ENC_CAP_SAME
    },
    {
          ADM_AVCODEC_SETTING_VERSION,
          0, // MT
          ME_EPZS,			// ME
          0,				// GMC     
          0,				// 4MV
          0,				// _QPEL;   
          1,				// _TREILLIS_QUANT
          2,				// qmin;
          31,				// qmax;
          3,				// max_qdiff;
          0,				// max_b_frames;
          0,				// mpeg_quant;
          1,				// is_luma_elim_threshold
          -2,				// luma_elim_threshold;
          1,				// is_chroma_elim_threshold
          -5,				// chroma_elim_threshold;
          0.05,				//lumi_masking;
          1,				// is lumi
          0.01,				//dark_masking; 
          1,				// is dark
          0.5,				// qcompress amount of qscale change between easy & hard scenes (0.0-1.0
          0.5,				// qblur;    amount of qscale smoothing over time (0.0-1.0) 
          0,				// min bitrate in kB/S
          0,				// max bitrate
          0,				// user matrix
          250,				// no gop size
          0,				// interlaced
          0,				// WLA: bottom-field-first
          0,				// wide screen
          2,				// mb eval = distortion
          8000,				// vratetol 8Meg
          0,				// is temporal
          0.0,				// temporal masking
          0,				// is spatial
          0.0,				// spatial masking
          0,				// NAQ
          0,                // xvid rc
          0,                // buffersize
          0,                // override ratecontrol
          0				    // DUMMY 
    }
};
/**
        \fn ADM_ffMsMp4Encoder
*/
ADM_ffMsMp4Encoder::ADM_ffMsMp4Encoder(ADM_coreVideoFilter *src,bool globalHeader) : ADM_coreVideoEncoderFFmpeg(src,&MsMp4Settings)
{
    printf("[ffMsMP4] Creating.\n");
   

}

/**
    \fn pre-open
*/
bool ADM_ffMpeg4Encoder::configureContext(void)
{
    
    switch(Settings.params.mode)
    {
      case COMPRESS_2PASS:
      case COMPRESS_2PASS_BITRATE:
           if(false==setupPass())
            {
                printf("[ffMsMP4] Multipass setup failed\n");
                return false;
            }
            break;
      case COMPRESS_SAME:
      case COMPRESS_CQ:
            _context->flags |= CODEC_FLAG_QSCALE;
            _context->bit_rate = 0;
            break;
      case COMPRESS_CBR:
              _context->bit_rate=Settings.params.bitrate*1000; // kb->b;
            break;
     default:
            return false;
    }
    presetContext(&Settings);
    
    return true;
}

/**
    \fn setup
*/
bool ADM_ffMsMp4Encoder::setup(void)
{

    if(false== ADM_coreVideoEncoderFFmpeg::setup(CODEC_ID_MSMPEG4V3))
        return false;

    printf("[ffMsMP4] Setup ok\n");
    return true;
}


/** 
    \fn ~ADM_ffMsMp4Encoder
*/
ADM_ffMsMp4Encoder::~ADM_ffMsMp4Encoder()
{
    printf("[ffMsMP4] Destroying.\n");
   
    
}

/**
    \fn encode
*/
bool         ADM_ffMsMp4Encoder::encode (ADMBitstream * out)
{
int sz,q;
again:
    sz=0;
    if(false==preEncode()) // Pop - out the frames stored in the queue due to B-frames
    {
        return false;
    }
    q=image->_Qp;
    
    if(!q) q=2;
    switch(Settings.params.mode)
    {
      case COMPRESS_SAME:
                // Keep same frame type & same Qz as the incoming frame...
            _frame.quality = (int) floor (FF_QP2LAMBDA * q+ 0.5);
            if(image->flags & AVI_KEY_FRAME)    _frame.pict_type=FF_I_TYPE;
            else                                _frame.pict_type=FF_P_TYPE;

            break;
      case COMPRESS_2PASS:
      case COMPRESS_2PASS_BITRATE:
            switch(pass)
            {
                case 1: 
                        break;
                case 2: 
                        break; // Get Qz for this frame...
            }
      case COMPRESS_CQ:
            _frame.quality = (int) floor (FF_QP2LAMBDA * Settings.params.qz+ 0.5);
            break;
      case COMPRESS_CBR:
            break;
     default:
            printf("[ffMsMP4] Unsupported encoding mode\n");
            return false;
    }
    aprintf("[CODEC] Flags = 0x%x, QSCALE=%x, bit_rate=%d, quality=%d qz=%d incoming qz=%d\n",_context->flags,CODEC_FLAG_QSCALE,
                                     _context->bit_rate,  _frame.quality, _frame.quality/ FF_QP2LAMBDA,q);     
    
    _frame.reordered_opaque=image->Pts;
    int r=encodeWrapper(_frame,out);
    if(r<0)
    {
        ADM_warning("[ffMsMP4] Error %d encoding video\n",sz);
        return false;
    }
    return postEncode(out,r);
}

/**
    \fn isDualPass

*/
bool         ADM_ffMsMp4Encoder::isDualPass(void) 
{
    if(Settings.params.mode==COMPRESS_2PASS || Settings.params.mode==COMPRESS_2PASS_BITRATE ) return true;
    return false;

}

/**
    \fn jpegConfigure
    \brief UI configuration for jpeg encoder
*/

bool         ffMsMp4Configure(void)
{         
diaMenuEntry meE[]={
  {1,QT_TRANSLATE_NOOP("ffmsmpeg4","None")},
  {2,QT_TRANSLATE_NOOP("ffmsmpeg4","Full")},
  {3,QT_TRANSLATE_NOOP("ffmsmpeg4","Log")},
  {4,QT_TRANSLATE_NOOP("ffmsmpeg4","Phods")},
  {5,QT_TRANSLATE_NOOP("ffmsmpeg4","EPZS")},
  {6,QT_TRANSLATE_NOOP("ffmsmpeg4","X1")}
};       

diaMenuEntry qzE[]={
  {0,QT_TRANSLATE_NOOP("ffmsmpeg4","H.263")},
  {1,QT_TRANSLATE_NOOP("ffmsmpeg4","MPEG")}
};       

diaMenuEntry rdE[]={
  {0,QT_TRANSLATE_NOOP("ffmsmpeg4","MB comparison")},
  {1,QT_TRANSLATE_NOOP("ffmsmpeg4","Fewest bits (vhq)")},
  {2,QT_TRANSLATE_NOOP("ffmsmpeg4","Rate distortion")}
};     

        FFcodecSettings *conf=&MsMp4Settings;

uint32_t me=(uint32_t)conf->lavcSettings.me_method;  
#define PX(x) &(conf->lavcSettings.x)

         diaElemBitrate   bitrate(&(MsMp4Settings.params),NULL);
         diaElemUInteger  qminM(PX(qmin),QT_TRANSLATE_NOOP("ffmsmpeg4","Mi_n. quantizer:"),1,31);
         diaElemUInteger  qmaxM(PX(qmax),QT_TRANSLATE_NOOP("ffmsmpeg4","Ma_x. quantizer:"),1,31);
         diaElemUInteger  qdiffM(PX(max_qdiff),QT_TRANSLATE_NOOP("ffmsmpeg4","Max. quantizer _difference:"),1,31);
         diaElemToggle    trellis(PX(_TRELLIS_QUANT),QT_TRANSLATE_NOOP("ffmsmpeg4","_Trellis quantization"));
         diaElemUInteger filetol(PX(vratetol),QT_TRANSLATE_NOOP("ffmsmpeg4","_Filesize tolerance (kb):"),0,100000);
         
         diaElemFloat    qzComp(PX(qcompress),QT_TRANSLATE_NOOP("ffmsmpeg4","_Quantizer compression:"),0,1);
         diaElemFloat    qzBlur(PX(qblur),QT_TRANSLATE_NOOP("ffmsmpeg4","Quantizer _blur:"),0,1);
         
        diaElemUInteger GopSize(PX(gop_size),QT_TRANSLATE_NOOP("ffmsmpeg4","_Gop Size:"),1,500); 
          /* First Tab : encoding mode */
        diaElem *diamode[]={&GopSize,&bitrate};
        diaElemTabs tabMode(QT_TRANSLATE_NOOP("ffmsmpeg4","User Interface"),2,diamode);
        
        /* 3nd Tab : Qz */
        
         diaElem *diaQze[]={&qminM,&qmaxM,&qdiffM,&trellis};
        diaElemTabs tabQz(QT_TRANSLATE_NOOP("ffmsmpeg4","Quantization"),4,diaQze);
        
        /* 4th Tab : RControl */
        
         diaElem *diaRC[]={&filetol,&qzComp,&qzBlur};
        diaElemTabs tabRC(QT_TRANSLATE_NOOP("ffmsmpeg4","Rate Control"),3,diaRC);
        
         diaElemTabs *tabs[]={&tabMode,&tabQz,&tabRC};
        if( diaFactoryRunTabs(QT_TRANSLATE_NOOP("ffmsmpeg4","libavcodec MPEG-4 configuration"),3,tabs))
        {
          conf->lavcSettings.me_method=(Motion_Est_ID)me;
          return true;
        }
         return false;
}
// EOF
