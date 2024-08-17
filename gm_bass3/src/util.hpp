#include "globals.hpp"

#ifndef UTIL_H
#define UTIL_H

void thfnLoadStream(TChannelThreadArgs ThreadArgs);
void thfnCleanUp();

namespace UTIL
{
	namespace STRING
	{
		typedef bool (*ForEachSegmentFunc)(std::string& sFind, unsigned int iCount, void *pUserData);

		void Trim(std::string& sString);
		bool GetKeyValueFromSeperator(std::string &sInput, std::string &sSeperator, std::string &sKey, std::string &sValue, bool reverse = false);
		bool RemoveChars(std::string &sInput, const char* sCharsToRemove);
		unsigned int ForEachSegment(std::string sInput, std::string &sSeperator, ForEachSegmentFunc func, void *pUserData);
		bool ToNumber(const char* pString, double &fNumber);
		bool ToNumber(std::string &sInput, double &fNumber);

		char* safe_getnewcstr(std::string s);
		size_t safe_cpy(char* d, std::string s);
		size_t safe_cpy(char* d, std::string s, size_t n);
		size_t safe_cpy(char* d, char const *s, size_t n);
	}

	bool LoadStream(TChannelThreadArgs& ThreadArgs, bool bNotThreaded);
	unsigned int VersionToDecimal(DWORD iVer);
	unsigned int GetBASSVersionDecimal();
	void ClearLoadingThreads();
	std::string DecodeBassError(int iCode);
}

#endif