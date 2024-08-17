// Compiling with Source SDK 2013 for Linux/OSX? Don't forget this:
#include "steam/steamtypes.h"

#include "bassfilesys.hpp"
#include "util.hpp"

#include "GarrysMod/InterfacePointers.hpp"
#include "filesystem.h"

IFileSystem* g_pFileSystem = NULL;

// CALLBACKs start
void CALLBACK StreamCloseProc(void *param)
{
	if(ISNULLPTR(g_pFileSystem)) return;

	FileHandle_t pFile = (FileHandle_t) param;
	if(ISNULLPTR(pFile)) return;

	g_pFileSystem->Close(pFile); // close the file
}

QWORD CALLBACK StreamLenProc(void *param)
{
	if(ISNULLPTR(g_pFileSystem)) return 0;

	FileHandle_t pFile = (FileHandle_t) param;
	if(ISNULLPTR(pFile)) return 0;

    return (QWORD)g_pFileSystem->Size(pFile); // return the file length
}

DWORD CALLBACK StreamReadProc(void *buffer, DWORD length, void *param)
{
	if(ISNULLPTR(g_pFileSystem)) return -1;

	FileHandle_t pFile = (FileHandle_t) param;
	if(ISNULLPTR(pFile)) return -1;

	if(g_pFileSystem->EndOfFile(pFile)) 
	{
		return -1;
	}

	return (DWORD)g_pFileSystem->Read(buffer, length, pFile); // read from file;
}

BOOL CALLBACK StreamSeekProc(QWORD offset, void *param)
{
	if(ISNULLPTR(g_pFileSystem)) return false;

	FileHandle_t pFile = (FileHandle_t) param;
	if(ISNULLPTR(pFile)) return false;

	g_pFileSystem->Seek(pFile, (int)offset, FILESYSTEM_SEEK_HEAD);

	return ((QWORD)g_pFileSystem->Tell(pFile) == offset); // seek to offset
}

BASS_FILEPROCS StreamDataTable = {StreamCloseProc, StreamLenProc, StreamReadProc, StreamSeekProc}; // callback table
// CALLBACKs end

typedef IFileSystem* (*GetFilesystem_t)();

namespace BASSFILESYS
{
	bool Init()
	{
		if(!ISNULLPTR(g_pFileSystem)) return true;

		g_pFileSystem = InterfacePointers::FileSystem();
	}

	bool PlayFile(std::string& sFile, bass_flag eFlags, bass_p& pHandleOut, int& iErrOut)
	{
		pHandleOut = BASS_NULL;
		iErrOut = BASS_ERROR_FILESYSTEM;

		if(ISNULLPTR(g_pFileSystem)) return false;

		bass_p pHandle = BASS_NULL;
		int iErr = -1;
		FileHandle_t fh = NULL;

		char* sFileTemp = UTIL::STRING::safe_getnewcstr(sFile);

		V_FixSlashes(sFileTemp);
		V_FixDoubleSlashes(sFileTemp);

		sFile = std::string(sFileTemp);
		delete[] sFileTemp;

		fh = g_pFileSystem->Open(sFile.c_str(), "rb", "GAME");
		if(!ISNULLPTR(fh))
		{
			pHandle = BASS_StreamCreateFileUser(STREAMFILE_NOBUFFER, eFlags, &StreamDataTable, fh);
			iErr = BASS_ErrorGetCode();

			if(iErr == BASS_ERROR_FILEFORM)
			{
				fh = g_pFileSystem->Open(sFile.c_str(), "rb", "GAME");
				if(!ISNULLPTR(fh))
				{
					unsigned int iLength = g_pFileSystem->Size(fh);

					char* pData = new char[iLength];
					int iLengthRead = g_pFileSystem->Read(pData, iLength, fh);
					g_pFileSystem->Close(fh);

					pHandle = BASS_MusicLoad(true, pData, 0, iLengthRead, eFlags, 0);
					iErr = BASS_ErrorGetCode();

					delete [] pData;
				}
				else
				{
					iErr = BASS_ERROR_FILEOPEN;
				}
			}
		}
		else
		{
			iErr = BASS_ERROR_FILEOPEN;
		}

		if(pHandle == BASS_NULL)
		{
			iErr = BASS_ERROR_UNKNOWN;
		}

		pHandleOut = pHandle;
		iErrOut = iErr;

		return iErr == BASS_OK;
	}
}
