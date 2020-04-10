/** *************************************************************************
                    \fn       openGlFragmentSample.cpp  
                    \brief    simple fragment shader

    This one is combining the 3 textures and apply the shader once


    copyright            : (C) 2011 by mean

bench : 1280*720, null shader, 20 ms, 95% of it in download texture.
            Download Texture
                RGB2Y=5ms               (MMX it)
                toQimage=14 ms  <<==    TOO SLOW

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
#include "sampleGl.h"
#include "ADM_clock.h"
#include "DIA_factory.h"


#include "resize.h"
#include "resize_desc.cpp"

/**
    \class openGlResize
*/
class openGlResize : public  ADM_coreVideoFilterQtGl
{
protected:

protected:
                ADMImage    *original;
                gl_resize    configuration;
                        bool render(ADMImage *image,ADM_PLANE plane,QOpenGLFramebufferObject *fbo);
public:
                             openGlResize(ADM_coreVideoFilter *previous,CONFcouple *conf);
                            ~openGlResize();

        virtual const char   *getConfiguration(void);                   /// Return  current configuration as a human readable string
        virtual bool         getNextFrame(uint32_t *fn,ADMImage *image);    /// Return the next image
        virtual bool         getCoupledConf(CONFcouple **couples) ;   /// Return the current filter configuration
		virtual void setCoupledConf(CONFcouple *couples);
                bool         configure( void) ;
};

// Add the hook to make it valid plugin
DECLARE_VIDEO_FILTER(   openGlResize,   // Class
                        1,0,0,              // Version
                        ADM_UI_QT4+ADM_FEATURE_OPENGL,         // UI
                        VF_OPENGL,            // Category
                        "glResize",            // internal name (must be uniq!)
                        QT_TRANSLATE_NOOP("glResize","OpenGl Resize"),            // Display name
                        QT_TRANSLATE_NOOP("glResize","Resize using openGl.") // Description
                    );

// Now implements the interesting parts
/**
    \fn openGlResize
    \brief constructor
*/
openGlResize::openGlResize(  ADM_coreVideoFilter *in,CONFcouple *setup) : ADM_coreVideoFilterQtGl(in,setup)
{
UNUSED_ARG(setup);
        if(!setup || !ADM_paramLoad(setup,gl_resize_param,&configuration))
        {
            // Default value
            configuration.width=info.width;
            configuration.height=info.height;
        }
        original=new ADMImageDefault(in->getInfo()->width,in->getInfo()->height);
        info.width=configuration.width;
        info.height=configuration.height;
        resizeFBO(info.width,info.height);
        _parentQGL->makeCurrent();
        fboY->bind();
        printf("Compiling shader \n");
        glProgramY =    createShaderFromSource(QOpenGLShader::Fragment,myShaderY);
        if ( !glProgramY)
        {
            ADM_error("[GL Render] Cannot compile shader\n");            
        }

        fboY->release();
        _parentQGL->doneCurrent();
}
        
/**
    \fn openGlResize
    \brief destructor
*/
openGlResize::~openGlResize()
{
        delete original;
        original=NULL;
}

/**
    \fn getFrame
    \brief Get a processed frame
*/
bool openGlResize::getNextFrame(uint32_t *fn,ADMImage *image)
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
        ADM_warning("FlipFilter : Cannot get frame\n");
        return false;
    }
    _parentQGL->makeCurrent();
    glPushMatrix();
    fboY->bind();
    checkGlError("bind");
    uploadAllPlanes(original);
    glProgramY->setUniformValue("myTextureU", 1); 
    glProgramY->setUniformValue("myTextureV", 2); 
    glProgramY->setUniformValue("myTextureY", 0); 
    render(image,PLANAR_Y,fboY);
    downloadTextures(image,fboY);
    image->copyInfo(original);
    firstRun=false;
    fboY->release();
    glPopMatrix();
    _parentQGL->doneCurrent();
    checkGlError("last");
    return true;
}
/**
    \fn getCoupledConf
    \brief Return our current configuration as couple name=value
*/
bool         openGlResize::getCoupledConf(CONFcouple **couples)
{
    return ADM_paramSave(couples, gl_resize_param,&configuration);
}

void openGlResize::setCoupledConf(CONFcouple *couples)
{
    ADM_paramLoad(couples, gl_resize_param, &configuration);
}
/**
    \fn getConfiguration
    \brief Return current setting as a string
*/
const char *openGlResize::getConfiguration(void)
{
    static char bfer[256];
    snprintf(bfer,255,"Resize to %d x %d.",(int)configuration.width,(int)configuration.height);
    return bfer;
}


/**
    \fn render
*/
bool openGlResize::render(ADMImage *image,ADM_PLANE plane,QOpenGLFramebufferObject *fbo)
{
    int width=image->GetWidth(plane);
    int height=image->GetHeight(plane);

    int orgwidth=previousFilter->getInfo()->width;
    int orgheight=previousFilter->getInfo()->height;

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, 0, height, -1, 1);
    checkGlError("setup");
    //
    glBegin(GL_QUADS);
        glTexCoord2i(0, 0);
        glVertex2i(0, 0);
        glTexCoord2i(orgwidth, 0);
        glVertex2i(width, 0);
        glTexCoord2i(orgwidth, orgheight);
        glVertex2i(width ,height);
        glTexCoord2i(0, orgheight);
        glVertex2i(0, height);
	glEnd();	// draw cube background
    checkGlError("Draw");
    return true;
}
/**
    \fn updateInfo
*/
bool openGlResize::configure( void) 
{
    
     
     diaElemUInteger  tWidth(&(configuration.width),QT_TRANSLATE_NOOP("glResize","Width :"),16,2048);
     diaElemUInteger  tHeight(&(configuration.height),QT_TRANSLATE_NOOP("glResize","Height :"),16,2048);
     
     diaElem *elems[]={&tWidth,&tHeight};
     
     if(diaFactoryRun(QT_TRANSLATE_NOOP("glResize","glResize"),sizeof(elems)/sizeof(diaElem *),elems))
     {
                info.width=configuration.width;
                info.height=configuration.height;
                ADM_info("New dimension : %d x %d\n",info.width,info.height);
                firstRun=true;
                resizeFBO(info.width,info.height);
                return 1;
     }
    
     return 0;
}
//EOF
