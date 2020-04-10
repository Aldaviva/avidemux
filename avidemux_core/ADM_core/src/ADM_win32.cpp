#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <string>
#include <winnls.h>

#include "ADM_default.h"
#include "ADM_win32.h"
#include "ADM_misc.h"
#include <algorithm>
void ADM_usleep(unsigned long us)
{
    Sleep(us/1000);
}

uint8_t win32_netInit(void)
{
    WSADATA wsaData;
    int iResult;

    printf("Initializing WinSock\n");
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);

    if (iResult != NO_ERROR)
    {
        printf("Error at WSAStartup()\n");
        return 0;
    }

    printf("WinSock ok\n");
    return 1;
}

#ifndef HAVE_GETTIMEOFDAY
extern "C"
{
void gettimeofday(struct timeval *p, TIMZ *tz);
}
// https://stackoverflow.com/questions/10905892/equivalent-of-gettimeday-for-windows
void gettimeofday(struct timeval * tp, TIMZ * tzp)
{
    // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
    // This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
    // until 00:00:00 January 1, 1970 
    static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    time = ((uint64_t)file_time.dwLowDateTime);
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec = (long)((time - EPOCH) / 10000000L);
    tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
    return ;
}

#endif    // HAVE_GETTIMEOFDAY

int getpriority(int which, int who)
{
    unsigned int priorityClass;

    ADM_assert(which == PRIO_PROCESS);
    ADM_assert(who == 0);

    priorityClass = GetPriorityClass(GetCurrentProcess());

    switch (priorityClass)
    {
        case HIGH_PRIORITY_CLASS:
            return -18;
            break;
        case ABOVE_NORMAL_PRIORITY_CLASS:
            return -10;
            break;
        case NORMAL_PRIORITY_CLASS:
            return 0;
            break;
        case BELOW_NORMAL_PRIORITY_CLASS:
            return 10;
            break;
        case IDLE_PRIORITY_CLASS:
            return 18;
            break;
        default:
            ADM_assert(0);
    }
}

int setpriority(int which, int who, int value)
{
    unsigned int priorityClass;

    ADM_assert(which == PRIO_PROCESS);
    ADM_assert(who == 0);
    ADM_assert(value >= PRIO_MIN && value <= PRIO_MAX);

    if (value >= -20 && value <= -16)
    {
        priorityClass = HIGH_PRIORITY_CLASS;
    }
    else if (value >= -15 && value <= -6)
    {
        priorityClass = ABOVE_NORMAL_PRIORITY_CLASS;
    }
    else if (value >= -5 && value <= 4)
    {
        priorityClass = NORMAL_PRIORITY_CLASS;
    }
    else if (value >= 6 && value <= 15)
    {
        priorityClass = BELOW_NORMAL_PRIORITY_CLASS;
    }
    else if (value >= 16 && value <= 20)
    {
        priorityClass = IDLE_PRIORITY_CLASS;
    }

    if (!SetPriorityClass(GetCurrentProcess(), priorityClass))
    {
        return -1;
    }

    return 0;
}

int shutdown_win32(void)
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    // Get a token for this process.
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        return -1;
    }

    // Get the LUID for the shutdown privilege.
    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

    tkp.PrivilegeCount = 1;  // one privilege to set
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // Get the shutdown privilege for this process.
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

    if (GetLastError() != ERROR_SUCCESS)
    {
        return -1;
    }

    // Shut down the system and force all applications to close.
    //if (!ExitWindowsEx(EWX_POWEROFF | EWX_FORCE, SHTDN_REASON_FLAG_PLANNED))
    //{
        //return -1;
    //}

    return 0;
}

#ifndef PRODUCT_BUSINESS
#define PRODUCT_BUSINESS 0x00000006
#endif

#ifndef PRODUCT_BUSINESS_N
#define PRODUCT_BUSINESS_N 0x00000010
#endif

#ifndef PRODUCT_HOME_BASIC
#define PRODUCT_HOME_BASIC 0x00000002
#endif

#ifndef PRODUCT_HOME_BASIC_N
#define PRODUCT_HOME_BASIC_N 0x00000005
#endif

#ifndef PRODUCT_HOME_PREMIUM
#define PRODUCT_HOME_PREMIUM 0x00000003
#endif

#ifndef PRODUCT_STARTER
#define PRODUCT_STARTER 0x0000000B
#endif

#ifndef PRODUCT_ENTERPRISE
#define PRODUCT_ENTERPRISE 0x00000004
#endif

