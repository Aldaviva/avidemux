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

#include "ADM_default.h"
#include "ADM_image.h"
#include "ADM_bitmap.h"
#include "DIA_coreToolkit.h"
#include "ADM_imageLoader.h"
#include "ADM_colorspace.h"
#include "ADM_codec.h"
#include "fourcc.h"

//**********************************
static ADMImage *createImageFromFile_jpeg(const char *filename);
static ADMImage *createImageFromFile_Bmp2(const char *filename);
static ADMImage *createImageFromFile_png(const char *filename);
//***********************************

static int getFileSize(FILE *fd)
{
        fseek(fd, 0, SEEK_END);
        int imageSize = ftell(fd);
        fseek(fd, 0, SEEK_SET);
        return imageSize;
}

/**
		\fn 	createImageFromFile
		\brief 	Create and returns an ADMImage from a file, only YV12 jpg supported ATM

*/
ADMImage *createImageFromFile(const char *filename)
{
    uint32_t w,h;
    switch(ADM_identifyImageFile(filename,&w,&h))
    {
        case  ADM_PICTURE_UNKNOWN:
                ADM_warning("[imageLoader] Trouble identifying /loading %s\n",filename);
                return NULL;
        case ADM_PICTURE_JPG:
                return createImageFromFile_jpeg(filename);
                break;
        case ADM_PICTURE_PNG:
                return createImageFromFile_png(filename);
                break;
        case ADM_PICTURE_BMP2:
                return createImageFromFile_Bmp2(filename);
                break;
        default:
                ADM_assert(0);
                break;

    }
    ADM_assert(0);
    return NULL;
}
/**
 * 
 * @param source
 * @param w
 * @param h
 * @return 
 */
static ADMImage *convertImageColorSpace( ADMImage *source, int w, int h)
{
   
    	ADMImageDefault *image=new ADMImageDefault(w,h);        
        ADM_colorspace sourceFormat=source->_colorspace;   
        bool swap=false;
        
        if(ADM_COLOR_RGB32A==sourceFormat)
        {
            image->addAlphaChannel();
            // Extract alpha channel
#define ALPHA_OFFSET 3            
            uint8_t *alpha=source->GetReadPtr(PLANAR_Y)+ALPHA_OFFSET;
            uint8_t *alphaDest=image->GetWritePtr(PLANAR_ALPHA);
            int   sourceStride=source->GetPitch(PLANAR_Y);
            int   destStride=image->GetPitch(PLANAR_ALPHA);

            for(int y=0;y<h;y++)
            {
                uint8_t *inAlpha=alpha;
                alpha+=sourceStride;
                uint8_t *outAlpha=alphaDest;
                alphaDest+=destStride;
                for(int x=0;x<w;x++)
                {
                    *outAlpha=*inAlpha;
                    outAlpha++;
                    inAlpha+=4;
                }
            }
            swap=true;
        }

        if(ADM_COLOR_RGB24==sourceFormat || ADM_COLOR_BGR24==sourceFormat)
            swap=true;

        ADMColorScalerSimple converter(w,h,sourceFormat,ADM_COLOR_YV12);
        converter.convertImage(source,image);

        if(swap)
        {
            uint8_t **s=image->_planes,*v;
            v=s[1];
            s[1]=s[2];
            s[2]=v;
        }
        return image;
}
typedef struct 
{
    int code;
    const char *text;
}jpegCodes;
jpegCodes jCodes[]=
{
#define ENTRY(x,y) {x,y},
    ENTRY(0xe0,"APP0")
    ENTRY(0xc0,"Baseline marker")
    ENTRY(0xc2,"Progressive Huffman")
    ENTRY(0xc4,"Huffman table")
    ENTRY(0xda,"Start of scan")
    ENTRY(0xdb,"Quant table")
    ENTRY(0xf0,"Jpeg0")
    ENTRY(0xfe,"Comment")
    ENTRY(0xd8,"Start of image")
    ENTRY(0xd9,"End of image")
     {0,NULL}
};
static const char *jpegToString(int code)
{
    jpegCodes *p=jCodes;
    while(1)
    {
        if(p->text==NULL) return "???";
        if(code==p->code) return p->text;
        p++;
    }
    return "???";
}
/**
 * \fn lookupTag
 * \brief lookup for a valid non data marker
 * @param fd
 * @return 
 */
