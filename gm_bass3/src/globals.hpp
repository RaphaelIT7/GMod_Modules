#include <thread>
#include <atomic>
#include <mutex>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <list>
#include <map>

#include <math.h>

// Compiling with Source SDK 2013 for Linux/OSX? Don't forget this:
#include "steam/steamtypes.h"
#include "tier0/dbg.h"

#include "list.hpp"
#include "bass/bass.h"

#define GMOD_USE_SOURCESDK

#ifdef _WIN32
#else
	#include <unistd.h>
#endif

#ifndef GLOBALS_H
#define GLOBALS_H



#define GM_BASS_VERSION 15

#define META_IBASS3CHANNEL "IBASS3Channel"
#define META_IBASS3FXECHO "IBASS3FxEcho"

#define BASS_NO_CHANGE -1
#define BASS_AUTO -1
#define BASS_FX_UNKNOWN -1
#define BASS_NULL 0
#define BASS_MIN_DECIBELS -1000.0f
#define BASS_MAX_FX 2048

#define BASS_ERROR_FILESYSTEM 102
#define ERROR_PUFFER_SIZE 2048

#define BASS_WAVEFORM_TRIANGLE 0
#define BASS_WAVEFORM_SINE 1
#define BASS_WAVEFORM_SQUARE 1

#define BASS_CURVE_LINEAR 0
#define BASS_CURVE_LOGARITHMIC 1

#ifdef _WIN32
	#define snprintf sprintf_s
	#define SLEEP(X) Sleep(X)
#else
	#define SLEEP(X) usleep(X)
#endif

#define ENUM_TO_VALUE(ENUM, VALUE, VAR) case ENUM: VAR = VALUE; break

#define ISNULLPTR(x) ((x == NULL) || (x == nullptr))

#define UNREFDELETE(x) \
	if(!x->IsReferenced()){ \
		delete x; \
		x = NULL; \
		}

typedef DWORD bass_p;
typedef HDSP  bass_dsp;
typedef HFX  bass_fx;
typedef signed long long bass_flag;
typedef QWORD bass_time;

class TChannel;

struct TChannelCallbackData
{
	TChannel* pChannel;
	bool bIsRecycledChannel;
	int iError;
	int iCallbackRef;
	bool bNoPlay;
};

struct TChannelThreadArgs
{
	TChannel* pChannel;
	std::string sURL;
	bool bIsOnline;
	bass_flag eFlags;
	int iCallbackRef;
	bool bNoPlay;
};

extern bool g_CLIENT;
extern bool g_IsDedicatedServer;
extern bool g_SELFLOADED;
extern bool g_CLOSING;
extern bool g_IntialTickHappend;
extern int g_ChannelTypeID;

extern std::thread* g_thCleanUp;
extern SyncList<TChannelCallbackData *>* g_pListPendingCallbacks;
extern SyncList<std::thread *>* g_pListRunningThreads;
extern float* g_pfFFTBuffer;
extern char* g_pcErrorBuffer;

extern std::string g_oldAgentSettings;
extern std::string g_oldProxySettings;

#endif

