/***************************************************************************
   \file ADM_pyAvidemux.cpp
    \brief binding between tinyPy and avidemux
    \author mean/gruntster 2011/2012
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef _MSC_VER
#include <libgen.h>
#endif
#include "ADM_assert.h"
#include "ADM_pyAvidemux.h"
#include "PythonEngine.h"
#include "PythonScriptWriter.h"

#include "TinyParams.h"
#include "DIA_factory.h"
#include "ADM_scriptDF.h"
#include "ADM_scriptDFInteger.h"
#include "ADM_scriptDFTimeStamp.h"
#include "ADM_scriptDFMenu.h"
#include "ADM_scriptDFToggle.h"
#include "ADM_scriptDialogFactory.h"

#define ADM_PYID_USERDATA -1
#define ADM_PYID_AVIDEMUX 100
#define ADM_PYID_EDITOR   101
#define ADM_PYID_GUI      102
#define ADM_PYID_OS       103
#define ADM_PYID_DIALOGF     200
#define ADM_PYID_DF_TOGGLE   201
#define ADM_PYID_DF_INTEGER  202
#define ADM_PYID_DF_MENU     203

extern void re_init(TP);
/**
 * \fn pyRaise
 * @param vm
 * @param exception
 */
void pyRaise(tp_vm *vm,const char *exception)
{
        PythonEngine *engine = (PythonEngine*)tp_get(vm, vm->builtins, tp_string("userdata")).data.val;
        engine->raise(exception);
        
}
/**
 * \fn pyPrintf
 */
void pyPrintf(tp_vm *vm, const char *fmt, ...)
{
	PythonEngine *engine = (PythonEngine*)tp_get(vm, vm->builtins, tp_string("userdata")).data.val;
	static char print_buffer[1024];

	va_list list;
	va_start(list, fmt);
	vsnprintf(print_buffer, 1023, fmt, list);
	va_end(list);
	print_buffer[1023] = 0; // ensure the string is terminated

	engine->callEventHandlers(IScriptEngine::Information, NULL, -1, print_buffer);
}

#include "adm_gen.cpp"
#include "editor_gen.cpp"
#include "GUI_gen.cpp"
#include "pyDFInteger_gen.cpp"
#include "pyDFMenu_gen.cpp"
#include "pyDFToggle_gen.cpp"
#include "pyDFTimeStamp_gen.cpp"
#include "pyDialogFactory_gen.cpp"
#include "pyHelpers_gen.cpp"
#include "tinypy/init_math.cpp"

extern void tp_hook_set_syslib(const char *sysLib);

using namespace std;

extern "C"
{
#ifdef _MSC_VER
__declspec(dllexport)
#endif
	IScriptEngine* createEngine()
	{
		return new PythonEngine();
	}
}
void PythonEngine::raise(const char *exception)
{
    GUI_Error_HIG("TinyPy:Exception","%s",exception);
}
PythonEngine::~PythonEngine()
{
	this->callEventHandlers(IScriptEngine::Information, NULL, -1, "Closing Python");
	tp_deinit(_vm);
}

string PythonEngine::name()
{
    return "Tinypy";
}

void PythonEngine::initialise(IEditor *editor)
{
	ADM_assert(editor);
	_editor = editor;

	string sysLib = string(ADM_getAutoDir()) + string("/lib");

    tp_hook_set_syslib(sysLib.c_str());
	_vm = tp_init(0, NULL);
	ADM_assert(_vm);
	math_init(_vm);

	this->registerFunctions();
	this->callEventHandlers(IScriptEngine::Information, NULL, -1, "Python initialised");
}

void PythonEngine::registerFunctions()
{
	tp_obj userdata = tp_data(_vm, ADM_PYID_USERDATA, this);
	tp_set(_vm, _vm->builtins, tp_string("userdata"), userdata);

	pyFunc addonFunctions[] = {{"help", PythonEngine::dumpBuiltin},
		{"get_folder_content", PythonEngine::getFolderContent},
		{"get_file_size", PythonEngine::getFileSize},
		{"basename", PythonEngine::basename},
                {"dirname", PythonEngine::dirname},
		{NULL, NULL}};
        
        pyFunc osStaticClassFunctions[] = {{"environ", PythonEngine::pyenviron},
		{NULL, NULL}};
        
        
        re_init(_vm);
	this->registerFunction("addons", addonFunctions);
        
	this->registerClass("Avidemux", initClasspyAdm, "avidemux class");
	this->registerClass("Editor", initClasspyEditor, "add, remove videos");
	this->registerClass("Gui", initClasspyGui, "widget, alert boxes,..");
	this->registerClass("DFToggle", initClasspyDFToggle, "UI element : toggle");
	this->registerClass("DFInteger", initClasspyDFInteger, "UI element : integer");
	this->registerClass("DFMenu", initClasspyDFMenu, "UI element : drop down menu");
	this->registerClass("DFTimeStamp", initClasspyDFTimeStamp, "UI element : timestamp");
	this->registerClass("DialogFactory", initClasspyDialogFactory, "UI manager, handle all UI elements");
	this->registerFunction("test", pyHelpers_functions);
        this->registerStaticClass("os",osStaticClassFunctions,"Access to operating system");
}