static int lookupTag(FILE *fd,int fileSize)
{
#define JPEG_BUFFER_SIZE (32*1024)
    uint8_t buffer[JPEG_BUFFER_SIZE];
    int blockOffset=ftell(fd);
    while(1)
    {
        int remaining=fileSize-blockOffset;
        if(remaining>JPEG_BUFFER_SIZE) remaining=JPEG_BUFFER_SIZE;
        if(remaining<2)
            return 0;
        if(!fread(buffer,remaining,1,fd)) return 0;
        uint8_t *p=buffer;
        uint8_t *end=buffer+remaining;
        while(p<end) // this is slightly wrong if the marker is at the border of a block
        {
            if(*p!=0xff) 
            {
                p++;continue;
            }
            switch(p[1])
            {
                case 0x00:
                    p++; // escape
                    break;
                case 0xc0:
                case 0xd9:
                    return blockOffset+(intptr_t)(p-buffer);
                    break;
                default:
                    ADM_info("found embedded tag %x at %d\n",p[1],ftell(fd));
                    p++;
                    break;
            }
        }
        blockOffset+=remaining;
    }    
    return 0;
}
/**
 * 
 * @param fd
 * @param width
 * @param height
 * @return 
 */
static bool readJpegInfo(FILE *fd, int &width, int &height)
{
        uint16_t tag = 0, count = 0, off = 0;
        int w,h;
        fseek(fd, 0, SEEK_END);
        int fileLength=ftell(fd);
        fseek(fd, 0, SEEK_SET);
        BmpLowLevel reader(fd);
        reader.read16BE();	// skip jpeg ffd8
        while (count < 15 && tag != 0xFFC0)
        {
            tag = reader.read16BE();
            if ((tag >> 8) != 0xff)
            {
                ADM_warning("[imageLoader]invalid jpeg tag found (%x)\n", tag);
                break;
            }
            switch(tag &0xff)
            {
                
                case 0xD9: // end of image
                    ADM_info("End of image\n");
                    break;
                case 0xC0: // SOF0.. baseline
                case 0xC1:
                case 0xC2:
                    reader.read16BE();	// size
                    reader.read8();	// precision
                    h = reader.read16BE();
                    w = reader.read16BE();
                    if(w&1) w++;
                    if(h&1) h++;
                    ADM_info("Dimension %d x %d\n",(int)w,(int)h);
                    width=w;
                    height=h;
                    return true;
                    break;
                case 0xDA: // SOS
                {
                    ADM_info("Found tag 0x%x, %s, size=%d position=%d\n",tag&0xff,jpegToString(tag&0xff),(int)off,(int)ftell(fd));
                    int headerSize=reader.read16BE();
                    int nbComponents=reader.read8();
                    for(int i=0;i<nbComponents;i++) reader.read16BE();
                    reader.read16BE();reader.read8();
                    // data starts here
                    // Lookup for 0xff something
                    int pos=lookupTag(fd,fileLength); 
                    if(!pos)
                    {
                        return false;
                    }
                      fseek(fd, pos, SEEK_SET);
                    break;
                }
                default:
                    off = reader.read16BE();
                    ADM_info("Found tag 0x%x, %s, size=%d position=%d\n",tag&0xff,jpegToString(tag&0xff),(int)off,(int)ftell(fd));
                    if (off < 2)
                    {
                        ADM_warning("[imageLoader]Offset too short!\n");
                        return false;
                    }
                    fseek(fd, off - 2, SEEK_CUR);
                    break;
            }
            count++;
        }    
        return false;
}

/**
 * 	\fn createImageFromFile_jpeg
 *  \brief Create image from jpeg file
 */
