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
// 将char*转化为WCHAR*
static WCHAR* C2WC(const char* str)
{
	WCHAR* wstr = new WCHAR[2 * strlen(str) + 2];
	if (wstr == NULL) return NULL;
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str, -1, wstr, 2 * (int)strlen(str) + 2);
	return wstr;
}

//////////////////////////////////////////////////////////////////////////
// 将WCHAR*转化为char*
static char* WC2C(const WCHAR* wstr)
{
	char* str = new char[MAX_PATH];
	if (wstr == NULL) return NULL;
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, wstr, -1, str, sizeof(char)*MAX_PATH, NULL, NULL);
	return str;
}

static LPWSTR A2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars, UINT acp)
{
	int ret;
	if (lpw == NULL || lpa == NULL) return NULL;

	lpw[0] = '\0';
	ret = MultiByteToWideChar(acp, 0, lpa, -1, lpw, nChars);
	if (ret == 0) {
		return NULL;
	}        
	return lpw;
}

static LPSTR W2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars, UINT acp)
{
	int ret;
	if (lpa == NULL || lpw == NULL) return NULL;

	lpa[0] = '\0';
	ret = WideCharToMultiByte(acp, 0, lpw, -1, lpa, nChars, NULL, NULL);
	if (ret == 0) {
		return NULL;
	}
	return lpa;
}

#define WIN32_USE_CHAR_CONVERSION int _convert = 0; LPCWSTR _lpw = NULL; LPCSTR _lpa = NULL
#define S_ISDIR(_m) (((_m)&_S_IFMT) == _S_IFDIR) 
#define S_ISREG(_m) (((_m)&_S_IFMT) == _S_IFREG) 

// ANSI conversion to WCHAR for UTF8
#define WIN32_A2W_U(lpa) (\
	((_lpa = lpa) == NULL) ? NULL : (\
	_convert = (int)(strlen(_lpa)+1),\
	(INT_MAX/2<_convert)? NULL :  \
	A2WHelper((LPWSTR) alloca(_convert*sizeof(WCHAR)), _lpa, _convert, CP_UTF8)))

// UTF8 conversion to ansi for UTF8
#define WIN32_W2A_U(lpw) (\
	((_lpw = lpw) == NULL) ? NULL : (\
	(_convert = (lstrlenW(_lpw)+2), \
	(_convert>INT_MAX/2) ? NULL : \
	W2AHelper((LPSTR) alloca(_convert*sizeof(WCHAR)), _lpw, _convert*sizeof(WCHAR), CP_UTF8))))

// UTF8 conversion to ansi for ANSI
#define WIN32_W2A_A(lpw) (\
	((_lpw = lpw) == NULL) ? NULL : (\
	(_convert = (lstrlenW(_lpw)+2), \
	(_convert>INT_MAX/2) ? NULL : \
	W2AHelper((LPSTR) alloca(_convert*sizeof(WCHAR)), _lpw, _convert*sizeof(WCHAR), CP_ACP))))
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
	static int ListDir(const char * path, __int64 & nTotalSize, std::vector<_FileInfo>& vecEntries);
	// constructors and destructor
	UdtFile();
	~UdtFile();
};

#endif	// __UDTFILE_H__