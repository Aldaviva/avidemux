/**
        \file ADM_videoFilterApi.h
        \brief Public functions
*/


/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#ifndef ADM_VIDEO_FILTER_API_H
#define ADM_VIDEO_FILTER_API_H
#include <vector>
#include "ADM_filterCategory.h"
class ADM_coreVideoFilter;
class CONFcouple;

uint8_t     ADM_vf_loadPlugins(const char *path);

uint32_t    ADM_vf_getNbFilters(void);
uint32_t    ADM_vf_getNbFiltersInCategory(VF_CATEGORY cat);

bool        ADM_vf_getFilterInfo(VF_CATEGORY cat,int filter, const char **name,const char **desc, uint32_t *major,uint32_t *minor,uint32_t *patch);
const char *ADM_vf_getDisplayNameFromTag(uint32_t tag);
const char *ADM_vf_getInternalNameFromTag(uint32_t tag);

uint32_t    ADM_vf_getTagFromInternalName(const char *name);
bool        ADM_vf_addFilter(uint32_t tag,CONFcouple *couples);
VF_CATEGORY ADM_vf_getFilterCategoryFromTag(uint32_t tag);

bool        ADM_vf_canBePartialized(uint32_t tag);


#endif //ADM_VIDEO_FILTER_API_H
