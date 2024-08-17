#include "globals.hpp"

#ifndef BASSFILESYS_H
#define BASSFILESYS_H



namespace BASSFILESYS
{
	bool Init();
	bool PlayFile(std::string& sFile, bass_flag eFlags, bass_p& pHandleOut, int& iErrOut);
}

#endif