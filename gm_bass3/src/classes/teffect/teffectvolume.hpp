#include "../teffect.hpp"

#ifndef T_EFFECT_VOLUME_H
#define T_EFFECT_VOLUME_H

class TEffectVolume : public TEffect, public TEffectTrait_Curve
{
	GETSETLIMIT(float, fTarget, Target, 1, 0, 1000);
	GETSETLIMIT_WITH_NOCHANGE(float, fCurrent, Current, 1, 0, 1000);
	GETSETLIMIT(float, fTime, Time, 0, 0, 3600);

protected:
	BASS_FX_VOLUME_PARAM* pData = NULL;
	void* GetData();

public:
	static std::string LUAMETANAME;
	static int LUAMETAID;

	TEffectVolume();
	~TEffectVolume();

	std::string ToString();
	operator std::string();

	friend std::ostream& operator<<(std::ostream& os, TEffectVolume& FX);
};
#endif