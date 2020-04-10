/** *************************************************************************
                    \file     shaderLoader
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
#include <string>
#include "ADM_openGl.h"
#define ADM_LEGACY_PROGGY
#include "ADM_default.h"
#include "ADM_coreVideoFilterInternal.h"
#include "T_openGLFilter.h"
#include "ADM_clock.h"
#include "DIA_factory.h"
#include "shaderLoader.h"
#include "shaderLoader_desc.cpp"
#include "DIA_coreToolkit.h"
/**
    \class shaderLoader
*/
class shaderLoader : public  ADM_coreVideoFilterQtGl
{
protected:
                shaderLoaderConf params;
                GLuint       glList;
                ADMImage    *original;
                bool         ready;
                std::string  erString;
                bool         genQuad(void);

protected:
                bool        render(ADMImage *image,ADM_PLANE plane,QOpenGLFramebufferObject *fbo);
                bool        loadShader(const char *src);
                bool        reload( void);
    static      void        cb(void *c);
public:
                             shaderLoader(ADM_coreVideoFilter *previous,CONFcouple *conf);
                            ~shaderLoader();

        virtual const char   *getConfiguration(void);                   /// Return  current configuration as a human readable string
        virtual bool         getNextFrame(uint32_t *fn,ADMImage *image);    /// Return the next image
        virtual bool         getCoupledConf(CONFcouple **couples) ;   /// Return the current filter configuration
		virtual void         setCoupledConf(CONFcouple *couples);
        virtual bool         configure(void) ;             /// Start graphical user interface
};

// Add the hook to make it valid plugin
DECLARE_VIDEO_FILTER_PARTIALIZABLE(   shaderLoader,   // Class
                        1,0,0,              // Version
                        ADM_UI_QT4+ADM_FEATURE_OPENGL,         // UI
                        VF_OPENGL,            // Category
                        "shaderLoader",            // internal name (must be uniq!)
                        QT_TRANSLATE_NOOP("glShader","Shader Loader"),            // Display name
                        QT_TRANSLATE_NOOP("glShader","Run an external shader program.") // Description
                    );


/**
 * \fn loadShader
 * @param src
 * @return 
 */
bool shaderLoader::loadShader(const char *src)
{
    
    if(!ADM_fileExist(src))
    {
        ADM_warning("Shader file does not exist (%s)\n",src);
        return false;
    }
    
    int sourceSize=ADM_fileSize(src);
    if(sourceSize<5)
    {
        ADM_warning("Shader file is too short(%s)\n",src);
        return false;
    }
    
    uint8_t *buffer=(uint8_t *)admAlloca(sourceSize+1);
    FILE *f=fopen(params.shaderFile.c_str(),"rt");
    if(!f)
    {
        ADM_warning("Cannot open file %s\n",src);
        return false;
    }
        
    fread(buffer,sourceSize,1,f);
    buffer[sourceSize]=0;
    fclose(f);
    f=NULL;

    if(glProgramY)
    {
        delete glProgramY;
        glProgramY=NULL;
    }
    glProgramY =    createShaderFromSource(QOpenGLShader::Fragment,(char *)buffer);
    if(!glProgramY)
    {
        ADM_error("Shader compiling failed\n");
        return false;
    }
    ADM_info("Shader %s successfully loaded an compiled\n",src);
    return true;
}

/**
    \fn shaderLoader
    \brief constructor
*/
shaderLoader::shaderLoader(  ADM_coreVideoFilter *in,CONFcouple *setup) : ADM_coreVideoFilterQtGl(in,setup)
{
        if(!setup || !ADM_paramLoad(setup,shaderLoaderConf_param,&params))
        {
            params.shaderFile=strdup("");
        }
        original=new ADMImageDefault(in->getInfo()->width,in->getInfo()->height);
        _parentQGL->makeCurrent();
        fboY->bind();
        ready=false;

        ADM_info("Compiling shader %s \n",params.shaderFile.c_str());

        // Load the file info memory
        ready=loadShader(params.shaderFile.c_str());
        
        glList=glGenLists(1);
        genQuad();
        fboY->release();
        _parentQGL->doneCurrent();
}
/**
    \fn shaderLoader
    \brief destructor
*/
shaderLoader::~shaderLoader()
{
    if(original) delete original;
    original=NULL;
    glDeleteLists(glList,1);

}

