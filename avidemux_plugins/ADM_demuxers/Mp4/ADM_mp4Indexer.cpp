/***************************************************************************
    
    copyright            : (C) 2007 by mean
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


#include <string.h>
#include <math.h>

#include "ADM_default.h"
#include "ADM_Video.h"

#include "fourcc.h"
#include "ADM_mp4.h"
#include "DIA_coreToolkit.h"

#if 1
#define aprintf(...) {}
#else
#define aprintf printf
#endif


#define MAX_CHUNK_SIZE (4*1024)
uint32_t sample2byte(WAVHeader *hdr,uint32_t sample);
/**
 * \fn splitAudio
 * \brief Split audio chunks into small enough pieces
 * @param track
 * @param vinfo
 * @return 
 */
bool MP4Header::splitAudio(MP4Track *track,MPsampleinfo *info, uint32_t trackScale)
{
        uint32_t maxChunkSize=(MAX_CHUNK_SIZE>>5)<<5;
        // Probe if it is needed
        int extra=0;
        int sizeOfAudio=0;
        for(int i=0;i<track->nbIndex;i++)
        {
            int x=track->index[i].size/(maxChunkSize+1);
            extra+=x;
            sizeOfAudio+=track->index[i].size;
        }
        if(!extra)
        {
            ADM_info("No very large blocks found, %d bytes present over %d blocks\n",sizeOfAudio,track->nbIndex);
            return true;
        }   
        ADM_info("%d large blocks found, splitting into %d bytes block\n",extra,maxChunkSize);
        
        uint32_t newNbCo=track->nbIndex+extra*2; // *2 is enough, should be.       
        MP4Index *newindex=new MP4Index[newNbCo];
        int w=0;

          for(int i=0;i<track->nbIndex;i++)
          {
                uint32_t sz;
                sz=track->index[i].size;
                if(sz<=maxChunkSize)
                {
                    memcpy(&(newindex[w]),&(track->index[i]),sizeof(MP4Index));
                    w++;
                    continue;
                }
                // We have to split it...
                int part=0;
                
                uint64_t offset=track->index[i].offset;
                uint32_t samples=track->index[i].dts;
                uint32_t totalSamples=samples;
                uint32_t originalSize=sz;
                while(sz>maxChunkSize)
                {
                      newindex[w].offset=offset+part*maxChunkSize;
                      newindex[w].size=maxChunkSize;
                      newindex[w].dts=(samples*maxChunkSize)/originalSize;
                      newindex[w].pts=ADM_COMPRESSED_NO_PTS; // No seek
                      totalSamples-=newindex[w].dts;
                      ADM_assert(w<newNbCo);
                      w++;
                      part++;
                      sz-=maxChunkSize;
                }
                // The last one...
                  newindex[w].offset=offset+part*maxChunkSize;
                  newindex[w].size=sz;
                  newindex[w].dts=totalSamples; 
                  newindex[w].pts=ADM_COMPRESSED_NO_PTS;
                  w++;
        }
      delete [] track->index;
      track->index=newindex;
      track->nbIndex=w;
      uint32_t total=0;
      for(int i=0;i<track->nbIndex;i++)
          total+=track->index[i].size;
      ADM_info("After split, we have %u bytes across %d blocks\n",total,w);
      
      return true;
}
/**
 * \fn processAudio
 * \brief used when all samples have the same size. We make some assumptions here, 
 * might not work with all mp4/mov files.
 * @param track
 * @param trackScale
 * @param info
 * @param outNbChunk
 * @return 
 */
