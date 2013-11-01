#include "UdtFile.h"

using namespace std;

UdtFile::UdtFile()
{
}


UdtFile::~UdtFile()
{
}

#ifdef WIN32
int UdtFile::ListDir(const char * path, __int64 & nTotalSize, std::vector<_FileInfo>& vecEntries, std::vector<std::string>& vecDirs)
{
	// check the arguments
	if (path == NULL || path[0] == '\0')
		return 1;

	string szPath = path;
	if (szPath.compare(szPath.size()-1, 1, "\\") == 0 || szPath.compare(szPath.size()-1, 1, "/") == 0)
		szPath += "*";
	else
		szPath += "\\*";

	// list the entries
	WIN32_FIND_DATAW find_data;
	//HANDLE find_handle = FindFirstFileW(WIN32_A2W_U(szPath.c_str()), &find_data);
	HANDLE find_handle = FindFirstFileW(WIN32_C2WC(szPath.c_str()), &find_data);
	if (find_handle == INVALID_HANDLE_VALUE)
	{
		struct _stat64 stat_buffer;
		char strAPath[MAX_PATH];
		char strUPath[MAX_PATH];
		sprintf_s(strAPath, "%s", path);
		sprintf_s(strUPath, "%s", WIN32_WC2C(WIN32_C2WC(path)));
		int nRet = _wstat64(WIN32_C2WC(path), &stat_buffer);
		if (nRet == 0)
		{
			if (S_ISDIR(stat_buffer.st_mode))
			{
				vecDirs.push_back(strAPath);
				ListDir(strAPath, nTotalSize, vecEntries, vecDirs);
			}
			else
			{
				_FileInfo info;
				info.asciPath = strAPath;
				info.utf8Path = strUPath;
				info.nFileSize = stat_buffer.st_size;
				nTotalSize += info.nFileSize;
				vecEntries.push_back(info);
			}
		}
		return 1;
	}

	vecDirs.push_back(path);

	do 
	{
		if (WIN32_WC2C(find_data.cFileName) != NULL)
		{
			if (strcmp(WIN32_WC2C(find_data.cFileName), ".") == 0 || strcmp(WIN32_WC2C(find_data.cFileName), "..") == 0)
				continue;

			struct _stat64 stat_buffer;
			char strAPath[MAX_PATH];
			char strUPath[MAX_PATH];
			sprintf_s(strAPath, "%s\\%s", path, WIN32_WC2C(find_data.cFileName));
			sprintf_s(strUPath, "%s\\%s", path, WIN32_WC2CU(find_data.cFileName));
			int nRet = _wstat64(WIN32_C2WC(strUPath), &stat_buffer);
			if (nRet == 0)
			{
				if (S_ISDIR(stat_buffer.st_mode))
				{
					ListDir(strAPath, nTotalSize, vecEntries, vecDirs);
				}
				else
				{
					_FileInfo info;
					info.asciPath = strAPath;
					info.utf8Path = strUPath;
					info.nFileSize = stat_buffer.st_size;
					nTotalSize += info.nFileSize;
					vecEntries.push_back(info);
				}
			}
		}
	} while (FindNextFileW(find_handle, &find_data));
	DWORD last_error = GetLastError();
	FindClose(find_handle);
	if (last_error != ERROR_NO_MORE_FILES)
		return 1;

	return 0;
}

