#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <sstream>
#include <filesystem>

namespace fs = std::experimental::filesystem;

namespace dvalvegen {
	using uint = unsigned int;

	class Class;
	class ClassProp;
	class RecvTable;
	class RecvProp;
	class CRecvProxyData;
	class IClientNetworkable;
	enum SendPropType;

	class DVariant
	{
	public:

		union
		{
			float	m_Float;
			int		m_Int;
			const char	*m_pString;
			void	*m_pData;
			float	m_Vector[3];
			__int64	m_Int64;
		};
		SendPropType	m_Type;
	};

	enum SendPropType
	{
		DPT_Int = 0,
		DPT_Float,
		DPT_Vector,
		DPT_VectorXY, // Only encodes the XY of a vector, ignores Z
		DPT_String,
		DPT_Array,	// An array of the base types (can't be of datatables).
		DPT_DataTable,
		DPT_Int64,
		DPT_NUMSendPropTypes
	};

	typedef void(*RecvVarProxyFn)(const CRecvProxyData *pData, void *pStruct, void *pOut);
	typedef void(*DataTableRecvVarProxyFn)(const RecvProp *pProp, void **pOut, void *pData, int objectID);
	typedef void(*ArrayLengthRecvProxyFn)(void *pStruct, int objectID, int currentArrayLength);

	class RecvProp
	{

	public:

		char* m_pVarName;
		SendPropType			m_RecvType;
		int						m_Flags;
		int						m_StringBufferSize;


	private:

		bool					m_bInsideArray;		// Set to true by the engine if this property sits inside an array.

													// Extra data that certain special property types bind to the property here.
		const void *m_pExtraData;

		// If this is an array (DPT_Array).
		RecvProp				*m_pArrayProp;
		ArrayLengthRecvProxyFn	m_ArrayLengthProxy;

		RecvVarProxyFn			m_ProxyFn;
		DataTableRecvVarProxyFn	m_DataTableProxyFn;	// For RDT_DataTable.

		RecvTable				*m_pDataTable;		// For RDT_DataTable.
		int						m_Offset;

		int						m_ElementStride;
		int						m_nElements;

		// If it's one of the numbered "000", "001", etc properties in an array, then
		// these can be used to get its array property name for debugging.
		const char				*m_pParentArrayPropName;

		// This info comes from the receive data table.
	public:

		int GetNumElements() const
		{
			return m_nElements;
		}

		int GetElementStride() const
		{
			return m_ElementStride;
		}

		int GetFlags() const
		{
			return m_Flags;
		}

		std::string GetName() const
		{
			if (m_pVarName)
			{
				return m_pVarName;
			}
			else
			{
				return "Unknown";
			}
		}

		char* szGetName() const
		{
			if (m_pVarName)
			{
				return m_pVarName;
			}
			else
			{
				return (char*)"Unknown";
			}
		}

		SendPropType GetType() const
		{
			return m_RecvType;
		}

		RecvTable* GetDataTable() const
		{
			return m_pDataTable;
		}

		RecvVarProxyFn GetProxyFn() const
		{
			return m_ProxyFn;
		}

		void SetProxyFn(RecvVarProxyFn Proxy)
		{
			m_ProxyFn = Proxy;
		}

		DataTableRecvVarProxyFn GetDataTableProxyFn() const
		{
			return m_DataTableProxyFn;
		}

		int GetOffset() const
		{
			return m_Offset;
		}

		// Arrays only.
		RecvProp* GetArrayProp() const
		{
			return m_pArrayProp;
		}

		// Arrays only.
		ArrayLengthRecvProxyFn GetArrayLengthProxy() const
		{
			return m_ArrayLengthProxy;
		}

		bool IsInsideArray() const
		{
			return m_bInsideArray;
		}

		// Some property types bind more data to the prop in here.
		const void* GetExtraData() const
		{
			return m_pExtraData;
		}

		// If it's one of the numbered "000", "001", etc properties in an array, then
		// these can be used to get its array property name for debugging.
		const char* GetParentArrayPropName()
		{
			return m_pParentArrayPropName;
		}
	};

	class RecvTable
	{

	public:

		// Properties described in a table.
		RecvProp * m_pProps;
		int			m_nProps;

