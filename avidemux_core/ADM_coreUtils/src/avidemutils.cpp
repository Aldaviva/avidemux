/***************************************************************************
                          avidemutils.cpp  -  description
                             -------------------
    begin                : Sun Nov 11 2001
    copyright            : (C) 2001 by mean
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
#include <math.h>

#ifdef _WIN32
#	include <windows.h>
#endif

#include "avifmt.h"
#include "avifmt2.h"
#include "fourcc.h"

#include "ADM_bitmap.h"
#include "ADM_coreUtils.h"


uint8_t  mk_hex(uint8_t a, uint8_t b);
void     memcpyswap(uint8_t *dest, uint8_t *src, uint32_t size);

// misc dump functions
extern "C"
{
	void mixDump_c(uint8_t * ptr, uint32_t len)
	{
		mixDump(ptr,len);
	}
}
void mixDump(uint8_t * ptr, uint32_t len)
{
    char string[200];
    char string2[200];
    char tiny[10];

    char *str, *str2;
    uint32_t i;

    str = (char *) string;
    str2 = (char *) string2;
    *str = *str2 = 0;

    for (i = 0; i < len; i++)
    {
        if (*ptr < 32 || *ptr > 126)
        {
            strcat(str, ".");
        } else
        {
            sprintf(tiny, "%c", *ptr);
            strcat(str, tiny);
        }

        sprintf(tiny, " %02x", *ptr);
        strcat(str2, tiny);
        ptr++;

        if ((i % 16) == 15)
        {
            printf("\n %04" PRIx32" : %s  %s", (i >> 4) << 4, str, str2);
            *str = 0;
            *str2 = 0;
        }
    }
    // left over
    uint32_t leftover = len % 16;
    if (leftover)
    {
        for (i = 0; i < 16 - leftover; i++)
        {
            strcat(str, " ");
        }
        printf("\n %04" PRIx32" : %s  %s", (len >> 4) << 4, str, str2);
    }
    printf("\n");
}
/*
	A bunch of Endianness swapper to ease handling
	avi on BE processor (sparc/Mac)

*/
void Endian_AviStreamHeader(AVIStreamHeader *s)
{

#ifdef ADM_BIG_ENDIAN
	#define SWAP32(x)  s->x=R32(s->x);
		SWAP32(fccType);
		SWAP32(fccHandler);
		SWAP32(dwFlags);
		SWAP32(dwInitialFrames);
		SWAP32(dwScale);
		SWAP32(dwRate);
		SWAP32(dwStart);
		SWAP32(dwLength);
		SWAP32(dwSuggestedBufferSize);
		SWAP32(dwQuality);
		SWAP32(dwSampleSize);
	#undef SWAP32

#endif

}
void Endian_AviMainHeader(MainAVIHeader *m)
{
#ifdef ADM_BIG_ENDIAN
	#define SWAP32(x) m->x=R32(m->x)

		SWAP32(dwMicroSecPerFrame);
		SWAP32(dwMaxBytesPerSec);
		SWAP32(dwPaddingGranularity);

		SWAP32(dwFlags);
		SWAP32(dwTotalFrames);
		SWAP32(dwInitialFrames);
		SWAP32(dwStreams);
		SWAP32(dwSuggestedBufferSize);

		SWAP32(dwWidth);
		SWAP32(dwHeight);

	#undef SWAP32

#endif

}

void Endian_BitMapInfo( ADM_BITMAPINFOHEADER *b)
{
#ifdef ADM_BIG_ENDIAN
	#define SWAP32(x) b->x=R32(b->x)
	#define SWAP16(x) b->x=R16(b->x)

		SWAP32(biSize);
		SWAP32(biWidth);
		SWAP32(biHeight);
		SWAP16(biPlanes);
		SWAP16(biBitCount);
		SWAP32(biCompression);
		SWAP32(biSizeImage);
		SWAP32(biXPelsPerMeter);
		SWAP32(biYPelsPerMeter);
		SWAP32(biClrUsed);
	#undef SWAP32(x)
	#undef SWAP16
#endif
}


