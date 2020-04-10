/**
    \fn ComputeTimeStamp
    \brief Compute and fill the missing PTS/DTS

*/

#include "ADM_default.h"
#include "fourcc.h"
#include "DIA_coreToolkit.h"
#include "ADM_indexFile.h"
#include "ADM_ps.h"

#include <math.h>

#define DMX_RECOMPUTE_PTS_DTS_MAX 20

#if 1
#define aprintf(...) {}
#else
#define aprintf printf
#endif
/**
        \fn updatePtsDts
        \brief Update the PTS/DTS

TODO / FIXME : Handle wrap
TODO / FIXME : Handle PTS reordering 
*/

bool psHeader::updatePtsDts(void)
{
        uint64_t lastDts=0,lastPts=0,dtsIncrement=0;


        // Let's make sure the fist audio seek point is not ADM_NO_PTS
        for(int i=0;i<listOfAudioTracks.size();i++)
        {

            BVector          <ADM_mpgAudioSeekPoint > *seekPoints=&(listOfAudioTracks[i]->access->seekPoints);
            uint64_t dts=(*seekPoints)[0].dts;
            if(dts!=ADM_NO_PTS) continue;
            ADM_warning("[PS] Audio track %d has no timestamp for first seek point, guessing...\n",i);
            int offset=0;
            int max=(*seekPoints).size();
            while(offset<max)
            {
                    if(   (*seekPoints)[offset].dts!=ADM_NO_PTS) break;
                    offset++;
            }
            if(offset>=max)
            {
                    ADM_error("No valid DTS in audio track\n");
                    continue;
            }
#if 1
            // Remove first seek point with no PTS...
            if(offset) ADM_info("Deleting %d seekPoints with no timestamp\n",offset);
            for(int rm=0;rm<offset;rm++)
                    (*seekPoints).popFront();
#else

            // Compute the missing DTS using byterate and assuming CBR
            uint64_t future=(*seekPoints)[offset].dts;
            uint64_t bytes=(*seekPoints)[offset].size;
            double   byteRate= listOfAudioTracks[i]->header.byterate;
            if(!byteRate)
            {
                ADM_error("No bytereate, cannot fix seekpoints\n");
                continue;
            }
            for(int point=offset-1;point>=0;point++)
            {
                double deltaBytes=(*seekPoints)[point].size;
                double deltaTime=0;
                printf("Computing DTS for seek point %d, bytes %"PRIu64", with reference bytes %"PRIu64" time %"PRIu64" us\n",
                                    point,(uint64_t)deltaBytes,bytes,future);
                if(deltaBytes>bytes)
                {
                        ADM_error("Size going back at seek point %d\n",i);
                        goto next;
                }
                deltaBytes=(double)bytes-deltaBytes; // Bytes -> us
                deltaBytes=deltaBytes*1000*1000.; 
                deltaTime=deltaBytes/byteRate;
                if(deltaTime>future)
                {
                        ADM_error("Computed time correction too big : %"PRIu64" max=%"PRIu64"\n",
                                            (uint64_t)deltaTime, (uint64_t)future);
                        (*seekPoints)[point].dts=0;
                }else
                (*seekPoints)[point].dts=(uint64_t)(future-deltaTime);
            }
#endif
next:
                ;
        }

        // For audio, The first packet in the seekpoints happens a bit after the action
        // It means some audio may have been seen since we locked on video.
        // So we compute the DTS of the first real packet
        // by using the size field and byterate and arbitrarily make it begins
        // at video
        for(int i=0;i<listOfAudioTracks.size();i++)
        {
            BVector          <ADM_mpgAudioSeekPoint > *seekPoints=&(listOfAudioTracks[i]->access->seekPoints);
            uint64_t secondDts=(*seekPoints)[0].dts;
            uint64_t secondSize=(*seekPoints)[0].size;
            if(secondSize && listOfAudioTracks[i]->header.byterate)
            {
                    // it is actually an offset in time to the 2nd packet Dts
                double  firstFloatDts=secondSize*1000*1000.; 
                        firstFloatDts/=listOfAudioTracks[i]->header.byterate; // in us
                uint64_t firstDts=(uint64_t)firstFloatDts;

                        if(firstDts>secondDts) firstDts=0;
                            else firstDts=secondDts-firstDts;

                // Now add our seek point
                ADM_mpgAudioSeekPoint sk;
                sk.dts=firstDts;
                sk.size=0;
                sk.position=ListOfFrames[0]->startAt;
                (*seekPoints).pushFront(sk);

            }
        }


        // Make sure everyone starts at 0
        // Search first timestamp (audio/video)

        switch( _videostream.dwRate)
        {
            case 25000:   dtsIncrement=40000;break;
            case 23976:   dtsIncrement=41708;break;
            case 29970:   dtsIncrement=33367;break;
            default : dtsIncrement=1;
                    printf("[psDemux] Fps not handled for DTS increment\n");

        }
        uint64_t startDts=ListOfFrames[0]->dts;
        uint64_t startPts=ListOfFrames[0]->pts;
        // Special case when we dont have DTS but only PTS
        if(startDts==ADM_NO_PTS) // Do we have DTS ?
        {
            if(startPts!=ADM_NO_PTS)
            {
                if(startPts>=2*dtsIncrement)
                {
                    startDts=startPts-2*dtsIncrement;
                } else startDts=0;
                ListOfFrames[0]->dts=startDts;
            }
        }
        for(int i=0;i<listOfAudioTracks.size();i++)
        {
            uint64_t a=listOfAudioTracks[i]->access->seekPoints[0].dts;
            if(a<startDts) startDts=a;
        }
        // Rescale all so that it starts ~ 0
        // Video..
        for(int i=0;i<ListOfFrames.size();i++)
        {
            dmxFrame *f=ListOfFrames[i];
            if(f->pts!=ADM_NO_PTS) f->pts-=startDts;
            if(f->dts!=ADM_NO_PTS) f->dts-=startDts;
        }
        // Audio start at 0 too
        for(int i=0;i<listOfAudioTracks.size();i++)
        {
            ADM_psTrackDescriptor *track=listOfAudioTracks[i];
            ADM_psAccess    *access=track->access;
            access->setTimeOffset(startDts);
        }

        // Now fill in the missing timestamp and convert to us
        // for video
        // We are sure to have both PTS & DTS for 1st image
        // Guess missing DTS/PTS for video
#if 1
        for(int i=0;i<ListOfFrames.size();i++)
        {
            dmxFrame *frame=ListOfFrames[i];
            if(frame->dts!=ADM_NO_PTS) frame->dts=timeConvert(frame->dts);
            if(frame->pts!=ADM_NO_PTS) frame->pts=timeConvert(frame->pts);
        }
#else
        int noUpdate=0;
        for(int i=0;i<ListOfFrames.size();i++)
        {
            dmxFrame *frame=ListOfFrames[i];
            aprintf("[psUpdate] frame:%d raw DTS: %"PRId64" PTS:%"PRId64"\n",i,frame->dts,frame->pts);
            if(frame->pts==ADM_NO_PTS || frame->dts==ADM_NO_PTS)
            {
                noUpdate++;
                // We have PTS or DTS and it has been a long time we did not have both
                if(noUpdate>DMX_RECOMPUTE_PTS_DTS_MAX && frame->type==1 && (frame->pts!=ADM_NO_PTS || frame->dts!=ADM_NO_PTS))
                {
                    if(frame->pts!=ADM_NO_PTS)
                    {
                        frame->dts=frame->pts-2*dtsIncrement;
                    }else   
                    {
                        frame->pts=frame->dts+2*dtsIncrement;
                    }
                    uint64_t oldDts=lastDts;
                    frame->dts=lastDts=timeConvert(frame->dts);
                    frame->pts=lastPts=timeConvert(frame->pts);
                    if(oldDts>lastDts) printf("[psRead] Warning DTS going backward frame %d, old:%"PRId64" new:%"PRId64" delta=%"PRId64"\n",
                                                       i,oldDts/1000,lastDts/1000,(oldDts-lastDts)/1000);

                }else
                {
                    lastDts+=dtsIncrement;
                    lastPts+=dtsIncrement;
                    frame->dts=lastDts;
                    frame->pts=ADM_NO_PTS;
                }
            }else    // We got both, use them  
            {
                
                frame->dts=lastDts=timeConvert(frame->dts);
                frame->pts=lastPts=timeConvert(frame->pts);
            }
        }
#endif
        // convert to us for Audio tracks (seek points)
        for(int i=0;i<listOfAudioTracks.size();i++)
        {
            ADM_psTrackDescriptor *track=listOfAudioTracks[i];
            ADM_psAccess    *access=track->access;
            
            for(int j=0;j<access->seekPoints.size();j++)
            {
                if( access->seekPoints[j].dts!=ADM_NO_PTS) 
                    access->seekPoints[j].dts=access->timeConvert( access->seekPoints[j].dts);
            }
        }
#if 0
        for(int i=0;i<ListOfFrames.size();i++)
        {
            dmxFrame *frame=ListOfFrames[i];
            int64_t pts,dts;
            pts=frame->pts;
            dts=frame->dts;
            if(pts!=ADM_NO_PTS) pts/=1000;
            if(dts!=ADM_NO_PTS) dts/=1000;
            printf("[psVideo] Framex %d PTS:%"PRId64" ms DTS:%"PRId64" ms, delta %"PRId64" ms\n",i,pts,dts,pts-dts);
        }
#endif
        return 1;
                    
}
