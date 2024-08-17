#include "tchannel.hpp"

#include "../bassfilesys.hpp"
#include "../util.hpp"

using namespace std;

string TChannel::LUAMETANAME = "IBASS3Channel";
int TChannel::LUAMETAID = 0;
map<unsigned long long, TChannel*> TChannel::g_mapObjectInstances = map<unsigned long long, TChannel*>();


// +--------------------------------------------------+
// |                    Friends                       |
// +--------------------------------------------------+
void thfnSeekTo(TChannel* pChannel)
{
	if(ISNULLPTR(pChannel)) return;
	if(!pChannel->CanSeek()) return;

	while(1)
	{
		if(!pChannel->CanSeek()) return;
		if(!pChannel->Seek()) this_thread::yield();
		SLEEP(5);
	}
}

// +--------------------------------------------------+
// |                 Private Methods                  |
// +--------------------------------------------------+

void TChannel::Init()
{
	lock_guard<mutex> Lock(MutexLock);
	g_mapObjectInstances[id] = this;

	pthSeeker = NULL;
	sFilename = "";
	iSeekingTo = 0;
	fVolumeBoost = 0;
	fVolume = 0;

	bCanSeek = false;
	bSeaking = false;

	pHandle = BASS_NULL;
	bIsOnline = false;
	bIsRemoved = true;

	lsFX.clear();
}

bool TChannel::IsValidInternal()
{
	// Empty on propose channels are valid aswell.
	if (bIsRemoved) return true;

	// Invalid if unexpectedly empty.
	if (pHandle == BASS_NULL) return false;
	return true;
}

bool TChannel::Is3DInternal()
{
	// Server shouldn't have 3D support.
	if(!g_CLIENT) return false;
	return HasFlag(BASS_SAMPLE_3D);
}

void TChannel::RemoveInternal()
{
	BASS_ChannelStop(pHandle);
	BASS_MusicFree(pHandle);
	BASS_StreamFree(pHandle);
	sFilename = "";

	bCanSeek = false;

	pHandle = BASS_NULL;
	bIsOnline = false;

	if(!ISNULLPTR(pthSeeker))
	{
		pthSeeker->join();
		delete pthSeeker;
	}

	iSeekingTo = 0;
	pthSeeker = NULL;
	bIsRemoved = true;
}

void TChannel::ClearFxInternal()
{
	list<TEffect*>::iterator iter = lsFX.begin();

	while (iter != lsFX.end())
	{
		TEffect* pFX = *iter;
		if (ISNULLPTR(pFX))
		{
			iter++;
			continue;
		}

		pFX->Remove();
		UNREFDELETE(pFX);
		iter++;
	}

	lsFX.clear();
}

void TChannel::ApplyFxInternal()
{
	list<TEffect*>::iterator iter = lsFX.begin();

	while (iter != lsFX.end())
	{
		TEffect* pFX = *iter;
		if (ISNULLPTR(pFX))
		{
			iter = lsFX.erase(iter);
			continue;
		}

		if (pFX->IsRemoved())
		{
			iter = lsFX.erase(iter);
			UNREFDELETE(pFX);
			continue;
		}

		pFX->Apply(pHandle);
		iter++;
	}
}

void TChannel::UpdateFxInternal()
{
	if (!g_CLIENT) return;

	list<TEffect*>::iterator iter = lsFX.begin();

	while (iter != lsFX.end())
	{
		TEffect* pFX = *iter;
		if (ISNULLPTR(pFX))
		{
			iter = lsFX.erase(iter);
			continue;
		}

		if (pFX->IsRemoved())
		{
			iter = lsFX.erase(iter);
			UNREFDELETE(pFX);
			continue;
		}

		pFX->Update();
		iter++;
	}
}


void TChannel::Reset3D()
{
	if(!Is3DInternal()) return;

	BASS_3DVECTOR vNullpos(0, 0, 0);

	BASS_ChannelSet3DAttributes(pHandle, BASS_3DMODE_NORMAL, 200, 1000000000, 360, 360, 0);
	BASS_ChannelSet3DPosition(pHandle, &vNullpos, &vNullpos, &vNullpos);
	BASS_ChannelSetAttribute(pHandle, BASS_ATTRIB_EAXMIX, BASS_AUTO);
	BASS_Apply3D();
}

