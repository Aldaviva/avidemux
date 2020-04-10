#include <math.h>

#include "ADM_toolkitGtk.h"
#include "DIA_coreToolkit.h"
#include "GUI_glade.h"
#include "ADM_vidMisc.h"
#include "avi_vars.h"
#include "ADM_coreUtils.h"

#define FILL_ENTRY(x) gtk_label_set_text((GtkLabel *) glade.getWidget(#x), text);
#define SET_YES(x,y) gtk_label_set_text((GtkLabel *) glade.getWidget(#x), yesno[y])
#define DISABLE_WIDGET(x) gtk_widget_set_sensitive(glade.getWidget(#x), false);

extern const char *getStrFromAudioCodec( uint32_t codec);

void DIA_properties( void )
{
 char text[80];
 uint32_t hh, mm, ss, ms;
 GtkWidget *dialog;
 uint8_t gmc, qpel,vop;
 uint32_t info=0;
 const char *yesno[2]={QT_TR_NOOP("No"),QT_TR_NOOP("Yes")};
 uint32_t war,har;

    if (playing)
        return;

    text[0] = 0;
    if (!avifileinfo)
        return;

        // Fetch info
        //info=video_body->getSpecificMpeg4Info();
        //vop=!!(info & ADM_VOP_ON);
        //qpel=!!(info & ADM_QPEL_ON);
        //gmc=!!(info & ADM_GMC_ON);
    admGlade glade;
    glade.init();
    if (!glade.loadFile("properties.gtkBuilder"))
    {
        GUI_Error_HIG(QT_TR_NOOP("Cannot load dialog"), 
                      QT_TR_NOOP("File \"properties.gtkBuilder\" could not be loaded."));
        return;
    }
    // create top window
    dialog = glade.getWidget("dialogProperties");
    gtk_register_dialog(dialog);
    
        sprintf(text, QT_TR_NOOP("%"PRIu32" x %"PRIu32), avifileinfo->width,avifileinfo->height);
        FILL_ENTRY(label_size);

        sprintf(text, QT_TR_NOOP("%2.3f fps"), (float) avifileinfo->fps1000 / 1000.F);
        FILL_ENTRY(label_fps);

        sprintf(text, "%s", fourCC::tostring(avifileinfo->fcc));
        FILL_ENTRY(label_videofourcc);

        uint64_t duration=video_body->getVideoDuration();
        duration/=1000;
        ms2time((uint32_t)duration,&hh,&mm,&ss,&ms);
        sprintf(text, QT_TR_NOOP("%02d:%02d:%02d.%03d"), hh, mm, ss, ms);
        FILL_ENTRY(label_duration);
        // Fill in vop, gmc & qpel
#if 0
        SET_YES(labelPacked,vop);
        SET_YES(labelGMC,gmc);
        SET_YES(labelQP,qpel);
#endif
        // Aspect ratio
        const char *s;
        war=video_body->getPARWidth();
        har=video_body->getPARHeight();
        getAspectRatioFromAR(war,har, &s);
        sprintf(text, QT_TR_NOOP("%s (%u:%u)"), s,war,har);
        FILL_ENTRY(labelAspectRatio);
        // Now audio
        WAVHeader *wavinfo=NULL;
        ADM_edAudioTrack   *trk=video_body->getDefaultEdAudioTrack();
        if(trk)
            wavinfo=trk->getInfo();

          if(wavinfo)
          {

              switch (wavinfo->channels)
                {
                case 1:
                    sprintf(text,"%s", QT_TR_NOOP("Mono"));
                    break;
                case 2:
                    sprintf(text,"%s", QT_TR_NOOP("Stereo"));
                    break;
                default:
                    sprintf(text, "%d",wavinfo->channels);
                    break;
                }
                FILL_ENTRY(label1_audiomode);

                sprintf(text, QT_TR_NOOP("%"PRIu32" Hz"), wavinfo->frequency);
                FILL_ENTRY(label_fq);
                sprintf(text, QT_TR_NOOP("%"PRIu32" Bps / %"PRIu32" kbps"), wavinfo->byterate,      wavinfo->byterate * 8 / 1000);
                FILL_ENTRY(label_bitrate);
                sprintf(text, "%s", getStrFromAudioCodec(wavinfo->encoding));
                FILL_ENTRY(label1_audiofourcc);
                // Duration in seconds too
                if( wavinfo->byterate>1)
                {
                        uint64_t l=trk->getDurationInUs();//video_body->getDurationInUs();
                        double du;
                        du=l;
                        du/=1000.; // us->ms
                        ms2time((uint32_t)floor(du), &hh, &mm, &ss, &ms);
                        sprintf(text, QT_TR_NOOP("%02d:%02d:%02d.%03d"), hh, mm, ss, ms);
                        FILL_ENTRY(label_audioduration);
                }
        //        SET_YES(labelVbr, currentaudiostream->isVBR());
        } else
          {
                DISABLE_WIDGET(table2);
          }

        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_unregister_dialog(dialog);
        gtk_widget_destroy(dialog);
}