void Endian_WavHeader(WAVHeader *w)
{
#ifdef ADM_BIG_ENDIAN
	#define SWAP32(x) w->x=R32(w->x)
	#define SWAP16(x) w->x=R16(w->x)
		SWAP16(encoding);
		SWAP16(channels);
		SWAP32(frequency);
		SWAP32(byterate);
		SWAP16(blockalign);
		SWAP16(bitspersample);

	#undef SWAP32
	#undef SWAP16

#endif


}
// Here we copy in reverse order
// Useful to do LE/BE conv on nomber
//
void memcpyswap(uint8_t *dest, uint8_t *src, uint32_t size)
{
	dest+=size-1;
	while(size--)
	{
		*dest=*src;
		dest--;
		src++;
	}

}
/**
    \fn ADM_findMpegStartCode
    \brief Compatibility wrapper
*/
bool ADM_findMpegStartCode(uint8_t *start, uint8_t *end,uint8_t *outstartcode,uint32_t *offset)
{
    bool zero=false;
    return ADM_findAnnexBStartCode(start,end,outstartcode,offset,&zero);
}
/**
    \fn     ADM_findAnnexBStartCode
    \brief  Find Annex B video stream startcode 00 00 01 xx yy
            and check whether it is preceded by zero byte,
            return xx + offset to payload (yy).
*/
bool ADM_findAnnexBStartCode(uint8_t *start, uint8_t *end, uint8_t *outstartcode, uint32_t *offset, bool *zero)
{
    uint32_t startcode=0xffffffff;
    uint8_t  *ptr=start;
    *zero=false;
    end--;

    while(ptr<end)
    {
        startcode=(startcode<<8)+*ptr;
        if(1==(startcode&0xffffff))
        {
            if(!(startcode>>24))
                *zero=true;
            *outstartcode=ptr[1];
            *offset=ptr-start+2;
            return true;
        }
        ptr++;
    }
    return false; // startcode not found
}

//**********************************************************
// Convert \ to \\
// Needed for win32 which uses \ to store filename+path
//**********************************************************
char *ADM_escape(const ADM_filename *incoming)
{
char *out,*cur;
int to_escape=0;
int l=0;

    if(incoming)     l=strlen((char *)incoming);
    if(!l)
    {
        printf("[ADM_escape] Null string ?\n");
        out=new char[1];
        out[0]=0;
        return out;
    }

    for(int i=0;i<l;i++) if(incoming[i]=='\\') to_escape++;
    out=new char[l+to_escape+1];
    cur=out;
    for(int i=0;i<l;i++)
    {
        *cur++=incoming[i];
        if(incoming[i]=='\\') *cur++=incoming[i];
    }
    *cur++=0;
    return out;
}
/**
    \fn ADM_computeBitrate
    \brief         Return average bitrate in bit/s
*/
uint32_t ADM_computeBitrate(uint32_t fps1000, uint32_t nbFrame, uint32_t sizeInMB)
{
  double    db,    ti;
  uint32_t    vbr = 0;

  db = sizeInMB;
  db = db * 1024. * 1024. * 8.;
  // now deb is in Bits

  // compute duration
  ti = nbFrame;
  ti *= 1000;
  ti /= fps1000;			// nb sec
  db = db / ti;

  vbr = (uint32_t) floor (db);
  return vbr;
}
/**
    \fn ADM_computeBitrate
    \brief         Return average bitrate in bit/s
*/
bool ADM_computeAverageBitrateFromDuration(uint64_t duration, uint32_t sizeInMB, uint32_t *avgInKbits)
{
    float f;
    if(duration==ADM_NO_PTS)
    {
        ADM_error("[ADM_computeBitrateFromDuration] No source duration!\n");
        return false;
    }
    if(!duration) 
    {
        ADM_error("[ADM_computeBitrateFromDuration] No source duration!\n");
        return false;
    }
    f=sizeInMB; 
    f=f*1024*1024*8; // in bits
    f*=1000*1000;
    f/=duration; // bit/s
    f/=1000; // in kbps
    *avgInKbits=(uint32_t)f;
    return true;
}
/**
        \fn ADM_UsecFromFps1000
*/
uint32_t ADM_UsecFromFps1000(uint32_t fps1000)
{
double f;
      if(fps1000>250000) fps1000=25000; // safe default;
      if(!fps1000) fps1000=25000; // safe default;

      f=fps1000;
      f=1/f;
      f=f*1000; // In seconds
      f=f*1000000; // In us;
      return (uint32_t) floor(f);

}
/**
    \fn ADM_Fps1000FromUs
    \brief time increment to fps1000
*/
uint32_t ADM_Fps1000FromUs(uint64_t us)
{
    if(us<1000) return 1000;
    double f;
    f=us;
    f=1000000./f;
    f*=1000;
    return (uint32_t)(f+0.5);

}
void printBih(ADM_BITMAPINFOHEADER *bi)
{
#undef X_DUMP
#define X_DUMP(x) printf(#x":\t\t:%ld\n",(long int)bi->x);
    	X_DUMP( biSize);
       	X_DUMP( biWidth);
        X_DUMP( biHeight);
        X_DUMP( biBitCount);
        X_DUMP( biCompression);fourCC::print(bi->biCompression);printf("\n");
        X_DUMP( biSizeImage);
        X_DUMP( biXPelsPerMeter);
        X_DUMP( biYPelsPerMeter);
        X_DUMP( biClrUsed);
}

/* Compute aspect ration from common ARWidth / AR Height value */
typedef struct
{
  int width;
  int height;
  ADM_ASPECT ar;
  const char *string;
}ARDescriptor;

