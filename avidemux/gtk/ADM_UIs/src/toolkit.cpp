/***************************************************************************
                          toolkit.cpp  -  description
                            -------------------



    begin                : Fri Dec 14 2001
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
#include "ADM_toolkitGtk.h"

#ifndef _WIN32
#include <unistd.h>
#endif


static  GtkWidget *guiRootWindow=NULL;

GtkWidget *GUI_PixmapButtonDefault(GdkWindow * window, const gchar ** xpm,
                                  const gchar * tooltip);
GtkWidget *GUI_PixmapButton(GdkWindow * window, const gchar ** xpm,
                            const gchar * tooltip, gint border);




/**
 *      \fn ADM_initUIGtk
 *      \brief Init the library with application windows id
 */
void ADM_initUIGtk(GtkWidget *window)
{
    
    guiRootWindow=window;
}

//
//_______________________________

//
GtkWidget *GUI_PixmapButtonDefault(GdkWindow * window, const gchar ** xpm,
                                  const gchar * tooltip)
{
    //return GUI_PixmapButton(window, xpm, tooltip, 2);
#warning broken
        return NULL;
}

void runDialog(volatile int *lock)
{
        
              while (!*lock)
              {
                                while (gtk_events_pending())    		  	gtk_main_iteration();         
                        ADM_usleep(  50000L);			// wait 50 ms
                }
}

// read an entry as an integer

int gtk_read_entry(GtkWidget *entry)
{
char *str;
int value=0;
                str =		  gtk_editable_get_chars(GTK_EDITABLE ((entry)), 0, -1);
                value = (int) atoi(str);
                return value;
}
float gtk_read_entry_float(GtkWidget *entry)
{
char *str;
float value=0;
                str =		  gtk_editable_get_chars(GTK_EDITABLE ((entry)), 0, -1);
                value = (int) atof(str);
                return value;
}

void gtk_write_entry(GtkWidget *entry, int value)
{
char string[400];
gint r=0;
                sprintf(string,"%d",value);
                gtk_editable_delete_text(GTK_EDITABLE(entry), 0,-1);
                gtk_editable_insert_text(GTK_EDITABLE(entry), string, strlen(string), &r);
}
void gtk_write_entry_float(GtkWidget *entry, float value)
{

gint r=0;
char string[400];
                sprintf(string,"%f",value);		
                gtk_editable_delete_text(GTK_EDITABLE(entry), 0,-1);
                gtk_editable_insert_text(GTK_EDITABLE(entry), string, strlen(string), &r);
}

void gtk_write_entry_string(GtkWidget *entry, char *value)
{

gint r=0;
                if(!value) return;
                
                gtk_editable_delete_text(GTK_EDITABLE(entry), 0,-1);
                gtk_editable_insert_text(GTK_EDITABLE(entry), value, strlen(value), &r);
}

/**
    \fn UI_getPhysicalScreenSize
    \brief return the physical size of display in pixels
*/
uint8_t UI_getPhysicalScreenSize(void *window, uint32_t *w, uint32_t *h)
{
	GtkWindow* gtkWindow = (GtkWindow*)window;
	GdkScreen *screen = gdk_screen_get_default();

	if (!gtkWindow)
		gtkWindow = (GtkWindow*)guiRootWindow;

	int monitorNo = gdk_screen_get_monitor_at_window(screen, gtk_widget_get_window(GTK_WIDGET(gtkWindow)));
	GdkRectangle rect;

	gdk_screen_get_monitor_geometry(screen, monitorNo, &rect);

	*w = rect.width;
	*h = rect.height;

	return 1;
}


#ifdef ENABLE_WINDOW_SIZING_HACK

// This version depends on GUI_bindings.cpp providing maxWindowWidth and maxWindowHeight.

