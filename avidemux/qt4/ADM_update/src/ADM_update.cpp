/**
    \file ADM_update.cpp
    \brief Check for update
    \author mean (c) 2016
*/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "ADM_update.h"
#include "ADM_updateImpl.h"
#include "QTimer"
#include "ADM_default.h"
#include "ADM_string.h"
/**
 */
ADMCheckUpdate::ADMCheckUpdate(ADM_updateComplete *up)
{
    this->_updateCallback=up;
    connect(&manager, SIGNAL(finished(QNetworkReply*)),
            SLOT(downloadFinished(QNetworkReply*)));
}
/**
 */
ADMCheckUpdate::~ADMCheckUpdate()
{
    
}
/**
 */
void ADMCheckUpdate::execute()
{
    // Construct URL
    std::string url=std::string(ADM_UPDATE_SERVER)+std::string("/")+std::string(ADM_UPDATE_TARGET);
    QUrl qurl = QUrl::fromUserInput(QString(url.c_str()));
    QNetworkRequest request(qurl);
    QNetworkReply *reply = manager.get(request);
}
/**
 */
void ADMCheckUpdate::downloadFinished(QNetworkReply *reply)
{
    ADM_info("Download finished\n");
    QString st;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0) 
        st=reply->url().toDisplayString().toUtf8();
#else
        st=reply->url().toString().toUtf8();
#endif
    if(reply->error())
    {
        ADM_warning("Error downloading update %s\n",st.toUtf8().constData());
        ADM_warning("Er=%s\n",reply->errorString().toUtf8().constData());
        return;
    }
    ADM_warning("Success downloading update %s\n",st.toUtf8().constData());
    QByteArray ba=reply->readAll();
    if(!ba.size())
   	return; 
    std::string output=std::string(ba.data());
    output=QString(output.c_str()).simplified().toUtf8().constData();
    printf("wget output is <%s>\n",output.c_str());
    std::vector<std::string>result;
    ADM_splitString(" ",output,result);
    if(result.size())
        ADM_info("API version  = <%s>\n",result [0].c_str());
    if(result.size()!=4)
    {
        ADM_warning("Invalid output\n");
        return;
    }
    if(!result[0].compare(std::string("1")))
    {        
        ADM_info("Version  = <%s>\n",result [1].c_str());
        ADM_info("Release date = <%s>\n",result [2].c_str());
        ADM_info("Download URL = <%s>\n",result [3].c_str());
        int a,b,c,version;
        sscanf(result[1].c_str(),"%d.%d.%d",&a,&b,&c);
        version=(a*10000)+(b*100)+c;
        // Compute current version
        if(version>ADM_CURRENT_VERSION)        
            _updateCallback(version,result[2],result[3]);
        else
            ADM_info("Already up to date (%d/%d)\n",version,ADM_CURRENT_VERSION);
    }

}

/**
 * 
 */
void ADM_checkForUpdate(ADM_updateComplete *up)
{
    
    ADMCheckUpdate *update=new ADMCheckUpdate(up);
    QTimer::singleShot(0, update, SLOT(execute()));
}
//EOF
