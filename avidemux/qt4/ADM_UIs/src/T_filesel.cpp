/***************************************************************************
  FAC_filesel.cpp
  Handle dialog factory element : Filesel
  (C) 2006 Mean Fixounet@free.fr 
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "T_filesel.h"
#include "ADM_default.h"
//#include "prefs.h"
#include "DIA_factory.h"
#include "DIA_fileSel.h"
#include "ADM_dialogFactoryQt4.h"
#include "ADM_last.h"

extern const char *shortkey(const char *);
#define MAX_SEL 2040

namespace ADM_Qt4Factory
{
class diaElemFile : public diaElemFileBase
{

protected:
    
public:
  
  diaElemFile(uint32_t writeMode,std::string &filename,const char *toggleTitle,
              const char *defaultSuffix = 0,const char *tip=NULL);
  virtual ~diaElemFile() ;
  void setMe(void *dialog, void *opaque,uint32_t line);
  void getMe(void);
  
  void   changeFile(void);
  void   enable(uint32_t onoff);
  int    getRequiredLayout(void);
};
class diaElemDirSelect : public diaElemDirSelectBase
{

public:
  
  diaElemDirSelect(std::string &filename,const char *toggleTitle,const char *tip=NULL);
  virtual ~diaElemDirSelect() ;
  void setMe(void *dialog, void *opaque,uint32_t line);
  void getMe(void);
  
  void changeFile(void);
  void   enable(uint32_t onoff);
  int getRequiredLayout(void);
};

void ADM_Qfilesel::buttonPressed(QAbstractButton *s)
{ 
	uint8_t r=0;
	char buffer[MAX_SEL+1];
        //const char *txt="";
	std::string lastFolder;
	switch(fileMode)
	{
		case ADM_FILEMODE_READ:
			admCoreUtils::getLastReadFolder(lastFolder);
			r=FileSel_SelectRead(selectDesc,buffer,MAX_SEL,lastFolder.c_str());
			break;
		case ADM_FILEMODE_WRITE:
//#warning FIXME
#if 0
			if (defaultSuffix)
			{
				const char * lastfilename;
				if (prefs->get(LASTFILES_FILE1,(ADM_filename **)&lastfilename))
				{
					strcpy (buffer, lastfilename);
					char * cptr = buffer + strlen (buffer);
					while (cptr > buffer)
					{
						if (*cptr == '.')
						{
							strcpy (cptr + 1, defaultSuffix);
							txt = buffer;
							printf ("Default output filename is %s based on "
								"%s + %s\n",
								txt, lastfilename, defaultSuffix);
							break;
						}
						--cptr;
					}
				}
			}
#endif
			admCoreUtils::getLastWriteFolder(lastFolder);
			r=FileSel_SelectWrite(selectDesc,buffer,MAX_SEL,lastFolder.c_str());
			break;

		case ADM_FILEMODE_DIR:
			admCoreUtils::getLastReadFolder(lastFolder);
			r=FileSel_SelectDir(selectDesc,buffer,MAX_SEL,lastFolder.c_str());
			break;
		default:
			ADM_assert(0);break;
	}
	if(r)
	{
            edit->setText(QString::fromUtf8(buffer));
	}
}

ADM_Qfilesel::ADM_Qfilesel(const char *title,std::string &init,QGridLayout *layout,int line, ADM_fileMode mode, const char * defaultSuffix, const char* selectDesc) : 
	defaultSuffix (defaultSuffix), selectDesc (selectDesc)
{          
	fileMode=mode;
	edit=new QLineEdit(QString::fromUtf8(init.c_str()));
	button=new QDialogButtonBox(QDialogButtonBox::Open,Qt::Horizontal);
	text=new QLabel(QString::fromUtf8(title));

	text->setBuddy(edit);
	layout->addWidget(text,line,0);
	layout->addWidget(edit,line,1);
	layout->addWidget(button,line,2);
	//QObject::connect(&button, SIGNAL(accepted()), NULL, SLOT(accept())); 
	connect( button,SIGNAL(clicked(QAbstractButton  *)),this,SLOT(buttonPressed(QAbstractButton  *)));
}

ADM_Qfilesel::~ADM_Qfilesel() 
{
#if 1 //Memleak or autoclean ?
	if(edit) delete edit;
	if(button) delete button;
	if(text) delete text;
#endif
};

diaElemFile::diaElemFile(uint32_t writemode, std::string &filename,const char *toggleTitle,
                         const char *defaultSuffix,const char *selectFileDesc)
  : diaElemFileBase()
    
{
  this->defaultSuffix=defaultSuffix;
  param=(void *)&filename;
  paramTitle=shortkey(toggleTitle);

  if (!selectFileDesc || *(selectFileDesc) == 0)
	  tip = toggleTitle;
  else
      tip = selectFileDesc;

  _write=writemode;
}

diaElemFile::~diaElemFile()
{
  if(paramTitle)
    ADM_dealloc(paramTitle);
}

void diaElemFile::setMe(void *dialog, void *opaque,uint32_t line)
{
 QGridLayout *layout=(QGridLayout*) opaque;
 ADM_Qfilesel *fs;
  if(_write)
      fs=new ADM_Qfilesel(paramTitle, *((std::string *)param), layout, line,ADM_FILEMODE_WRITE, defaultSuffix, tip);
  else
      fs=new ADM_Qfilesel(paramTitle, *((std::string *)param), layout, line,ADM_FILEMODE_READ, 0, tip);
  myWidget=(void *)fs; 
}

void diaElemFile::getMe(void)
{
   std::string *n=( std::string *)param;
   ADM_Qfilesel *fs=(ADM_Qfilesel *)myWidget;
   QString s=(fs->edit)->text();
   *n=std::string(s.toUtf8().constData());
}

void diaElemFile::enable(uint32_t onoff)
{
	ADM_Qfilesel *fs = (ADM_Qfilesel*)myWidget;
	ADM_assert(fs);

	fs->text->setEnabled(onoff);
	fs->edit->setEnabled(onoff);
	fs->button->setEnabled(onoff);
}
void diaElemFile::changeFile(void) {}

int diaElemFile::getRequiredLayout(void) { return FAC_QT_GRIDLAYOUT; }

//****************************
diaElemDirSelect::diaElemDirSelect(std::string &filename,const char *toggleTitle,const char *selectDirDesc) :
	diaElemDirSelectBase()
{
  param=(void *)&filename;
  paramTitle=shortkey(toggleTitle);

  if (!selectDirDesc || *(selectDirDesc) == 0)
	  tip = toggleTitle;
  else
      tip = selectDirDesc;
}

diaElemDirSelect::~diaElemDirSelect() 
{
if(paramTitle)
    ADM_dealloc( paramTitle);
}

void diaElemDirSelect::setMe(void *dialog, void *opaque,uint32_t line)
{
 QGridLayout *layout=(QGridLayout*) opaque;
  
  ADM_Qfilesel *fs=new ADM_Qfilesel(paramTitle, *(std::string *)param, layout, line, ADM_FILEMODE_DIR, 0, tip);
  myWidget=(void *)fs; 
}

void diaElemDirSelect::getMe(void) 
{
  ADM_Qfilesel *fs=(ADM_Qfilesel *)myWidget;
  QString s=(fs->edit)->text();
  std::string *n=( std::string *)param;
  *n=std::string(s.toUtf8().constData());
}

void diaElemDirSelect::enable(uint32_t onoff)
{
	ADM_Qfilesel *fs = (ADM_Qfilesel*)myWidget;

	ADM_assert(fs);

	fs->text->setEnabled(onoff);
	fs->edit->setEnabled(onoff);
	fs->button->setEnabled(onoff);
}

void diaElemDirSelect::changeFile(void) {}

int diaElemDirSelect::getRequiredLayout(void) { return FAC_QT_GRIDLAYOUT; }
} // End of namespace
//****************************Hoook*****************

diaElem  *qt4CreateFile(uint32_t writeMode,std::string &filename,const char *toggleTitle,
        const char *defaultSuffix ,const char *tip)
{
	return new  ADM_Qt4Factory::diaElemFile(writeMode,filename,toggleTitle,defaultSuffix ,tip);
}
void qt4DestroyFile(diaElem *e)
{
	ADM_Qt4Factory::diaElemFile *a=(ADM_Qt4Factory::diaElemFile *)e;
	delete a;
}

diaElem  *qt4CreateDir(std::string &filename,const char *toggleTitle,const char *tip)
{
	return new  ADM_Qt4Factory::diaElemDirSelect(filename,toggleTitle,tip);
}
void qt4DestroyDir(diaElem *e)
{
	ADM_Qt4Factory::diaElemDirSelect *a=(ADM_Qt4Factory::diaElemDirSelect *)e;
	delete a;
}
//EOF
