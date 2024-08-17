#include "../teffect.hpp"

#ifndef T_EFFECT_PARAMEQ_H
#define T_EFFECT_PARAMEQ_H

class TEffectParamEQ : public TEffect
{
	GETSETLIMIT(float, fBandwidth, Bandwidth, 12, 1, 36);
	GETSETLIMIT(float, fGain, Gain, 0, -15, 15);

protected:
	std::atomic<float> fCenter = { 80 };

	BASS_DX8_PARAMEQ* pData = NULL;
	void* GetData();

public:
	static std::string LUAMETANAME;
	static int LUAMETAID;

	TEffectParamEQ();
	~TEffectParamEQ();

	float GetCenter();
	void SetCenter(float fCenter);

	std::string ToString();
	operator std::string();

	friend std::ostream& operator<<(std::ostream& os, TEffectParamEQ& FX);
};
#endif