ADMImage *createImageFromFile_jpeg(const char *filename)
{

FILE *fd;
uint32_t _imgSize;
int  w = 0, h = 0;

        fd = ADM_fopen(filename, "rb");
        if(!fd)
        {
            ADM_warning("Cannot open jpeg file\n");
            return NULL;
        }
        _imgSize = getFileSize(fd);

        if(!readJpegInfo(fd,w,h))
        {
            ADM_warning("Cannot get info from jpeg\n");
            fclose(fd);
            fd=NULL;
            return NULL;
        }
        
        ADM_info("[imageLoader] %d x %d.., total Size : %u \n", w, h,_imgSize);

        // Load the binary coded image
        ADM_byteBuffer buffer(_imgSize);
        fseek(fd, 0, SEEK_SET);
        int r=fread(buffer.at(0),_imgSize,1,fd);
        fclose(fd);
        if(!r)
        {
            ADM_warning("Cannot read JPEG file.\n");
            return NULL;
        }
        //

        ADMImageRef tmpImage(w,h); // It is a reference image
        // Now unpack it ...
        decoders *dec=ADM_coreCodecGetDecoder (fourCC::get((uint8_t *)"MJPG"),   w,   h, 0 , NULL,0);
        if(!dec)
        {
            ADM_warning("Cannot find decoder for mpjeg");
            return NULL;
        }
        ADMCompressedImage bin;
        bin.data=buffer.at(0);
        bin.dataLength=_imgSize; // This is more than actually, but who cares...

        dec->uncompress (&bin, &tmpImage);
        ADMImage *image=convertImageColorSpace(&tmpImage,w,h);        
        delete dec;
        dec=NULL;
        return image;
}

/**
 * 	\fn createImageFromFile_png
 *  \brief Create image from PNG
 */
ADMImage *createImageFromFile_png(const char *filename)
{
    
    uint32_t size;
    FILE *fd=NULL;
    uint32_t w,h;

       fd = ADM_fopen(filename, "rb");
       if(!fd)
       {
           ADM_warning("Cannot open png file\n");
           return NULL;
       }
       BmpLowLevel reader(fd);
       size=getFileSize(fd);
       reader.read32BE();
       reader.read32BE();
       reader.read32BE();
       reader.read32BE();
       w=reader.read32BE();
       h=reader.read32BE();
       fseek(fd,0,SEEK_SET);
       ADM_byteBuffer buffer(size);

       int r=fread(buffer.at(0),size,1,fd);
       fclose(fd);
       if(!r)
       {
           ADM_warning("Cannot read PNG file.\n");
           return NULL;
       }
       ADMImageRef tmpImage(w,h);
    	// Decode PNG
        decoders *dec=ADM_coreCodecGetDecoder (fourCC::get((uint8_t *)"PNG "),   w,   h, 0 , NULL,0);
    	if(!dec)
        {
            ADM_warning("Cannot get PNG decoder");
            return NULL;
        }
    	ADMCompressedImage bin;
    	bin.data=buffer.at(0);
    	bin.dataLength=size; // This is more than actually, but who cares...

        if(!dec->uncompress (&bin, &tmpImage))
        {
            ADM_warning("PNG Decompressing failed\n");
            delete dec;
            dec=NULL;
            return NULL;
        }
        ADMImage *image=convertImageColorSpace(&tmpImage,w,h);
        if(tmpImage._alpha)
        {
            ADM_info("We do have alpha channel\n");
        }
        delete dec;
        dec=NULL;
    	return image;
}
/**
 * 	\fn createImageFromFile_bmp2
 *  \brief Create image from Bmp2 (BM6)
 */