ARDescriptor  allArs[]=
{
  {8,9,ADM_ASPECT_4_3,     QT_TRANSLATE_NOOP("adm","NTSC 4:3")},
  {32,27,ADM_ASPECT_16_9,  QT_TRANSLATE_NOOP("adm","NTSC 16:9")},
  {128,81,ADM_ASPECT_16_9, QT_TRANSLATE_NOOP("adm","NTSC 16:9")},
  {16,15,ADM_ASPECT_4_3,   QT_TRANSLATE_NOOP("adm","PAL 4:3")},
  {64,45,ADM_ASPECT_16_9,  QT_TRANSLATE_NOOP("adm","PAL 16:9")},
  {1,1,ADM_ASPECT_1_1,     QT_TRANSLATE_NOOP("adm","1:1")},
};
const char *unknown=QT_TRANSLATE_NOOP("adm","Unknown");

ADM_ASPECT getAspectRatioFromAR(uint32_t width, uint32_t height, const char **string)
{
int mx=sizeof(allArs)/sizeof(ARDescriptor);
ARDescriptor *desc=allArs;
    for(int i=0;i<mx;i++)
    {

      if(width==desc->width && height==desc->height)
      {
        *string=desc->string;
        return desc->ar;
      }
      desc++;
    }
    *string=unknown;
    return ADM_ASPECT_1_1;
}

// Get nice value for a priority level (0 - 4).
int32_t ADM_getNiceValue(uint32_t priorityLevel)
{
	switch (priorityLevel)
	{
		case 0:	// High
			return -18;
			break;
		case 1: // Above Normal
			return -10;
			break;
		case 2: // Normal
			return 0;
			break;
		case 3: // Below Normal
			return 10;
			break;
		case 4: // Low
			return 18;
			break;
	}
    return 0;
}
uint8_t mk_hex(uint8_t a, uint8_t b)
{
	int a1 = a, b1 = b;

	if (a >= 'a')
	{
		a1 = a1 + 10;
		a1 = a1 - 'a';
	}
	else
		a1 = a1 - '0';

	if (b >= 'a')
	{
		b1 = b1 + 10;
		b1 = b1 - 'a';
	}
	else
		b1 = b1 - '0';

	return (a1 << 4) + b1;
}
/**
    \fn ADM_splitSequencedFile
    \brief split file foobar001.foo into foobar, 001 and 3
*/
bool ADM_splitSequencedFile(const char *filename, char **left, char **right,uint32_t *nbDigit,uint32_t *base)
{
        const char *dot = strrchr( filename, '.' );
        *left=NULL;
        *right=NULL;

        // count the decimals before the dot
        int decimals = 1;
        while( (dot != NULL) && ((dot - decimals) != filename) &&
               (dot[0 - decimals] >= '0') && (dot[0 - decimals] <= '9') )
                { decimals++; }
        decimals--;

        // Nuv files can have 20 decimals
        // Keep it down to 10000
        if(decimals>4) decimals=4;
        // split the filename in <left>, <number> and <right>
        // -----
        if(!decimals) return false;
        // <left> part
        
        *left = new char[(dot - filename - decimals) + 1];
        char *aleft=*left;
        strncpy( aleft, filename, (dot - filename - decimals) );
        aleft[(dot - filename - decimals)] = '\0';

        // <number> part
        char *number = new char[decimals + 1];
        strncpy( number, (dot - decimals), decimals );
        number[decimals] = '\0';
        *base=atoi(number);
        *nbDigit=decimals;

		delete [] number;

        // <right> part
        *right = new char[ strlen(dot)+1 ];
        strcpy( *right, dot );
        return true;
}
/**
    \fn ADM_probeSequencedFile
*/
bool ADM_probeSequencedFile(const char *fileName)
{
    char *left=NULL;
    char *right=NULL;
    uint32_t nbDigit,base;
     if(false==ADM_splitSequencedFile(fileName, &left, &right,&nbDigit,&base))
            return false;

    // check whether the filesize approx. matches 2^n GiB, the usual
    // threshold for automatically split streams
    bool success=false;
    uint64_t fileSize,threshold,tolerance;
    threshold=tolerance=1;
    threshold<<=28; // we start at 256 MiB, this value is hardcoded in some devices
    tolerance<<=20; // 1 MiB
    int64_t sz=ADM_fileSize(fileName);
    if(sz<0)
        return false;
    fileSize=sz;
    for(int i=0;i<5;i++)
    {
        if(fileSize >= threshold-tolerance && fileSize <= threshold+tolerance)
        {
            success=true;
            break;
        }
        threshold<<=1;
        if(i==1)
            tolerance<<=3; // 8 MiB starting with 1 GiB fragment size
    }
    if(!success)
        return false;

    // check if at least one sequence exists...
    std::string aLeft(left);
    std::string aRight(right);
    delete [] left;
    delete [] right;

    char match[16];
    match[0]='%';
    match[1]='0';
    sprintf(match+2,"%d",nbDigit); // snprintf instead ...
    strcat(match,"d");
    match[15]=0;

    
    char names[16];
    sprintf(names,match,base+1);
    std::string middle(names);
    std::string target=aLeft+middle+aRight;
    sz=ADM_fileSize(target.c_str());
    if(sz<0 || sz > threshold+tolerance)
        return false;
    return true;
}
//EOF
