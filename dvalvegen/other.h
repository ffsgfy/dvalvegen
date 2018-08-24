#pragma once

#include <windows.h>
#include "dvalvegen.h"

template <typename Type>
inline Type call_vfunc(void* thisPtr, int index)
{
	DWORD* ClassBase = (DWORD*)thisPtr;
	DWORD* VMT = *(DWORD**)ClassBase;
	DWORD FnAddress = VMT[index];
	return (Type)FnAddress;
}

typedef void* (*CreateInterfaceFn)(const char* pName, int* pReturnCode);

CreateInterfaceFn captureFactory(const char* dllname) {
	HMODULE mod = GetModuleHandleA(dllname);
	return (CreateInterfaceFn)GetProcAddress(mod, "CreateInterface");
}

template <class T>
T* captureInterface(const char* dllname, std::string interfacename)
{
	CreateInterfaceFn factory = captureFactory(dllname);
	if (!factory) {
		return nullptr;
	}

	char lastchar = interfacename[interfacename.size() - 1];
	if (isdigit(lastchar)) {
		return (T*)factory(interfacename.c_str(), 0);
	}

	std::string stry;
	int itry = 100;
	char cbuf[4] = "000";

	// Brute force ftw
	for (; itry >= 0; itry--) {
		std::snprintf(cbuf, 4, "%03d", itry);
		stry = interfacename + cbuf;
		void* r = factory(stry.c_str(), 0);

		if (r) {
			return (T*)r;
		}
	}

	return (T*)nullptr;
}

class IBaseClient
{
public:
	dvalvegen::ClientClass* GetAllClasses() {
		// https://www.unknowncheats.me/forum/1522872-post33.html

		int idx = -1;

		std::uint8_t** vtable = *(std::uint8_t***)this;

		for (int i = 0; i < 16; i++) {
			
			std::uint8_t* func = vtable[i];

			if (func[0] == 0xA1 && func[5] == 0xC3) {
				idx = i;
				break;
			}
		}

		if (idx == -1)
			return nullptr;

		typedef dvalvegen::ClientClass*(__thiscall*Fn)(void* thisPtr);
		return call_vfunc<Fn>(this, idx)(this);
	}
};