bass_time TChannel::TimeToByte(double fTime)
{
	if (fTime < 0) fTime = 0; // No negatives
	return BASS_ChannelSeconds2Bytes(pHandle, fTime);
}

double TChannel::ByteToTime(bass_time iBytes)
{
	return BASS_ChannelBytes2Seconds(pHandle, iBytes);;
}

bass_time TChannel::GetRawTime()
{
	return BASS_ChannelGetPosition(pHandle, BASS_POS_BYTE);
}

void TChannel::SetRawTime(bass_time iTime)
{
	BASS_ChannelSetPosition(pHandle, iTime, BASS_POS_BYTE | BASS_POS_SCAN);

	signed long long int iDeltaTime = iSeekingTo - GetRawTime();
	if(iDeltaTime < 8192 && iDeltaTime > -8192)
	{
		bSeaking = false;
	}
}

bool TChannel::HasFlag(bass_flag eFlag)
{
	if(!IsValidInternal()) return false;
	return (BASS_ChannelFlags(pHandle, 0, 0) & eFlag) > 0; // has the flag
}

void TChannel::SetFlag(bass_flag eFlag, bool bVar)
{
	if(!IsValidInternal()) return;
	BASS_ChannelFlags(pHandle, bVar ? eFlag : 0, eFlag); // set the flag
}

int TChannel::LoadURL(string sURL, bass_flag eFlags)
{
	lock_guard<mutex> Lock(MutexLoadingLock);

	if(sURL.empty())
	{
		lock_guard<mutex> Lock2(MutexLock);
		RemoveInternal();
		return BASS_ERROR_FILEOPEN;
	}

	bass_p pLoadedHandle = BASS_NULL;
	int iErr = 0;
	bCanSeek = false;
		
	try
	{
		pLoadedHandle = BASS_StreamCreateURL(sURL.c_str(), 0, eFlags, NULL, NULL);
		iErr = BASS_ErrorGetCode();
	}
	catch(...)
	{
		pLoadedHandle = BASS_NULL;
		iErr = BASS_ERROR_MEM;
	}

	lock_guard<mutex> Lock2(MutexLock);
	RemoveInternal();
	sFilename = string(sURL);

	if(iErr == BASS_OK)
	{
		pHandle = pLoadedHandle;
		bIsOnline = true;
		bCanSeek = true;
		bIsRemoved = false;
		Reset3D();
		ApplyFxInternal();
	}
	else
	{
		BASS_ChannelStop(pLoadedHandle);
		BASS_MusicFree(pLoadedHandle);
		BASS_StreamFree(pLoadedHandle);
		RemoveInternal();
	}
	return iErr;
}

int TChannel::LoadFile(string sURL, bass_flag eFlags)
{
	lock_guard<mutex> Lock(MutexLoadingLock);

	if (sURL.empty())
	{
		lock_guard<mutex> Lock2(MutexLock);
		RemoveInternal();
		return BASS_ERROR_FILEOPEN;
	}

	bass_p pLoadedHandle = NULL;
	int iErr = 0;
	bCanSeek = false;

	try
	{
		BASSFILESYS::PlayFile(sURL, eFlags, pLoadedHandle, iErr);
	}
	catch(...)
	{
		pLoadedHandle = NULL;
		iErr = BASS_ERROR_MEM;
	}

	lock_guard<mutex> Lock2(MutexLock);
	RemoveInternal();
	sFilename = string(sURL);

	if(iErr == BASS_OK)
	{
		pHandle = pLoadedHandle;
		bIsOnline = false;
		bCanSeek = true;
		bIsRemoved = false;
		Reset3D();
		ApplyFxInternal();
	}
	else
	{
		BASS_ChannelStop(pLoadedHandle);
		BASS_MusicFree(pLoadedHandle);
		BASS_StreamFree(pLoadedHandle);
		RemoveInternal();
	}
	return iErr;
}




// +--------------------------------------------------+
// |              Constructor/Destructor              |
// +--------------------------------------------------+

TChannel::TChannel()
{
	Init();
}

TChannel::TChannel(string sURL, bool bIsOnline, bass_flag eFlags)
{
	Init();
	Load(sURL, bIsOnline, eFlags);
}

TChannel::TChannel(string sURL, bool bIsOnline)
{
	Init();
	Load(sURL, bIsOnline);
}


