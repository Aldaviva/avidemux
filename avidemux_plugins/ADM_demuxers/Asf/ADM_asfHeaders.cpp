/** *************************************************************************
    \file ADM_asf.cpp
    \brief ASF/WMV demuxer
    copyright            : (C) 2006/2009 by mean
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
#include "ADM_Video.h"
#include "ADM_assert.h"

#include "fourcc.h"
#include "DIA_coreToolkit.h"
#include "ADM_asf.h"
#include "ADM_asfPacket.h"
#include "DIA_working.h"
#include "ADM_vidMisc.h"
#include "ADM_coreUtils.h"

#if 0
#define aprintf printf
#else
#define aprintf(...) {}
#endif

static const uint8_t asf_audio[16]={0x40,0x9e,0x69,0xf8,0x4d,0x5b,0xcf,0x11,0xa8,0xfd,0x00,0x80,0x5f,0x5c,0x44,0x2b};
static const uint8_t asf_video[16]={0xc0,0xef,0x19,0xbc,0x4d,0x5b,0xcf,0x11,0xa8,0xfd,0x00,0x80,0x5f,0x5c,0x44,0x2b};
/**
    \fn shiftAudioVideoBy
    \brief shift audio and video so that they start close to zero
*/
bool asfHeader::shiftAudioVideoBy(uint64_t s)
{
    int n=_index.size();
    ADM_info("Shifting by %s\n",ADM_us2plain(s));
    for(int i=0;i<n;i++)
    {
        if(_index[i].pts!=ADM_NO_PTS)
        {
            if(_index[i].pts<s)
            {
                ADM_error("Shifting too big for frame %d PTS: %s\n",i,ADM_us2plain(_index[i].pts));
            }else
                _index[i].pts-=s;
        }
        _index[i].dts=ADM_NO_PTS;
/*
        if(_index[i].dts!=ADM_NO_PTS)
        {
            if(_index[i].dts<s)
            {
                ADM_error("Shifting too big for frame %d DTS: %s\n",i,ADM_us2plain(_index[i].dts));
                _index[i].dts=ADM_NO_PTS;
            }else
                _index[i].dts-=s;
        }
*/
    }
    _duration-=s;
    _shiftUs=s;
    return true;
}
/**
    \fn setFps
*/
bool asfHeader::setFps(uint64_t usPerFrame)
{
    if(!usPerFrame) return false;
    double f=usPerFrame;
    if(f<10) f=10;
    f=1000000.*1000./f;
    _videostream.dwRate=(uint32_t)f;
    ADM_info("AverageFps=%d\n",(int)_videostream.dwRate);
    return true;
}
/**
    \fn decodeStreamHeader
*/
bool asfHeader::decodeStreamHeader(asfChunk *s)
{
uint8_t gid[16];
    // Client GID
        uint32_t audiovideo=0; // video=1, audio=2, 0=unknown
        uint32_t sid;
        s->read(gid,16);
        printf("Type            :");
        for(int z=0;z<16;z++) printf("0x%02x,",gid[z]);
        if(!memcmp(gid,asf_video,16))
        {
          printf("(video)");
          audiovideo=1;
        } else
        {
          if(!memcmp(gid,asf_audio,16))
          {
            printf("(audio)"); 
            audiovideo=2;
          } else printf("(? ? ? ?)"); 
        }
        printf("\nConceal       :");
        for(int z=0;z<16;z++) printf(":%02x",s->read8());
        printf("\n");
        printf("Reserved    : %08" PRIx64"\n",s->read64());
        printf("Total Size  : %04" PRIx32"\n",s->read32());
        printf("Size        : %04" PRIx32"\n",s->read32());
        sid=s->read16();
        printf("Stream nb   : %04d\n",sid);
        printf("Reserved    : %04" PRIx32"\n",s->read32());
        switch(audiovideo)
        {
          case 1: // Video
             {
                if(_videoIndex==-1) // take the 1st video track
                {
                    _videoIndex=sid;
                    _videoStreamId= sid;
                    if(!loadVideo(s))
                    {
                      return 0; 
                    }
                    ADM_info("Average fps available from ext header\n");
                }
              } 
              break;
          case 2: // audio
            loadAudio(s,sid);
            break;
          default: 
            break;
        }
        return true;
}
/**
    \fn decodeExtHeader
    \brief we must decode it as it may contain a video stream (e.g. france2 wmv files)
*/ 
bool asfHeader::decodeExtHeader(asfChunk *s)
{
            int streamNumber=0xff;
            int streamLangIndex=0xff;
            uint64_t avgTimePerFrameUs=0;
            ADM_usPerFrameMapping map;
            s->read32();s->read32(); // start time
            s->read32();s->read32(); // end time
            s->read32(); // bitrate
            s->read32(); // buffer size
            s->read32(); //Initial buffer fullness
            s->read32(); // alt bitrate
            s->read32(); // alt buffer size
            s->read32(); // alt Initial buffer fullness
            s->read32(); // max object size
            s->read32(); // flags
            streamNumber=s->read16(); // Stream #
            streamLangIndex=s->read16(); // stream lang index
            printf("\tstream number     :%d\n",streamNumber);
            printf("\tstream langIndex  :%d\n",streamLangIndex);
            uint64_t avg=s->read64();
            avg/=10.;
            printf("\t avg time/frame  : %" PRIu64" us\n",avg);
            avgTimePerFrameUs=avg;
            int streamNameCount=s->read16(); // stream name count
            int payloadExtCount=s->read16(); // payload ext system count
            printf("\tName       count : %d\n",streamNameCount);
            printf("\tPayloadExt count : %d\n",payloadExtCount);
            // stream names
            for(int i=0;i<streamNameCount;i++)
            {
                    printf("\t lang %d\n",s->read16());
                    int size=s->read16();
                    s->skip(size);
            }
            // payload system count
            for(int i=0;i<payloadExtCount;i++)
            {
                // GUID
                s->read32();s->read32();s->read32();s->read32();
                printf("\tExt data size %d\n",s->read16());
                int sz=s->read32();
                s->skip(sz);
            }
            // stream properties object
            if(ftello(_fd)+16+4<s->endPos())
            {       uint8_t gid[16];
                    asfChunk *x=new asfChunk(_fd);
                    x->nextChunk();
                    x->dump();
                    if(x->chunkId()->id==ADM_CHUNK_STREAM_HEADER_CHUNK)
                        decodeStreamHeader(x);
                    x->skipChunk();
                    delete x;
            }
            map.streamNb=streamNumber;
            map.usPerFrame=avg;
            frameDurationMapping.push_back(map);
            return true;
}
/** *****************************************
    \fn getHeaders
    \brief Read Headers to collect information 
********************************************/
uint8_t asfHeader::getHeaders(void)
{
  uint32_t i=0,nbSubChunk,hi,lo;
  const chunky *id;
  uint32_t mn=0,mx=0;
  asfChunk chunk(_fd);
  // The first header is header chunk
  chunk.nextChunk();
  id=chunk.chunkId();
  if(id->id!=ADM_CHUNK_HEADER_CHUNK)
  {
    printf("[ASF] expected header chunk\n"); 
    return 0;
  }
  printf("[ASF] getting headers\n");
  chunk.dump();
  nbSubChunk=chunk.read32();
  printf("NB subchunk :%u\n",nbSubChunk);
  chunk.read8();
  chunk.read8();
  for(i=0;i<nbSubChunk;i++)
  {
    asfChunk *s=new asfChunk(_fd);
    uint32_t skip;
    s->nextChunk();
    printf("***************\n");  
    id=s->chunkId();
    s->dump();
    switch(id->id)
    {
      case ADM_CHUNK_HEADER_EXTENSION_CHUNK:
      {
        printf("Got header extension chunk\n");
        // 128+16 reserved=
        s->read32();s->read32();s->read32();s->read32();s->read16();
        int x=s->read32();
        uint64_t avgTimePerFrame;
        printf("Dumping object ext : %d data bytes\n",x);
        asfChunk *son=new asfChunk(_fd);
        do
        {
            son->nextChunk();
            son->dump();
            if(son->chunkId()->id==ADM_CHUNK_EXTENDED_STREAM_PROP)
            {
                decodeExtHeader(s);
            }
            son->skipChunk();
            
        }while(son->endPos()+24<s->endPos());
        delete son;
        break;
        }

      case ADM_CHUNK_FILE_HEADER_CHUNK:
        {
            uint64_t playDuration,sendDuration;
            // Client GID
            printf("Client        :");
            for(int z=0;z<16;z++) printf(":%02x",s->read8());
            printf("\n");
            printf("File size     : %08" PRIu64"\n",s->read64());
            printf("Creation time : %08" PRIu64"\n",s->read64());
            printf("Number of pack: %08" PRIu64"\n",s->read64());
            playDuration=s->read64()/10LL;
            sendDuration=s->read64()/10LL;
            _duration=playDuration;
            printf("Play duration : %s\n",ADM_us2plain(playDuration));
            printf("Send duration : %s\n",ADM_us2plain(sendDuration));
            printf("Preroll   3   : %s\n",ADM_us2plain(s->read64()/10LL));
            printf("Flags         : %04" PRIx32"\n",s->read32());
            mx=s->read32();
            mn=s->read32();
            if(mx!=mn)
            {
              printf("Variable packet size!!\n");
              delete s;
              return 0; 
            }
            _packetSize=mx;
            printf("Min size      : %04" PRIx32"\n",mx);
            printf("Max size      : %04" PRIx32"\n",mn);
            printf("Uncompres.size: %04" PRIx32"\n",s->read32());
          }
          break;
      case ADM_CHUNK_STREAM_HEADER_CHUNK:
      {
          decodeStreamHeader(s);
          break;
      }
      default:
         break;
    }
    s->skipChunk();
    delete s;
  }
  printf("End of headers\n");
  return 1;
}
/**
    \fn loadAudio
*/

