/***************************************************************************

    \file ADM_edPtsDts.h
    \brief Try to guess Pts from Dts. Mostly used for crappy format like avi.

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
#include "ADM_cpp.h"
#include "ADM_default.h"
#include "math.h"

#include "ADM_edPtsDts.h"

#define GOT_NO_PTS 2
#define GOT_NO_DTS 1
#define GOT_NONE   3
#define GOT_BOTH   0
/* Specific cases */
static bool setDtsFromPts(vidHeader *hdr,uint64_t timeIncrementUs,uint64_t *delay);
static bool setPtsFromDts(vidHeader *hdr,uint64_t timeIncrementUs,uint64_t *delay);
static bool updatePtsAndDts(vidHeader *hdr,uint64_t timeIncrementUs,uint64_t *delay);
/**
    \fn verifyDts
    \brief detect & fix out of order DTS
*/
bool ADM_verifyDts(vidHeader *hdr,uint64_t timeIncrementUs)
{
        aviInfo info;
        hdr->getVideoInfo(&info);
        uint32_t nbFrames=0;
        nbFrames=info.nb_frames;
        if(!nbFrames)
        {
            ADM_warning("The demuxer reports zero frames in this video, can't verify DTS\n");
            return false;
        }
        //
        ADM_info("Verifying DTS....\n");
        uint64_t got_dts=0,startDts=0;
        int start=0;
        for(int i =0;i<nbFrames-1;i++)
        {
            uint64_t pts3,dts3;
            hdr->getPtsDts(i,&pts3,&dts3);
            if(dts3!=ADM_NO_PTS)
            {
                got_dts=dts3;
                start=i+1;
                break;
            }
        }
        ADM_info("Checking from %d to %d\n",start,nbFrames);
        startDts=got_dts;
        // Pass 1 : detect too high DTS i.e. 0 1 2 <9> 4 5 6
        for(int i=start;i<nbFrames-1;i++)
        {
            
            uint64_t pts2,dts2;
            uint64_t pts3,dts3;
            hdr->getPtsDts(i,&pts2,&dts2);
            hdr->getPtsDts(i+1,&pts3,&dts3);
            if(dts2!=ADM_NO_PTS && dts3!=ADM_NO_PTS)
            {
                if(dts3>got_dts &&dts2>got_dts && dts2>dts3)
                {
                    ADM_warning("Out of order dts at frame %d %" PRIu64",%" PRIu64",%" PRIu64"\n",i,got_dts,dts2,dts3);
                    dts2=got_dts+timeIncrementUs;
                    ADM_info("Setting to %" PRIu64"\n",dts2);
                    hdr->setPtsDts(i,pts2,dts2);
                }
            }
            if(dts2==ADM_NO_PTS)
                got_dts+=timeIncrementUs;
            else        
                got_dts=dts2;
        }
        // Pass 2 : detect too low DTS i.e. 0 1 2 4 <1> 6 7
        got_dts=startDts;
        ADM_info("Pass 2..\n");
        for(int i=start;i<nbFrames-1;i++)
        {
            
            uint64_t pts2,dts2;
            uint64_t pts3,dts3;
            hdr->getPtsDts(i,&pts2,&dts2);
            hdr->getPtsDts(i+1,&pts3,&dts3);
            if(dts2!=ADM_NO_PTS && dts3!=ADM_NO_PTS)
            {
                //ADM_info("%d %"PRIu64" %"PRIu64" %"PRIu64"\n",i,got_dts,dts2,dts3);
                if(dts3>got_dts &&dts2<got_dts )
                {
                    ADM_warning("Low dts : Out of order dts at frame %d %" PRIu64",%" PRIu64",%" PRIu64"\n",i,got_dts,dts2,dts3);
                    dts2=got_dts+timeIncrementUs;
                    ADM_info("Setting to %" PRIu64"\n",dts2);
                    hdr->setPtsDts(i,pts2,dts2);
                }
            }
            if(dts2==ADM_NO_PTS)
                got_dts+=timeIncrementUs;
            else        
                got_dts=dts2;
        }
        ADM_info("DTS verified\n");
        return true;
}
/**
    \fn countMissingPts
*/
int countMissingPts(vidHeader *hdr)
{
        int nbMissing=0;
        aviInfo info;
        hdr->getVideoInfo(&info);
        uint32_t nbFrames=0;
        nbFrames=info.nb_frames;
        for(int i=0;i<nbFrames;i++)
        {
            uint64_t pts,dts;
            hdr->getPtsDts(i,&pts,&dts);
            if(pts==ADM_NO_PTS)
                {
                    nbMissing++;
                }
        }
        return nbMissing;
}
/**
    \fn guessH264
    \brief  Try to guess missing PTS at index missingIndex by searching for a hole.
            It is really crude and seldom works.
            The main objective is to avoid stoping when using copy/copy due to missing PTS
            (corrupted stream...)
*/
bool guessH264(vidHeader *hdr,uint64_t timeIncrementUs,int missingIndex)
{
#define LOOKUP_RANGE 10
        int start=0,end=0;
        aviInfo info;
        hdr->getVideoInfo(&info);
        uint32_t nbFrames=0;
        nbFrames=info.nb_frames;

        if(missingIndex<LOOKUP_RANGE) start=0;
                else start=missingIndex-LOOKUP_RANGE;
        if(missingIndex+LOOKUP_RANGE>=nbFrames) end=nbFrames;
                    else end=missingIndex+LOOKUP_RANGE;

        if(end-start<4)
        {
            ADM_warning("Not enough samples to guess missing pts\n");
            return false;
        }

        int nbMissing=0;
        vector<uint64_t> neighbour;
        for(int i=start;i<end;i++)
        {
            uint64_t pts,dts;
            hdr->getPtsDts(i,&pts,&dts);
            if(pts==ADM_NO_PTS)
                {
                    nbMissing++;
                    continue;
                }
            neighbour.push_back(pts);
        }
        // We can use a hole in the PTS if there are several missing PTS...
        if(nbMissing>1)
        {
            ADM_warning("Too much unknown PTS (%d), aborting\n",nbMissing);
            return false;
        }
        // Sort the list
        int n=neighbour.size();
        for(int i=0;i<n;i++)
            for(int j=i+1;j<n;j++)
            {
                uint64_t pts1=neighbour[i];
                uint64_t pts2=neighbour[j];
                if(pts1>pts2)
                    {
                        neighbour[i]=pts2;
                        neighbour[j]=pts1;
                    }
            }
        //for(int i=0;i<n;i++) ADM_info("%d: %"PRIu64"\n",i,neighbour[i]);
        // Now look for the biggest hole in the sorted list
        uint64_t maxDelta=0;
        int maxIndex=-1;
        for(int i=0;i<n-1;i++)
        {
            uint64_t d=neighbour[i+1]-neighbour[i];
            if(d>maxDelta) 
            {
                maxDelta=d;
                maxIndex=i;
            }
            //ADM_info("%d Delta=%"PRIu64"\n",i,d);
        }
        if(maxIndex==-1) 
        {
            ADM_error("Oops, no gap detected\n");
            return false;
        }
        if(maxDelta>=2*timeIncrementUs)
        {
            ADM_info("Our best guess is at %" PRIu64"\n",neighbour[maxIndex]);
            uint64_t pts,dts,pts2;
            pts2=neighbour[maxIndex]+timeIncrementUs;
            hdr->getPtsDts(missingIndex,&pts,&dts); 
            if(pts2>dts)
            {
                ADM_error("Our guessed PTS is too early, aborting (%" PRIu64"/%" PRIu64")\n",pts2,dts);
            }else
                hdr->setPtsDts(missingIndex,pts2,dts);   
        }else   
            ADM_info("Gap found but too small\n");
        //for(int i=0;i<n;i++) ADM_info("%d: %"PRIu64"\n",i,neighbour[i]);
    return true;
}

