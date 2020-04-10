/***************************************************************************
                             DIA_preview.cpp
                             ---------------

       Previewer, we switch to RGB (only one Xv at a time)   and display the incoming
       YV12 buffer in a nice windowd

    begin                : Mon Mar 25 2002
    copyright            : (C) 2002 by mean
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
#include "DIA_flyDialog.h"
#include "DIA_flyPreview.h"

static GtkWidget *create_dialog1(void);

static GtkWidget *dialog = NULL;
static GtkWidget *vbox1;
static GtkWidget *drawingarea1;

static flySeekablePreview *seekablePreview;
static void seekablePreview_draw(void);
static void seekablePreview_frame_changed(void);

/* =================================
            Filter Preview
   ================================= */
uint8_t DIA_filterPreview(const char *captionText, ADM_coreVideoFilter *videoStream, uint32_t frame)
{
	GtkWidget *hbuttonbox1, *buttonOk, *scale;

	dialog = create_dialog1();
	
	scale = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 110, 1, 10, 10)));
	gtk_widget_show (scale);
	gtk_box_pack_start (GTK_BOX(vbox1), scale, FALSE, TRUE, 0);

	hbuttonbox1 = gtk_hbutton_box_new ();
	gtk_widget_show (hbuttonbox1);
	gtk_box_pack_start (GTK_BOX(vbox1), hbuttonbox1, FALSE, TRUE, 0);
	gtk_button_box_set_layout (GTK_BUTTON_BOX(hbuttonbox1), GTK_BUTTONBOX_END);

	buttonOk = gtk_button_new_from_stock ("gtk-ok");
	gtk_widget_show(buttonOk);
	gtk_container_add (GTK_CONTAINER(hbuttonbox1), buttonOk);
	
	gtk_register_dialog(dialog);

	if (captionText)
		gtk_window_set_title(GTK_WINDOW(dialog), captionText);

	uint32_t width, height;

	width = videoStream->getInfo()->width;
	height = videoStream->getInfo()->height;

	g_signal_connect(scale, "value_changed", G_CALLBACK(seekablePreview_frame_changed), NULL);
	g_signal_connect(drawingarea1, "expose_event", G_CALLBACK(seekablePreview_draw), NULL);
	gtk_dialog_add_action_widget(GTK_DIALOG(dialog), buttonOk, GTK_RESPONSE_OK);

	gtk_widget_show(dialog);

	seekablePreview = new flySeekablePreview(width, height, videoStream, drawingarea1, scale);
//	seekablePreview->process();
	seekablePreview->sliderSet(frame);
	seekablePreview->sliderChanged();

	int response;

	while((response = gtk_dialog_run(GTK_DIALOG(dialog))) == GTK_RESPONSE_APPLY)
	{
		seekablePreview_draw();
	}

	gtk_unregister_dialog(dialog);
	gtk_widget_destroy(dialog);
	delete seekablePreview;

	return (response == GTK_RESPONSE_OK);
}

void seekablePreview_draw(void)
{
	seekablePreview->sameImage();
}

void seekablePreview_frame_changed(void)
{
	seekablePreview->sliderChanged();
}

/* =================================
                Preview
   ================================= */
static flyPreview *preview;
static void preview_draw(void);

void DIA_previewInit(uint32_t width, uint32_t height)
{
	dialog = create_dialog1();
	gtk_widget_set_size_request(drawingarea1, width, height);

	g_signal_connect(drawingarea1, "expose_event", G_CALLBACK(preview_draw), NULL);
	gtk_widget_show(dialog);

	preview = new flyPreview(width, height, drawingarea1);
}

uint8_t DIA_previewStillAlive(void)
{
	return (dialog != NULL);
}

uint8_t DIA_previewUpdate(uint8_t *buffer)
{
	if (dialog)
	{
		preview->display(buffer);
		return 1;
	}

	return 0;
}

void DIA_previewEnd(void)
{
	if(dialog)
	{
		delete preview;

		gtk_widget_destroy(dialog);
		dialog = NULL;
	}
}

void preview_draw(void)
{
	preview->sameImage();
}


/* =================================
             Common Dialog
   ================================= */
GtkWidget*
create_dialog1 (void)
{
	GtkWidget *dialog1;
	GtkWidget *dialog_vbox1;

	dialog1 = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (dialog1), QT_TR_NOOP("Preview"));

	dialog_vbox1 = gtk_dialog_get_content_area (GTK_DIALOG (dialog1));
	gtk_widget_show (dialog_vbox1);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), vbox1, TRUE, TRUE, 0);

	drawingarea1 = gtk_drawing_area_new ();
	gtk_widget_show (drawingarea1);
	gtk_box_pack_start (GTK_BOX (vbox1), drawingarea1, FALSE, TRUE, 0);
	gtk_widget_set_size_request (drawingarea1, 100, 100);

	return dialog1;
}
