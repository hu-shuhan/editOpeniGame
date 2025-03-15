#ifndef iGameFilter_h
#define iGameFilter_h

#include "iGameObject.h"
#include "iGameDataObject.h"
#include "iGameProgressObserver.h"
#include "iGameModel.h"

#define NOMINMAX
IGAME_NAMESPACE_BEGIN
class Filter : public Object {
public:
	I_OBJECT(Filter);
	static Pointer New() { return new Filter; }
	using DataObjectArray = ElementArray<DataObject::Pointer>;
	
	// The function that executes the algorithm
	virtual bool Execute() { return true; }

	// Set/Get the number of input's data object.
	void SetNumberOfInputs(int n);
    int GetNumberOfInputs() const;

    // Set/Get the number of output's data object.
	void SetNumberOfOutputs(int n);
	int GetNumberOfOutputs() const;

	// Set input's data object at index i.
	bool SetInput(int i, DataObject::Pointer data);
    // Set input's data object at index 0.
	bool SetInput(DataObject::Pointer data);
    // Get input's data object at index i.
	DataObject::Pointer GetInput(int i);
	// Get input's data object according to name
	DataObject::Pointer GetNamedInput(const std::string& name);

	// Set output's data object at index i.
	bool SetOutput(int index, DataObject::Pointer data);
	bool SetOutput(DataObject::Pointer data);
	DataObject::Pointer GetOutput(int index);
	virtual DataObject::Pointer GetOutput();
	DataObject::Pointer GetNamedOutput(const std::string& name);

	// Update the current task progress.'amount'is between 0 and 1.
	void UpdateProgress(double amount);
	// Switch to the next task. Progress will be reset.
    void ResetProgress();
    void ResetProgress(double scale);
	
	void SetModel(Model::Pointer model);

protected:
	Filter();
	~Filter() override = default;

	Model::Pointer m_Model{};
	DataObjectArray::Pointer m_Inputs{};  // Input data object array
    DataObjectArray::Pointer m_Outputs{}; // Output data object array

	ProgressObserver* m_ProgressObserver{nullptr};
	double m_Progress{ 0.0 };           // The current task progress
    double m_ProgressShift{0.0};        // Previous task's progress
    double m_ProgressScale{1.0};        // The current task progress range
};
IGAME_NAMESPACE_END
#endif