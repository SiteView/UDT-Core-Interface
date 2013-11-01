#ifndef __UDTFILE_H__
#define __UDTFILE_H__

#ifndef WIN32
#include <sys/time.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <dirent.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#include <direct.h>
#include <io.h>
#endif

#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <vector>


#ifdef WIN32
//////////////////////////////////////////////////////////////////////////
// ASCI char* 转化为 WCHAR*
static WCHAR* C2WC(const char* str)
{
	WCHAR* wstr = new WCHAR[2 * strlen(str) + 2];
	if (wstr == NULL) return NULL;
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str, -1, wstr, 2 * (int)strlen(str) + 2);
	return wstr;
}

//////////////////////////////////////////////////////////////////////////
// UTF-8 char* 转化为 WCHAR*
static WCHAR* CU2WC(const char* str)
{
	WCHAR* wstr = new WCHAR[2 * strlen(str) + 2];
	if (wstr == NULL) return NULL;
	MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, 2 * (int)strlen(str) + 2);
	return wstr;
}

//////////////////////////////////////////////////////////////////////////
// WCHAR*转化为char*
static char* WC2C(const WCHAR* wstr)
{
	char* str = new char[MAX_PATH];
	if (wstr == NULL) return NULL;
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, wstr, -1, str, sizeof(char)*MAX_PATH, NULL, NULL);
	return str;
}

//////////////////////////////////////////////////////////////////////////
// WCHAR*转化为char* for utf8
static char* WC2CU(const WCHAR* wstr)
{
	char* str = new char[MAX_PATH];
	if (wstr == NULL) return NULL;
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, sizeof(char*)*MAX_PATH, NULL, NULL);
	return str;
}

#define S_ISDIR(_m) (((_m)&_S_IFMT) == _S_IFDIR) 
#define S_ISREG(_m) (((_m)&_S_IFMT) == _S_IFREG) 
#define WIN32_C2WC(lpa) (C2WC(lpa))
#define WIN32_CU2WC(lpa) (CU2WC(lpa))
#define WIN32_WC2C(lpw) (WC2C(lpw))
#define WIN32_WC2CU(lpw) (WC2CU(lpw))
#endif

struct _FileInfo
{
	__int64 nFileSize;
	std::string asciPath;
	std::string utf8Path;
};

class UdtFile
{
public:
	// class methods
	static int GetInfo(const char * path, _FileInfo & info);
	static int CreateDir(const char * path);
	static int GetWorkingDir(char * path);
	static int ListDir(const char * path, __int64 & nTotalSize, std::vector<_FileInfo>& vecEntries, std::vector<std::string>& vecDirs);
	// constructors and destructor
	UdtFile();
	~UdtFile();
};

#endif	// __UDTFILE_H__