TChannel::~TChannel()
{
	{ // Scope
		lock_guard<mutex> Lock(MutexLock);
		auto it = g_mapObjectInstances.find(id);
		if (it != g_mapObjectInstances.end())
		{
			g_mapObjectInstances.erase(it);
		}
	}

	Remove();
}




// +--------------------------------------------------+
// |                  Public Methods                  |
// +--------------------------------------------------+

void TChannel::Remove()
{
	thread* pthSeekerTmp;  

	{ // Scope
		lock_guard<mutex> Lock(MutexLock);
		ClearFxInternal();

		BASS_ChannelStop(pHandle);
		BASS_MusicFree(pHandle);
		BASS_StreamFree(pHandle);
		sFilename = "";

		bCanSeek = false;

		pHandle = BASS_NULL;
		bIsOnline = false;

		pthSeekerTmp = pthSeeker;
	}

	if(!ISNULLPTR(pthSeekerTmp))
	{
		pthSeekerTmp->join();
		delete pthSeekerTmp;
	}

	{ // Scope
		lock_guard<mutex> Lock(MutexLock);
		iSeekingTo = 0;
		pthSeeker = NULL;
		bIsRemoved = true;
	}
}

bool TChannel::Update(DWORD ilength)
{
	lock_guard<mutex> Lock(MutexLock);

	if(!IsValidInternal()) return false;
	return (BASS_ChannelUpdate(pHandle, ilength) == TRUE);
}

bool TChannel::Update()
{
	return Update(0);
}

void TChannel::Think()
{
	lock_guard<mutex> Lock(MutexLock);
	if (!g_CLIENT) return;

	UpdateFxInternal();
}

int TChannel::Load(string sURL, bool bIsOnline, bass_flag eFlags)
{
	if(bIsOnline)
	{
		return LoadURL(sURL, eFlags);
	}
	else
	{
		return LoadFile(sURL, eFlags);
	}
}

int TChannel::Load(string sURL, bool bIsOnline)
{
	return Load(sURL, bIsOnline, BASS_NULL);
}

bass_p TChannel::GetBassHandle()
{
	return pHandle;
}

bool TChannel::IsValid()
{
	lock_guard<mutex> Lock(MutexLock);
	return IsValidInternal();
}

void TChannel::Play(bool bRestart)
{
	lock_guard<mutex> Lock(MutexLock);
	if(!IsValidInternal()) return;
	BASS_ChannelPlay(pHandle, bRestart);
}

void TChannel::Play()
{
	Play(false);
}

void TChannel::Stop()
{
	lock_guard<mutex> Lock(MutexLock);
	BASS_ChannelStop(pHandle);
}

void TChannel::Pause()
{
	lock_guard<mutex> Lock(MutexLock);
	if(!IsValidInternal()) return;
	BASS_ChannelPause(pHandle);
}

bass_flag TChannel::GetState()
{
	lock_guard<mutex> Lock(MutexLock);
	return (bass_flag)BASS_ChannelIsActive(pHandle);
}

void TChannel::SetVolume(float fVolume)
{
	lock_guard<mutex> Lock(MutexLock);
	if(!IsValidInternal()) return;
	if (fVolume < 0) fVolume = 0; // No negatives
	if (fVolume > 1000) fVolume = 1000;

	this->fVolume = fVolume;

	BASS_ChannelSlideAttribute(pHandle, BASS_ATTRIB_VOL, fVolume, 0);
}

float TChannel::GetVolume()
{
	float fVolume;

	lock_guard<mutex> Lock(MutexLock);
	if(!IsValidInternal()) return 0;

	BASS_ChannelGetAttribute(pHandle, BASS_ATTRIB_VOL, &fVolume);

	if (fVolume < 0) fVolume = 0; // No negatives
	if (fVolume > 1000) fVolume = 1000;

	return fVolume;
}

void TChannel::SetVolumeBoost(float fVolumeBoost)
{
	lock_guard<mutex> Lock(MutexLock);

	if (!IsValidInternal()) return;
	if (fVolumeBoost < 0) fVolumeBoost = 0; // No negatives
	if (fVolumeBoost > 1000) fVolumeBoost = 1000;

	float fVolume = this->fVolume;
	this->fVolumeBoost = fVolumeBoost;

	SetVolume(fVolume * (1 + fVolumeBoost));

	this->fVolume = fVolume;
}

