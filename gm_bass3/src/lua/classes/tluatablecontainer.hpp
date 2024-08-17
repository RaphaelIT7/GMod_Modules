#include "../lua.hpp"

#ifndef T_LUATABLECONTAINER_H
#define T_LUATABLECONTAINER_H

namespace LUAINTERFACE
{
	class TLuaTableContainer
	{
	private:
		int iRef = 0;
		std::string sName;

		std::map<std::string, GarrysMod::Lua::CFunc> mapFunctions;
		std::map<std::string, bass_flag> mapEnums;

		std::map<std::string, std::string> mapConstants_str;
		std::map<std::string, double> mapConstants_n;

	protected:
		std::mutex MutexLock;
		void Build(ILuaBase* pLUA, std::string name);

	public:
		static std::unordered_map<std::string, TLuaTableContainer*> g_mapTablesInstances;
		static std::unordered_map<TLuaTableContainer*, std::string> g_mapTablesInstances_r;

		TLuaTableContainer();
		~TLuaTableContainer();

		void AddFunction(std::string name, GarrysMod::Lua::CFunc func);

		void AddEnum(std::string name, bass_flag iEnum);
		void AddEnum(std::string name, double iEnum);
		void AddEnum(std::string name, unsigned int iEnum);
		void AddEnum(std::string name, signed int iEnum);

		void AddConstant(std::string name, std::string value);
		void AddConstant(std::string name, double value);
		void AddConstant(std::string name, unsigned int value);
		void AddConstant(std::string name, signed int value);

		bool IsValid();

		std::string GetName() const;

		virtual void Init(ILuaBase* pLUA);
		virtual void Think(ILuaBase* pLUA);
		virtual void Dispose(ILuaBase* pLUA);
	};
}
#endif