/***************************************************************************
    \file   ADM_audioAccessfileAACADTS_indexer
    \brief  build a map to seek into the file
    \author mean (c) 2016 fixounet@free.fr
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define AAC_SEEK_PERIOD (200000LL) // 0.2 sec

/**
 * \class adtsIndexer
 * @param fd
 */
class adtsIndexer
{
public:
    
            adtsIndexer(FILE *fd,int off,int fq,int chan)
            {
                f=fd;
                startOffset=off;
                this->fq=fq;
                this->channels=chan;
                payload=0;
                nbPackets=0;
            }
           
    virtual ~adtsIndexer()
            {
                
            }
    int     getPayloadSize() {return payload;}    
    bool    index(std::vector<aacAdtsSeek> &seekPoints);    
    int     getNbPackets() {return nbPackets;}
    
protected:
    FILE *f;
    int  startOffset;
    int  fq;
    int  channels;
    int  payload;
    int  nbPackets;
    
};
/**
 * \fn index
 * @param seekPoints
 * @return 
 */
bool adtsIndexer::index(std::vector<aacAdtsSeek>& seekPoints)
{
   uint8_t  buffer[ADTS_MAX_AAC_FRAME_SIZE];
   uint64_t lastPoint=0;
   int      len;
   
   audioClock   clk(fq);
   ADM_adts2aac aac;
   aacAdtsSeek  start;
   int offset;

   start.dts=0;
   start.position=startOffset;
   seekPoints.push_back(start); // initial start
   
   while(1)
   {
       ADM_adts2aac::ADTS_STATE s=aac.getAACFrame(&len,buffer,&offset);
       offset+=startOffset;
       switch(s)
       {
           case ADM_adts2aac::ADTS_OK:
            {
               uint64_t currentPoint=clk.getTimeUs();
               if( (currentPoint-lastPoint)>AAC_SEEK_PERIOD) // one seek point every 200 ms
               {
                   start.dts=currentPoint;
                   start.position=offset;
                   seekPoints.push_back(start);
                   lastPoint=currentPoint;
               }
                payload+=len;
                clk.advanceBySample(1024);
                nbPackets++;
                //printf("Found %d packet, len is %d, offset is %d, time is %s\n",nbPackets,len,offset,ADM_us2plain(clk.getTimeUs()));
                continue;
                break;
            }
           case ADM_adts2aac::ADTS_ERROR:
           {
               return true;
               break;
           }
           case ADM_adts2aac::ADTS_MORE_DATA_NEEDED:
           {
              
               int n=fread(buffer,1,ADTS_MAX_AAC_FRAME_SIZE,f);
               if(n<=0)
                   return true;
               if(!aac.addData(n,buffer))
                   return true;
               break;
           }
           default:
               ADM_assert(0);
               break;
       }
    
   }
   return true; 
}
// EOF