float TChannel::GetVolumeBoost()
{
	lock_guard<mutex> Lock(MutexLock);
	if (!IsValidInternal()) return 0;

	return this->fVolumeBoost;
}


void TChannel::VolumeFadeTo(float fVolume, DWORD iTime)
{
	lock_guard<mutex> Lock(MutexLock);
	if(!IsValidInternal()) return;
	if (fVolume < 0) fVolume = 0; // No negatives
	if (fVolume > 1) fVolume = 1000;

	BASS_ChannelSlideAttribute(pHandle, BASS_ATTRIB_VOL, fVolume, iTime);
}

bool TChannel::VolumeIsFading()
{
	lock_guard<mutex> Lock(MutexLock);
	if(!IsValidInternal()) return false;
	return (BASS_ChannelIsSliding(pHandle, BASS_ATTRIB_VOL) == TRUE);
}

void TChannel::SetBalance(float fBalance)
{
	lock_guard<mutex> Lock(MutexLock);
	if(!IsValidInternal()) return;
	if(Is3DInternal()) return;

	if (fBalance < -1) fBalance = -1;
	if (fBalance > 1) fBalance = 1;

	BASS_ChannelSlideAttribute(pHandle, BASS_ATTRIB_PAN, fBalance, 0);
}

float TChannel::GetBalance()
{
	float fBalance;

	lock_guard<mutex> Lock(MutexLock);
	if(!IsValidInternal()) return 0;
	if(Is3DInternal()) return 0;

	BASS_ChannelGetAttribute(pHandle, BASS_ATTRIB_PAN, &fBalance);
	return fBalance;
}

void TChannel::BalanceFadeTo(float fBalance, DWORD iTime)
{
	lock_guard<mutex> Lock(MutexLock);
	if(!IsValidInternal()) return;
	if(Is3DInternal()) return;

	if (fBalance < -1) fBalance = -1;
	if (fBalance > 1) fBalance = 1;

	BASS_ChannelSlideAttribute(pHandle, BASS_ATTRIB_PAN, fBalance, iTime);
}

bool TChannel::BalanceIsFading()
{
	lock_guard<mutex> Lock(MutexLock);
	if(!IsValidInternal()) return false;
	if(Is3DInternal()) return false;

	return (BASS_ChannelIsSliding(pHandle, BASS_ATTRIB_PAN) == TRUE);
}

bool TChannel::Is3D()
{
	lock_guard<mutex> Lock(MutexLock);
	return Is3DInternal();
}

bool TChannel::IsLooping()
{
	lock_guard<mutex> Lock(MutexLock);
	return HasFlag(BASS_SAMPLE_LOOP);
}

bool TChannel::IsOnline()
{
	lock_guard<mutex> Lock(MutexLock);
	return bIsOnline;
}

bool TChannel::IsBlockStreamed()
{
	lock_guard<mutex> Lock(MutexLock);
	return HasFlag(BASS_STREAM_BLOCK);
}

void TChannel::EnableLooping(bool bLoop)
{
	lock_guard<mutex> Lock(MutexLock);
	SetFlag(BASS_SAMPLE_LOOP, bLoop);
}

bool TChannel::FFT(bass_flag eMode, float *pfSpectrum)
{
	lock_guard<mutex> Lock(MutexLock);
	if(ISNULLPTR(pfSpectrum)) return false;

	if(!IsValidInternal()) return false;

	bass_flag iState = BASS_ChannelIsActive(pHandle);
	if(iState != BASS_ACTIVE_PLAYING && iState != BASS_ACTIVE_PAUSED) return false;

	BASS_ChannelGetData(pHandle, pfSpectrum, eMode);

	return true;
}

bool TChannel::GetLevel(WORD* piLevelLeft, WORD* piLevelRight)
{
	lock_guard<mutex> Lock(MutexLock);
	if(ISNULLPTR(piLevelLeft)) return false;
	if(ISNULLPTR(piLevelRight)) return false;

	if(!IsValidInternal()) return false;
	bass_flag iState = BASS_ChannelIsActive(pHandle);
	if(iState != BASS_ACTIVE_PLAYING && iState != BASS_ACTIVE_PAUSED) return false;

	DWORD iLevel = BASS_ChannelGetLevel(pHandle);
	*piLevelLeft = HIWORD(iLevel);
	*piLevelRight = LOWORD(iLevel);

	return true;
}