#ifndef PRODUCT_ULTIMATE
#define PRODUCT_ULTIMATE 0x00000001
#endif

bool getWindowsVersion(char* version)
{
    int index = 0;
    OSVERSIONINFOEX osvi = {};

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if (!GetVersionEx((OSVERSIONINFO*)&osvi))
        return false;

    if (osvi.dwPlatformId != VER_PLATFORM_WIN32_NT)
        return false;
// Windows Vista / Windows 7
    if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion <= 1)
    {
        if (osvi.wProductType == VER_NT_WORKSTATION)
        {
            if (osvi.dwMinorVersion == 1)
                index += sprintf(version + index, "Microsoft Windows 7");
            else
                index += sprintf(version + index, "Microsoft Windows Vista");

            uint32_t productType = 0;

            HMODULE hKernel = GetModuleHandle("KERNEL32.DLL");

            if (hKernel)
            {
                typedef bool (__stdcall *funcGetProductInfo)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t*);
                funcGetProductInfo pGetProductInfo = (funcGetProductInfo)GetProcAddress(hKernel, "GetProductInfo");

                if (pGetProductInfo)
                    pGetProductInfo(6, 0, 0, 0, &productType);

                switch (productType)
                {
                case PRODUCT_STARTER:
                {
                    index += sprintf(version + index, " Starter");
                    break;
                }
                case PRODUCT_HOME_BASIC_N:
                {
                    index += sprintf(version + index, " Home Basic N");
                    break;
                }
                case PRODUCT_HOME_BASIC:
                {
                    index += sprintf(version + index, " Home Basic");
                    break;
                }
                case PRODUCT_HOME_PREMIUM:
                {
                    index += sprintf(version + index, " Home Premium");
                    break;
                }
                case PRODUCT_BUSINESS_N:
                {
                    index += sprintf(version + index, " Business N");
                    break;
                }
                case PRODUCT_BUSINESS:
                {
                    index += sprintf(version + index, " Business");
                    break;
                }
                case PRODUCT_ENTERPRISE:
                {
                    index += sprintf(version + index, " Enterprise");
                    break;
                }
                case PRODUCT_ULTIMATE:
                {
                    index += sprintf(version + index, " Ultimate");
                    break;
                }
                default:
                    break;
                }
            }
        }
        else if (osvi.wProductType == VER_NT_SERVER)
        {
            if (osvi.dwMinorVersion == 1)
                index += sprintf(version + index, "Microsoft Windows Server 2008 R2");
            else
                index += sprintf(version + index, "Microsoft Windows Server 2008");

            if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
                index += sprintf(version + index, " Datacenter Edition");
            else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                index += sprintf(version + index, " Enterprise Edition");
            else if (osvi.wSuiteMask == VER_SUITE_BLADE)
                index += sprintf(version + index, " Web Edition");
            else
                index += sprintf(version + index, " Standard Edition");
        }
    }
// Windows Server 2003
    else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2)
    {
        index += sprintf(version + index, "Microsoft Windows Server 2003");

        if (GetSystemMetrics(SM_SERVERR2))
            index += sprintf(version + index, " R2");

        if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
            index += sprintf(version + index, " Datacenter Edition");
        else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
            index += sprintf(version + index, " Enterprise Edition");
        else if (osvi.wSuiteMask == VER_SUITE_BLADE)
            index += sprintf(version + index, " Web Edition");
        else
            index += sprintf(version + index, " Standard Edition");
    }
// Windows XP
    else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
    {
        index += sprintf(version + index, "Microsoft Windows XP");

        if (GetSystemMetrics(SM_MEDIACENTER))
            index += sprintf(version + index, " Media Center Edition");
        else if (GetSystemMetrics(SM_STARTER))
            index += sprintf(version + index, " Starter Edition");
        else if (GetSystemMetrics(SM_TABLETPC))
            index += sprintf(version + index, " Tablet PC Edition");
        else if (osvi.wSuiteMask & VER_SUITE_PERSONAL)
            index += sprintf(version + index, " Home Edition");
        else
            index += sprintf(version + index, " Professional");
    }
// Windows 2000
    else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
    {
        index += sprintf(version + index, "Microsoft Windows 2000");

        if (osvi.wProductType == VER_NT_WORKSTATION)
        {
            index += sprintf(version + index, " Professional");
        }
        else if (osvi.wProductType == VER_NT_SERVER)
        {
            if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
                index += sprintf(version + index, " Datacenter Server");
            else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                index += sprintf(version + index, " Advanced Server");
            else
                index += sprintf(version + index, " Server");
        }
    }