ADMImage *createImageFromFile_Bmp2(const char *filename)
{

    ADM_BITMAPINFOHEADER bmph;
    uint32_t offset;
    FILE *fd=NULL;
    uint32_t w,h,bpp;
    int bufSize;

	fd = ADM_fopen(filename, "rb");
        if(!fd)
        {
            ADM_warning("Cannot open BMP picture\n");
            return NULL;
        }
 	fseek(fd, 10, SEEK_SET);

        BmpLowLevel reader(fd);
        
 
        offset = reader.read32LE();
        reader.readBmphLE(bmph);
        if(bmph.biCompression && bmph.biCompression!=3)
        {
            ADM_warning("[imageLoader] BMP2:Cannot handle compressed bmp (%08x)\n",bmph.biCompression);
            fclose(fd);
            return NULL;
        }
        w = bmph.biWidth;
        h = bmph.biHeight;
        if(w > MAXIMUM_SIZE)
        {
            ADM_warning("Width %u exceeds maximum supported (%u)\n",w,MAXIMUM_SIZE);
            fclose(fd);
            return NULL;
        }
        if(h > MAXIMUM_SIZE)
        {
            ADM_warning("Height %u exceeds maximum supported (%u)\n",h,MAXIMUM_SIZE);
            fclose(fd);
            return NULL;
        }
        bpp = bmph.biBitCount;
        switch(bpp)
        {
            case 24:
            case 32:
                break;
            default:
                ADM_warning("Only 24 and 32 bpp bitmaps are supported.\n");
                fclose(fd);
                return NULL;
        }

        if(bpp == 32)
        {
            if(bmph.biCompression == 3)
            { // read channel masks, FIXME: BGR is assumed
                reader.read32LE(); // red
                reader.read32LE(); // green
                uint32_t bmask=reader.read32LE(); // blue
                uint32_t amask=reader.read32LE(); // alpha
                if((!amask && bmask == 0xff00) || amask == 0xff)
                    bpp=96; // xBGR
            }
            bufSize=w*h*4;
        }else // 24
        {
            bufSize=w*h*3;
        }

        ADM_info("[imageLoader] BMP2 W: %" PRIu32" H: %" PRIu32" offset : %" PRIu32"\n", w, h, offset);
// Load the binary coded image
 	fseek(fd,offset,SEEK_SET);
        
        ADM_byteBuffer buffer(bufSize);
        
        uint32_t count=fread(buffer.at(0),bufSize,1,fd);
        fclose(fd);
        if(!count)
        {
            ADM_warning("Read incomplete.\n");
            return NULL;
        }

        // Decode DIB
        decoders *dec=ADM_coreCodecGetDecoder (fourCC::get((uint8_t *)"DIB "), w, h, 0, NULL, bpp);
        if(!dec)
        {
            ADM_warning("Cannot get DIB decoder");
            return NULL;
        }

        ADMCompressedImage bin;
        bin.data=buffer.at(0);
        bin.dataLength=bufSize;

        ADMImageRef ref(w,h);

        if(!dec->uncompress(&bin,&ref))
        {
            ADM_warning("DIB decoding failed\n");
            delete dec;
            dec=NULL;
            return NULL;
        }

        ADMImage *image=convertImageColorSpace(&ref,w,h);
        delete dec;
        dec=NULL;
        return image;
}
/**
 * 		\fn ADM_identidyImageFile
 * 		\brief Identidy image type, returns type and width/height
 */
#define MAX_JPEG_TAG 20
ADM_PICTURE_TYPE ADM_identifyImageFile(const char *filename,uint32_t *w,uint32_t *h)
{
    uint8_t fcc_tab[4];
    FILE *fd;

    // 1- identity the file type
    //
    
    fd = ADM_fopen(filename, "rb");
    if (!fd)
    {
        ADM_info("[imageIdentify] Cannot open that file!\n");
        return ADM_PICTURE_UNKNOWN;
    }
    if (!fread(fcc_tab, 4, 1, fd))
    {
        ADM_warning("Cannot read image file.\n");
        fclose(fd);
        return ADM_PICTURE_UNKNOWN;
    }

    // 2- JPEG ?
    if (fcc_tab[0] == 0xff && fcc_tab[1] == 0xd8)
    {
        int width,height;
         if(readJpegInfo(fd,width,height))
         {
             ADM_info("Identified as jpeg (%d x %d)\n",width,height);
             *w=width;
             *h=height;
             fclose(fd);
             return ADM_PICTURE_JPG;
         }else
         {
             fclose(fd);
             return ADM_PICTURE_UNKNOWN;
         }     
    }
    // PNG ?
    if (fcc_tab[1] == 'P' && fcc_tab[2] == 'N' && fcc_tab[3] == 'G')
    {
        fseek(fd, 0, SEEK_SET);
         BmpLowLevel reader(fd);
         reader.read32BE();
         reader.read32BE();
         reader.read32BE();
         reader.read32BE();
        *w= reader.read32BE();
        *h= reader.read32BE();
        fclose(fd);
        return ADM_PICTURE_PNG;
    }
    // BMP2?
    if (fcc_tab[0] == 'B' && fcc_tab[1] == 'M')
    {
        ADM_BITMAPINFOHEADER bmph;

            fseek(fd, 10, SEEK_SET);
            BmpLowLevel reader(fd);
            reader.read32LE();
            reader.readBmphLE(bmph);
            if (bmph.biCompression != 0 && bmph.biCompression != 3)
            {
                ADM_warning("[imageIdentify] BMP2:Cannot handle compressed bmp 0x%008x\n",bmph.biCompression);
                fclose(fd);
                return ADM_PICTURE_UNKNOWN;
            }
            *w = bmph.biWidth;
            *h = bmph.biHeight;
            fclose(fd);
            return ADM_PICTURE_BMP2;
    }
    // Unknown filetype...
    fclose(fd);
    return ADM_PICTURE_UNKNOWN;
}
//EOF