		// The decoder. NOTE: this covers each RecvTable AND all its children (ie: its children
		// will have their own decoders that include props for all their children).
		void	*m_pDecoder;

		char* m_pNetTableName;	// The name matched between client and server.


	private:

		bool			m_bInitialized;
		bool			m_bInMainList;

	public:

		int			GetNumProps()
		{
			return m_nProps;
		}

		RecvProp* GetProp(int i)
		{
			return &m_pProps[i];
		}

		std::string GetName()
		{
			if (m_pNetTableName)
			{
				return m_pNetTableName;
			}
			else
			{
				return "Unknown";
			}
		}

		bool IsInitialized() const
		{
			return m_bInitialized;
		}

		bool IsInMainList() const
		{
			return m_bInMainList;
		}
	};

	class CRecvProxyData
	{
	public:
		const RecvProp	*m_pRecvProp;		// The property it's receiving.

		DVariant		m_Value;			// The value given to you to store.

		int				m_iElement;			// Which array element you're getting.

		int				m_ObjectID;			// The object being referred to.
	};

	class IClientNetworkable;

	typedef IClientNetworkable*	(*CreateClientClassFn)(int entnum, int serialNum);
	typedef IClientNetworkable*	(*CreateEventFn)();

	class ClientClass
	{
	public:
		CreateClientClassFn			m_pCreateFn;
		CreateEventFn			m_pCreateEventFn;	// Only called for event objects.
		char*			m_pNetworkName;
		RecvTable*			m_pRecvTable;
		ClientClass*	m_pNext;
		int				m_ClassID;	// Managed by the engine.
	};

	std::unordered_map<std::string, Class> g_Classes;

	class FCharBuffer {
		/// String buffer that expands but never shrinks
	private:
		void relocate(uint newsize) {
			if (newsize <= m_size) {
				return;
			}

			char* newmem = (char*)std::malloc(sizeof(char) * (newsize + 1));

			if (m_data) {
				std::memcpy(newmem, m_data, sizeof(char) * (m_size + 1));
				std::free(m_data);
			}

			m_data = newmem;
			m_size = newsize;
		}

	public:
		FCharBuffer(uint size = 0) {
			m_size = size;
			m_data = (char*)std::calloc(size + 1, sizeof(char));
		}

		~FCharBuffer() {
			if (m_data) {
				std::free(m_data);
			}
		}

		void append(const char* string) {
			uint stringsize = std::strlen(string);
			if (m_null + stringsize > m_size) {
				relocate(m_size + stringsize);
			}

			std::memcpy(&m_data[m_null], string, stringsize + 1);
			m_null += stringsize;
		}

		void append(const std::string string) {
			append(string.c_str());
		}

		void pop(uint n = 1) {
			if (n > m_size) {
				n = m_size;
			}

			m_size -= n;
			m_null = m_size;
			m_data[m_null] = 0;
		}

		const char* str() {
			return m_data;
		}

	private:
		uint m_size = 0;
		uint m_null = 0;
		char* m_data = nullptr;
	};

	class Indenter {
	public:
		Indenter(std::string indent) {
			m_indent = indent;
			m_indentsize = indent.size();
		}

		const char* get(uint n) {
			while (m_curindents > n) {
				m_sindents.pop(m_indentsize);
				m_curindents--;
			}

			while (m_curindents < n) {
				m_sindents.append(m_indent);
				m_curindents++;
			}

			return m_sindents.str();
		}

	private:
		std::string m_indent;
		uint m_indentsize;
		FCharBuffer m_sindents;
		uint m_curindents = 0;
	};


	class ClassProp {
	public:
		ClassProp(RecvProp* prop, uint addoffset = 0) {
			m_prop = prop;
			m_type = prop->GetType();
			m_addoffset = addoffset;
		}

		RecvProp* prop() {
			return m_prop;
		}

		std::string getFormattedName() {
			if (m_fname == "") {
				m_fname = m_prop->GetName();

				while (true) {
					int p = m_fname.find("[");
					if (p != std::string::npos) {
						m_fname[p] = '_';
					}
					else {
						break;
					}
				}

				while (true) {
					int p = m_fname.find("]");
					if (p != std::string::npos) {
						m_fname.erase(m_fname.begin() + p);
					}
					else {
						break;
					}
				}

				while (true) {
					int p = m_fname.find(".");
					if (p != std::string::npos) {
						m_fname[p] = '_';
					}
					else {
						break;
					}
				}

				while (true) {
					int p = m_fname.find("\"");
					if (p != std::string::npos) {
						m_fname.erase(m_fname.begin() + p);
					}
					else {
						break;
					}
				}

				if (m_fname[0] >= '0' && m_fname[0] <= '9') {
					m_fname = "_" + m_fname;
				}
			}

			return m_fname;
		}