// Windows NT 4
    else if (osvi.dwMajorVersion == 4)
    {
        index += sprintf(version + index, "Microsoft Windows NT 4");

        if (osvi.wProductType == VER_NT_WORKSTATION)
        {
            index += sprintf(version + index, " Workstation");
        }
        else if (osvi.wProductType == VER_NT_SERVER)
        {
            if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                index += sprintf(version + index, " Server, Enterprise Edition");
            else
                index += sprintf(version + index, " Server");
        }
    }
    else
    {
        index += sprintf(version + index, "Microsoft Windows");
    }

// Service pack and full version info
    if (strlen(osvi.szCSDVersion) > 0)
    {
        index += sprintf(version + index, " %s", osvi.szCSDVersion);
    }

    index += sprintf(version + index, " (%d.%d.%d", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber & 0xFFFF);

// 64-bit Windows
#ifdef _WIN64
    index += sprintf(version + index, "; 64-bit");
#else
    bool isWow64 = false;
    HMODULE hKernel = GetModuleHandle("kernel32.dll");

    if (hKernel)
    {
        typedef bool (__stdcall *funcIsWow64Process)(void*, bool*);

        funcIsWow64Process pIsWow64Process = (funcIsWow64Process)GetProcAddress(hKernel, "IsWow64Process");

        if (pIsWow64Process)
        {
            pIsWow64Process(GetCurrentProcess(), &isWow64);
        }
    }

    if (isWow64)
        index += sprintf(version + index, "; 64-bit");
    else
        index += sprintf(version + index, "; 32-bit");
#endif

    index += sprintf(version + index, ")");

    return true;
}


// Convert string from ANSI code page to wide char
int ansiStringToWideChar(const char *ansiString, int ansiStringLength, wchar_t *wideCharString)
{
    int wideCharStringLen = MultiByteToWideChar(CP_ACP, 0, ansiString, ansiStringLength, NULL, 0);

    if (wideCharString)
        MultiByteToWideChar(CP_ACP, 0, ansiString, ansiStringLength, wideCharString, wideCharStringLen);

    return wideCharStringLen;
}

// Convert UTF-8 string to wide char
int utf8StringToWideChar(const char *utf8String, int utf8StringLength, wchar_t *wideCharString)
{
    int wideCharStringLength = MultiByteToWideChar(CP_UTF8, 0, utf8String, utf8StringLength, NULL, 0);

    if (wideCharString)
        MultiByteToWideChar(CP_UTF8, 0, utf8String, utf8StringLength, wideCharString, wideCharStringLength);

    return wideCharStringLength;
}
// Convert string from Wide Char to ANSI code page
int wideCharStringToAnsi(const wchar_t *wideCharString, int wideCharStringLength, char *ansiString, const char *filler)
{
    int flags = WC_COMPOSITECHECK;

    if (filler)
        flags |= WC_NO_BEST_FIT_CHARS | WC_DEFAULTCHAR;

    int ansiStringLen = WideCharToMultiByte(CP_ACP, flags, wideCharString, wideCharStringLength, NULL, 0, filler, NULL);

    if (ansiString)
        WideCharToMultiByte(CP_ACP, flags, wideCharString, wideCharStringLength, ansiString, ansiStringLen, filler, NULL);

    return ansiStringLen;
}

static int utf8_to_wc(const char *in, wchar_t **out)
{
      int dirLength= (int)utf8StringToWideChar(in, strlen(in), NULL) + 1;
      wchar_t *wcDirectory=new wchar_t[dirLength];
      memset(wcDirectory,0,dirLength*sizeof(wchar_t));
      utf8StringToWideChar(in, (int)strlen(in),wcDirectory);
      *out=wcDirectory;
      return dirLength;
}
static int convert_to_short_dir(wchar_t *in, wchar_t **out,int fileLength)
{
         int shortDirLength = GetShortPathNameW(in, NULL, 0);
        int reserved=shortDirLength + fileLength;
    wchar_t *wcShortDir = new wchar_t[reserved];
    memset(wcShortDir, 0, reserved * sizeof(wchar_t));
    GetShortPathNameW(in, wcShortDir, shortDirLength);
        *out=wcShortDir;
        return shortDirLength;
}

/**
 * \fn utf8StringToAnsi
 * \brief Straight from avidemux 2.5
 */
