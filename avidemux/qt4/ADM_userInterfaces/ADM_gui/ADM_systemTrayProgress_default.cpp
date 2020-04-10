#pragma once

#include "ADM_cpp.h"
#include "ADM_default.h"
#include "ADM_systemTrayProgress.h"
/**
 */
class defaultTaskBarProgress : public admUITaskBarProgress
{
    virtual bool enable() {return true;}
    virtual bool disable() {return true;}
    virtual bool setProgress(int percent) {return true;} 
    virtual bool setParent(void *qwin) {return true;};
};

/**
 */
admUITaskBarProgress *createADMTaskBarProgress()
{
    return new defaultTaskBarProgress;
}
// EOF