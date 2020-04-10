/***************************************************************************
                          DIA_crop.cpp  -  description
                             -------------------

			    GUI for cropping including autocrop
			    +Revisted the Gtk2 way
			     +Autocrop now in RGB space (more accurate)

    begin                : Fri May 3 2002
    copyright            : (C) 2002/2007 by mean
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

#include "Q_hue.h"
#include "ADM_toolkitQt.h"

//
//	Video is in YV12 Colorspace
//
//
  Ui_hueWindow::Ui_hueWindow(QWidget *parent, hue *param,ADM_coreVideoFilter *in) : QDialog(parent)
  {
    uint32_t width,height;
        ui.setupUi(this);
        lock=0;
        // Allocate space for green-ised video
        width=in->getInfo()->width;
        height=in->getInfo()->height;

        canvas=new ADM_QCanvas(ui.graphicsView,width,height);
        
        myCrop=new flyHue( this,width, height,in,canvas,ui.horizontalSlider);
        memcpy(&(myCrop->param),param,sizeof(hue));
        myCrop->_cookie=&ui;
        myCrop->addControl(ui.toolboxLayout);
        myCrop->upload();
        myCrop->sliderChanged();


        connect( ui.horizontalSlider,SIGNAL(valueChanged(int)),this,SLOT(sliderUpdate(int)));
#define SPINNER(x) connect( ui.horizontalSlider##x,SIGNAL(valueChanged(int)),this,SLOT(valueChanged(int))); 
          SPINNER(Hue);
          SPINNER(Saturation);

        setModal(true);
  }
  void Ui_hueWindow::sliderUpdate(int foo)
  {
    myCrop->sliderChanged();
  }
  void Ui_hueWindow::gather(hue *param)
  {
    
        myCrop->download();
        memcpy(param,&(myCrop->param),sizeof(hue));
  }
Ui_hueWindow::~Ui_hueWindow()
{
  if(myCrop) delete myCrop;
  myCrop=NULL; 
  if(canvas) delete canvas;
  canvas=NULL;
}
void Ui_hueWindow::valueChanged( int f )
{
  if(lock) return;
  lock++;
   myCrop->download();
  myCrop->sameImage();
  lock--;
}

void Ui_hueWindow::resizeEvent(QResizeEvent *event)
{
    if(!canvas->height())
        return;
    uint32_t graphicsViewWidth = canvas->parentWidget()->width();
    uint32_t graphicsViewHeight = canvas->parentWidget()->height();
    myCrop->fitCanvasIntoView(graphicsViewWidth,graphicsViewHeight);
    myCrop->adjustCanvasPosition();
}

void Ui_hueWindow::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    myCrop->adjustCanvasPosition();
    canvas->parentWidget()->setMinimumSize(30,30); // allow resizing after the dialog has settled
}

#define MYSPIN(x) w->horizontalSlider##x
#define MYCHECK(x) w->checkBox##x
//************************
uint8_t flyHue::upload(void)
{
      Ui_hueDialog *w=(Ui_hueDialog *)_cookie;

        MYSPIN(Saturation)->setValue((int)(param.saturation*10));
        MYSPIN(Hue)->setValue((int)param.hue);
        return 1;
}
uint8_t flyHue::download(void)
{
       Ui_hueDialog *w=(Ui_hueDialog *)_cookie;
         param.hue=MYSPIN(Hue)->value();
         param.saturation=MYSPIN(Saturation)->value()/10.;
return 1;
}

/**
      \fn     DIA_getCropParams
      \brief  Handle crop dialog
*/
uint8_t DIA_getHue(hue *param,ADM_coreVideoFilter *in)
{
        uint8_t ret=0;
        Ui_hueWindow dialog(qtLastRegisteredDialog(), param,in);

		qtRegisterDialog(&dialog);

        if(dialog.exec()==QDialog::Accepted)
        {
            dialog.gather(param); 
            ret=1;
        }

		qtUnregisterDialog(&dialog);

        return ret;
}
//____________________________________
// EOF


