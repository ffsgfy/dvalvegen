# dvalvegen
Generates an SDK based on netvars when injected into any Source Engine based game

Offsets are aquired dynamically at runtime, members can be accesed via inline functions which return a pointer (can easily be changed to return a reference if that's your thing, look [here](https://github.com/ffsgfy/dvalvegen/blob/master/dvalvegen/dvalvegen.h#L579))

Inspired by [ValveGen](https://github.com/CallumCVM/ValveGen)

**Example output:**
```c++
#pragma once

#include "CBaseEntity.h"
#include "dvalvegen.h"

class CEntityDissolve : public CBaseEntity {
public:
	inline float* m_flFadeOutModelLength() {
		static int offset = dvalvegen::getOffset("DT_EntityDissolve", "m_flFadeOutModelLength");
		return (float*)((int)this + offset);
	}

	inline float* m_flFadeOutModelStart() {
		static int offset = dvalvegen::getOffset("DT_EntityDissolve", "m_flFadeOutModelStart");
		return (float*)((int)this + offset);
	}

	inline float* m_flStartTime() {
		static int offset = dvalvegen::getOffset("DT_EntityDissolve", "m_flStartTime");
		return (float*)((int)this + offset);
	}

	inline float* m_flFadeOutStart() {
		static int offset = dvalvegen::getOffset("DT_EntityDissolve", "m_flFadeOutStart");
		return (float*)((int)this + offset);
	}

	inline float* m_flFadeOutLength() {
		static int offset = dvalvegen::getOffset("DT_EntityDissolve", "m_flFadeOutLength");
		return (float*)((int)this + offset);
	}

	inline float* m_flFadeInStart() {
		static int offset = dvalvegen::getOffset("DT_EntityDissolve", "m_flFadeInStart");
		return (float*)((int)this + offset);
	}

	inline Vector* m_vDissolverOrigin() {
		static int offset = dvalvegen::getOffset("DT_EntityDissolve", "m_vDissolverOrigin");
		return (Vector*)((int)this + offset);
	}

	inline float* m_flFadeInLength() {
		static int offset = dvalvegen::getOffset("DT_EntityDissolve", "m_flFadeInLength");
		return (float*)((int)this + offset);
	}

	inline __int32* m_nDissolveType() {
		static int offset = dvalvegen::getOffset("DT_EntityDissolve", "m_nDissolveType");
		return (__int32*)((int)this + offset);
	}

	inline __int32* m_nMagnitude() {
		static int offset = dvalvegen::getOffset("DT_EntityDissolve", "m_nMagnitude");
		return (__int32*)((int)this + offset);
	}
};
```
