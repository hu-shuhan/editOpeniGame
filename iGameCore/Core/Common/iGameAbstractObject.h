#ifndef iGameAbstractObject_h
#define iGameAbstractObject_h

#include "iGameSmartPointer.h"
#include "iGameMacro.h"
#include "iGameType.h"

IGAME_NAMESPACE_BEGIN
class AbstractObject {
public:
	I_OBJECT(AbstractObject);

	// Delete this object to free memory
    virtual void Delete() const
	{
		m_ReferenceCount = 0;
		delete this;
	}

	// Add one to the reference count
	virtual void Register() const
	{
		++m_ReferenceCount;
	}

	// The reference count is decremented by one.If the
	// reference count reaches 0, the object is deleted
	virtual void UnRegister() const noexcept
	{
		if (--m_ReferenceCount <= 0)
		{
			delete this;
		}
	}

protected:
	AbstractObject() {}
	virtual ~AbstractObject() {};

	// Reference count: One keeps track of how many times
	// an object has been referenced so that memory is
	// automatically freed when it is no longer needed.
	mutable std::atomic<int> m_ReferenceCount{};
};
IGAME_NAMESPACE_END
#endif