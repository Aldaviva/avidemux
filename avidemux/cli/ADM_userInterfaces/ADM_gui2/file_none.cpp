
// C++ Interface: 
//
// Description: 
//
//
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "config.h"
#include "ADM_default.h"

#include "DIA_fileSel.h"
#include "DIA_coreToolkit.h"
#include "DIA_coreUI_internal.h"

namespace  ADM_CLI_fileSel
{
//********************
static void GUI_FileSelRead(const char *label,SELFILE_CB cb);
static void GUI_FileSelWrite(const char *label,SELFILE_CB cb);
static void GUI_FileSelRead(const char *label, char * * name);
static void GUI_FileSelWrite(const char *label, char * * name);
static uint8_t FileSel_SelectDir(const char *title,char *target,uint32_t max, const char *source, const char *extension);
static uint8_t FileSel_SelectWrite(const char *title,char *target,uint32_t max, const char *source, const char *extension);
static uint8_t FileSel_SelectRead(const char *title,char *target,uint32_t max, const char *source, const char *extension);
static void init(void);
//********************
void GUI_FileSelRead(const char *label,SELFILE_CB *cb) 
{}
void GUI_FileSelWrite(const char *label,SELFILE_CB *cb) 
{}
void GUI_FileSelRead(const char *label, char * * name)
{}
void GUI_FileSelWrite(const char *label, char * * name)
{}
void GUI_FileSelWriteExtension(const char *label, const char *ext, SELFILE_CB * name)
{}
void GUI_FileSelReadExtension(const char *label, const char *ext,SELFILE_CB * name)
{}

uint8_t FileSel_SelectWrite(const char *title,char *target,uint32_t max, const char *source, const char *extension)
{
	return 0;
}
uint8_t FileSel_SelectRead(const char *title,char *target,uint32_t max, const char *source, const char *extension)
{
  return 0;
}
uint8_t FileSel_SelectDir(const char *title,char *target,uint32_t max, const char *source, const char *extension)
{
	return 0;
}
void init(void)
{
	return ;
}

} //--- end namespace-------------
static DIA_FILESEL_DESC_T CliFileSelDesc=
{
	ADM_CLI_fileSel::init,	
	ADM_CLI_fileSel::GUI_FileSelRead,
	ADM_CLI_fileSel::GUI_FileSelWrite,
	ADM_CLI_fileSel::GUI_FileSelRead,
	ADM_CLI_fileSel::GUI_FileSelWrite,
	ADM_CLI_fileSel::FileSel_SelectWrite,
	ADM_CLI_fileSel::FileSel_SelectRead,
	ADM_CLI_fileSel::FileSel_SelectDir,
        ADM_CLI_fileSel::GUI_FileSelWriteExtension,
        ADM_CLI_fileSel::GUI_FileSelReadExtension
};



// Hook our functions
void initFileSelector(void)
{
	DIA_fileSelInit(&CliFileSelDesc);
}


uint8_t UI_getPhysicalScreenSize(void *window, uint32_t *w,uint32_t *h)
{
  *w=*h=10000; 

  return 1;
}

void UI_setAudioTrackCount( int nb )
{
    
}