		void print(std::ostream& stream, int indents, Class& parent, std::unordered_set<std::string>& dependencies);

	private:
		std::string m_fname;
		SendPropType m_type;
		uint m_addoffset;
		RecvProp* m_prop;
	};

	class Class {
		// ikr
	public:
		Class(RecvTable* table) {
			m_table = table;
		}

		Class() {

		}

		void addBaseclass(std::string baseclass) {
			for (auto& it : m_baseclasses) {
				if (it == baseclass) {
					return;
				}
			}

			m_baseclasses.push_back(baseclass);
		}

		void addProp(RecvProp* prop, uint addoffset = 0) {
			ClassProp p{ prop, addoffset };
			m_props.try_emplace(p.getFormattedName(), p);
		}

		std::string getBaseclass(int i) {
			return m_baseclasses[i];
		}

		std::vector<std::string>& baseclasses() {
			return m_baseclasses;
		}

		std::unordered_map<std::string, ClassProp>& props() {
			return m_props;
		}

		std::string getFormattedName() {
			if (m_fname == "") {
				m_fname = m_table->GetName();
				if (m_fname.find("DT_") == 0) {
					m_fname.replace(m_fname.begin(), m_fname.begin() + 3, "C");
				}
			}

			return m_fname;
		}

		std::string getName() {
			return m_table->GetName();
		}

		void print(std::ostream& stream, int indents, std::unordered_set<std::string>& dependencies) {
			static Indenter ind{ "\t" };

			stream << ind.get(indents) << "class " << getFormattedName();

			for (uint i = 0; i < m_baseclasses.size(); i++) {
				dependencies.emplace(g_Classes[m_baseclasses[i]].getFormattedName());

				if (i == 0) {
					stream << " : public " << g_Classes[m_baseclasses[i]].getFormattedName();
				}
				else {
					stream << ", public " << g_Classes[m_baseclasses[i]].getFormattedName();
				}
			}

			if (m_props.size() == 0) {
				stream << " {};" << std::endl;
			}
			else {
				dependencies.emplace("dvalvegen");

				stream << " {" << std::endl << ind.get(indents) << "public:" << std::endl;

				int propsdone = 0;
				for (auto p : m_props) {
					p.second.print(stream, indents + 1, *this, dependencies);
					
					if (propsdone != m_props.size() - 1) {
						stream << std::endl;
					}

					propsdone++;
				}

				stream << ind.get(indents) << "};" << std::endl;;
			}
		}

	private:
		std::string m_fname = "";
		std::vector<std::string> m_baseclasses;
		std::unordered_map<std::string, ClassProp> m_props;
		RecvTable* m_table = nullptr;
	};

	std::string type2str(RecvProp* p) {
		std::string r;

		switch (p->GetType()) {
		case DPT_Int:
			r = "__int32";
			break;
		case DPT_Float:
			r = "float";
			break;
		case DPT_Int64:
			r = "__int64";
			break;
		case DPT_Vector:
			r = "Vector";
			break;
		case DPT_VectorXY:
			r = "Vector2D";
			break;
		case DPT_String:
			r = "const char";
			break;
		case DPT_DataTable:
			r = g_Classes[p->GetDataTable()->GetName()].getFormattedName();
			break;
		default:
			r = "";
			break;
		}

		return r;
	}

