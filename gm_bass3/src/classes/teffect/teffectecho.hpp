#include "../teffect.hpp"

#ifndef T_EFFECT_ECHO_H
#define T_EFFECT_ECHO_H

class TEffectEcho : public TEffect
{
	GETSETLIMIT(float, fWetDryMix, WetDryMix, 50, 0, 100);
	GETSETLIMIT(float, fFeedback, Feedback, 50, 0, 100);
	GETSETLIMIT(float, fLeftDelay, LeftDelay, 500, 1, 2000);
	GETSETLIMIT(float, fRightDelay, RightDelay, 500, 1, 2000);
	GETSETBOOL(lPanDelay, PanDelay, FALSE);

protected:
	BASS_DX8_ECHO* pData = NULL;
	void* GetData();

public:
	static std::string LUAMETANAME;
	static int LUAMETAID;

	TEffectEcho();
	~TEffectEcho();

	std::string ToString();
	operator std::string();

	friend std::ostream& operator<<(std::ostream& os, TEffectEcho& FX);
};
#endif