bool TChannel::GetLevelEx(float* pfLevels)
{
	return GetLevelEx(pfLevels, 0.02, false);
}

bool TChannel::GetLevelEx(float* pfLevels, float fTimeFrame)
{
	return GetLevelEx(pfLevels, fTimeFrame, false);
}

bool TChannel::GetLevelEx(float* pfLevels, float fTimeFrame, bool bRMS)
{
	lock_guard<mutex> Lock(MutexLock);
	if(ISNULLPTR(pfLevels)) return false;

	if(!IsValidInternal()) return false;
	bass_flag iState = BASS_ChannelIsActive(pHandle);
	if(iState != BASS_ACTIVE_PLAYING && iState != BASS_ACTIVE_PAUSED) return false;

	if(fTimeFrame < 0.001) fTimeFrame = 0.001;
	if(fTimeFrame > 1) fTimeFrame = 1;

	return (BASS_ChannelGetLevelEx(pHandle, pfLevels, fTimeFrame, BASS_LEVEL_STEREO | (bRMS ? BASS_LEVEL_RMS : 0)) == TRUE);
}

double TChannel::GetTime()
{
	lock_guard<mutex> Lock(MutexLock);
	if(!IsValidInternal()) return 0;

	bass_time iPos = BASS_ChannelGetPosition(pHandle, BASS_POS_BYTE);
	return ByteToTime(iPos);
}

void TChannel::SetTime(double fTime)
{
	lock_guard<mutex> Lock(MutexLock);
	if(!IsValidInternal()) return;
	if(HasFlag(BASS_STREAM_BLOCK)) return;

	bass_time iSeekTo = TimeToByte(fTime);
	bass_time iLen = BASS_ChannelGetLength(pHandle, BASS_POS_BYTE);

	if(iSeekTo > iLen) iSeekTo = iLen;

	signed long long int iDeltaTime = iSeekTo - GetRawTime();
	if(iDeltaTime < 8192 && iDeltaTime > -8192)
	{
		iSeekingTo = iSeekTo;
		SetRawTime(iSeekTo);
		bSeaking = false;
		return;
	}

	bSeaking = true;
	iSeekingTo = iSeekTo;

	try
	{
		if(ISNULLPTR(pthSeeker)) pthSeeker = new thread(thfnSeekTo, this);
	}
	catch(...) // Failback
	{
		SetRawTime(iSeekingTo);
		bSeaking = false;
	}

}

double TChannel::GetLength()
{
	lock_guard<mutex> Lock(MutexLock);
	if(!IsValidInternal()) return 0;

	bass_time iPos = BASS_ChannelGetLength(pHandle, BASS_POS_BYTE);
	return ByteToTime(iPos);
}

bool TChannel::IsEndless()
{
	return GetLength() < 0;
}

bool TChannel::Seek()
{
	lock_guard<mutex> Lock(MutexLock);

	if(!IsValidInternal()) return false;
	if(!bCanSeek) return false;
	if(!bSeaking) return false;

	if(HasFlag(BASS_STREAM_BLOCK)) return false;

	bass_time iCurTime = GetRawTime();

	signed long long int iDeltaTime = iSeekingTo - iCurTime;
	signed long long int iMaxSeekDelta = 1024*256;

	if(iDeltaTime < 8192 && iDeltaTime > -8192)
	{
		SetRawTime(iSeekingTo);
		return bSeaking;
	}

	if(iDeltaTime > 0)
	{
		if(iMaxSeekDelta > iDeltaTime) iMaxSeekDelta = iDeltaTime;
		SetRawTime(iCurTime + iMaxSeekDelta);
	}
	else
	{
		if(iMaxSeekDelta > (-iDeltaTime)) iMaxSeekDelta = (-iDeltaTime);
		SetRawTime(iCurTime - iMaxSeekDelta);
	}

	return bSeaking;
}

bool TChannel::IsSeeking()
{
	lock_guard<mutex> Lock(MutexLock);
	if(!IsValidInternal()) return false;
	if(HasFlag(BASS_STREAM_BLOCK)) return false;

	return bSeaking;
}

