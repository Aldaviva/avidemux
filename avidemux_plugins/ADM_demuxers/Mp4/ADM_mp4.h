/***************************************************************************
                          ADM_pics.h  -  description
                             -------------------
    begin                : Mon Jun 3 2002
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
 


#ifndef __3GPHEADER__
#define __3GPHEADER__
#include "ADM_Video.h"
#include "ADM_atom.h"
#include <vector>
/**
 */
class MPsampleinfo
{
  public:
      uint32_t nbCo;
      uint32_t SzIndentical;
      uint32_t nbSz;
      uint32_t nbSc;
      uint32_t nbStts;
      uint32_t nbSync;
      uint32_t nbCtts;
      
	  uint64_t *Co;
      uint32_t *Sz;
      uint32_t *Sc;
      uint32_t *Sn;
      uint32_t *SttsN;
      uint32_t *SttsC;
      uint32_t *Sync; 
      uint32_t *Ctts;
  
      uint32_t samplePerPacket;
      uint32_t bytePerPacket;
      uint32_t bytePerFrame;
      
      MPsampleinfo(void);
      ~MPsampleinfo(void);
};
/**
 */
typedef struct MP4Index
{
	uint64_t offset; // Offset in file to get frame
	uint64_t size;   // Size of frame in bytes
	uint32_t intra;  // Flags associated with frame
	uint64_t pts;   // Decoder time in ms
    uint64_t dts; // Delta in frame between pts & dts

}MP4Index;
class mp4Fragment;
/**
 */
class MP4Track
{
public:    
    MP4Index   *index;
    uint32_t    id;
    uint32_t    scale;
    uint32_t    nbIndex;
    uint32_t    extraDataSize;
    uint8_t     *extraData;
    WAVHeader   _rdWav;
    uint64_t    totalDataSize;
    int64_t     delay; // in movie scale units
    int64_t     startOffset; // media time in track scale units
    std::vector <mp4Fragment>     fragments;
                MP4Track(void);
                ~MP4Track();
};

/**
 */
class ADM_mp4AudioAccess : public ADM_audioAccess
{
protected:
                uint32_t        _nb_chunks;
                uint32_t        _current_index;
                MP4Index        *_index;
                FILE            *_fd;
                // Suppress repeated debug messages from getPacket
                ADMCountdown    *_msg_ratelimit;
                uint32_t        _msg_counter;
public:
                                  ADM_mp4AudioAccess(const char *name,MP4Track *trak) ;
                virtual           ~ADM_mp4AudioAccess();
                                    /// Hint, the stream is pure CBR (AC3,MP2,MP3)
                virtual bool      isCBR(void) { return false;}
                                    /// Return true if the demuxer can seek in time
                virtual bool      canSeekTime(void) {return true;};
                                    /// Return true if the demuxer can seek by offser
                virtual bool      canSeekOffset(void) {return false;};
                                    /// Return true if we can have the audio duration
                virtual bool      canGetDuration(void) {return true;};
                                    /// Returns audio duration in us
                virtual uint64_t  getDurationInUs(void);
                                    /// Returns length in bytes of the audio stream
                virtual uint32_t  getLength(void){return 0;}
                                    /// Set position in bytes
                virtual bool      setPos(uint64_t pos){ADM_assert(0); return 0;};
                                    /// Get position in bytes
                virtual uint64_t  getPos(){ADM_assert(0); return 0;};
                                    /// Go to a given time
                virtual bool      goToTime(uint64_t timeUs);
                virtual bool      getPacket(uint8_t *buffer, uint32_t *size, uint32_t maxSize,uint64_t *dts);
};


#define _3GP_MAX_TRACKS 8
#define VDEO _tracks[0]
#define ADIO _tracks[nbAudioTrack+1]._rdWav
#define AUDIO_BYTERATE_UNSET 0xFFFFFFFF
/**
 * 
 */
class mp4TrafInfo
{
public:
    uint32_t trackID;
    uint64_t baseOffset;
    uint64_t baseDts;
    uint32_t sampleDesc;
    uint32_t defaultDuration;
    uint32_t defaultSize;
    uint32_t defaultFlags;
    bool     emptyDuration;
    bool     baseIsMoof;
    
    mp4TrafInfo()
    {
        baseOffset=baseDts=0;
        trackID=0;
        sampleDesc=defaultDuration=defaultSize=defaultFlags=0;
        emptyDuration=baseIsMoof=false;
    }
};
/**
 * 
 */
class mp4Fragment
{
public:
        mp4Fragment()
        {
            duration=0;
            size=0;
            flags=0;
            composition=0;
            offset=0;
        }
        uint32_t duration;
        uint32_t size;
        uint32_t flags;
        int32_t  composition;
        uint64_t offset;
};