/**
    \fn ADM_setH264MissingPts(vidHeader *hdr,uint64_t timeIncrementUs,uint64_t *delay)
    \brief look for AVC case where 1 field has PTS, the 2nd one is Bottom without PTS
            in such a case PTS (2nd field)=PTS (First field)+timeincrement
*/
bool ADM_setH264MissingPts(vidHeader *hdr,uint64_t timeIncrementUs,uint64_t *delay)
{
    aviInfo info;
    hdr->getVideoInfo(&info);
    uint32_t nbFrames=0;
    nbFrames=info.nb_frames;
    uint32_t flags,flagsNext;
    uint64_t pts,dts;
    uint64_t ptsNext,dtsNext;
    uint32_t fail=0;
    // Scan if it is the scheme we support
    // i.t. interlaced with Top => PTS, Bottom => No Pts
    // Check we have all PTS...
    fail=countMissingPts(hdr);
    ADM_info("We have %d missing PTS\n",fail);
    if(!fail) return true; // Have all PTS, ok...
    ADM_info("Some PTS are missing, try to guess them...\n");
    fail=0;
    bool interlaced=false;
    //
    for(int i=0;i<nbFrames-1;i+=1)
    {
        hdr->getFlags(i,&flags);
        hdr->getFlags(i+1,&flagsNext);
        hdr->getPtsDts(i,&pts,&dts);
        hdr->getPtsDts(i+1,&ptsNext,&dtsNext);
        if(!(flags & AVI_TOP_FIELD))
        {
            continue;
        }
        if(!(flagsNext & AVI_BOTTOM_FIELD)) 
        {
            continue;
        }
        interlaced=true;
        // TOP / BOTTOM
        if(pts==ADM_NO_PTS)
        {
            fail++;
            continue;
        }
    }
    ADM_info("H264 AVC scheme: %" PRIu32"/%" PRIu32" failures.\n",fail,nbFrames/2);
    if(!interlaced || fail) goto nextScheme;
    {
    ADM_info("Filling 2nd field PTS\n");
    uint32_t fixed=0;
    for(int i=0;i<nbFrames-1;i+=1)
    {
        hdr->getFlags(i,&flags);
        hdr->getFlags(i+1,&flagsNext);
        hdr->getPtsDts(i,&pts,&dts);
        hdr->getPtsDts(i+1,&ptsNext,&dtsNext);
        if(!(flags & AVI_TOP_FIELD))
        {
            continue;
        }
        if(!(flagsNext & AVI_BOTTOM_FIELD)) 
        {
            continue;
        }
        if(pts!=ADM_NO_PTS && ptsNext==ADM_NO_PTS)
        {
            ptsNext=pts+timeIncrementUs;
            hdr->setPtsDts(i+1,ptsNext,dtsNext);
            fixed++;
        }
    }
    ADM_info("Fixed %d PTS\n",fixed);
    }
nextScheme:
    // Try to guess random missing PTS
    // Really guess job
    fail=0;
    vector <int> listOfMissingPts;
    for(int i=0;i<nbFrames;i++)
    {
          hdr->getPtsDts(i,&pts,&dts);
          if(pts==ADM_NO_PTS) 
            {
                fail++;
                listOfMissingPts.push_back(i);
            }
    }
    // 2nd part: Try to really guess the missing ones by spotting gaps...
    int n=listOfMissingPts.size();
    if(n)
    {
        if(n>nbFrames/20)
        {
            ADM_warning("There is more than 5%% unavailable PTS, wont even try (%d/%d)...\n",
                            n,nbFrames);
            goto theEnd;
        }
        for(int i=0;i<n;i++)
        {
              guessH264(hdr,timeIncrementUs,listOfMissingPts[i]);
        }
        
    }
theEnd:
    fail=countMissingPts(hdr);
    ADM_info("End of game, we have %d missing PTS\n",fail);
    return true;
}
/**
    \fn ADM_computeMissingPtsDts

*/
bool ADM_computeMP124MissingPtsDts(vidHeader *hdr,uint64_t timeIncrementUs,uint64_t *delay)
{
    aviInfo info;
    uint32_t flags,previousFlags=0;
    hdr->getVideoInfo(&info);
    uint32_t nDts=0,nPts=0;
    uint32_t nbB=0,ooo=0;
    uint64_t pts,dts,previousPts=ADM_NO_PTS;
    uint32_t nbFields=0,nbFrames=0;
    // Look how much bframes + how much valid PTS/DTS we have....
    for(int i=0;i<info.nb_frames;i++)
    {
        hdr->getFlags(i,&flags);
        if(flags & AVI_FIELD_STRUCTURE) 
                    nbFields++;
            else 
                    nbFrames++;
        if(flags & AVI_B_FRAME)
                nbB++;
        if(true!=hdr->getPtsDts(i,&pts,&dts))
        {
            goto next;
        }
        if(pts!=ADM_NO_PTS)
            nPts++;
        if(dts!=ADM_NO_PTS)
            nDts++;
        if(pts!=ADM_NO_PTS && previousPts!=ADM_NO_PTS && (flags & AVI_B_FRAME) && !(previousFlags & AVI_B_FRAME))
        {
            if(pts>previousPts)
                ooo++;
        }
        previousPts=pts;
        previousFlags=flags;
    }
next:
        ADM_info("Out of %" PRIi32" frames, we have %" PRIi32" valid DTS and %" PRIi32" valid PTS\n",info.nb_frames,nDts,nPts);
        ADM_info("We also have %" PRIi32" bframes\n",nbB);
        if(ooo)
            ADM_info("Unreliable PTS detected\n");
        ADM_info("We have %" PRIu32" fields and %" PRIu32" frames\n",nbFields,nbFrames);
        if(nbFields>2)
        {
            ADM_info("Cannot recompute PTS/DTS for field encoded picture.\n");
            return true;
        }
        // Case 1 : We have both, nothing to do
        if(nDts>=info.nb_frames-2 && nPts>=info.nb_frames-2)
        {
            if(ooo)
            {
                ADM_info("Trying to correct out-of-order PTS\n");
                return setPtsFromDts(hdr,timeIncrementUs,delay);
            }
            ADM_info("Nothing to do\n");
            *delay=0;
            return true;
        }
        // Case 2: We have PTS but not DTS
        if(nPts>=info.nb_frames-2 )
        {
            ADM_info("Got PTS, compute dts\n");
            if(false== setDtsFromPts(hdr,timeIncrementUs,delay)) return false;
            return ADM_verifyDts(hdr,timeIncrementUs);
        }
        // No b frames, PTS=DTS
        if(!nbB)
        {
            ADM_info("No bframe, set pts=dts\n");
            delay=0;
            return setPtsEqualDts(hdr,timeIncrementUs);
        }
        // Case 3: We have DTS but not PTS
        if(nDts>=info.nb_frames-2 )
        {
            ADM_info("Got DTS, compute  PTS\n");
            return  setPtsFromDts(hdr,timeIncrementUs,delay);
        }
        // Case 4: We have a bit of both
        ADM_info("Get some dts and pts\n");
        return updatePtsAndDts(hdr,timeIncrementUs,delay);
}

