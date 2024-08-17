#include "main.hpp"

using namespace GarrysMod::Lua;

//
// Called when you module is opened
//

int Init(ILuaBase* pLUA)
{
	g_pListPendingCallbacks = new SyncList<TChannelCallbackData *>;
	g_pListRunningThreads = new SyncList<std::thread *>;
	g_pfFFTBuffer = new float[65536];

	try
	{
		g_thCleanUp = new std::thread(thfnCleanUp);
	}
	catch (...)
	{
		g_thCleanUp = NULL;
		pLUA->ThrowError("BASS Init failed, error creating cleanup thread.");
		return 0;
	}

	BASS_Stop();
	BASSFILESYS::Init();

	LUAINTERFACE::Init(pLUA);
	BASS_Start();

	try
	{
		BASS_Stop();

		BASS_SetConfig(BASS_CONFIG_MF_VIDEO, TRUE);
		BASS_SetConfig(BASS_CONFIG_NET_PLAYLIST, 2);
		BASS_SetConfig(BASS_CONFIG_DEV_DEFAULT, TRUE);
		BASS_SetConfig(BASS_CONFIG_VISTA_TRUEPOS, TRUE);
		BASS_SetConfig(BASS_CONFIG_OGG_PRESCAN, TRUE);
		BASS_SetConfig(BASS_CONFIG_FLOATDSP, TRUE);

		BASS_SetConfig(BASS_CONFIG_3DALGORITHM, BASS_3DALG_FULL);
		BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 2);

		BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 33);
		BASS_SetConfig(BASS_CONFIG_BUFFER, 500);
		BASS_SetConfig(BASS_CONFIG_NET_BUFFER, 5000);
		BASS_SetConfig(BASS_CONFIG_NET_TIMEOUT, 12500);
		BASS_SetConfig(BASS_CONFIG_NET_READTIMEOUT, 15000);
		BASS_SetConfig(BASS_CONFIG_VERIFY, 0x8000); // 32 kB
		BASS_SetConfig(BASS_CONFIG_VERIFY_NET, 0x8000); // 32 kB

		g_oldAgentSettings = "";
		g_oldProxySettings = "";
		char* cSettings = NULL;
			
		cSettings = (char*)BASS_GetConfigPtr(BASS_CONFIG_NET_AGENT);
		if(!ISNULLPTR(cSettings))
		{
			g_oldAgentSettings = std::string(cSettings);
		}

		cSettings = (char*)BASS_GetConfigPtr(BASS_CONFIG_NET_PROXY);
		if(!ISNULLPTR(cSettings))
		{
			g_oldProxySettings = std::string(cSettings);
		}

		BASS_Start();
	}
	catch(...)
	{
		BASS_Stop();
		BASS_Start();

		pLUA->ThrowError("BASS Init failed, exception: Unknown");
		return 0;
	}

	return 0;
}

