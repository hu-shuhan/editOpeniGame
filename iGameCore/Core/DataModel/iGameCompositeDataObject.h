#ifndef iGameCompositeDataSet_h
#define iGameCompositeDataSet_h

#include "iGameDataObject.h"

IGAME_NAMESPACE_BEGIN
class CompositeDataObject : public DataObject {
public:
	I_OBJECT(CompositeDataObject)
		static Pointer New() { return new CompositeDataObject; }

	/**
	 * Restore data object to initial state,
	 */
	virtual void Initialize() {}

	IGenum GetDataObjectType() const override { return IG_COMPOSITE_DATA_OBJECT; }

	virtual DataObject* GetChild(int index) { return nullptr; }
	virtual DataObject* GetNextChild(int index) { return nullptr; }
	virtual DataObject* GetPrevChild(int index) { return nullptr; }

	virtual void SetChild(unsigned int index, DataObject::Pointer) {}

	virtual void RemoveChild(unsigned int index) {}

	virtual void AddChildToBack(DataObject::Pointer) {}

	virtual void SetNumberOfChildren(unsigned int num) {}
	virtual unsigned int GetNumberOfChildren() { return 0; }
	IGsize GetRealMemorySize() override { return 0; }
protected:
	CompositeDataObject();
	~CompositeDataObject() override;
};
IGAME_NAMESPACE_END
#endif