	void ClassProp::print(std::ostream& stream, int indents, Class& parent, std::unordered_set<std::string>& dependencies) {
		static Indenter ind{ "\t" };

		if (m_type == DPT_DataTable) {
			if (g_Classes.count(m_prop->GetDataTable()->GetName()) == 0) {
				// If there is no class for this table, it must be an array
				// Ok so if it's an array of other arrays this is going to break, but let's hope Valve never does this				
				stream << ind.get(indents) << "inline " << type2str(m_prop->GetDataTable()->GetProp(0)) << "* " << getFormattedName() << "() {" << std::endl;
				stream << ind.get(indents + 1) << "static int offset = dvalvegen::getOffset(\"" << parent.getName() << "\", \"" << getFormattedName() << "\");" << std::endl;
				stream << ind.get(indents + 1) << "return (" << type2str(m_prop->GetDataTable()->GetProp(0)) << "*)((int)this + offset);" << std::endl;
				stream << ind.get(indents) << "}" << std::endl;
				stream << std::endl;
				stream << ind.get(indents) << "inline int " << getFormattedName() << "_Size() {" << std::endl;
				stream << ind.get(indents + 1) << "static int ret = dvalvegen::getDTArraySize(\"" << parent.getName() << "\", \"" << getFormattedName() << "\");" << std::endl;
				stream << ind.get(indents + 1) << "return ret;" << std::endl;
				stream << ind.get(indents) << "}" << std::endl;
				return;
			}
			else {
				dependencies.emplace(g_Classes[m_prop->GetDataTable()->GetName()].getFormattedName());
			}
		}

		if (m_prop->GetName() == "\"m_flLastDamageTime\"") {
			std::string();
		}

		stream << ind.get(indents) << "inline " << type2str(m_prop) << "* " << getFormattedName() << "() {" << std::endl;
		stream << ind.get(indents + 1) << "static int offset = dvalvegen::getOffset(\"" << parent.getName() << "\", \"" << getFormattedName() << "\");" << std::endl;
		stream << ind.get(indents + 1) << "return (" << type2str(m_prop) << "*)((int)this + offset);" << std::endl;
		stream << ind.get(indents) << "}" << std::endl;
	}

	void createClass(RecvTable* table, Class* parent = nullptr) {
		std::string tablename = table->GetName();

		if (table->GetNumProps() > 0) {
			if (table->GetProp(0)->GetName()[0] >= '0' && table->GetProp(0)->GetName()[0] <= '9') {
				// This must be an array
				if (table->GetProp(0)->GetType() == DPT_DataTable) {
					// It's an array of classes, so first we need to create them
					createClass(table->GetProp(0)->GetDataTable());
				}
				return;
			}
		}

		// Create the class if it doesn't exist yet
		if (g_Classes.count(table->GetName()) == 0) {
			g_Classes.try_emplace(tablename, Class{ table });
		}
		else {
			// Nothing new here...
			return;
		}

		Class* ctx = &g_Classes[tablename];

		// Now iterate over its props
		for (int i = 0; i < table->GetNumProps(); i++) {
			RecvProp* prop = table->GetProp(i);

			if (prop->GetName() == "baseclass") {
				// If it's a baseclass, record it
				ctx->addBaseclass(prop->GetDataTable()->GetName());
				createClass(prop->GetDataTable());
			}
			else {
				SendPropType propt = prop->GetType();

				if (propt == DPT_Array || propt == DPT_NUMSendPropTypes) {
					// Nobody uses these anyway
					continue;
				}

				if (propt == DPT_DataTable) {
					// It's another class...
					if (prop->GetDataTable()->GetNumProps() > 0) {
						// We don't want empty classes here
						if (prop->GetName() == prop->GetDataTable()->GetName()) {
							// If prop name == class name, it must be an array, so just add it
							if (prop->GetDataTable()->GetProp(0)->GetType() != DPT_Array && prop->GetDataTable()->GetProp(0)->GetType() != DPT_NUMSendPropTypes) {
								// Say 'no' to arrays of Arrays
								ctx->addProp(prop);
							}
							continue;
						}
						else {
							// It's an actual other class, create it
							createClass(prop->GetDataTable());
						}
					}
				}

				// Now there are some numbered (e.g. 000, 001 etc.) props which are not in an array like one handled above
				// Since no one will ever use one of these I'm just gonna add them as props
				// Even if they have the same offset and are literally the same thing, fu I don't care

				ctx->addProp(prop);
			}
		}
	}

	void createClasses(void* clientclass) {
		ClientClass* cclass = (ClientClass*)clientclass;
		while (cclass) {
			if (cclass->m_pRecvTable) {
				createClass(cclass->m_pRecvTable);
			}

			cclass = cclass->m_pNext;
		}
	}

