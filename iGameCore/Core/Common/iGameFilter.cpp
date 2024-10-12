#include "iGameFilter.h"
#include <cassert>

IGAME_NAMESPACE_BEGIN
Filter::Filter()
{
    m_Inputs = DataObjectArray::New();
    m_Outputs = DataObjectArray::New();
    m_ProgressObserver = ProgressObserver::Instance();
}

int Filter::GetNumberOfInputs() const
{
    return m_Inputs->Size();
}

int Filter::GetNumberOfOutputs() const
{
    return m_Outputs->Size();
}

bool Filter::SetInput(int index, DataObject::Pointer data)
{
    if (index < 0 || index >= this->GetNumberOfInputs())
    {
        return false;
    }
    m_Inputs->SetElement(index, data);
    return true;
}

bool Filter::SetInput(DataObject::Pointer data)
{
    return this->SetInput(0, data);
}

DataObject::Pointer Filter::GetInput(int index)
{
    if (index < 0 || index >= this->GetNumberOfInputs())
    {
        return nullptr;
    }
    
    return m_Inputs->GetElement(index);
}

DataObject::Pointer Filter::GetNamedInput(const std::string& name)
{
    for (int i = 0; i < this->GetNumberOfInputs(); i++) {
        if (name == m_Inputs->GetElement(i)->GetName()) {
            return m_Inputs->GetElement(i);
        }
    }
    return nullptr;
}

bool Filter::SetOutput(int index, DataObject::Pointer data)
{
    if (index < 0 || index >= this->GetNumberOfOutputs())
    {
        return false;
    }
    m_Outputs->SetElement(index, data);
    return true;
}

bool Filter::SetOutput(DataObject::Pointer data)
{
    return this->SetOutput(0, data);
}

DataObject::Pointer Filter::GetOutput(int index)
{
    if (index < 0 || index >= this->GetNumberOfOutputs())
    {
        return nullptr;
    }
    return m_Outputs->GetElement(index);
}

DataObject::Pointer Filter::GetOutput()
{
    return this->GetOutput(0);
}

DataObject::Pointer Filter::GetNamedOutput(const std::string& name)
{
    for (int i = 0; i < this->GetNumberOfOutputs(); i++) {
        if (name == m_Outputs->GetElement(i)->GetName()) {
            return m_Outputs->GetElement(i);
        }
    }
    return nullptr;
}

void Filter::UpdateProgress(double amount)
{
    amount = m_ProgressShift + m_ProgressScale * amount;

    amount = std::min(1.0, amount);
    amount = std::max(0.0, amount);

    m_Progress = amount;
    if (m_ProgressObserver)
    { 
        m_ProgressObserver->UpdateProgress(amount);
    }
}

void Filter::ResetProgress() 
{
    m_ProgressShift = m_Progress;
    m_ProgressScale = 1 - m_Progress;
}

void Filter::ResetProgress(double scale) {
    if (scale > 1 - m_Progress) { return; }

    m_ProgressShift = m_Progress;
    m_ProgressScale = scale;
}

void Filter::SetNumberOfInputs(int n)
{
    m_Inputs->Resize(n);
}

void Filter::SetNumberOfOutputs(int n)
{
    m_Outputs->Resize(n);
}

IGAME_NAMESPACE_END