bool	MP4Header::processAudio( MP4Track *track,  uint32_t trackScale,  
                                    MPsampleinfo *info,uint32_t *nbOut)
{
    uint64_t totalBytes=info->SzIndentical*info->nbSz;
    uint32_t totalSamples=0;
    double   skewFactor=1;
    ADM_info("All the same size: %u (total size %" PRIu64" bytes)\n",info->SzIndentical,totalBytes);
    ADM_info("Byte per frame =%d\n",(int)info->bytePerFrame);
    ADM_info("SttsC[0] = %d, sttsN[0]=%d\n",info->SttsC[0],info->SttsN[0]);

    track->totalDataSize=totalBytes;
    
    if(info->nbStts!=1) 
    {
        ADM_info("WARNING: Same size, different duration\n");
        return 1;
    }
    
      if(info->SttsC[0]!=1)
      {
          ADM_warning("Not regular (time increment is not 1=%d)\n",(int)info->SttsC[0]);
          return 1;
      }
    //
    // Each chunk contains N samples=N bytes
    int *samplePerChunk=(int *)malloc(info->nbCo*sizeof(int));
    memset(samplePerChunk,0,info->nbCo*sizeof(int));
    int total=0;
    for(int i=0;i<info->nbSc;i++)
    {
        for(int j=info->Sc[i]-1;j<info->nbCo;j++)
        {
              aprintf("For chunk %lu, %lu \n",j,info->Sn[i] );
              samplePerChunk[j]=info->Sn[i];
        }
    }
    /**/
    for(int i=0;i<info->nbCo;i++)
    {
        aprintf("Chunk %d Samples=%d\n",i,samplePerChunk[i]);
        total+=samplePerChunk[i];
    }

    ADM_info("Total size in sample : %u\n",total);
    ADM_info("Sample size          : %u\n",info->SzIndentical);
    
      if(info->SttsN[0]!=total)
      {
          ADM_warning("Not regular (Nb sequential samples (%d)!= total samples (%d))\n",info->SttsN[0],total);
          //free(samplePerChunk);
          //return 1;
      }
    
    track->index=new MP4Index[info->nbCo];
    memset(track->index,0,info->nbCo*sizeof(MP4Index));
    track->nbIndex=info->nbCo;;
    
    totalBytes=0;
    totalSamples=0;
#if 0   
#define ADM_PER info->bytePerPacket
#else
#define ADM_PER info->bytePerFrame    
#endif
    for(int i=0;i<info->nbCo;i++)
    {
        uint32_t sz;

        track->index[i].offset=info->Co[i];
        sz=samplePerChunk[i];
        sz=sz/info->samplePerPacket;
        sz*=ADM_PER; //*track->_rdWav.channels;;
        
        track->index[i].size=sz;
        track->index[i].dts=samplePerChunk[i]; // No seek
        track->index[i].pts=ADM_NO_PTS; // No seek

        totalBytes+=track->index[i].size;
        totalSamples+=samplePerChunk[i];
        aprintf("Block %d , size=%d,total=%d,samples=%d,total samples=%d\n",i,track->index[i].size,totalBytes,samplePerChunk[i],totalSamples);
    }
    free(samplePerChunk);
    if(info->nbCo)
        track->index[0].pts=0;
    ADM_info("Found %u bytes, spread over %d blocks\n",totalBytes,info->nbCo);
    track->totalDataSize=totalBytes;

    // split large chunk into smaller ones if needed
    splitAudio(track,info, trackScale);


    // Now time to update the time...
    // Normally they have all the same duration with a time increment of 
    // 1 per sample
    // so we have so far all samples with a +1 time increment
      uint32_t samplesSoFar=0;
      double scale=trackScale*track->_rdWav.channels;
      switch(track->_rdWav.encoding)
      {
        default:break;
        case WAV_PCM: // wtf ?
        case WAV_LPCM: // wtf ?
        case WAV_ULAW: // Wtf ?
        case WAV_IMAADPCM:
        case WAV_MSADPCM:
                scale/=track->_rdWav.channels;
                break;
      }
      if(info->bytePerPacket!=info->samplePerPacket)
      {
          ADM_info("xx Byte per packet =%d\n",info->bytePerPacket);
          ADM_info("xx Sample per packet =%d\n",info->samplePerPacket);
      }
      for(int i=0;i< track->nbIndex;i++)
      {     
            uint32_t thisSample=track->index[i].dts;
            double v=samplesSoFar; // convert offset in sample to regular time (us)
            v=(v)/(scale);
            v*=1000LL*1000LL;
#if 1
            track->index[i].dts=track->index[i].pts=(uint64_t)v;
#else
            track->index[i].dts=track->index[i].pts=ADM_NO_PTS;
#endif
            samplesSoFar+=thisSample;
            aprintf("Block %d, size=%d, dts=%d\n",i,track->index[i].size,track->index[i].dts);
      }
     // track->index[0].dts=0;
    ADM_info("Index done (sample same size)\n");
    return 1;
}
/**
        \fn indexify
        \brief build the index from the stxx atoms
*/
uint8_t	MP4Header::indexify(
                          MP4Track *track,   
                          uint32_t trackScale,
                         MPsampleinfo *info,
                         uint32_t isAudio,
                         uint32_t *outNbChunk)

