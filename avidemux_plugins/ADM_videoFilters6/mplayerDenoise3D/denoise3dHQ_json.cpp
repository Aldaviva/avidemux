// automatically generated by admSerialization.py, do not edit!
#include "ADM_default.h"
#include "ADM_paramList.h"
#include "ADM_coreJson.h"
#include "denoise3dHQ.h"
bool  denoise3dhq_jserialize(const char *file, const denoise3dhq *key){
admJson json;
json.addUint32("mode",key->mode);
json.addFloat("luma_spatial",key->luma_spatial);
json.addFloat("chroma_spatial",key->chroma_spatial);
json.addFloat("luma_temporal",key->luma_temporal);
json.addFloat("chroma_temporal",key->chroma_temporal);
return json.dumpToFile(file);
};
bool  denoise3dhq_jdeserialize(const char *file, const ADM_paramList *tmpl,denoise3dhq *key){
admJsonToCouple json;
CONFcouple *c=json.readFromFile(file);
if(!c) {ADM_error("Cannot read json file");return false;}
bool r= ADM_paramLoadPartial(c,tmpl,key);
delete c;
return r;
};