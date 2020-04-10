/** *************************************************************************
                    \file     rotateGl
                    \brief    Rotate picture

   
                    copyright            : (C) 2011 by mean

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "ADM_openGl.h"
#define ADM_LEGACY_PROGGY
#include "ADM_default.h"
#include "ADM_coreVideoFilterInternal.h"
#include "T_openGLFilter.h"
#include "rotateGl.h"
#include "ADM_clock.h"
#include "rotate.h"
#include "rotate_desc.cpp"
#include "DIA_factory.h"
/**
    \class rotateGl
*/
class rotateGl : public  ADM_coreVideoFilterQtGl
{
protected:
                gl_rotate    params;
                ADMImage    *original;
                GLuint       glList;
                bool         genQuad(void);
protected:
                        bool render(ADMImage *image,ADM_PLANE plane,QOpenGLFramebufferObject *fbo);
public:
                             rotateGl(ADM_coreVideoFilter *previous,CONFcouple *conf);
                            ~rotateGl();

        virtual const char   *getConfiguration(void);                   /// Return  current configuration as a human readable string
        virtual bool         getNextFrame(uint32_t *fn,ADMImage *image);    /// Return the next image
        virtual bool         getCoupledConf(CONFcouple **couples) ;   /// Return the current filter configuration
		virtual void setCoupledConf(CONFcouple *couples);
        virtual bool         configure(void) ;             /// Start graphical user interface
};

// Add the hook to make it valid plugin
DECLARE_VIDEO_FILTER(   rotateGl,   // Class
                        1,0,0,              // Version
                        ADM_UI_QT4+ADM_FEATURE_OPENGL,         // UI
                        VF_OPENGL,            // Category
                        "glRotate",            // internal name (must be uniq!)
                        QT_TRANSLATE_NOOP("glRotate","OpenGl Rotate"),            // Display name
                        QT_TRANSLATE_NOOP("glRotate","Rotate image by a small amount.") // Description
                    );

// Now implements the interesting parts
/**
    \fn rotateGl
    \brief constructor
*/
rotateGl::rotateGl(  ADM_coreVideoFilter *in,CONFcouple *setup) : ADM_coreVideoFilterQtGl(in,setup)
{
UNUSED_ARG(setup);
        original=new ADMImageDefault(in->getInfo()->width,in->getInfo()->height);
        if(!setup || !ADM_paramLoad(setup,gl_rotate_param,&params))
        {
            // Default value
            params.angle=0;
        }
        _parentQGL->makeCurrent();
        fboY->bind();
        printf("Compiling shader \n");
        
        glProgramY =    createShaderFromSource(QOpenGLShader::Fragment,myShaderY);
        if(!glProgramY)
        {
            ADM_error("Cannot setup shader\n");
        }        
                
        glList=glGenLists(1);
        genQuad();
        fboY->release();
        _parentQGL->doneCurrent();

}
/**
    \fn rotateGl
    \brief destructor
*/
rotateGl::~rotateGl()
{
    if(original) delete original;
    original=NULL;
    glDeleteLists(glList,1);
}

/**
    \fn getFrame
    \brief Get a processed frame
*/
bool rotateGl::getNextFrame(uint32_t *fn,ADMImage *image)
{
    if(!glProgramY)
    {
        char strxx[80];
        snprintf(strxx,80,"Shader was not compiled succesfully");
        image->printString(2,4,strxx); 
        return true;
    }    
    
    
    // since we do nothing, just get the output of previous filter
    if(false==previousFilter->getNextFrame(fn,original))
    {
        ADM_warning("glRotate : Cannot get frame\n");
        return false;
    }
    _parentQGL->makeCurrent();
    glPushMatrix();
    // size is the last one...
    fboY->bind();

    glProgramY->setUniformValue("myTextureU", 1); 
    glProgramY->setUniformValue("myTextureV", 2); 
    glProgramY->setUniformValue("myTextureY", 0); 

    uploadAllPlanes(original);

    render(image,PLANAR_Y,fboY);

    downloadTextures(image,fboY);

    fboY->release();
    firstRun=false;
    glPopMatrix();
    _parentQGL->doneCurrent();
    image->copyInfo(original);
    return true;
}
/**
    \fn getCoupledConf
    \brief Return our current configuration as couple name=value
*/
bool         rotateGl::getCoupledConf(CONFcouple **couples)
{
    return ADM_paramSave(couples, gl_rotate_param,&params);
}

void rotateGl::setCoupledConf(CONFcouple *couples)
{
    ADM_paramLoad(couples, gl_rotate_param, &params);
}
/**
    \fn getConfiguration
    \brief Return current setting as a string
*/
const char *rotateGl::getConfiguration(void)
{
    static char st[200];
    snprintf(st,199,"openGl rotate :%d degrees",params.angle);
    return st;
}

/**
    \fn configure
*/
bool rotateGl::configure( void) 
{
    
     
     diaElemInteger  tAngle(&(params.angle),QT_TRANSLATE_NOOP("glRotate","Angle (°):"),-190,190);
   
     
     diaElem *elems[]={&tAngle};
     
     if(diaFactoryRun(QT_TRANSLATE_NOOP("glRotate","glRotate"),sizeof(elems)/sizeof(diaElem *),elems))
     {
                ADM_info("New angle : %d \n",params.angle);
                _parentQGL->makeCurrent();
                genQuad();
                _parentQGL->doneCurrent();
                return 1;
     }
    
     return 0;
}
/**
    \fn     translate
    \brief  compute the quad after rotation
*/

static bool translate(int &x,int &y,float c,float s,int w2,int h2)
{
    float xx=x,xx2;
    float yy=y,yy2;
    xx-=w2;
    yy-=h2;
    
    xx2=xx*c-yy*s;
    yy2=xx*s+yy*c;

    x=xx2+w2;
    y=yy2+h2;
    return true;
}
/**
    \fn     genQuad
    \brief  compute the quad
*/

bool rotateGl::genQuad(void)
{
  int width=info.width;
  int height=info.height;
  int w2=width/2;
  int h2=height/2;

#define PI 3.14159265
  float angleRad=(float)params.angle;
    angleRad=angleRad*PI/180.;

  float c=cos(angleRad);
  float s=sin(angleRad);


#define POINT(a,b) x=a;y=b;translate(x,y,c,s,w2,h2); glTexCoord2i(a, b);glVertex2i(x, y);
  
  glDeleteLists(glList,1);
  glNewList(glList,GL_COMPILE);
  glBegin(GL_QUADS);
        int x=0,y=0;

        POINT(0,0);
        POINT(width,0);
        POINT(width,height);
        POINT(0,height);

  glEnd();
  glEndList();
  return true;
}
/**
    \fn render
*/
bool rotateGl::render(ADMImage *image,ADM_PLANE plane,QOpenGLFramebufferObject *fbo)
{
    int width=image->GetWidth(plane);
    int height=image->GetHeight(plane);

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, 0, height, -1, 1);
    glCallList(glList);
    
    return true;
}
//EOF