// Calculate the zoom ratio required to fit the whole image on the screen.
float UI_calcZoomToFitScreen(GtkWindow* window, GtkWidget* drawingArea, uint32_t imageWidth, uint32_t imageHeight)
{
    int windowWidth, windowHeight;
    int drawingWidth, drawingHeight;
    int reqWidth, reqHeight;
    uint32_t screenWidth, screenHeight;
        
    gtk_window_get_size(window, &windowWidth, &windowHeight);
    GtkRequisition size_req;
    gtk_widget_size_request (drawingArea, &size_req);
    drawingWidth = size_req.width;
    drawingHeight = size_req.height;
    gtk_widget_get_size_request(drawingArea, &reqWidth, &reqHeight);

    // Take borders and captions into consideration (GTK doesn't seem to
    // support this so we'll have to guess)
    windowWidth += 10;
    windowHeight += 10;

    // Take drawing area out of the equation, how much extra do we need for additional controls?
    // and then how much does that leave us for the image?
    uint32_t availableWidth = maxWindowWidth - (windowWidth - drawingWidth);
    uint32_t availableHeight = maxWindowHeight - (windowHeight - drawingHeight);

    float ratio;
    // Calculate zoom ratio
    if (imageWidth > availableWidth || imageHeight > availableHeight)
    {
        float wratio = (imageWidth <= availableWidth) ? 1
                       : (float (availableWidth) / float (imageWidth));
        float hratio = (imageHeight <= availableHeight) ? 1
                       : (float (availableHeight) / float (imageHeight));
        if (wratio < hratio)
            ratio = wratio;
        else
            ratio = hratio;
    }
    else
        ratio = 1;

    printf ("UI_calcZoomToFitScreen(): max %dx%d, win %dx%d, drawarea %dx%d (%dx%d), "
            "available %dx%d, image %dx%d, scale %.6f\n",
            maxWindowWidth, maxWindowHeight, windowWidth, windowHeight,
            drawingWidth, drawingHeight, reqWidth, reqHeight,
            availableWidth, availableHeight, imageWidth, imageHeight, ratio);

    return ratio;
}

#else

// Calculate the zoom ratio required to fit the whole image on the screen.
float UI_calcZoomToFitScreen(GtkWindow* window, GtkWidget* drawingArea, uint32_t imageWidth, uint32_t imageHeight)
{
        int windowWidth, windowHeight;
        int drawingWidth, drawingHeight;
        uint32_t screenWidth, screenHeight;
        
        gtk_window_get_size(window, &windowWidth, &windowHeight);
        gtk_widget_get_size_request(drawingArea, &drawingWidth, &drawingHeight);

        UI_getPhysicalScreenSize(window, &screenWidth, &screenHeight);

        // Take drawing area out of the equation, how much extra do we need for additional controls?
        windowWidth -= drawingWidth;
        windowHeight -= drawingHeight;

        // Take borders and captions into consideration (GTK doesn't seem to support this so we'll have to guess)
        windowWidth += 10;
        windowHeight += 40;

        // This is the true amount of screen real estate we can work with
        screenWidth -= windowWidth;
        screenHeight -= windowHeight;

        // Calculate zoom ratio
        if (imageWidth > screenWidth || imageHeight > screenHeight)
        {
                if ((int)(imageWidth - screenWidth) > (int)(imageHeight - screenHeight))
                        return (float)screenWidth / (float)imageWidth;
                else
                        return (float)screenHeight / (float)imageHeight;
        }
        else
                return 1;
}

#endif

// GTK doesn't centre the window correctly.  Use this function to centre windows with a canvas that is yet to resized.
void UI_centreCanvasWindow(GtkWindow *window, GtkWidget *canvas, int newCanvasWidth, int newCanvasHeight)
{
        int winWidth, winHeight, widgetWidth, widgetHeight;
        GdkScreen *screen = gdk_screen_get_default();
        GdkRectangle rect;
        GtkWidget *referenceWidget;

        if (gtk_window_get_transient_for(window) == NULL)
                referenceWidget = guiRootWindow;
        else
                referenceWidget = GTK_WIDGET(gtk_window_get_transient_for(window));

        int monitorNo = gdk_screen_get_monitor_at_window(screen, gtk_widget_get_window(referenceWidget));

        gdk_screen_get_monitor_geometry(screen, monitorNo, &rect);
        gtk_widget_get_size_request((GtkWidget*)canvas, &widgetWidth, &widgetHeight);
        gtk_window_get_size(window, &winWidth, &winHeight);

        winWidth = newCanvasWidth;
        winHeight = (winHeight - widgetHeight) + newCanvasHeight;

        // Take borders and captions into consideration (GTK doesn't seem to support this so we'll have to guess)
        winWidth += 10;
        winHeight += 40;

        gtk_window_move(window, rect.x + (rect.width - winWidth) / 2, rect.y + (rect.height - winHeight) / 2);
}

//EOF