bool TChannel::CanSeek()
{
	lock_guard<mutex> Lock(MutexLock);
	if(!IsValidInternal()) return false;
	if(HasFlag(BASS_STREAM_BLOCK)) return false;

	return bCanSeek;
}

bass_time TChannel::GetSeekingTo()
{
	lock_guard<mutex> Lock(MutexLock);
	if(!IsValidInternal()) return 0;
	if(HasFlag(BASS_STREAM_BLOCK)) return 0;

	return iSeekingTo;
}

string TChannel::GetFileName()
{
	BASS_CHANNELINFO info;

	lock_guard<mutex> Lock(MutexLock);
	if(!IsValidInternal()) return string("NULL");
	if(!bIsOnline) return sFilename;

	BASS_ChannelGetInfo(pHandle, &info);
	if (ISNULLPTR(info.filename)) return string("NULL");

	return string(info.filename);
}

const char* TChannel::GetTag(bass_flag eMode)
{
	lock_guard<mutex> Lock(MutexLock);

	if(!IsValidInternal()) return NULL;
	return BASS_ChannelGetTags(pHandle, eMode);
}

string TChannel::GetFileFormat()
{
	BASS_CHANNELINFO info;
	string sType;

	lock_guard<mutex> Lock(MutexLock);
	if(!IsValidInternal()) return string("NULL");

	BASS_ChannelGetInfo(pHandle, &info);
	switch(info.ctype)
	{
		ENUM_TO_VALUE(BASS_CTYPE_SAMPLE,			"SAMPLE",	sType);
		ENUM_TO_VALUE(BASS_CTYPE_STREAM,			"STREAM",	sType);
		ENUM_TO_VALUE(BASS_CTYPE_STREAM_OGG,		"OGG",		sType);
		//ENUM_TO_VALUE(BASS_CTYPE_STREAM_FLAC,		"FLAC",		sType);
		//ENUM_TO_VALUE(BASS_CTYPE_STREAM_FLAC_OGG,	"OGG",		sType);
		ENUM_TO_VALUE(BASS_CTYPE_STREAM_MP1,		"MP1",		sType);
		ENUM_TO_VALUE(BASS_CTYPE_STREAM_MP2,		"MP2",		sType);
		ENUM_TO_VALUE(BASS_CTYPE_STREAM_MP3,		"MP3",		sType);
		//ENUM_TO_VALUE(BASS_CTYPE_STREAM_MP4,		"MP4",		sType);
		//ENUM_TO_VALUE(BASS_CTYPE_STREAM_AAC,		"AAC",		sType);
		//ENUM_TO_VALUE(BASS_CTYPE_STREAM_ALAC,		"ALAC",		sType);
		ENUM_TO_VALUE(BASS_CTYPE_STREAM_AIFF,		"AIFF",		sType);
		ENUM_TO_VALUE(BASS_CTYPE_STREAM_CA,			"CA",		sType);

		ENUM_TO_VALUE(BASS_CTYPE_STREAM_WAV_PCM,	"WAV",		sType);
		ENUM_TO_VALUE(BASS_CTYPE_STREAM_WAV_FLOAT,	"WAV",		sType);
		ENUM_TO_VALUE(BASS_CTYPE_STREAM_WAV,		"WAV",		sType);
			
		// MOD stuff
		ENUM_TO_VALUE(BASS_CTYPE_MUSIC_MOD,			"MOD",		sType);
		ENUM_TO_VALUE(BASS_CTYPE_MUSIC_MTM,			"MTM",		sType);
		ENUM_TO_VALUE(BASS_CTYPE_MUSIC_S3M,			"S3M",		sType);
		ENUM_TO_VALUE(BASS_CTYPE_MUSIC_XM,			"XM",		sType);
		ENUM_TO_VALUE(BASS_CTYPE_MUSIC_IT,			"IT",		sType);
		ENUM_TO_VALUE(BASS_CTYPE_MUSIC_MO3,			"MO3",		sType);

		ENUM_TO_VALUE(BASS_CTYPE_STREAM_MF,			"MF",		sType);

		default: sType = "UNKNOWN";
	}

	return sType;
}

