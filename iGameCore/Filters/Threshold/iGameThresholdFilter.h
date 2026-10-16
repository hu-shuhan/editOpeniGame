/**
 * @class   ThresholdFilter
 * @brief   Keep cells whose scalar values fall inside a configured range.
 *
 * The filter accepts point-associated or cell-associated scalar data and
 * produces an unstructured mesh containing the selected cells. Point data is
 * evaluated with AllScalars by default; this can be changed to AnyScalars.
 */
#ifndef iGameThresholdFilter_h
#define iGameThresholdFilter_h

#include "iGameFilter.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN

class ThresholdFilter : public Filter {
public:
	I_OBJECT(ThresholdFilter);
	static Pointer New() { return new ThresholdFilter; }
	~ThresholdFilter() override = default;

	enum class Association {
		Point,
		Cell
	};

	enum class PointEvaluation {
		AllScalars,
		AnyScalar
	};

	enum class BoundaryMode {
		Closed,
		Open,
		LowerInclusive,
		UpperInclusive
	};

	/**
	 * @brief 设置参与阈值判定的标量数组。
	 * @param array       标量数组(点关联或单元关联)。
	 * @param association 标量的关联对象(点或单元)。
	 * @param dimension   分量下标:>= 0 取对应分量,-1 取模长(各分量平方和开方)。
	 */
	void SetScalarData(ArrayObject::Pointer array, Association association = Association::Point,
					   int dimension = 0);
	ArrayObject::Pointer GetScalarData() const { return m_ScalarData; }
	Association GetAssociation() const { return m_Association; }

	void SetLowerThreshold(double value) { m_LowerThreshold = value; }
	void SetUpperThreshold(double value) { m_UpperThreshold = value; }
	double GetLowerThreshold() const { return m_LowerThreshold; }
	double GetUpperThreshold() const { return m_UpperThreshold; }

	void SetThreshold(double lower, double upper) {
		m_LowerThreshold = lower;
		m_UpperThreshold = upper;
	}

	void SetBoundaryMode(BoundaryMode mode) { m_BoundaryMode = mode; }
	BoundaryMode GetBoundaryMode() const { return m_BoundaryMode; }
	void SetPointEvaluation(PointEvaluation evaluation) { m_PointEvaluation = evaluation; }
	PointEvaluation GetPointEvaluation() const { return m_PointEvaluation; }

	bool Execute() override;
	UnstructuredMesh::Pointer GetThresholdMesh() {
		return DynamicCast<UnstructuredMesh>(GetOutput());
	}

protected:
	ThresholdFilter();

private:
	bool IsInRange(double value) const;
	bool BuildOutput(UnstructuredMesh::Pointer input);

	ArrayObject::Pointer m_ScalarData{nullptr};
	Association m_Association{Association::Point};
	PointEvaluation m_PointEvaluation{PointEvaluation::AllScalars};
	BoundaryMode m_BoundaryMode{BoundaryMode::Closed};
	int m_Dimension{0};
	double m_LowerThreshold{0.0};
	double m_UpperThreshold{1.0};
};

IGAME_NAMESPACE_END
#endif