{

uint32_t i,j,cur;

        ADM_info("Build Track index\n");
	*outNbChunk=0;
	aprintf("+_+_+_+_+_+\n");
	aprintf("co : %lu sz: %lu sc: %lu co[0] %" PRIu64"\n",info->nbCo,info->nbSz,info->nbSc,info->Co[0]);
	aprintf("+_+_+_+_+_+\n");

	ADM_assert(info->Sc);
	ADM_assert(info->Sn);
	ADM_assert(info->Co);
	if(!info->SzIndentical)
        {
          ADM_assert(info->Sz);
        }

        // Audio with all samples of the same size and regular
        if(info->SzIndentical && isAudio && info->nbStts==1 && info->SttsC[0]==1)
            return processAudio(track,trackScale,info,outNbChunk);

        // Audio with variable sample size or video
        track->index=new MP4Index[info->nbSz];
        memset(track->index,0,info->nbSz*sizeof(MP4Index));

        if(info->SzIndentical) // Video, all same size (DV ?)
        {
            aprintf("\t size for all %u frames : %u\n",info->nbSz,info->SzIndentical);
            for(i=0;i<info->nbSz;i++)
                track->index[i].size=info->SzIndentical;
            track->totalDataSize+=info->nbSz*info->SzIndentical;
        }else // Different size
        {
            for(i=0;i<info->nbSz;i++)
            {
                track->index[i].size=info->Sz[i];
                aprintf("\t size : %d : %u\n",i,info->Sz[i]);
                track->totalDataSize+=info->Sz[i];
            }
        }
	// if no sample to chunk we map directly
	// first build the # of sample per chunk table
        uint32_t totalchunk=0;

        // Search the maximum
        if(info->nbSc)
        {
            for(i=0;i<info->nbSc-1;i++)
            {
                totalchunk+=(info->Sc[i+1]-info->Sc[i])*info->Sn[i];
            }
            totalchunk+=(info->nbCo-info->Sc[info->nbSc-1]+1)*info->Sn[info->nbSc-1];
        }
        aprintf("# of chunks %d, max # of samples %d\n",info->nbCo, totalchunk);

        uint32_t *chunkCount = new uint32_t[totalchunk+1];
#if 0
	for(i=0;i<info->nbSc;i++)
	{
		for(j=info->Sc[i]-1;j<info->nbCo;j++)
		{
			chunkCount[j]=info->Sn[i];
                        ADM_assert(j<=totalchunk);
		}
		aprintf("(%d) sc: %lu sn:%lu\n",i,info->Sc[i],info->Sn[i]);
	}
#else
        if(info->nbSc)
        {
            for(i=0;i<info->nbSc-1;i++)
            {
                int mn=info->Sc[i]-1;
                int mx=info->Sc[i+1]-1;
                if(mn<0 || mx<0 || mn>totalchunk || mx > totalchunk || mx<mn)
                {
                    ADM_warning("Corrupted file\n");
                    return false;
                }
                for(j=mn;j<mx;j++)
                {
                        chunkCount[j]=info->Sn[i];
                        ADM_assert(j<=totalchunk);
                }
                aprintf("(%d) sc: %lu sn:%lu\n",i,info->Sc[i],info->Sn[i]);
            }
            // Last one
            for(j=info->Sc[info->nbSc-1]-1;j<info->nbCo;j++)
            {
                chunkCount[j]=info->Sn[i];
                ADM_assert(j<=totalchunk);
            }        
        }
#endif

	// now we have for each chunk the number of sample in it
	cur=0;
        for(j=0;j<info->nbCo;j++)
        {
            uint64_t tail=0;
            aprintf("--starting at %lu , %lu to go\n",info->Co[j],chunkCount[j]);
            for(uint32_t k=0;k<chunkCount[j];k++)
            {
                track->index[cur].offset=info->Co[j]+tail;
                tail+=track->index[cur].size;
                aprintf(" sample : %d offset : %lu\n",cur,track->index[cur].offset);
                aprintf("Tail : %lu\n",tail);
                cur++;
            }
        }

	delete [] chunkCount;
        
        
        track->nbIndex=cur;;
	
	
	// Now deal with duration
	// the unit is us FIXME, probably said in header
	// we put each sample duration in the time entry
	// then sum them up to get the absolute time position
        if(!info->nbStts)
        {
            ADM_warning("No time-to-sample table (stts) found.\n");
            return 0;
        }

        uint32_t nbChunk=track->nbIndex;
        uint32_t start=0;
        if(info->nbStts>1 || info->SttsC[0]!=1)
        {
            for(uint32_t i=0;i<info->nbStts;i++)
            {
                for(uint32_t j=0;j<info->SttsN[i];j++)
                {
                    track->index[start].dts=(uint64_t)info->SttsC[i];
                    track->index[start].pts=ADM_COMPRESSED_NO_PTS;
                    start++;
                    ADM_assert(start<=nbChunk);
                }
            }
        }else // All same duration
        {
            for(uint32_t i=0;i<nbChunk;i++)
            {
                track->index[i].dts=(uint64_t)info->SttsC[0]; // this is not an error!
                track->index[i].pts=ADM_COMPRESSED_NO_PTS;
            }
        }

        if(isAudio)
            splitAudio(track,info, trackScale);
        // now collapse
        uint64_t total=0;
        double   ftot;
        uint32_t thisone;

        for(uint32_t i=0;i<nbChunk;i++)
        {
            thisone=track->index[i].dts;
            ftot=total;
            ftot*=1000.*1000.;
            ftot/=trackScale;
            track->index[i].dts=(uint64_t)floor(ftot);
            track->index[i].pts=ADM_COMPRESSED_NO_PTS;
            total+=thisone;
            aprintf("Audio chunk : %lu time :%lu\n",i,track->index[i].dts);
        }
        // Time is now built, it is in us
        ADM_info("Index done\n");
	return true;
}
/**
      \fn sample2byte
      \brief Convert the # of samples into the # of bytes needed
*/
uint32_t sample2byte(WAVHeader *hdr,uint32_t sample)
{
  float f;
        f=hdr->frequency; // 1 sec worth of data
        f=sample/f;       // in seconds
        f*=hdr->byterate; // in byte
    return (uint32_t)floor(f);
}
// EOF
