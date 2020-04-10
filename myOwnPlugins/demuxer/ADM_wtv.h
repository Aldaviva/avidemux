/***************************************************************************
                          ADM_pics.h  -  description
                             -------------------
    begin                : Mon Jun 3 2002
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

#ifndef ADM_ASF_H
#define ADM_ASF_H
#include <vector>
using std::vector;
#include "ADM_Video.h"
//#include "ADM_queue.h"


#define ASF_MAX_AUDIO_TRACK 8

#define WTV_CHUNK_SIZE (0x40*0x1000)

class wtvHeader;

typedef enum
{
   ADM_CHUNK_WTV_TIMING_CHUNK,
   ADM_CHUNK_WTV_DATA_CHUNK,
   ADM_CHUNK_WTV_STREAM_CHUNK,
   ADM_CHUNK_WTV_SUBTITLE_CHUNK,
   ADM_CHUNK_WTV_LANGUAGE_CHUNK,

   ADM_CHUNK_WTV_UNKNOWN_CHUNK
}ADM_KNOWN_CHUNK;

typedef struct
{
  const char *name;
  uint32_t len;
  uint8_t val[16];
  ADM_KNOWN_CHUNK id;
}chunky;

typedef struct
{
  uint32_t     streamIndex;
  uint32_t     extraDataLen;
  uint8_t      *extraData;
  uint32_t     nbPackets;
  uint32_t     length;
  uint64_t     lastDts;
  WAVHeader    wavHeader;
}wtvAudioTrak;

/**
    \class wtvAudioAccess
    \brief Audio access class for asf/wmv
*/
class wtvAudioAccess : public ADM_audioAccess
{
  protected:
    uint32_t                _myRank;
    char                    *myName;
    uint32_t                _streamId;
    uint32_t                _dataStart;
    FILE                    *_fd;
    //ADM_queue               readQueue;
    uint32_t                _packetSize;
    class wtvHeader         *_father;
    wtvAudioTrak            *_track;
  public:
                                wtvAudioAccess(wtvHeader *father,uint32_t rank);
    virtual                     ~wtvAudioAccess();

    virtual bool      canSeekTime(void) {return true;};
    virtual bool      canSeekOffset(void) {return true;};
    virtual bool      canGetDuration(void) {return true;};

    virtual uint32_t  getLength(void) {return _track->length;}
    virtual bool      goToTime(uint64_t timeUs) ;
    virtual bool      isCBR(void) {return true;};


    virtual uint64_t  getPos(void);
    virtual bool      setPos(uint64_t pos);
    virtual uint64_t  getDurationInUs(void) ;

    virtual bool   getPacket(uint8_t *buffer, uint32_t *size, uint32_t maxSize,uint64_t *dts);

};

/**
    \class wtvHeader
    \brief Asf Demuxer
*/

class wtvHeader         :public vidHeader
{
  protected:
    uint64_t                _duration;  // Duration 100 ns
  protected:

    FILE                    *_fd;

    int32_t                 _videoIndex;
    uint32_t                _videoStreamId;

    uint8_t                 close(void);


  public: // Shared with audio track
    char                    *myName;

    uint32_t                nbImage;
    uint32_t                _nbAudioTrack;
    wtvAudioAccess          *_audioAccess[ASF_MAX_AUDIO_TRACK];
    wtvAudioTrak             _allAudioTracks[ASF_MAX_AUDIO_TRACK];
    ADM_audioStream         *_audioStreams[ASF_MAX_AUDIO_TRACK];
    // / Shared
  public:
                                        wtvHeader(void);
   virtual                              ~wtvHeader();
                uint8_t                 open(const char *name);
      //__________________________
      //				 Audio
      //__________________________

    virtual 	WAVHeader              *getAudioInfo(uint32_t i )  ;
    virtual 	uint8_t                 getAudioStream(uint32_t i,ADM_audioStream  **audio);
    virtual     uint8_t                 getNbAudioStreams(void);
    // Frames
      //__________________________
      //				 video
      //__________________________

    virtual 	uint8_t                 setFlag(uint32_t frame,uint32_t flags);
    virtual 	uint32_t                getFlags(uint32_t frame,uint32_t *flags);
    virtual 	uint8_t                 getFrameSize(uint32_t frame,uint32_t *size);
    virtual 	uint8_t                 getFrame(uint32_t framenum,ADMCompressedImage *img);

    virtual   void                       Dump(void);
    virtual   uint64_t                   getTime(uint32_t frameNum);
    virtual   uint64_t                   getVideoDuration(void);

    // Return true if the container provides pts informations
    virtual   bool                       providePts(void) {return false;};
    //
    virtual   bool                       getPtsDts(uint32_t frame,uint64_t *pts,uint64_t *dts);
    virtual   bool                       setPtsDts(uint32_t frame,uint64_t pts,uint64_t dts);


};
#endif