void PythonEngine::registerClass(const char *className, pyRegisterClass classPy, const char *desc)
{
	this->callEventHandlers(IScriptEngine::Information, NULL, -1,
		(string("Registering class: ") + string(className)).c_str());

	pyClassDescriptor classDesc;
	classDesc.className = string(className);
	classDesc.desc = string(desc);
	_pyClasses.push_back(classDesc);

	tp_set(_vm, _vm->builtins, tp_string(className), classPy(_vm));
}
/**
 * 
 * @param vm
 * @return 
 */
static tp_obj myCtorpyStatic(tp_vm *vm)
{
  return tp_None;
}
/**
 * \fn registerStaticClass
 */
void PythonEngine::registerStaticClass(const char *thisClass,pyFunc *funcs,const char *desc)
{
	this->callEventHandlers(IScriptEngine::Information, NULL, -1,
		(string("Registering static class ") + string(thisClass)).c_str());

        tp_obj classObj =  tp_dict(_vm);
        
        pyClassDescriptor classDesc;
	classDesc.className = string(thisClass);
	classDesc.desc = string(desc);
	_pyClasses.push_back(classDesc);
        
	while (funcs->funcName)
	{
		this->callEventHandlers(IScriptEngine::Information, NULL, -1,
			(string("\tRegistering: ") + string(funcs->funcName)).c_str());

		tp_set(_vm, classObj, tp_string(funcs->funcName), tp_fnc(_vm, funcs->funcCall));

		funcs++;
	}
	tp_set(_vm, _vm->modules, tp_string(thisClass), classObj);

}
void PythonEngine::registerFunction(const char *group, pyFunc *funcs)
{
	this->callEventHandlers(IScriptEngine::Information, NULL, -1,
		(string("Registering group ") + string(group)).c_str());

	while (funcs->funcName)
	{
		this->callEventHandlers(IScriptEngine::Information, NULL, -1,
			(string("\tRegistering: ") + string(funcs->funcName)).c_str());

		tp_set(_vm, _vm->builtins, tp_string(funcs->funcName), tp_fnc(_vm, funcs->funcCall));

		funcs++;
	}
}

void PythonEngine::registerEventHandler(eventHandlerFunc *func)
{
	_eventHandlerSet.insert(func);
}

void PythonEngine::unregisterEventHandler(eventHandlerFunc *func)
{
	_eventHandlerSet.erase(func);
}

void PythonEngine::callEventHandlers(EventType eventType, const char *fileName, int lineNo, const char *message)
{
	EngineEvent event = { this, eventType, fileName, lineNo, message };
	set<eventHandlerFunc*>::iterator it;

	for (it = _eventHandlerSet.begin(); it != _eventHandlerSet.end(); ++it)
	{
		(*it)(&event);
	}
}

bool PythonEngine::runScript(string script, RunMode mode)
{
    if (setjmp(_vm->nextexpr))
    {
        return false;
    }
    else
    {
		tp_obj c = tp_eval(_vm, script.c_str(), _vm->builtins);

        return true;
    }
}

bool PythonEngine::runScriptFile(string name, RunMode mode)
{
	if (setjmp(_vm->nextexpr))
	{
		return false;
	}
	else
	{
		this->callEventHandlers(IScriptEngine::Information, NULL, -1,
			(string("Executing ") + string(name) + string("...")).c_str());

		tp_import(_vm, name.c_str(), "avidemux6", NULL, 0);

		this->callEventHandlers(IScriptEngine::Information, NULL, -1, "Done");

		return true;
	}
}

IEditor* PythonEngine::editor()
{
	return _editor;
}