GMOD_MODULE_OPEN()
{
	g_CLOSING = false;
	g_IntialTickHappend = false;
	g_SELFLOADED = false;
	g_pcErrorBuffer = new char[ERROR_PUFFER_SIZE];

	unsigned int iVer = HIWORD(BASS_GetVersion());
	if(iVer < BASSVERSION)
	{
		unsigned int iHeaderVer = UTIL::VersionToDecimal(BASSVERSION << 16);

		snprintf(g_pcErrorBuffer, ERROR_PUFFER_SIZE, "BASS Init failed, outdated BASS Version (%x expected, got %i).\n", iHeaderVer, UTIL::GetBASSVersionDecimal());
		LUA->ThrowError(g_pcErrorBuffer);

		return 0;
	}

	LUAINTERFACE::SetupRealm(LUA);

	if(g_CLIENT)
	{
		return Init(LUA);
	}
	else // Only for the server
	{
		try
		{
			BASS_SetConfig(BASS_CONFIG_DEV_DEFAULT, true);
			if(!BASS_Init(-1, 44100, BASS_NULL, NULL, NULL)) // Try to load Bass.
			{
				int error = BASS_ErrorGetCode();
				if (error == BASS_ERROR_ALREADY) return Init(LUA); // Bass is already loaded, so use that.

				if (error == BASS_ERROR_DX)
				{
					snprintf(g_pcErrorBuffer, ERROR_PUFFER_SIZE, "BASS Init failed, error code %d.\nDirectX or ALSA is not available.\nMake sure you installed ASIO on Linux or DirectX 9+ on Windows.", error);
					LUA->ThrowError(g_pcErrorBuffer);

					return 0;
				}

				if (error == BASS_ERROR_DRIVER)
				{
					snprintf(g_pcErrorBuffer, ERROR_PUFFER_SIZE, "BASS Init failed, error code %d.\nSound driver is not available.\nIf you are on Linux, make sure you installed ASIO.\nOn Linux the user running this application needs 'xrw' access to the sound interface aswell.", error);
					LUA->ThrowError(g_pcErrorBuffer);

					return 0;
				}

				if (error == BASS_ERROR_DEVICE)
				{
					snprintf(g_pcErrorBuffer, ERROR_PUFFER_SIZE, "BASS Init failed, error code %d.\nSound driver is not available or the device was not found.\nIf you are on Linux, make sure you installed ASIO.", error);
					LUA->ThrowError(g_pcErrorBuffer);

					return 0;
				}

				snprintf(g_pcErrorBuffer, ERROR_PUFFER_SIZE, "BASS Init failed, error code %d.\n", error);
				LUA->ThrowError(g_pcErrorBuffer);

				return 0;
			}
		}
		catch(const std::overflow_error& e)
		{
			snprintf(g_pcErrorBuffer, ERROR_PUFFER_SIZE, "BASS Init failed, overflow_error exception: %s\n", e.what());
			LUA->ThrowError(g_pcErrorBuffer);

			return 0;
		}
		catch(const std::runtime_error& e)
		{
			snprintf(g_pcErrorBuffer, ERROR_PUFFER_SIZE, "BASS Init failed, runtime_error exception: %s\n", e.what());
			LUA->ThrowError(g_pcErrorBuffer);

			return 0;
		}
		catch(const std::exception& e)
		{
			snprintf(g_pcErrorBuffer, ERROR_PUFFER_SIZE, "BASS Init failed, exception: %s\n", e.what());
			LUA->ThrowError(g_pcErrorBuffer);

			return 0;
		}
		catch(std::string s)
		{
			snprintf(g_pcErrorBuffer, ERROR_PUFFER_SIZE, "BASS Init failed, exception: %s\n", s.c_str());
			LUA->ThrowError(g_pcErrorBuffer);

			return 0;
		}
		catch(char* s)
		{
			snprintf(g_pcErrorBuffer, ERROR_PUFFER_SIZE, "BASS Init failed, exception: %s\n", s);
			LUA->ThrowError(g_pcErrorBuffer);

			return 0;
		}
		catch(...)
		{
			LUA->ThrowError("BASS Init failed, unknown exception.\n");
			return 0;
		}
	}

	g_SELFLOADED = true;

	try
	{
		BASS_Stop();
		BASS_Start();
	}
	catch(...)
	{
		LUA->ThrowError("BASS Init failed, exception error: Unknown");
		return 0;
	}

	return Init(LUA);
}

//
// Called when your module is closed
//
GMOD_MODULE_CLOSE()
{
	g_CLOSING = true;

	if(!ISNULLPTR(g_thCleanUp))
	{
		g_thCleanUp->join();
		delete g_thCleanUp;
		g_thCleanUp = NULL;
	}

	if(!ISNULLPTR(g_pListRunningThreads))
	{
		delete g_pListRunningThreads;
		g_pListRunningThreads = NULL;
	}

	if(!ISNULLPTR(g_pListPendingCallbacks))
	{
		delete g_pListPendingCallbacks;
		g_pListPendingCallbacks = NULL;
	}

	if(!ISNULLPTR(g_pfFFTBuffer))
	{
		delete [] g_pfFFTBuffer;
		g_pfFFTBuffer = NULL;
	}

	if (!ISNULLPTR(g_pcErrorBuffer))
	{
		delete [] g_pcErrorBuffer;
		g_pcErrorBuffer = NULL;
	}

	LUAINTERFACE::Dispose(LUA);

	BASS_SetConfigPtr(BASS_CONFIG_NET_AGENT, g_oldAgentSettings.c_str());
	BASS_SetConfigPtr(BASS_CONFIG_NET_PROXY, g_oldProxySettings.c_str());

	if(g_SELFLOADED)
	{
		try
		{
			BASS_Stop();

			BASS_PluginFree(BASS_NULL);
			BASS_Free();
		}
		catch(...)
		{
		}
	}

	return 0;
}