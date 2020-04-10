/***************************************************************************
                                Q_about.cpp
                                -----------

    begin                : Fri May 5 2008
    copyright            : (C) 2008 by gruntster
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "config.h"
#include "Q_about.h"
#include "ADM_inttype.h"
#include "ADM_toolkitQt.h"

extern uint8_t DIA_license(void);

Ui_aboutWindow::Ui_aboutWindow(QWidget* parent) : QDialog(parent)
{
    Q_INIT_RESOURCE(about);

    ui.setupUi(this);

    connect(ui.licenseButton, SIGNAL(clicked(bool)), this, SLOT(licenseButton_clicked(bool)));

    char subversion[128]={0};
    const char *compiler="";
#ifdef WIN32
        #ifdef _MSC_VER
            compiler="-VC++";
        #else
            compiler="-Gcc";
        #endif
#endif
#if defined(ADM_SUBVERSION)
        sprintf(subversion,"%s <br><small>(%s-fflibs %s%s)</small>", ADM_VERSION, ADM_SUBVERSION, ADM_FFMPEG_VERSION,compiler);
#else
        sprintf(subversion,"%s - Release",ADM_VERSION);
#endif
        QString  sv(subversion);
    ui.versionLabel->setTextFormat(Qt::RichText);
    ui.versionLabel->setText(ui.versionLabel->text() + sv);
}

void Ui_aboutWindow::licenseButton_clicked(bool)
{
    DIA_license();
}

uint8_t DIA_about(void)
{
    Ui_aboutWindow dialog(qtLastRegisteredDialog());
    qtRegisterDialog(&dialog);

    dialog.exec();

    qtUnregisterDialog(&dialog);

    return 1;
}