tp_obj PythonEngine::getFileSize(tp_vm *tp)
{
	TinyParams pm(tp);
	const char *file = pm.asString();

	uint32_t size = ADM_fileSize(file);
	tp_obj v = tp_number(size);

	return v;
}

/**
    \fn getFolderContent
    \brief get_folder_content(root, ext) : Return a list of files in  root with extention ext
*/
tp_obj PythonEngine::getFolderContent(tp_vm *tp)
{
	TinyParams pm(tp);
	const char *root = pm.asString();
	const char *ext = pm.asString();

	ADM_info("Scanning %s for file with ext : %s\n", root, ext);

	uint32_t nb;
#define MAX_ELEM 200
	char *items[MAX_ELEM];

	if (!buildDirectoryContent(&nb, root, items, MAX_ELEM, ext))
	{
		ADM_warning("Cannot get content\n");
		return tp_None;
	}

	// create a list
	tp_obj list = tp_list(tp);

	if (!nb)
	{
		ADM_warning("Folder empty\n");
		return tp_None;
	}

	// add items to the list
	for (int i = 0; i < nb; i++)
	{
		char *tem = items[i];
		_tp_list_append(tp, list.list.val, tp_string_copy(tp, tem, strlen(tem)));
	}

	// free the list
	clearDirectoryContent(nb, items);

	return list;
}
/**
    \fn basename
    \brief similar to python os.basename()
*/
tp_obj PythonEngine::basename(tp_vm *tp)
{
	TinyParams pm(tp);
        const char *path = pm.asString();
	char *copy=strdup(path);
#ifdef _MSC_VER
        char fname[2048]={0};
        char fext[50]={0};
        char b[2048]={0};
        _splitpath(copy,NULL,NULL,fname,fext);
        strcpy(b,fname);
        strcat(b,fext);
#else
        char *b=::basename(copy);
#endif
        tp_obj r;
        if(!b)
        {
            r=tp_None;
        }else
            r=tp_string_copy(tp,b,strlen(b));
        free(copy);
        return r;
}
/**
    \fn basename
    \brief similar to python os.basename()
*/
tp_obj PythonEngine::dirname(tp_vm *tp)
{
	TinyParams pm(tp);
        const char *path = pm.asString();
        char *copy=strdup(path);
#ifdef _MSC_VER
        char dir[2048]={0};
        char drive[50]={0};
        char b[2048]={0};
        _splitpath(copy,drive,dir,NULL,NULL);
        strcpy(b,drive);
        strcat(b,dir);
#else
        char *b=::dirname(copy);
#endif
        tp_obj r;
        if(!b)
        {
            r=tp_None;
        }else
            r=tp_string_copy(tp,b,strlen(b));
        free(copy);
        return r;
}

/**
    \fn dumpBuiltin
*/
tp_obj PythonEngine::dumpBuiltin(tp_vm *tp)
{
	PythonEngine *engine = (PythonEngine*)tp_get(tp, tp->builtins, tp_string("userdata")).data.val;
	int n = engine->_pyClasses.size();

	pyPrintf(tp, "You can get more help using CLASSNAME.help()");

	for (int i = 0; i < n; i++)
	{
		pyPrintf(tp, "%s \t%s\n", engine->_pyClasses[i].className.c_str(), engine->_pyClasses[i].desc.c_str());
	}

	return tp_None;
}

IScriptEngine::Capabilities PythonEngine::capabilities()
{
	return IScriptEngine::None;
}

IScriptWriter* PythonEngine::createScriptWriter()
{
    return new PythonScriptWriter();
}

void PythonEngine::openDebuggerShell() {}

string PythonEngine::defaultFileExtension()
{
	return string("py");
}

int PythonEngine::maturityRanking()
{
    return 1;
}

string PythonEngine::referenceUrl()
{
    return "";
}
/**
 * \fn environ
 * \brief os.environ
 * @param tp
 * @return 
 */
tp_obj PythonEngine::pyenviron(tp_vm *tp)
{
	TinyParams pm(tp);
	const char *file = pm.asString();
        const char *defaultRet="";
        char       *output=NULL;        
        PythonEngine *engine = (PythonEngine*)tp_get(tp, tp->builtins, tp_string("userdata")).data.val;
        
        
        output=pyGetEnv(engine->editor(),file);
        if(output) defaultRet=output;
	tp_obj v =  tp_string_copy(tp,defaultRet,strlen(defaultRet));
	return v;
}