/**
    \fn setPtsEqualDts
    \brief for Low delay codec, set PTS=DTS, fill the missing values
*/
bool setPtsEqualDts(vidHeader *hdr,uint64_t timeIncrementUs)
{
    aviInfo info;
    hdr->getVideoInfo(&info);
    uint64_t first=ADM_NO_PTS;
    for(int i=0;i<info.nb_frames;i++)
    {
        uint64_t pts,dts;
        if(true!=hdr->getPtsDts(i,&pts,&dts))
        {
            printf("[Editor] GetPtsDts failed for frame %" PRIu32"\n",i);
            return false;
        }
        int k=0;
        if(pts==ADM_NO_PTS) k+=GOT_NO_PTS;
        if(dts==ADM_NO_PTS) k+=GOT_NO_DTS;
        switch(k)
        {
            case GOT_BOTH : // Got both
                if(pts!=dts)
                            {
                                    printf("[Editor] Pts!=Dts for frame %" PRIu32"\n",i);
                            }
                first=pts; // do nothing since we already have both...
                continue;            
                break;
            case GOT_NONE: // Got none
                {
                        if(first!=ADM_NO_PTS)
                        {
                            first+=timeIncrementUs; // Say this one = previous + timeIncrement
                            pts=dts=first;
                        }else
                            continue;   // We dont have a previous skip that one
                }
                break;
            case GOT_NO_DTS :  // got only pts
                first=dts=pts;
                break;
            case GOT_NO_PTS: // got only dts
                first=pts=dts;
                break;
            default:
                ADM_assert(0);
                break;
        }
        // update
        if(true!=hdr->setPtsDts(i,pts,dts))
        {
            printf("[Editor] SetPtsDts failed for frame %" PRIu32"\n",i);
            return false;
        }
    }
    return true;
}
/**
    \fn setDtsFromPts
    \brief For mpeg4 SP/ASP, recompute PTS DTS using the simple I/P/B frame reordering
    Works also for mpeg1/2
    It absolutely NEEDS to have the proper frame type set (PTS/DTS/...)
*/
bool setPtsFromDts(vidHeader *hdr,uint64_t timeIncrementUs,uint64_t *delay)
{
    int last=0;
    int nbFrames;
    int nbBframe=0;
    int maxBframe=0;
    uint32_t flags;
    uint64_t pts,dts;

    ADM_info("computing pts...\n");
    aviInfo info;
    hdr->getVideoInfo(&info);
    nbFrames=info.nb_frames;
    for(int i=1;i<nbFrames;i++)
    {
        hdr->getFlags(i,&flags);
        if(true!=hdr->getPtsDts(i,&pts,&dts))
        {
                    ADM_warning("Cannot get PTS/DTS\n");
                    return false;
        }
        if(flags & AVI_B_FRAME) nbBframe++;
        else        
            {
                if(nbBframe>maxBframe) maxBframe=nbBframe;
                nbBframe=0;
            }
    }
    ADM_info("max bframe = %d\n",maxBframe);
    nbBframe=0;
    // We have now maxBframe = max number of Bframes in sequence
    for(int i=1;i<nbFrames;i++)
    {
        hdr->getFlags(i,&flags);
        hdr->getPtsDts(i,&pts,&dts);
        if(flags & AVI_B_FRAME)
        {
            pts=dts;
            hdr->setPtsDts(i,pts,dts);
            nbBframe++;
        }
        else
        {
            uint64_t oldPts,oldDts;
            uint64_t fwdPts,fwdDts;
              hdr->getPtsDts(last,&oldPts,&oldDts);
              hdr->getPtsDts(i,&fwdPts,&fwdDts);
              oldPts=fwdDts;
              hdr->setPtsDts(last,oldPts,oldDts);
              last=i;
        }
    }
    return 1;
}

