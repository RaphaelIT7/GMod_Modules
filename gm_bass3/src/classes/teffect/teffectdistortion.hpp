#include "../teffect.hpp"

#ifndef T_EFFECT_DISTORTION_H
#define T_EFFECT_DISTORTION_H

class TEffectDistortion : public TEffect
{
	GETSETLIMIT(float, fGain, Gain, -18, -60, 0);
	GETSETLIMIT(float, fEdge, Edge, 15, 0, 100);
	GETSETLIMIT(float, fPostEQCenterFrequency, PostEQCenterFrequency, 2400, 100, 8000);
	GETSETLIMIT(float, fPostEQBandwidth, PostEQBandwidth, 2400, 100, 8000);
	GETSETLIMIT(float, fPreLowpassCutoff, PreLowpassCutoff, 8000, 100, 8000);

protected:
	BASS_DX8_DISTORTION* pData = NULL;
	void* GetData();

public:
	static std::string LUAMETANAME;
	static int LUAMETAID;

	TEffectDistortion();
	~TEffectDistortion();

	std::string ToString();
	operator std::string();

	friend std::ostream& operator<<(std::ostream& os, TEffectDistortion& FX);
};
#endif