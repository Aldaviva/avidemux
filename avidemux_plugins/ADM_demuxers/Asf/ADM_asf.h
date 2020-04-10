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
#include "ADM_Video.h"
#include "ADM_queue.h"
#include "ADM_asfPacket.h"
#include "BVector.h"
#include <vector>
#define ASF_MAX_AUDIO_TRACK 8

typedef struct
{
    uint64_t pts;
    uint64_t packetNb;
}asfAudioSeekPoint;

typedef BVector <asfAudioSeekPoint> listOfAudioSeekPoint;

typedef struct 
{
  uint64_t packetNb;
  uint32_t frameLen;
  uint32_t segNb;
  uint32_t flags;
  uint64_t dts;
  uint64_t pts;
}asfIndex;

typedef  BVector <asfIndex> AsfVectorIndex;

typedef enum 
{
  ADM_CHUNK_HEADER_CHUNK ,
  ADM_CHUNK_FILE_HEADER_CHUNK,
  ADM_CHUNK_NO_AUDIO_CONCEAL,
  ADM_CHUNK_STREAM_HEADER_CHUNK,
  ADM_CHUNK_STREAM_GROUP_ID,
  ADM_CHUNK_DATA_CHUNK,
  ADM_CHUNK_HEADER_EXTENSION_CHUNK,
  ADM_CHUNK_CLOCK_TYPE_EX,
  ADM_CHUNK_LANGUAGE_LIST_EX,
  ADM_CHUNK_EXTENDED_STREAM_PROP,
  ADM_CHUNK_CONTENT_DESC,
  ADM_CHUNK_EXT_CONTENT_DESC,
  ADM_CHUNK_UNKNOWN_CHUNK,
  
}ADM_KNOWN_CHUNK;

typedef struct 
{
  const char *name;
  uint32_t len;
  uint8_t val[16];
  ADM_KNOWN_CHUNK id; 
}chunky;
/**
    \class asfChunk
*/
class asfChunk
{
  protected:
    FILE        *_fd;
    
    
  public:
  uint64_t  _chunkStart;
            asfChunk(FILE *f);
            ~asfChunk();
  uint8_t   dump(void);
  uint8_t   guId[16];
  uint64_t  chunkLen;
  
  uint8_t   readChunkPayload(uint8_t *data, uint32_t *dataLen);
  uint8_t   nextChunk(int shortChunk=0);
  uint8_t   skipChunk(void);
  uint64_t  read64(void);
  uint32_t  read32(void);
  uint32_t  read16(void);
  uint8_t   read8(void);
  uint64_t  endPos(void) {return _chunkStart+chunkLen;}
  uint8_t   read(uint8_t *where, uint32_t how);
  const chunky    *chunkId(void);
  uint8_t   skip(uint32_t skip);
};

typedef struct 
{
  uint32_t     streamIndex;
  uint32_t     extraDataLen;
  uint8_t      *extraData;
  uint32_t     nbPackets;
  uint32_t     length;
  uint64_t     lastDts;
  WAVHeader    wavHeader;
}asfAudioTrak;
/**
    \class ADM_usPerFrameMapping
*/
class ADM_usPerFrameMapping
{
public:
    int         streamNb;
    uint64_t    usPerFrame;
};

/**
    \class asfAudioAccess
    \brief Audio access class for asf/wmv
*/
class asfAudioAccess : public ADM_audioAccess
{
  protected:
    uint32_t                _myRank;
    char                    *myName;
    uint32_t                _streamId;
    uint32_t                _dataStart;
    asfPacket               *_packet;
    FILE                    *_fd;
    queueOfAsfBits          readQueue;
    queueOfAsfBits          storageQueue;
    uint32_t                _packetSize;
    class asfHeader         *_father;
    asfAudioTrak            *_track;
    listOfAudioSeekPoint    *_seekPoints;
  public:
                                asfAudioAccess(asfHeader *father,uint32_t rank);
    virtual                     ~asfAudioAccess();

    virtual bool      canSeekTime(void) {return true;};
    virtual bool      canSeekOffset(void) {return false;};
    virtual bool      canGetDuration(void) {return true;};
    
    virtual uint32_t  getLength(void) {return _track->length;}
    virtual bool      goToTime(uint64_t timeUs) ;
    virtual bool      isCBR(void) {return true;};
    virtual bool      constantSamplesPerPacket(void) {return false;}

    virtual uint64_t  getPos(void);
    virtual bool      setPos(uint64_t pos);
    virtual uint64_t  getDurationInUs(void) ;

    virtual bool   getPacket(uint8_t *buffer, uint32_t *size, uint32_t maxSize,uint64_t *dts);
    
};

/**
    \class asfHeader
    \brief Asf Demuxer
*/  

class asfHeader         :public vidHeader
{
  protected:
    std::vector             <ADM_usPerFrameMapping> frameDurationMapping;
    uint8_t                 getHeaders( void);
    uint8_t                 buildIndex(void);
    uint8_t                 loadVideo(asfChunk *s);
    bool                    loadAudio(asfChunk *s,uint32_t sid);
    bool                    decodeExtHeader(asfChunk *a);
    bool                    decodeStreamHeader(asfChunk *s);
    queueOfAsfBits          readQueue;
    queueOfAsfBits          storageQueue;
               
    uint32_t                curSeq;
    asfPacket               *_packet;
    //uint32_t                _currentAudioStream;
    uint64_t                _duration;  // Duration 100 ns
  protected:
                                
    FILE                    *_fd;

    int32_t                 _videoIndex;    
    uint32_t                _videoStreamId;

    uint8_t                 close(void);
    bool                    setFps(uint64_t usPerFrame);
    bool                    shiftAudioVideoBy(uint64_t s);
  public: // Shared with audio track
    char                    *myName;
    uint32_t                nbImage;
    AsfVectorIndex          _index;
    uint32_t                _packetSize;
    uint64_t                _dataStartOffset;
    uint32_t                _nbAudioTrack;
    listOfAudioSeekPoint    audioSeekPoints[ASF_MAX_AUDIO_TRACK];
    asfAudioAccess          *_audioAccess[ASF_MAX_AUDIO_TRACK];
    asfAudioTrak             _allAudioTracks[ASF_MAX_AUDIO_TRACK];
    ADM_audioStream         *_audioStreams[ASF_MAX_AUDIO_TRACK];
    uint64_t                 _nbPackets;
    uint64_t                 _shiftUs;
    uint64_t                 getShift(void) { return _shiftUs;}
    
    
    // / Shared
  public:
                                        asfHeader(void);
   virtual                              ~asfHeader();
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


