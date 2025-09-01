#ifndef iGameConvertToPointCloud_h
#define iGameConvertToPointCloud_h

#include "iGameFilter.h"
#include "iGameVolumeMesh.h"
#include "iGameUnstructuredMesh.h"

IGAME_NAMESPACE_BEGIN
class ConvertToPointCloud : public Filter {
public:
    I_OBJECT(ConvertToPointCloud);
    static Pointer New() { return new ConvertToPointCloud; }

	bool Execute() override 
	{ 
        if (GetInput(0) == nullptr)
            return false;
        
        PointSet::Pointer NewMesh = PointSet::New();
        SetOutput(NewMesh);
        if (DynamicCast<SurfaceMesh>(GetInput(0)))
        { 
            return Convert(DynamicCast<SurfaceMesh>(GetInput(0)), NewMesh);
        } 
        else if (DynamicCast<VolumeMesh>(GetInput(0))) 
        {
            return Convert(DynamicCast<VolumeMesh>(GetInput(0)), NewMesh);
        }
        else if(DynamicCast<UnstructuredMesh>(GetInput(0))) 
        {
            return Convert(DynamicCast<UnstructuredMesh>(GetInput(0)), NewMesh);
        }

		return false;
    }

	static bool Convert(SurfaceMesh::Pointer OldMesh, PointSet::Pointer NewMesh)
	{ 
		if (OldMesh == nullptr) return false;

		auto OldAttrs = OldMesh->GetAttributeSet();
		auto NewAttrs = NewMesh->GetAttributeSet();

        auto NewPoints = NewMesh->GetPoints();
        NewPoints->Reset();
		for (int i = 0; i < OldMesh->GetNumberOfPoints(); i++) { 
			NewPoints->AddPoint(OldMesh->GetPoint(i));
		}

		for (int i = 0; i < OldAttrs->GetNumberOfAttributes(); i++) { 
			auto& attr = OldAttrs->GetAttribute(i);
			if (attr.attachmentType == IG_POINT) { 
				if (attr.GetPointer()->GetArrayType() == IG_FLOAT)
				{
                    FloatArray::Pointer NewAttr = FloatArray::New();
                    NewAttr->DeepCopy(DynamicCast<FloatArray>(attr.GetPointer()));
                    NewAttr->SetName(attr.GetPointer()->GetName());
                    NewAttrs->AddAttribute(attr.GetType(), IG_POINT, NewAttr);

                } else if (attr.GetPointer()->GetArrayType() == IG_DOUBLE) {
                    DoubleArray::Pointer NewAttr = DoubleArray::New();
                    NewAttr->DeepCopy(DynamicCast<DoubleArray>(attr.GetPointer()));
                    NewAttr->SetName(attr.GetPointer()->GetName());
                    NewAttrs->AddAttribute(attr.GetType(), IG_POINT, NewAttr);
                }
			}
		}
        return true;
	}

	static bool Convert(VolumeMesh::Pointer OldMesh, PointSet::Pointer NewMesh) {
        if (OldMesh == nullptr) return false;

        auto OldAttrs = OldMesh->GetAttributeSet();
        auto NewAttrs = NewMesh->GetAttributeSet();

        auto NewPoints = NewMesh->GetPoints();
        NewPoints->Reset();
        for (int i = 0; i < OldMesh->GetNumberOfPoints(); i++) { NewPoints->AddPoint(OldMesh->GetPoint(i)); }

        for (int i = 0; i < OldAttrs->GetNumberOfAttributes(); i++) {
            auto& attr = OldAttrs->GetAttribute(i);
            if (attr.attachmentType == IG_POINT) {
                if (attr.GetPointer()->GetArrayType() == IG_FLOAT) {
                    FloatArray::Pointer NewAttr = FloatArray::New();
                    NewAttr->DeepCopy(DynamicCast<FloatArray>(attr.GetPointer()));
                    NewAttr->SetName(attr.GetPointer()->GetName());
                    NewAttrs->AddAttribute(attr.GetType(), IG_POINT, NewAttr);

                } else if (attr.GetPointer()->GetArrayType() == IG_DOUBLE) {
                    DoubleArray::Pointer NewAttr = DoubleArray::New();
                    NewAttr->DeepCopy(DynamicCast<DoubleArray>(attr.GetPointer()));
                    NewAttr->SetName(attr.GetPointer()->GetName());
                    NewAttrs->AddAttribute(attr.GetType(), IG_POINT, NewAttr);
                }
            }
        }
        return true;
    }

    static bool Convert(UnstructuredMesh::Pointer OldMesh, PointSet::Pointer NewMesh) {
        if (OldMesh == nullptr) return false;

        auto OldAttrs = OldMesh->GetAttributeSet();
        auto NewAttrs = NewMesh->GetAttributeSet();

        auto NewPoints = NewMesh->GetPoints();
        NewPoints->Reset();
        for (int i = 0; i < OldMesh->GetNumberOfPoints(); i++) { NewPoints->AddPoint(OldMesh->GetPoint(i)); }

        for (int i = 0; i < OldAttrs->GetNumberOfAttributes(); i++) {
            auto& attr = OldAttrs->GetAttribute(i);
            if (attr.attachmentType == IG_POINT) {
                if (attr.GetPointer()->GetArrayType() == IG_FLOAT) {
                    FloatArray::Pointer NewAttr = FloatArray::New();
                    NewAttr->DeepCopy(DynamicCast<FloatArray>(attr.GetPointer()));
                    NewAttr->SetName(attr.GetPointer()->GetName());
                    NewAttrs->AddAttribute(attr.GetType(), IG_POINT, NewAttr);

                } else if (attr.GetPointer()->GetArrayType() == IG_DOUBLE) {
                    DoubleArray::Pointer NewAttr = DoubleArray::New();
                    NewAttr->DeepCopy(DynamicCast<DoubleArray>(attr.GetPointer()));
                    NewAttr->SetName(attr.GetPointer()->GetName());
                    NewAttrs->AddAttribute(attr.GetType(), IG_POINT, NewAttr);
                }
            }
        }
        return true;
    }

protected:
    ConvertToPointCloud()
	{
		SetNumberOfInputs(1);
		SetNumberOfOutputs(1);
	}
    ~ConvertToPointCloud() override = default;
};
IGAME_NAMESPACE_END
#endif