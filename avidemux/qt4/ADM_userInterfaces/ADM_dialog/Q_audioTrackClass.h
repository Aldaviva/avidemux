
#include "ADM_inttype.h"
#include "Q_audioTracks.h"

#include "DIA_coreToolkit.h"
#include "ADM_vidMisc.h"
#include "ADM_toolkitQt.h"
#include "ADM_edit.hxx"
#include "DIA_audioTracks.h"
#include "ADM_edAudioTrackFromVideo.h"
#include "ADM_edAudioTrackExternal.h"
#include "ADM_iso639.h"


uint32_t audioEncoderGetNumberOfEncoders(void);
const char  *audioEncoderGetDisplayName(int i);
/**
    \fn audioTrackQt4
*/
class audioTrackQt4: public QObject,public DIA_audioTrackBase
{
            Q_OBJECT
protected:
            audioTrackWindow *window;
            ActiveAudioTracks active;
            int                nbLanguage;
            const ADM_iso639_t *languages;
            void            setupMenu(int dex,int forcedIndex=-1);
            void            enable(int i);
            void            disable(int i);
            void            setLanguageFromPool(int menuIndex, int poolIndex);
            
public:
       
                            audioTrackQt4( PoolOfAudioTracks *pool, ActiveAudioTracks *xactive );
            virtual		~audioTrackQt4();
                          
                       bool  updateActive(void);
            virtual   bool  run(void);
public slots:
                       bool  filtersClicked(bool a);
                       bool  codecConfClicked(bool a);
                       bool  enabledStateChanged(int state);
                       void  inputChanged(int signal);
};