bool asfHeader::loadAudio(asfChunk *s,uint32_t sid)
{
  asfAudioTrak *trk=&(_allAudioTracks[_nbAudioTrack]);
    ADM_assert(_nbAudioTrack<ASF_MAX_AUDIO_TRACK);
    trk->streamIndex=sid;
    s->read((uint8_t *)&(trk->wavHeader),sizeof(WAVHeader));
    printf("[Asf] Encoding for audio 0x%x\n",trk->wavHeader.encoding);
#ifdef ADM_BIG_ENDIAN
    Endian_WavHeader(&(trk->wavHeader));
#endif

    trk->extraDataLen=s->read16();
    printf("Extension :%u bytes\n",trk->extraDataLen);
    if(trk->extraDataLen)
    {
      trk->extraData=new uint8_t[trk->extraDataLen];
      s->read(trk->extraData,trk->extraDataLen);
    }
      printf("#block in group   :%d\n",s->read8());
      printf("#byte in group    :%d\n",s->read16());
      printf("Align1            :%d\n",s->read16());
      printf("Align2            :%d\n",s->read16());
      _nbAudioTrack++;
    return true;
}
/**
    \fn loadVideo
*/

uint8_t asfHeader::loadVideo(asfChunk *s)
{
  uint32_t w,h,x;
            printf("--\n");
            w=s->read32();
            h=s->read32();
            s->read8();
            x=s->read16();
            _isvideopresent=1;

            memset(&_mainaviheader,0,sizeof(_mainaviheader));
            _mainaviheader.dwWidth=w;
            _mainaviheader.dwHeight=h;
            _video_bih.biWidth=w;
            _video_bih.biHeight=h;
            printf("Pic Width  %04d\n",w);
            printf("Pic Height %04d\n",h);
            printf(" BMP size  %04d (%04d)\n",x,(int)sizeof(ADM_BITMAPINFOHEADER));
            s->read((uint8_t *)&_video_bih,sizeof(ADM_BITMAPINFOHEADER));

		#ifdef ADM_BIG_ENDIAN
			Endian_BitMapInfo(&_video_bih);
		#endif


            _videostream.fccHandler=_video_bih.biCompression;
            printf("Codec : <%s> (%04x)\n",
                    fourCC::tostring(_video_bih.biCompression),_video_bih.biCompression);
            if(fourCC::check(_video_bih.biCompression,(uint8_t *)"DVR "))
            {
              // It is MS DVR, fail so that the mpeg2 indexer can take it from here
              _videostream.fccHandler=_video_bih.biCompression=fourCC::get((uint8_t *)"MPEG");
              printf("This is MSDVR, not ASF\n");
              return 0; 
            }
            printBih(&_video_bih);
#if 1
            if(_video_bih.biSize>sizeof(ADM_BITMAPINFOHEADER))
            {
                    x=_video_bih.biSize;
#else
            if(x>sizeof(ADM_BITMAPINFOHEADER))
            {
#endif
              _videoExtraLen=x-sizeof(ADM_BITMAPINFOHEADER);
              _videoExtraData=new uint8_t[_videoExtraLen];
              s->read(_videoExtraData,_videoExtraLen);
              ADM_info("We have %d bytes of extra data for video.\n",(int)_videoExtraLen);
            }else
            {
                ADM_info("No extra data for video\n");
            }
            uint64_t l=ftello(_fd);
            printf("Bytes left : %d\n",(int)(s->endPos()-l));
            return 1;
}
/**
    \fn      buildIndex
    \brief   Scan the file to build an index
    
    Header Chunk
            Chunk
            Chunk
            Chunk
            
    Data chunk
            Chunk
            Chunk
            
    We skip the 1st one, and just read the header of the 2nd one
    
*/
uint8_t asfHeader::buildIndex(void)
{
  uint64_t fSize,len;
  const chunky *id;
  uint32_t chunkFound;
  uint32_t r=5;
  
  fseeko(_fd,0,SEEK_END);
  fSize=ftello(_fd);
  fseeko(_fd,0,SEEK_SET);
  
  asfChunk h(_fd);
  printf("[ASF] ********** Building index **********\n");
  printf("[ASF] Searching data\n");
  while(r--)
  {
    h.nextChunk();    // Skip headers
    id=h.chunkId();
    h.dump();
    if(id->id==ADM_CHUNK_DATA_CHUNK) break;
    h.skipChunk();
  }
  if(id->id!=ADM_CHUNK_DATA_CHUNK) return 0;
  // Remove leftover from DATA_chunk
 // Unknown	GUID	16
//       Number of packets	UINT64	8
//       Unknown	UINT8	1
//       Unknown	UINT8	1
//   
  h.read32();
  h.read32();
  h.read32();
  h.read32();
  _nbPackets=h.read64();
  h.read16();
  
  len=h.chunkLen-16-8-2-24;
  
  printf("[ASF] nbPacket  : %" PRIu64"\n",_nbPackets);
  printf("[ASF] len to go : %" PRIu64"\n",len);
  printf("[ASF] scanning data\n");
  _dataStartOffset=ftello(_fd);
  
  // Here we go
  asfPacket *aPacket=new asfPacket(_fd,_nbPackets,_packetSize,
                                   &readQueue,&storageQueue,_dataStartOffset);
  uint32_t sequence=0;
  nbImage=0;
  
  len=0;
  asfIndex indexEntry;
  memset(&indexEntry,0,sizeof(indexEntry));
  bool first=true;
  bool cancelled=false;
  DIA_workingBase *progressBar=createWorking(QT_TRANSLATE_NOOP("asfdemuxer","Indexing"));
  uint32_t fileSizeMB=(uint32_t)(fSize>>10);

  uint64_t lastDts[ASF_MAX_AUDIO_TRACK];
  for(int i=0;i<ASF_MAX_AUDIO_TRACK;i++)
  {
        lastDts[i]=0;
  }

  uint64_t packet=0;
  while(packet<_nbPackets)
  {
    while(readQueue.size())
    {
      asfBit *bit=NULL;

      // update UI
      uint32_t curPos=(uint32_t)(ftello(_fd)>>10);
      progressBar->update(curPos,fileSizeMB);

      bit=readQueue.front();
      readQueue.pop_front();

      // --
      uint64_t dts=bit->dts;
      uint64_t pts=bit->pts;
        aprintf("** DTS=%s\n",ADM_us2plain(dts));
        aprintf("** PDTS=%s\n",ADM_us2plain(pts));
      if(bit->stream==_videoStreamId)
      {
          aprintf(">found video packet of size=%d off=%" PRIu64" seq %d, while curseq =%d, dts=%s",
                        bit->len,bit->offset,  bit->sequence,curSeq, ADM_us2plain(dts));
          aprintf(" pts=%s\n",ADM_us2plain(pts));
          if(bit->sequence!=sequence || first==true)
          {
            if(first==false)
            {
                indexEntry.frameLen=len;
                aprintf("Pushing video frame index=%d seq=%d pts=%s \n",
                        _index.size(),
                        indexEntry.segNb,ADM_us2plain(indexEntry.pts));
                aprintf("dts=%s\n",ADM_us2plain(indexEntry.dts));
                _index.append(indexEntry);
            }
            
            aprintf("New sequence\n");
            if( ((sequence+1)&0xff)!=(bit->sequence&0xff))
            {
                ADM_warning("!!!!!!!!!!!! non continuous sequence %u %u\n",sequence,bit->sequence); 
            }
            
            
            indexEntry.frameLen=0;
            indexEntry.segNb=bit->sequence;
            indexEntry.packetNb=bit->packet;
            indexEntry.flags=bit->flags;
            indexEntry.dts=dts;
            indexEntry.pts=pts;
            if(first==false)
            {
                sequence=bit->sequence;
                readQueue.push_front(bit); // reuse it next time
                len=0;
                continue;

            }else
            {
                sequence=bit->sequence;
                first=false; // first packet
            }
            
          }
          len+=bit->len;
      } // End of video stream Id
      else  // Audio ?
      {
        int found=0;
        for(int i=0;i<_nbAudioTrack && !found;i++)
        {
          if(bit->stream == _allAudioTracks[i].streamIndex)
          {
            if(bit->pts!=ADM_NO_PTS)
            {
                if(!lastDts[i] || (bit->pts>lastDts[i]+500000L)) // seek point every 500 ms
                {
                    asfAudioSeekPoint seek;
                        seek.pts=bit->pts;
                        seek.packetNb=bit->packet;
                        (audioSeekPoints[i]).append(seek);
      
#if 1
                        if(!lastDts[i])
                            printf("Adding seek point for track %d at %s (packet=%d)\n",
                            i,ADM_us2plain(bit->pts),(int)seek.packetNb);
#endif
                        lastDts[i]=bit->pts;
                        aprintf("Adding seek point for track %d at %s (packet=%d)\n",
                            i,ADM_us2plain(bit->pts),(int)seek.packetNb);
                }
            }
            found=1;
          }
        }
        if(!found) 
        {
          printf("Unmapped stream %u\n",bit->stream); 
        }
      }
      delete [] bit->data;
      bit->data=NULL;
      storageQueue.push_back(bit);
    }
    // allow to cancel indexing
    if(!progressBar->isAlive())
    {
        cancelled=true;
        break;
    }

    packet++;
    aPacket->nextPacket(0xff); // All packets
    aPacket->skipPacket();
  }
  delete progressBar;
  delete aPacket;
  //delete working;
  /* Compact index */
  
  fseeko(_fd,_dataStartOffset,SEEK_SET);
  if(cancelled) return ADM_IGN;
  printf("[ASF] ******** End of buildindex *******\n");
  nbImage=_index.size();
  printf("[ASF] %u images found\n",nbImage);
  if(!nbImage) return 0;

  uint64_t shift=60*1000*1000;
  bool canShift=false;
  uint64_t tPts;
  tPts=_index[0].pts;
    ADM_info("First image pts: %s, dts: %s\n",ADM_us2plain(tPts), ADM_us2plain(_index[0].dts));
   if(tPts != ADM_NO_PTS)
    {
        shift=tPts;
        ADM_info("Video shift = %s\n",ADM_us2plain(tPts));
        canShift=true;
    }else
        canShift=false;
    for(int i=0;i<_nbAudioTrack;i++)
    {
        if(!audioSeekPoints[i].size())
        {
            ADM_info("audio track : %d, no seek\n",i);
            canShift=false;
        }
    }
    if(canShift)
    {
        for(int i=0;i<_nbAudioTrack;i++)
        {
            uint64_t a=shift;
            for(int j=0;j<audioSeekPoints[i].size();j++)
            {
                if(audioSeekPoints[i][j].pts>shift)
                    break;
                a=audioSeekPoints[i][j].pts;
            }
            shift=a;
        }
        ADM_info("Shifting a/v raw=%s\n",ADM_us2plain(shift));
    }else
    {
        ADM_info("Can't shift\n");
        shift=0;
    }
  _videostream.dwLength=_mainaviheader.dwTotalFrames=nbImage;
  
  _index[0].flags=AVI_KEY_FRAME;
  if(_index[0].pts==ADM_NO_PTS) _index[0].pts=_index[0].dts;
  // Update fps
  // In fact it is an average fps
  //
    _videostream.dwScale=1000;
    // check if we have a duration per frame for video...
    int n=frameDurationMapping.size();
    int dex=-1;
    for(int i=0;i<n;i++)
    {
        if(frameDurationMapping[i].streamNb==_videoStreamId)
            dex=i;
    }
    if(dex!=-1)
    {
        ADM_info("Average fps provided\n");
        setFps(frameDurationMapping[dex].usPerFrame);
    }
    else
    {
      ADM_info("Fps not provided, guessing it from nbFrame and duration\n");
      uint32_t avgFps;
      if(_index[nbImage-1].pts!=ADM_NO_PTS && _index[0].pts!=ADM_NO_PTS)
      {
          float f=(_index[nbImage-1].pts-_index[0].pts);
            f/=nbImage; // average duration of 1 image in us
            setFps((uint64_t) f);
      }else
        {
            printf("[Asf] No pts, setting 30 fps hardcoded\n");
            _videostream.dwRate=(uint32_t)30000;;
        }
    }
    if(shift)
    {
        double frames3=_videostream.dwScale;
                frames3/=_videostream.dwRate;
                frames3*=3*1000*1000; // 
            ADM_info("3 frames time = %s\n",ADM_us2plain((uint64_t)frames3));
        uint64_t frame64=(uint64_t)frames3;
        if(frame64<shift) shift=shift-frame64;
        else shift=0;

        shiftAudioVideoBy(shift);
    }
    return 1;
  
}