#else
int UdtFile::ListDir(const char * path, __int64 & nTotalSize, std::vector<_FileInfo>& vecEntries, std::vector<std::string>& vecDirs)
{
	// check the arguments
	if (path == NULL) return 1;
	// list the entries
	DIR * directory = opendir(path);
	if (directory == NULL)
	{
		int nRet = 0;
		struct _stat64 stat_buffer;
		nRet = _stat64(path, stat_buffer);
		if (nRet == 0)
		{
			_FileInfo info;
			info.asciPath = strAPath;
			info.utf8Path = strUPath;
			info.nFileSize = stat_buffer.st_size;
			nTotalSize += info.nFileSize;
			vecEntries.push_back(info);
		}
		return 1;
	}
	for (;;)
	{
		struct dirent* entry_pointer = NULL;
#if defined(NPT_CONFIG_HAVE_READDIR_R)
		struct dirent entry;
		int result = readdir_r(directory, &entry, &entry_pointer);
		if (result != 0 || entry_pointer == NULL) break;
#else
		entry_pointer = readdir(directory);
		if (entry_pointer == NULL) break;
#endif
		// ignore odd names
		if (entry_pointer->d_name[0] == '\0') continue;

		// ignore . and ..
		if (entry_pointer->d_name[0] == '.' && 
			entry_pointer->d_name[1] == '\0') {
				continue;
		}
		if (entry_pointer->d_name[0] == '.' && 
			entry_pointer->d_name[1] == '.' &&
			entry_pointer->d_name[2] == '\0') {
				continue;
		}        

		// continue if we still have some items to skip
		//if (start > 0) {
		//	--start;
		//	continue;
		//}

		char strAPath[MAX_PATH];
		char strUPath[MAX_PATH];
		sprintf_s(strAPath, "%s\\%s", path, entry_pointer->d_name);
		sprintf_s(strUPath, "%s\\%s", path, entry_pointer->d_name);
		struct _stat64 stat_buffer;
		int nRet = _stat64(path, stat_buffer);
		if (nRet == 0)
		{
			_FileInfo info;
			info.asciPath = strAPath;
			info.utf8Path = strUPath;
			info.nFileSize = stat_buffer.st_size;
			nTotalSize += info.nFileSize;
			vecEntries.push_back(info);
		}

		// stop when we have reached the maximum requested
		//if (max && ++count == max) break;
	}

	closedir(directory);
	return 0;
}
#endif

int UdtFile::CreateDir(const char* path)
{
	int nLen = 0, nPos = 0;
	string szFolder = "", szSlash = "";
	string szTmpPath = path;
	// WIN32Â·¾¶·Ö¸ô·û»»³É£º"\", LINUX£º'/'
	do 
	{
#ifdef WIN32
		szSlash = '\\';
		nPos = szTmpPath.find('/', 0);
		if (nPos >= 0)
		{
			szTmpPath.replace(nPos, 1, szSlash);
		}
#else
		szSlash = '/';
		nPos = szTmpPath.find('\\', 0);
		if (nPos >= 0)
		{
			szTmpPath.replace(nPos, 1, szSlash);
		}
#endif
	} while (nPos >= 0);

	if ('\\' != szTmpPath[szTmpPath.size()-1] || '/' != szTmpPath[szTmpPath.size()-1])
	{
		szTmpPath += szSlash;
	}
	string szTmp = szTmpPath;
	while (true)
	{
		int nPos = szTmp.find_first_of(szSlash);
		if (nPos >= 0)
		{
			nLen += nPos;
			szTmp = szTmp.substr(nPos+1);
			szFolder = szTmpPath.substr(0, nLen);
			nPos = szTmp.find_first_of(szSlash);
			if (nPos >= 0)
			{
				nLen = nLen + 1 + nPos;
				szTmp = szTmp.substr(nPos);
				szFolder = szTmpPath.substr(0, nLen);
#ifdef WIN32
				if (_access(szFolder.c_str(), 0))
				{
					BOOL result = ::CreateDirectoryW(WIN32_C2WC(szFolder.c_str()), NULL);
					if (result == 0)
						return 1;
					//_mkdir(szFolder.c_str());
				}
#else
				if (access(szFolder.c_str(), F_OK) != 0)
				{
					int result = mkdir(szFolder.c_str(), 0755);
					if (result != 0)
						return 1;
					//mkdir(szFolder.c_str(),770);
				}
#endif
			}
			else
				break;
		}
		else
			break;
	}

	return 0;
}

int UdtFile::GetInfo(const char* path, _FileInfo & info)
{
	int nRet = 0;
	struct _stat64 stat_buffer;

#ifdef WIN32
	nRet = _wstat64(WIN32_C2WC(path), &stat_buffer);
#else
	nRet = _stat64(path, stat_buffer);
#endif

	if (nRet == 0)
	{
		info.nFileSize = stat_buffer.st_size;
	}

	return true;
}