std::string utf8StringToAnsi(const char *utf8String)
{

  std::string original=std::string(utf8String);
  std::string dupe;

   dupe.reserve(original.length());
   std::string::size_type xlastPos = 0;
   std::string::size_type findPos;
	 std::string to=std::string("\\\\");
   while( std::string::npos != ( findPos = original.find( "/", xlastPos )))
   {
        dupe.append( original, xlastPos, findPos - xlastPos );
        dupe += to;
        xlastPos = findPos + 1;
   }
   dupe += original.substr( xlastPos );
   std::string fileName=ADM_getFileName(dupe);
   std::string pathName=ADM_extractPath(dupe);



    int filenameLength = (int)fileName.size();
    int directoryLength = (int)pathName.size();

    printf("Path=%s\n",pathName.c_str());
    printf("Name=%s\n",fileName.c_str());

    // Convert directory to wide char
    int wcDirLength = utf8StringToWideChar(pathName.c_str(), directoryLength, NULL) + 1;
    int wcFileLength = utf8StringToWideChar(fileName.c_str(), filenameLength, NULL) + 1;
    wchar_t *wcDirectory = new wchar_t[wcDirLength];

    memset(wcDirectory, 0, wcDirLength * sizeof(wchar_t));
    utf8StringToWideChar(pathName.c_str(), directoryLength, wcDirectory);

    // Get short directory
    int shortDirLength = GetShortPathNameW(wcDirectory, NULL, 0);
    wchar_t *wcShortDir = new wchar_t[shortDirLength + wcFileLength];

    memset(wcShortDir, 0, (shortDirLength + wcFileLength) * sizeof(wchar_t));
    GetShortPathNameW(wcDirectory, wcShortDir, shortDirLength);
    delete [] wcDirectory;

    // Append filename to directory
    utf8StringToWideChar(fileName.c_str(), filenameLength, wcShortDir + (shortDirLength - 1));

    // Convert path to ANSI
    int dirtyAnsiPathLength = wideCharStringToAnsi(wcShortDir, -1, NULL, "?");
    char *dirtyAnsiPath = new char[dirtyAnsiPathLength];

    wideCharStringToAnsi(wcShortDir, -1, dirtyAnsiPath, "?");

    // Clean converted path
    std::string cleanPath = std::string(dirtyAnsiPath);
        printf("Dirty path=%s\n",dirtyAnsiPath);
    std::string::iterator lastPos = std::remove(cleanPath.begin(), cleanPath.end(), '?');

    cleanPath.erase(lastPos, cleanPath.end());
    delete [] dirtyAnsiPath;

    delete [] wcShortDir;
        printf("clean path=%s\n",cleanPath.c_str());

        return cleanPath;
}

// Convert Wide Char string to UTF-8
int wideCharStringToUtf8(const wchar_t *wideCharString, int wideCharStringLength, char *utf8String)
{
    int utf8StringLen = WideCharToMultiByte(CP_UTF8, 0, wideCharString, wideCharStringLength, NULL, 0, NULL, NULL);

    if (utf8String)
        WideCharToMultiByte(CP_UTF8, 0, wideCharString, wideCharStringLength, utf8String, utf8StringLen, NULL, NULL);

    return utf8StringLen;
}

// Convert string from ANSI code page to UTF-8
int ansiStringToUtf8(const char *ansiString, int ansiStringLength, char *utf8String)
{
    int wideCharStringLen = ansiStringToWideChar(ansiString, ansiStringLength, NULL);
    wchar_t *wideCharString = new wchar_t[wideCharStringLen];

    ansiStringToWideChar(ansiString, ansiStringLength, wideCharString);

    int multiByteStringLen = wideCharStringToUtf8(wideCharString, wideCharStringLen, NULL);

    if (utf8String)
        wideCharStringToUtf8(wideCharString, wideCharStringLen, utf8String);

    delete [] wideCharString;

    return multiByteStringLen;
}

void getUtf8CommandLine(int *argc, char **argv[])
{
    wchar_t **wargv = CommandLineToArgvW(GetCommandLineW(), argc);

    if (wargv)
    {
        *argv = new char *[*argc];

        for (int i = 0; i < *argc; i++)
        {
            int utf8Length = wideCharStringToUtf8(wargv[i], -1, NULL);
            char *utf8 = new char[utf8Length];

            wideCharStringToUtf8(wargv[i], -1, utf8);
            (*argv)[i] = utf8;
        }

        LocalFree(wargv);
    }
    else
    {
        *argc = 0;
        *argv = NULL;
    }
}

void freeUtf8CommandLine(int argc, char *argv[])
{
    for (int i = 0; i < argc; i++)
    {
        delete [] argv[i];
    }

    delete [] argv;
}

#endif    // _WIN32
