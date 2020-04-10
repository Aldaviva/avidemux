/***************************************************************************
  FAC_toggle.cpp
  Handle dialog factory element : Toggle
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


#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>

#include "ADM_default.h"
#include "DIA_factory.h"
#include "ADM_dialogFactoryQt4.h"

extern const char *shortkey(const char *);

namespace ADM_qt4Factory
{

class diaElemReadOnlyText : public diaElem,QtFactoryUtils
{

public:
  
  diaElemReadOnlyText(const char *readyOnly,const char *toggleTitle,const char *tip=NULL);
  virtual ~diaElemReadOnlyText() ;
  void setMe(void *dialog, void *opaque,uint32_t line);
  void getMe(void);
  int getRequiredLayout(void);
};

class diaElemText : public diaElem,QtFactoryUtils
{

public:
  
  diaElemText(char **text,const char *toggleTitle,const char *tip=NULL);
  virtual ~diaElemText() ;
  void setMe(void *dialog, void *opaque,uint32_t line);
  void getMe(void);
  void enable(uint32_t onoff);
  int getRequiredLayout(void);
};

//********************************************************************
diaElemReadOnlyText::diaElemReadOnlyText(const char *readyOnly,const char *toggleTitle,const char *tip)
  : diaElem(ELEM_TOGGLE),QtFactoryUtils(toggleTitle)
{
  param=(void *)ADM_strdup(readyOnly);
  this->tip=tip;
 }

diaElemReadOnlyText::~diaElemReadOnlyText()
{
 
}
void diaElemReadOnlyText::setMe(void *dialog, void *opaque,uint32_t line)
{

  QGridLayout *layout=(QGridLayout*) opaque;

   
 
 QLabel *text=new QLabel( myQtTitle,(QWidget *)dialog);
  QLabel *text2=new QLabel( QString::fromUtf8((char *)param),(QWidget *)dialog);
 text->setBuddy(text2);
 layout->addWidget(text,line,0);
 layout->addWidget(text2,line,1);
 myWidget=(void *)text2;  
}
void diaElemReadOnlyText::getMe(void)
{

 
}

int diaElemReadOnlyText::getRequiredLayout(void) { return FAC_QT_GRIDLAYOUT; }

//*********************************

diaElemText::diaElemText(char **text,const char *toggleTitle,const char *tip)
    : diaElem(ELEM_TEXT),QtFactoryUtils(toggleTitle)
{
  
  if(!*text) *text=ADM_strdup("");
  param=(void *)text;
  this->tip=tip;
 }

diaElemText::~diaElemText()
{
 
}
void diaElemText::setMe(void *dialog, void *opaque,uint32_t line)
{

 QGridLayout *layout=(QGridLayout*) opaque;
 
 QLabel *text=new QLabel(myQtTitle,(QWidget *)dialog);
 QLineEdit *lineEdit = new QLineEdit( QString::fromUtf8(*(char **)param));
 
 text->setBuddy(lineEdit);
 layout->addWidget(text,line,0);
 layout->addWidget(lineEdit,line,1);
 myWidget=(void *)lineEdit;  
}
void diaElemText::getMe(void)
{
  char **c=(char **)param;
  QLineEdit *lineEdit=(QLineEdit *)myWidget;
  ADM_assert(lineEdit);
  if(*c) ADM_dealloc(*c);
  *c=ADM_strdup(lineEdit->text().toLatin1().data());
 
}
void diaElemText::enable(uint32_t onoff)
{
  ADM_assert(myWidget);
 QLineEdit *lineEdit=(QLineEdit *)myWidget;
  ADM_assert(lineEdit);
  if(onoff)
    lineEdit->setEnabled(true);
  else
    lineEdit->setDisabled(true);
}

int diaElemText::getRequiredLayout(void) { return FAC_QT_GRIDLAYOUT; }
} // End of namespace
//****************************Hoook*****************

diaElem  *qt4CreateRoText(const char *text,const char *toggleTitle, const char *tip)
{
	return new  ADM_qt4Factory::diaElemReadOnlyText(text,toggleTitle,tip);
}
void qt4DestroyRoText(diaElem *e)
{
	ADM_qt4Factory::diaElemReadOnlyText *a=(ADM_qt4Factory::diaElemReadOnlyText *)e;
	delete a;
}

diaElem  *qt4CreateText(char **text,const char *toggleTitle, const char *tip)
{
	return new  ADM_qt4Factory::diaElemText(text,toggleTitle,tip);
}
void qt4DestroyText(diaElem *e)
{
	ADM_qt4Factory::diaElemText *a=(ADM_qt4Factory::diaElemText *)e;
	delete a;
}
//EOF