DWORD TChannel::GetSamplingRate()
{
	BASS_CHANNELINFO info;
	lock_guard<mutex> Lock(MutexLock);

	if(!IsValidInternal()) return 0;
	BASS_ChannelGetInfo(pHandle, &info);

	return info.freq;
}

BYTE TChannel::GetBitsPerSample()
{
	BASS_CHANNELINFO info;
	lock_guard<mutex> Lock(MutexLock);

	if(!IsValidInternal()) return 0;
	BASS_ChannelGetInfo(pHandle, &info);

	return info.origres & 0xFF;
}

float TChannel::GetAverageBitRate()
{
	float fBitRate;

	lock_guard<mutex> Lock(MutexLock);
	if(!IsValidInternal()) return 0;

	BASS_ChannelGetAttribute(pHandle, BASS_ATTRIB_BITRATE, &fBitRate);
	return fBitRate;
}

bool TChannel::GetPos(BASS_3DVECTOR* pvPos, BASS_3DVECTOR* pvDir, BASS_3DVECTOR* pvVel)
{
	lock_guard<mutex> Lock(MutexLock);

	if(!Is3DInternal()) return false;
	return BASS_ChannelGet3DPosition(pHandle, pvPos, pvDir, pvVel) == TRUE;
}

bool TChannel::SetPos(BASS_3DVECTOR* pvPos, BASS_3DVECTOR* pvDir, BASS_3DVECTOR* pvVel)
{
	lock_guard<mutex> Lock(MutexLock);

	if(!Is3DInternal()) return false;
	BASS_ChannelSet3DPosition(pHandle, pvPos, pvDir, pvVel);
	BASS_Apply3D();

	return true;
}


bool TChannel::GetPos(BASS_3DVECTOR* pvPos, BASS_3DVECTOR* pvDir)
{
	return GetPos(pvPos, pvDir, NULL);
}

bool TChannel::SetPos(BASS_3DVECTOR* pvPos, BASS_3DVECTOR* pvDir)
{
	return SetPos(pvPos, pvDir, NULL);
}


bool TChannel::GetPos(BASS_3DVECTOR* pvPos)
{
	return GetPos(pvPos, NULL, NULL);
}

bool TChannel::SetPos(BASS_3DVECTOR* pvPos)
{
	return SetPos(pvPos, NULL, NULL);
}

bool TChannel::Get3DFadeDistance(float* pfMin, float* pfMax)
{
	lock_guard<mutex> Lock(MutexLock);

	if(!Is3DInternal()) return false;
	return BASS_ChannelGet3DAttributes(pHandle, NULL, pfMin, pfMax, NULL, NULL, NULL) == TRUE;
}

void TChannel::Set3DFadeDistance(float fMin, float fMax)
{
	lock_guard<mutex> Lock(MutexLock);

	if(!Is3DInternal()) return;
	BASS_ChannelSet3DAttributes(pHandle, BASS_NO_CHANGE, fMin, fMax, BASS_NO_CHANGE, BASS_NO_CHANGE, BASS_NO_CHANGE);
	BASS_Apply3D();
}

bool TChannel::Get3DFadeDistance(float* pfMin)
{
	return Get3DFadeDistance( pfMin, NULL );
}

void TChannel::Set3DFadeDistance(float fMin)
{
	Set3DFadeDistance( fMin, BASS_NO_CHANGE );
}

bool TChannel::Get3DCone(DWORD* piInnerAngle, DWORD* piOuterAngle, float* pfOuterVolume)
{
	lock_guard<mutex> Lock(MutexLock);

	if(!Is3DInternal()) return false;
	return BASS_ChannelGet3DAttributes(pHandle, NULL, NULL, NULL, piInnerAngle, piOuterAngle, pfOuterVolume) == TRUE;
}

void TChannel::Set3DCone(DWORD iInnerAngle, DWORD iOuterAngle, float fOuterVolume)
{
	lock_guard<mutex> Lock(MutexLock);

	if(!Is3DInternal()) return;
	BASS_ChannelSet3DAttributes(pHandle, BASS_NO_CHANGE, BASS_NO_CHANGE, BASS_NO_CHANGE, iInnerAngle, iOuterAngle, fOuterVolume);
	BASS_Apply3D();
}

bool TChannel::Get3DCone(DWORD* piInnerAngle, DWORD* piOuterAngle)
{
	return Get3DCone( piInnerAngle, piOuterAngle, NULL );
}

