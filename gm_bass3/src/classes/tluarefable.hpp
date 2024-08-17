#include "../globals.hpp"
#include "../util.hpp"

#ifndef T_LUAREFABLE_H
#define T_LUAREFABLE_H



class TLuaRefAble
{
private:
	// +--------------------------------------------------+
	// |                Private Variables                 |
	// +--------------------------------------------------+
	unsigned int iReferences = 0;

protected:
	// +--------------------------------------------------+
	// |               Protected Variables                |
	// +--------------------------------------------------+
	std::mutex MutexLock;
	bool bIsRemoved = false;
	unsigned long long id;

	// +--------------------------------------------------+
	// |                Protected Methods                 |
	// +--------------------------------------------------+
public:
	static std::string LUAMETANAME;
	static int LUAMETAID;
	static unsigned long long g_id;

	// +--------------------------------------------------+
	// |              Constructor/Destructor              |
	// +--------------------------------------------------+
	TLuaRefAble();
	~TLuaRefAble();

	// +--------------------------------------------------+
	// |                  Public Methods                  |
	// +--------------------------------------------------+
	virtual bool IsValid();

	virtual void Remove();
	virtual const bool IsRemoved() const;

	virtual unsigned int AddReference();
	virtual unsigned int RemoveReference();
	virtual const unsigned int GetReferences() const;
	virtual const bool IsReferenced() const;

	virtual std::string ToString();
	virtual operator std::string();
	virtual bool operator ==(TLuaRefAble& other);

	// +--------------------------------------------------+
	// |                    Friends                       |
	// +--------------------------------------------------+
	friend std::ostream& operator<<(std::ostream& os, TLuaRefAble& OBJ);
};
#endif