#include <windows.h>
#include <sstream>
#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>

#include "other.h"

using uint = unsigned __int32;

HANDLE g_MainThread = 0;
HMODULE g_Module = 0;

template<typename T>
void dumpTable(dvalvegen::RecvTable* table, std::basic_ostream<T>& ostream, int level) {
	static dvalvegen::Indenter ind{ " -> " };

	ostream << ind.get(level) << table->GetName() << std::endl;

	level++;
	for (int i = 0; i < table->GetNumProps(); i++) {
		dvalvegen::RecvProp* prop = table->GetProp(i);
		dvalvegen::SendPropType propt = prop->GetType();

		if (propt == dvalvegen::DPT_DataTable) {
			ostream << ind.get(level) << prop->GetName() << " [ 0x" << std::hex << prop->GetOffset() << " ] !" << std::endl;
			dumpTable(prop->GetDataTable(), ostream, level);
		}
		else {
			std::string spropt;

			switch (propt) {
			case dvalvegen::DPT_Int:
				spropt = "Int"; break;
			case dvalvegen::DPT_Float:
				spropt = "Float"; break;
			case dvalvegen::DPT_Vector:
				spropt = "Vector"; break;
			case dvalvegen::DPT_VectorXY:
				spropt = "VectorXY"; break;
			case dvalvegen::DPT_String:
				spropt = "String"; break;
			case dvalvegen::DPT_Int64:
				spropt = "Int64"; break;
			case dvalvegen::DPT_NUMSendPropTypes:
				spropt = "NUMSendPropTypes"; break;
			case dvalvegen::DPT_Array:
				spropt = "Array"; break;
			default: 
				spropt = "UnknownType"; break;
			}

			ostream << ind.get(level) << prop->GetName() << " [ 0x" << std::hex << prop->GetOffset() << " ] " << spropt << std::endl;
		}
	}
}

template<typename T>
void dumpTables(dvalvegen::ClientClass* cclass, std::basic_ostream<T>& ostream) {
	for (; cclass; cclass = cclass->m_pNext) {
		dumpTable(cclass->m_pRecvTable, ostream, 0);
		ostream << std::endl;
	}
}

template<typename T>
void dumpClassIds(dvalvegen::ClientClass* cclass, std::basic_ostream<T>& ostream) {
	static dvalvegen::Indenter ind{ "\t" };

	ostream << "enum ClassId {" << std::endl;

	for (; cclass; cclass = cclass->m_pNext) {
		std::string cname = cclass->m_pNetworkName;
		int cid = cclass->m_ClassID;
		
		ostream << ind.get(1) << cname << " = " << cid;

		if (cclass->m_pNext) {
			ostream << ",";
		}

		ostream << std::endl;
	}

	ostream << "};";
}

DWORD WINAPI Main(LPVOID thParam) {
	HMODULE hclient = nullptr;
	while (!hclient) {
		hclient = hclient ? hclient : GetModuleHandleA("client.dll");
		hclient = hclient ? hclient : GetModuleHandleA("client_panorama.dll");

		if (!hclient) {
			Sleep(100);
		}
	}

	IBaseClient* baseclient = captureInterface<IBaseClient>(hclient, "VClient");

	if (baseclient) {
		dvalvegen::ClientClass* cclass = baseclient->GetAllClasses();
		// std::ofstream ofnvs{ "NetVars.txt" };
		// dumpTables(cclass, ofnvs);
		// ofnvs.close();

		// std::ofstream ofcids{ "ClassIds.h" };
		// dumpClassIds(cclass, ofcids);
		// ofcids.close();

		dvalvegen::createClasses(baseclient->GetAllClasses());
		dvalvegen::printClasses(".");
	}

	FreeLibraryAndExitThread(g_Module, 0);

	return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		g_Module = hModule;
		g_MainThread = CreateThread(NULL, NULL, &Main, NULL, 0, NULL);
		break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