void TChannel::Set3DCone(DWORD iInnerAngle, DWORD iOuterAngle)
{
	Set3DCone( iInnerAngle, iOuterAngle, BASS_NO_CHANGE );
}

bool TChannel::Get3DEnabled()
{
	lock_guard<mutex> Lock(MutexLock);

	if (!Is3DInternal()) return false;

	DWORD iMode = BASS_3DMODE_OFF;
	BASS_ChannelGet3DAttributes(pHandle, &iMode, NULL, NULL, NULL, NULL, NULL);

	return (iMode != BASS_3DMODE_OFF);
}

void TChannel::Set3DEnabled(bool bEnabled)
{
	lock_guard<mutex> Lock(MutexLock);

	if (!Is3DInternal()) return;
	BASS_ChannelSet3DAttributes(pHandle, bEnabled ? BASS_3DMODE_NORMAL : BASS_3DMODE_OFF, BASS_NO_CHANGE, BASS_NO_CHANGE, BASS_NO_CHANGE, BASS_NO_CHANGE, BASS_NO_CHANGE);
	BASS_Apply3D();
}

float TChannel::GetEAXMix()
{
	lock_guard<mutex> Lock(MutexLock);

	float fMix = -1;
	if(!Is3DInternal()) return fMix;

	if(BASS_ChannelGetAttribute(pHandle, BASS_ATTRIB_EAXMIX, &fMix) == FALSE)
	{
		fMix = -1;
	}

	return fMix;
}

void TChannel::SetEAXMix(float fMix)
{
	lock_guard<mutex> Lock(MutexLock);
	if(!Is3DInternal()) return;

	if ((fMix < 0) && (fMix != BASS_AUTO)) fMix = 0;
	if (fMix > 1) fMix = 1;

	BASS_ChannelSetAttribute(pHandle, BASS_ATTRIB_EAXMIX, fMix);
}

bool TChannel::AddFx(TEffect* pFX)
{
	lock_guard<mutex> Lock(MutexLock);

	if (!g_CLIENT) return false;
	if (ISNULLPTR(pFX)) return false;

	list<TEffect*>::iterator iter = lsFX.begin();

	while (iter != lsFX.end())
	{
		TEffect* pFX = *iter;
		if (ISNULLPTR(pFX))
		{
			iter = lsFX.erase(iter);
			continue;
		}

		if (pFX->IsRemoved())
		{
			iter = lsFX.erase(iter);
			UNREFDELETE(pFX);
			continue;
		}

		iter++;
	}

	if (lsFX.size() >= BASS_MAX_FX) return false;

	bool suggess = pFX->Apply(pHandle);

	try
	{
		lsFX.push_back(pFX);
	}
	catch (...)
	{
		pFX->Remove();
		return false;
	}

	return true;
}

void TChannel::ClearFx()
{
	lock_guard<mutex> Lock(MutexLock);
	if (!g_CLIENT) return;

	ClearFxInternal();
}

string TChannel::ToString()
{
	return string(*this);
}

TChannel::operator string()
{
	stringstream out;

	out << (*this);

	return string(out.str());
}

TChannel::operator bass_p()
{
	return GetBassHandle();
}


bool TChannel::operator ==(TChannel& other)
{
	return this->GetBassHandle() == other.GetBassHandle();
}

// +--------------------------------------------------+
// |                    Friends                       |
// +--------------------------------------------------+
ostream& operator<<(ostream& os, TChannel& Channel)
{
	if (!Channel.IsValid())
	{
		os << "[NULL " << TChannel::LUAMETANAME << "]";
		return os;
	}

	if (Channel.IsRemoved())
	{
		os << "[EMPTY " << TChannel::LUAMETANAME << "]";
		return os;
	}

	os << TChannel::LUAMETANAME << ": " << &Channel << " ";
	os << "[file:\"" << Channel.GetFileName() << "\"]";
	os << "[online:" << (Channel.IsOnline() ? "true" : "false") << "]";
	os << "[loop:" << (Channel.HasFlag(BASS_SAMPLE_LOOP) ? "true" : "false") << "]";

	if (g_CLIENT)
	{
		os << "[3d:" << (Channel.HasFlag(BASS_SAMPLE_3D) ? "true" : "false") << "]";
	}

	return os;
}
