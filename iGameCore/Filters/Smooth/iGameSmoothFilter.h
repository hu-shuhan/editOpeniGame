#pragma once

#include "iGameFilter.h"

#include <string>

IGAME_NAMESPACE_BEGIN

class SmoothFilter : public Filter {
public:
    I_OBJECT(SmoothFilter);
    static Pointer New() { return new SmoothFilter; }

    void SetNumberOfIterations(int value);
    int GetNumberOfIterations() const { return m_NumberOfIterations; }
    void SetRelaxationFactor(double value);
    double GetRelaxationFactor() const { return m_RelaxationFactor; }
    void SetConvergence(double value);
    double GetConvergence() const { return m_Convergence; }
    void SetPreserveBoundary(bool value);
    bool GetPreserveBoundary() const { return m_PreserveBoundary; }
    int GetNumberOfIterationsPerformed() const { return m_NumberOfIterationsPerformed; }

    bool Execute() override;
    const std::string& GetMessage() const { return m_Message; }

protected:
    SmoothFilter();
    ~SmoothFilter() override = default;

private:
    int m_NumberOfIterations{20};
    double m_RelaxationFactor{0.01};
    double m_Convergence{0.0};
    bool m_PreserveBoundary{false};
    int m_NumberOfIterationsPerformed{0};
    std::string m_Message;
};

IGAME_NAMESPACE_END