/**
    \fn setDtsFromPts
    \brief 
*/
bool setDtsFromPts(vidHeader *hdr,uint64_t timeIncrementUs,uint64_t *delay)
{
    *delay=0;
    int last=0;
    int nbFrames;
    uint32_t flags;
    uint64_t pts,dts;

    ADM_info("computing dts...\n");

    aviInfo info;
    hdr->getVideoInfo(&info);
    nbFrames=info.nb_frames;
    for(int i=1;i<nbFrames;i++)
    {
        hdr->getFlags(i,&flags);
        if(true!=hdr->getPtsDts(i,&pts,&dts))
        {
                    ADM_warning("Cannot get PTS/DTS\n");
                    return false;
        }
        if(flags & AVI_B_FRAME)
        {
                dts=pts;
                hdr->setPtsDts(i,pts,dts);
                continue;
        }
   
        uint64_t oldPts,oldDts;
          hdr->getPtsDts(last,&oldPts,&oldDts);
          dts=oldPts;
          hdr->setPtsDts(i,pts,dts);
        last=i;
    }
    return 1;
}

/**
    \fn setPtsFromDts
    \brief Fill in the missing PTS DTS. We got some but not all.
*/
bool updatePtsAndDts(vidHeader *hdr,uint64_t timeIncrementUs,uint64_t *delay)
{
    aviInfo info;
    uint64_t pts,dts;
    uint32_t nbFrames;
    uint64_t myDelay=0;
    *delay=0;

    hdr->getVideoInfo(&info);
    nbFrames=info.nb_frames;
    // Search valid DTS
    int index=-1;
    for(int i=0;i<nbFrames;i++)
    {
        hdr->getPtsDts(i,&pts,&dts);
        if(dts!=ADM_NO_PTS) 
        {
            index=i;
            break;
        }
    }
    if(index==-1)
    {
        ADM_info("No dts found, aborting\n");
        return false;
    }
    if(index*timeIncrementUs > dts)
    {
        myDelay=index*timeIncrementUs-dts;
    }
    dts=dts+myDelay-index*timeIncrementUs;
    uint64_t curDts=dts; // First DTS...

    ADM_info("Computing missing DTS\n");

    int updated=0;
    // Special case, for 24/1001 video try to detect them
    // And alter timeIncrement accordingly
    int backIndex=-1;
    uint64_t backDts=ADM_NO_PTS;
    uint64_t increment24fps=41708;
    int nbFixup=0;
    if(timeIncrementUs>33300 && timeIncrementUs<33400) // 29.997 fps
    {
        for(int i=0;i<nbFrames;i++)
        {
            hdr->getPtsDts(i,&pts,&dts);
            if(dts!=ADM_NO_PTS)
            {
                if(backIndex!=-1)
                {
                    double deltaFrame=i-backIndex;
                    if(deltaFrame>1)
                    {
                        double deltaTime=dts-backDts;
                        deltaTime=deltaTime/(deltaFrame);
                        if(deltaTime>41700 && deltaTime<41800)
                        {
                            for(int j=backIndex+1;j<i;j++)
                            {
                                hdr->setPtsDts(j,ADM_NO_PTS,backDts+(j-backIndex)*increment24fps);
                            }
                            nbFixup+=i-backIndex; 
                        }
                    }
                }
                backIndex=i;
                backDts=dts;
            }
        }
        ADM_info("Fixed %d/%d frames as 24 fps\n",nbFixup,nbFrames);
    }


    // Fill-in the gaps
    for(int i=0;i<nbFrames;i++)
    {
        hdr->getPtsDts(i,&pts,&dts);
        //printf("Frame %d dts=%"PRIu64,i,dts);
        if(dts!=ADM_NO_PTS) dts+=myDelay;
        if(dts==ADM_NO_PTS) 
        {
            dts=curDts;
            updated++;
        }
        hdr->setPtsDts(i,pts,dts);
        //printf("new dts=%"PRIu64"\n",dts);
        curDts=dts+timeIncrementUs;
    }
    ADM_info("Updated %d Dts, now computing PTS\n",updated);
    
    bool r= setPtsFromDts(hdr,timeIncrementUs,delay);
    *delay+=myDelay;
    return r;
}
//EOF

