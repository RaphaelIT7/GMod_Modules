#include "../lua.hpp"
#include "../../classes/tchannel.hpp"
#include "../../classes/teffect/teffectecho.hpp"

#ifndef T_LUAOBJECTCONTAINER_H
#define T_LUAOBJECTCONTAINER_H

namespace LUAINTERFACE
{
	class TLuaObjectContainer
	{
	private:
		std::string sName;
		int iTypeID = 0;

		std::map<std::string, GarrysMod::Lua::CFunc> mapMethods;
		std::map<std::string, GarrysMod::Lua::CFunc> mapMetaMethods;

	protected:
		std::mutex MutexLock;
		int Build(ILuaBase* pLUA, std::string name);

		template<class T> void AddBaseMethods(ILuaBase* pLUA)
		{
			AddMethod("__gc", [](lua_State* state) -> int {
				ILuaBase* pLUA = LUAINTERFACE::GetLuaBase(state);
				if (ISNULLPTR(pLUA)) return 0;

				LUAINTERFACE::HELPER::Check<T>(pLUA, 1);
				T* pOBJ = LUAINTERFACE::HELPER::Get<T>(pLUA, 1);

				LUAINTERFACE::HELPER::Delete<T>(pOBJ);
				return 0;
			});

			AddMethod("__eg", [](lua_State* state) -> int {
				ILuaBase* pLUA = LUAINTERFACE::GetLuaBase(state);
				if (ISNULLPTR(pLUA)) return 0;

				LUAINTERFACE::HELPER::Check<T>(pLUA, 1);
				LUAINTERFACE::HELPER::Check<T>(pLUA, 2);

				T* pOBJA = LUAINTERFACE::HELPER::Get<T>(pLUA, 1);
				T* pOBJB = LUAINTERFACE::HELPER::Get<T>(pLUA, 2);

				if (pOBJA == pOBJB)
				{
					pLUA->PushBool(true);
					return 1;
				}

				pLUA->PushBool((*pOBJA) == (*pOBJB));
				return 1;
			});

			AddMethod("__tostring", [](lua_State* state) -> int {
				ILuaBase* pLUA = LUAINTERFACE::GetLuaBase(state);
				if (ISNULLPTR(pLUA)) return 0;

				LUAINTERFACE::HELPER::Check<T>(pLUA, 1);

				T* pOBJ = LUAINTERFACE::HELPER::Get<T>(pLUA, 1);
				std::string sOut = "";

				if (ISNULLPTR(pOBJ))
				{
					sOut = std::string("[NULL") + T::LUAMETANAME + std::string("]");
					pLUA->PushString(sOut.c_str());
					return 1;
				}

				sOut = pOBJ->ToString();
				pLUA->PushString(sOut.c_str());
				return 1;
			});

			AddMethod("IsValid", [](lua_State* state) -> int {
				ILuaBase* pLUA = LUAINTERFACE::GetLuaBase(state);
				if (ISNULLPTR(pLUA)) return 0;

				LUAINTERFACE::HELPER::Check<T>(pLUA, 1);
				T* pOBJ = LUAINTERFACE::HELPER::Get<T>(pLUA, 1);

				if (ISNULLPTR(pOBJ))
				{
					pLUA->PushBool(false);
					return 1;
				}

				pLUA->PushBool(pOBJ->IsValid());
				return 1;
			});

			AddMethod("Remove", [](lua_State* state) -> int {
				ILuaBase* pLUA = LUAINTERFACE::GetLuaBase(state);
				if (ISNULLPTR(pLUA)) return 0;

				LUAINTERFACE::HELPER::Check<T>(pLUA, 1);

				T* pOBJ = LUAINTERFACE::HELPER::Get<T>(pLUA, 1);
				if (ISNULLPTR(pOBJ)) return 0;

				pOBJ->Remove();
				return 0;
			});
		}

	public:
		static std::unordered_map<std::string, TLuaObjectContainer*> g_mapObjectInstances;

		TLuaObjectContainer();
		~TLuaObjectContainer();

		void AddMethod(std::string name, GarrysMod::Lua::CFunc func);

		virtual void Init(ILuaBase* pLUA);
		virtual void Think(ILuaBase* pLUA);
		virtual void Dispose(ILuaBase* pLUA);
	};
}
#endif