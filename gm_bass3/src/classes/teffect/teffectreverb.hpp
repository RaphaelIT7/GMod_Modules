#include "../teffect.hpp"

#ifndef T_EFFECT_REVERB_H
#define T_EFFECT_REVERB_H

class TEffectReverb : public TEffect
{
	GETSETLIMIT(float, fInGain, InGain, 0, -96, 0);
	GETSETLIMIT(float, fReverbMix, ReverbMix, 0, -96, 0);
	GETSETLIMIT(float, fReverbTime, ReverbTime, 1000, 0.001, 3000);
	GETSETLIMIT(float, fHighFreqRTRatio, HighFreqRTRatio, 0.001, 0.001, 0.999);

protected:
	BASS_DX8_REVERB* pData = NULL;
	void* GetData();

public:
	static std::string LUAMETANAME;
	static int LUAMETAID;

	TEffectReverb();
	~TEffectReverb();

	std::string ToString();
	operator std::string();

	friend std::ostream& operator<<(std::ostream& os, TEffectReverb& FX);
};
#endif