/**
    \fn getFrame
    \brief Get a processed frame
*/
bool shaderLoader::getNextFrame(uint32_t *fn,ADMImage *image)
{
    // since we do nothing, just get the output of previous filter
    if(false==previousFilter->getNextFrame(fn,original))
    {
        ADM_warning("glRotate : Cannot get frame\n");
        return false;
    }
    if(!ready)
    {
        ADM_info("OpenGl shader not loaded (%s)\n",erString.c_str());
        image->duplicateFull(original);
        image->printString(2,2,"Shader not loaded");
        image->printString(2,2,erString.c_str());
        return true;
    }
    _parentQGL->makeCurrent();
    glPushMatrix();
    // size is the last one...
    fboY->bind();

    glProgramY->setUniformValue("myTextureU", 1);
    glProgramY->setUniformValue("myTextureV", 2);
    glProgramY->setUniformValue("myTextureY", 0);
    glProgramY->setUniformValue("myResolution", (GLfloat)info.width,(GLfloat)info.height);

    glProgramY->setUniformValue("pts", (GLfloat)original->Pts);

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
bool         shaderLoader::getCoupledConf(CONFcouple **couples)
{
    return ADM_paramSave(couples, shaderLoaderConf_param,&params);
}

void shaderLoader::setCoupledConf(CONFcouple *couples)
{
    ADM_paramLoad(couples, shaderLoaderConf_param, &params);
}
/**
    \fn getConfiguration
    \brief Return current setting as a string
*/
const char *shaderLoader::getConfiguration(void)
{
    static char st[200];
    snprintf(st,199,"Shader Loader %s",params.shaderFile.c_str());
    return st;
}

void shaderLoader::cb(void *c)
{
    shaderLoader *sl=(shaderLoader *)c;
    sl->ready=sl->reload();
}
/**
 * 
 * @return 
 */
bool shaderLoader::reload( void)
{
        _parentQGL->makeCurrent();
        fboY->bind();
        ADM_info("Compiling shader %s \n",params.shaderFile.c_str());
        // Load the file info memory
        ready=loadShader(params.shaderFile.c_str());        
        glList=glGenLists(1);
        genQuad();
        fboY->release();
        _parentQGL->doneCurrent();
        if(!ready)
        {
             GUI_Error_HIG (QT_TRANSLATE_NOOP("adm","Cannot compile shader"), NULL);
             return false;
        }
        return true;
}
/**
    \fn configure
*/
bool shaderLoader::configure( void)
{
again:
    diaElemFile shader(0,params.shaderFile,QT_TRANSLATE_NOOP("glShader","ShaderFile to load"));    
    diaElemButton reloadButton(QT_TR_NOOP("Reload shader"),cb,this);
    diaElem *elems[]={&shader,&reloadButton};
    
     if(diaFactoryRun(QT_TRANSLATE_NOOP("glShader","ShaderLoader"),sizeof(elems)/sizeof(diaElem *),elems))
     {
        ready=reload();
        if(!ready)
        {
             goto again;
        }
        return true;                
     }
     return false;
}
/**
 *
 * @return
 */
bool shaderLoader::genQuad(void)
{
  int width=info.width;
  int height=info.height;

#define POINT(a,b)  glTexCoord2i(a, b);glVertex2i(a, b);

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
bool shaderLoader::render(ADMImage *image,ADM_PLANE plane,QOpenGLFramebufferObject *fbo)
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