	void printClasses(std::string dirpath) {
		if (dirpath[dirpath.size() - 1] == '/' || dirpath[dirpath.size() - 1] == '\\') {
			dirpath.erase(dirpath.begin() + dirpath.size() - 1);
		}

		dirpath += "/dvalvegen/";

		fs::create_directories(dirpath);

		auto write_dvalvegen = [&dirpath]() {
			std::ofstream oh{ dirpath + "dvalvegen.h" };
			oh <<
				"#pragma once\n"
				"\n"
				"#include <string>\n"
				"#include <vector>\n"
				"#include <unordered_map>\n"
				"#include \"Vector.h\"\n"
				"\n"
				"namespace dvalvegen {\n"
				"\tusing uint = unsigned int;\n"
				"\n"
				"\tclass Class;\n"
				"\tclass ClassProp;\n"
				"\tclass RecvTable;\n"
				"\tclass RecvProp;\n"
				"\tenum SendPropType;\n"
				"\n"
				"\tenum SendPropType {\n"
				"\t\tDPT_Int = 0,\n"
				"\t\tDPT_Float,\n"
				"\t\tDPT_Vector,\n"
				"\t\tDPT_VectorXY,\n"
				"\t\tDPT_String,\n"
				"\t\tDPT_Array,\n"
				"\t\tDPT_DataTable,\n"
				"\t\tDPT_Int64,\n"
				"\t\tDPT_NUMSendPropTypes\n"
				"\t};\n"
				"\n"
				"\tclass RecvProp {\n"
				"\tpublic:\n"
				"\t\tchar* m_pVarName;\n"
				"\t\tSendPropType m_RecvType;\n"
				"\t\tint m_Flags;\n"
				"\t\tint m_StringBufferSize;\n"
				"\n"
				"\tprivate:\n"
				"\t\tbool m_bInsideArray;\n"
				"\t\tconst void *m_pExtraData;\n"
				"\t\tRecvProp *m_pArrayProp;\n"
				"\t\tvoid* m_ArrayLengthProxy;\n"
				"\t\tvoid* m_ProxyFn;\n"
				"\t\tvoid* m_DataTableProxyFn;\n"
				"\t\tRecvTable *m_pDataTable;\n"
				"\t\tint m_Offset;\n"
				"\t\tint m_ElementStride;\n"
				"\t\tint m_nElements;\n"
				"\t\tconst char *m_pParentArrayPropName;\n"
				"\n"
				"\tpublic:\n"
				"\t\tint GetNumElements() const {\n"
				"\t\t\treturn m_nElements;\n"
				"\t\t}\n"
				"\n"
				"\t\tstd::string GetName() const {\n"
				"\t\t\tif (m_pVarName) {\n"
				"\t\t\t\treturn m_pVarName;\n"
				"\t\t\t}\n"
				"\t\t\telse {\n"
				"\t\t\t\treturn \"Unknown\";\n"
				"\t\t\t}\n"
				"\t\t}\n"
				"\n"
				"\t\tSendPropType GetType() const {\n"
				"\t\t\treturn m_RecvType;\n"
				"\t\t}\n"
				"\n"
				"\t\tRecvTable* GetDataTable() const {\n"
				"\t\t\treturn m_pDataTable;\n"
				"\t\t}\n"
				"\n"
				"\t\tint GetOffset() const {\n"
				"\t\t\treturn m_Offset;\n"
				"\t\t}\n"
				"\t};\n"
				"\n"
				"\tclass RecvTable\n"
				"\t{\n"
				"\n"
				"\tpublic:\n"
				"\t\tRecvProp* m_pProps;\n"
				"\t\tint m_nProps;\n"
				"\t\tvoid *m_pDecoder;\n"
				"\t\tchar* m_pNetTableName;\n"
				"\n"
				"\tprivate:\n"
				"\t\tbool m_bInitialized;\n"
				"\t\tbool m_bInMainList;\n"
				"\n"
				"\tpublic:\n"
				"\t\tint GetNumProps() {\n"
				"\t\t\treturn m_nProps;\n"
				"\t\t}\n"
				"\n"
				"\t\tRecvProp* GetProp(int i)  {\n"
				"\t\t\treturn &m_pProps[i];\n"
				"\t\t}\n"
				"\n"
				"\t\tstd::string GetName() {\n"
				"\t\t\tif (m_pNetTableName) {\n"
				"\t\t\t\treturn m_pNetTableName;\n"
				"\t\t\t}\n"
				"\t\t\telse {\n"
				"\t\t\t\treturn \"Unknown\";\n"
				"\t\t\t}\n"
				"\t\t}\n"
				"\t};\n"
				"\n"
				"\tclass ClientClass\n"
				"\t{\n"
				"\tpublic:\n"
				"\t\tvoid* m_pCreateFn;\n"
				"\t\tvoid* m_pCreateEventFn;\n"
				"\t\tchar* m_pNetworkName;\n"
				"\t\tRecvTable* m_pRecvTable;\n"
				"\t\tClientClass* m_pNext;\n"
				"\t\tint m_ClassID;\n"
				"\t};\n"
				"\n"
				"\textern std::unordered_map<std::string, Class> g_Classes;\n"
				"\n"
				"\tclass ClassProp {\n"
				"\tpublic:\n"
				"\t\tClassProp(RecvProp* prop, uint addoffset = 0) {\n"
				"\t\t\tm_prop = prop;\n"
				"\t\t\tm_type = prop->GetType();\n"
				"\t\t\tm_addoffset = addoffset;\n"
				"\t\t}\n"
				"\n"
				"\t\tRecvProp* prop() {\n"
				"\t\t\treturn m_prop;\n"
				"\t\t}\n"
				"\n"
				"\tprivate:\n"
				"\t\tSendPropType m_type;\n"
				"\t\tuint m_addoffset;\n"
				"\t\tRecvProp* m_prop;\n"
				"\t};\n"
				"\n"
				"\tclass Class {\n"
				"\tpublic:\n"
				"\t\tClass(RecvTable* table) {\n"
				"\t\t\tm_table = table;\n"
				"\t\t}\n"
				"\n"
				"\t\tClass() {\n"
				"\n"
				"\t\t}\n"
				"\n"
				"\t\tvoid addBaseclass(std::string baseclass) {\n"
				"\t\t\tfor (auto& it : m_baseclasses) {\n"
				"\t\t\t\tif (it == baseclass) {\n"
				"\t\t\t\t\treturn;\n"
				"\t\t\t\t}\n"
				"\t\t\t}\n"
				"\n"
				"\t\t\tm_baseclasses.push_back(baseclass);\n"
				"\t\t}\n"
				"\n"
				"\t\tvoid addProp(RecvProp* prop, uint addoffset = 0) {\n"
				"\t\t\tm_props.emplace(prop->GetName(), ClassProp{ prop, addoffset });\n"
				"\t\t}\n"
				"\n"
				"\t\tstd::unordered_map<std::string, ClassProp>& props() {\n"
				"\t\t\treturn m_props;\n"
				"\t\t}\n"
				"\n"
				"\t\tstd::string getName() {\n"
				"\t\t\treturn m_table->GetName();\n"
				"\t\t}\n"
				"\n"
				"\tprivate:\n"
				"\t\tstd::vector<std::string> m_baseclasses;\n"
				"\t\tstd::unordered_map<std::string, ClassProp> m_props;\n"
				"\t\tRecvTable* m_table = nullptr;\n"
				"\t};\n"
				"\n"
				"\tvoid createClass(RecvTable* table, Class* parent = nullptr);\n"
				"\tint getOffset(std::string base, std::string prop);\n"
				"\tint getDTArraySize(std::string base, std::string prop);\n"
				"\tvoid createClasses(void* clientclass);\n"
				"}";
			oh.close();

			std::ofstream ocpp{ dirpath + "dvalvegen.cpp" };
			ocpp <<
				"#include \"dvalvegen.h\"\n"
				"\n"
				"namespace dvalvegen {\n"
				"\tstd::unordered_map<std::string, Class> g_Classes;\n"
				"\n"
				"\tvoid createClass(RecvTable* table, Class* parent) {\n"
				"\t\tstd::string tablename = table->GetName();\n"
				"\n"
				"\t\tif (table->GetNumProps() > 0) {\n"
				"\t\t\tif (table->GetProp(0)->GetName()[0] >= '0' && table->GetProp(0)->GetName()[0] <= '9') {\n"
				"\t\t\t\tif (table->GetProp(0)->GetType() == DPT_DataTable) {\n"
				"\t\t\t\t\tcreateClass(table->GetProp(0)->GetDataTable());\n"
				"\t\t\t\t}\n"
				"\t\t\t\treturn;\n"
				"\t\t\t}\n"
				"\t\t}\n"
				"\n"
				"\t\tif (g_Classes.count(table->GetName()) == 0) {\n"
				"\t\t\tg_Classes.try_emplace(tablename, Class{ table });\n"
				"\t\t}\n"
				"\t\telse {\n"
				"\t\t\treturn;\n"
				"\t\t}\n"
				"\n"
				"\t\tClass* ctx = &g_Classes[tablename];\n"
				"\n"
				"\t\tfor (int i = 0; i < table->GetNumProps(); i++) {\n"
				"\t\t\tRecvProp* prop = table->GetProp(i);\n"
				"\n"
				"\t\t\tif (prop->GetName() == \"baseclass\") {\n"
				"\t\t\t\tctx->addBaseclass(prop->GetDataTable()->GetName());\n"
				"\t\t\t\tcreateClass(prop->GetDataTable());\n"
				"\t\t\t}\n"
				"\t\t\telse {\n"
				"\t\t\t\tSendPropType propt = prop->GetType();\n"
				"\n"
				"\t\t\t\tif (propt == DPT_Array || propt == DPT_NUMSendPropTypes) {\n"
				"\t\t\t\t\tcontinue;\n"
				"\t\t\t\t}\n"
				"\n"
				"\t\t\t\tif (propt == DPT_DataTable) {\n"
				"\t\t\t\t\tif (prop->GetDataTable()->GetNumProps() > 0) {\n"
				"\t\t\t\t\t\tif (prop->GetName() == prop->GetDataTable()->GetName()) {\n"
				"\t\t\t\t\t\t\tif (prop->GetDataTable()->GetProp(0)->GetType() != DPT_Array && prop->GetDataTable()->GetProp(0)->GetType() != DPT_NUMSendPropTypes) {\n"
				"\t\t\t\t\t\t\t\tctx->addProp(prop);\n"
				"\t\t\t\t\t\t\t}\n"
				"\t\t\t\t\t\t\tcontinue;\n"
				"\t\t\t\t\t\t}\n"
				"\t\t\t\t\t\telse {\n"
				"\t\t\t\t\t\t\tcreateClass(prop->GetDataTable());\n"
				"\t\t\t\t\t\t}\n"
				"\t\t\t\t\t}\n"
				"\t\t\t\t}\n"
				"\n"
				"\t\t\t\tctx->addProp(prop);\n"
				"\t\t\t}\n"
				"\t\t}\n"
				"\t}\n"
				"\n"
				"\tint getOffset(std::string base, std::string prop) {\n"
				"\t\treturn g_Classes.at(base).props().at(prop).prop()->GetOffset();\n"
				"\t}\n"
				"\n"
				"\tint getDTArraySize(std::string base, std::string prop) {\n"
				"\t\treturn g_Classes.at(base).props().at(prop).prop()->GetDataTable()->GetNumProps();\n"
				"\t}\n"
				"\n"
				"\tvoid createClasses(void* clientclass) {\n"
				"\t\tClientClass* cclass = (ClientClass*)clientclass;\n"
				"\t\twhile (cclass) {\n"
				"\t\t\tif (cclass->m_pRecvTable) {\n"
				"\t\t\t\tcreateClass(cclass->m_pRecvTable);\n"
				"\t\t\t}\n"
				"\n"
				"\t\t\tcclass = cclass->m_pNext;\n"
				"\t\t}\n"
				"\t}\n"
				"}\n";
			ocpp.close();
		};

		write_dvalvegen();

		for (auto c : dvalvegen::g_Classes) {
			std::unordered_set<std::string> dps;
			std::ofstream of{ dirpath + c.second.getFormattedName() + ".h" };

			of << "#pragma once" << std::endl << std::endl;

			std::stringstream ss;
			c.second.print(ss, 0, dps);

			for (auto& dp : dps) {
				of << "#include \"" << dp << ".h\"" << std::endl;
			}

			of << std::endl << ss.str();
			of.close();
		}
	}
}