/**
    \class MP4Header
*/
class MP4Header         :public vidHeader
{
protected:
        enum Mp4Flavor
        {
            Mp4Regular,
            Mp4Dash
        };
protected:
          
          bool                          indexVideoFragments(int trackNo);
          bool                          indexAudioFragments(int trackNo);
          /*****************************/
          uint64_t                      delayRelativeToVideo;
          uint8_t                       lookupMainAtoms(void *tom);
          void                          parseMvhd(void *tom);
          uint8_t                       parseTrack(void *ztom);
          uint8_t                       parseElst(void *tom,int64_t *delay,int64_t *skip);
          bool                          parseMoof(adm_atom &son);
          bool                          parseTraf(adm_atom &son,uint64_t moofStart);
          int                           lookupIndex(int desc)          ;
          bool                          parseTrun(int trackNo,adm_atom &son,const mp4TrafInfo &info);
          uint8_t                       decodeVideoAtom(void *ztom);
          uint8_t                       parseMdia(void *ztom,uint32_t *trackType,uint32_t *trackId);
          uint8_t                       parseEdts(void *ztom,uint32_t trackType);
          uint8_t                       parseStbl(void *ztom,uint32_t trackType,uint32_t trackScale);
          uint8_t                       decodeEsds(void *ztom,uint32_t trackType);
          uint8_t                       updateCtts(MPsampleinfo *info );
          bool                          refineFps(void);
          bool                          adjustElstDelay(void);
          uint32_t                      _videoScale;
          uint32_t                      _movieScale;
          int64_t			_movieDuration; // in ms
          uint32_t                      _videoFound;
          bool                          processAudio( MP4Track *track,  uint32_t trackScale,  
                                                            MPsampleinfo *info,uint32_t *outNbChunk);
          bool                          splitAudio(MP4Track *track,MPsampleinfo *vinfo, 
                                                            uint32_t trackScale);
          uint8_t	                     indexify(
                                                MP4Track *track,   
                                                uint32_t trackScale,
                                              MPsampleinfo *info,
                                              uint32_t isAudio,
                                              uint32_t *outNbChunk);
          /*****************************/
        uint8_t                       _reordered;		
        FILE                          *_fd;
        MP4Track                      _tracks[_3GP_MAX_TRACKS];
        int64_t                       _audioDuration;
        uint32_t                      _currentAudioTrack;
        int64_t                       _currentDelay;
        int64_t                       _currentStartOffset;
        Mp4Flavor                     _flavor;
        uint8_t                       parseAtomTree(adm_atom *atom);
        ADM_mp4AudioAccess            *audioAccess[_3GP_MAX_TRACKS-1];
        ADM_audioStream               *audioStream[_3GP_MAX_TRACKS-1];
        uint32_t                      nbAudioTrack;
        bool                          refineAudio(WAVHeader *head,uint32_t extraLen, uint8_t *extra);
        bool                          shiftTimeBy(uint64_t shift);
        bool                          shiftTrackByTime(int dex,uint64_t shift);
        bool                          checkDuplicatedPts(void);
         /*********************************/
	uint32_t                         readPackedLen(adm_atom *tom );
	
public:
virtual   void                          Dump(void) {};

                                        MP4Header( void ) ;
virtual                                 ~MP4Header(  ) ;
// AVI io
virtual 	uint8_t                 open(const char *name);
virtual 	uint8_t                 close(void) ;
  //__________________________
  //				 Info
  //__________________________
virtual   uint8_t                       getExtraHeaderData(uint32_t *len, uint8_t **data);
  //__________________________
  //				 Audio
  //__________________________

virtual 	WAVHeader              *getAudioInfo(uint32_t i )  ;
virtual 	uint8_t                 getAudioStream(uint32_t i,ADM_audioStream  **audio);
virtual         uint8_t                 getNbAudioStreams(void);
virtual         bool                    unreliableBFramePts (void);
// Frames
  //__________________________
  //				 video
  //__________________________

virtual 	uint8_t         setFlag(uint32_t frame,uint32_t flags);
virtual 	uint32_t        getFlags(uint32_t frame,uint32_t *flags);
virtual 	uint8_t         getFrame(uint32_t framenum,ADMCompressedImage *img);
virtual         uint8_t         getFrameSize (uint32_t frame, uint32_t * size);
                // Multi track
                uint8_t         changeAudioStream(uint32_t newstream);
                uint32_t        getCurrentAudioStreamNumber(void);
                uint8_t         getAudioStreamsInfo(uint32_t *nbStreams, audioInfo **infos);
virtual         uint64_t        getTime(uint32_t frameNum);
virtual         uint64_t        getVideoDuration(void);
virtual         bool            getPtsDts(uint32_t frame,uint64_t *pts,uint64_t *dts);
virtual         bool            setPtsDts(uint32_t frame,uint64_t pts,uint64_t dts);

};

#endif


