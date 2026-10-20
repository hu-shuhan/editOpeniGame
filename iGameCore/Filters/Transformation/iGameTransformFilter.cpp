#include "iGameTransformFilter.h"

#include <cmath>

IGAME_NAMESPACE_BEGIN

TransformFilter::TransformFilter(){
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);

    // 默认使用单位矩阵
    for (int i = 0; i < 4; ++i){
        for (int j = 0; j < 4; ++j){
            m_Matrix[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
}


void TransformFilter::SetTranslation(float tx,float ty,float tz){
    m_TranslateX = tx;
    m_TranslateY = ty;
    m_TranslateZ = tz;
}

void TransformFilter::SetRotation(float rx,float ry,float rz){
    m_RotationX = rx;
    m_RotationY = ry;
    m_RotationZ = rz;
}


void TransformFilter::SetScale(float sx,float sy,float sz){
    m_ScaleX = sx;
    m_ScaleY = sy;
    m_ScaleZ = sz;
}

void TransformFilter::SetPivot(float px,float py,float pz){
    m_PivotX = px;
    m_PivotY = py;
    m_PivotZ = pz;
}

void TransformFilter::BuildMatrix()
{
    constexpr float PI = 3.14159265358979323846f;

    float rx = m_RotationX * PI / 180.0f;
    float ry = m_RotationY * PI / 180.0f;
    float rz = m_RotationZ * PI / 180.0f;
    float cx = std::cos(rx);
    float sx = std::sin(rx);
    float cy = std::cos(ry);
    float sy = std::sin(ry);
    float cz = std::cos(rz);
    float sz = std::sin(rz);

    float S[4][4] =
    {
        {m_ScaleX, 0.0f,     0.0f,     0.0f},
        {0.0f,     m_ScaleY, 0.0f,     0.0f},
        {0.0f,     0.0f,     m_ScaleZ, 0.0f},
        {0.0f,     0.0f,     0.0f,     1.0f}
    };

    float Rx[4][4] =
    {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, cx,   -sx,  0.0f},
        {0.0f, sx,    cx,  0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    };

    float Ry[4][4] =
    {
        {cy,   0.0f, sy,   0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {-sy,  0.0f, cy,   0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    };

    float Rz[4][4] =
    {
        {cz,   -sz,  0.0f, 0.0f},
        {sz,    cz,  0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    };

    float T[4][4] =
    {
        {1.0f, 0.0f, 0.0f, m_TranslateX},
        {0.0f, 1.0f, 0.0f, m_TranslateY},
        {0.0f, 0.0f, 1.0f, m_TranslateZ},
        {0.0f, 0.0f, 0.0f, 1.0f}
    };

    auto MultiplyMatrix =[](const float A[4][4],const float B[4][4],float C[4][4])
    {
        for (int i = 0; i < 4; ++i){
            for (int j = 0; j < 4; ++j){
                C[i][j] = 0.0f;
            }
        }

        for (int i = 0; i < 4; ++i){
            for (int j = 0; j < 4; ++j){
                for (int k = 0; k < 4; ++k){
                    C[i][j] += A[i][k] * B[k][j];
                }
            }
        }
    };

    float RxS[4][4]{};
    MultiplyMatrix(Rx, S, RxS);
    float RyRxS[4][4]{};
    MultiplyMatrix(Ry, RxS, RyRxS);
    float RzRyRxS[4][4]{};
    MultiplyMatrix(Rz, RyRxS, RzRyRxS);

    // 轴心 (m_PivotX, m_PivotY, m_PivotZ)：旋转与缩放都绕该点进行
    //   M = T * Piv(o) * R * S * Piv(-o)  等价于   p' = t + o + R*S*(p - o)
    // 轴心取 (0,0,0) 时退化为 M = T * R * S，即原有行为
    float PivotOut[4][4] =
    {
        {1.0f, 0.0f, 0.0f, m_PivotX},
        {0.0f, 1.0f, 0.0f, m_PivotY},
        {0.0f, 0.0f, 1.0f, m_PivotZ},
        {0.0f, 0.0f, 0.0f, 1.0f}
    };

    float PivotIn[4][4] =
    {
        {1.0f, 0.0f, 0.0f, -m_PivotX},
        {0.0f, 1.0f, 0.0f, -m_PivotY},
        {0.0f, 0.0f, 1.0f, -m_PivotZ},
        {0.0f, 0.0f, 0.0f, 1.0f}
    };

    float RS_PivotIn[4][4]{};
    MultiplyMatrix(RzRyRxS, PivotIn, RS_PivotIn);
    float PivotOut_RS_PivotIn[4][4]{};
    MultiplyMatrix(PivotOut, RS_PivotIn, PivotOut_RS_PivotIn);
    MultiplyMatrix(T, PivotOut_RS_PivotIn, m_Matrix);
}


bool TransformFilter::Execute(){

    auto dataObject = this->GetInput(0);
    if (dataObject == nullptr){return false;}
    auto pointSet = DynamicCast<PointSet>(dataObject);
    if (pointSet == nullptr){return false;}

    BuildMatrix();

    PointSet::Pointer output = nullptr;

    switch(dataObject->GetDataObjectType()){
        case IG_SURFACE_MESH:{
            auto input = DynamicCast<SurfaceMesh>(dataObject);
            if (input == nullptr){return false;}

            auto surfaceOutput = SurfaceMesh::New();
            if (!surfaceOutput->DeepCopy(input)){
                return false;
            }

            output = surfaceOutput;
            break;
        }
        case IG_VOLUME_MESH:{
            auto input = DynamicCast<VolumeMesh>(dataObject);
            if (input == nullptr){return false;}

            auto volumeOutput = VolumeMesh::New();

            volumeOutput->SetVolumes(input->GetVolumes());
            volumeOutput->SetName(input->GetName());
            auto newPoints = Points::New();
            if (!newPoints->DeepCopy(input->GetPoints())){
                return false;
            }
            volumeOutput->SetPoints(newPoints);

            auto newAttributeSet = AttributeSet::New();
            if (!newAttributeSet->DeepCopy(input->GetAttributeSet())){
                 return false;
            }
            volumeOutput->SetAttributeSet(newAttributeSet);

            output = volumeOutput;
            break;
        }
        case IG_STRUCTURED_MESH:{
            auto input = DynamicCast<StructuredMesh>(dataObject);
            if (input == nullptr){return false;}

            auto structuredOutput = StructuredMesh::New();

            structuredOutput->SetDimensionSize(input->GetDimensionSize());
            structuredOutput->SetExtent(input->GetExtent());
            structuredOutput->SetName(input->GetName());
            auto newPoints = Points::New();
            if (!newPoints->DeepCopy(input->GetPoints())){
                return false;
            }
            structuredOutput->SetPoints(newPoints);

            auto newAttributeSet = AttributeSet::New();
            if (!newAttributeSet->DeepCopy(input->GetAttributeSet())){
                return false;
            }
            structuredOutput->SetAttributeSet(newAttributeSet);

            output = structuredOutput;
            break;
        }
        case IG_UNSTRUCTURED_MESH:{
            auto input = DynamicCast<UnstructuredMesh>(dataObject);
            if (input == nullptr){return false;}

            auto unstructuredOutput = UnstructuredMesh::New();

            unstructuredOutput->SetCells(input->GetCells(),input->GetCellTypes());
            unstructuredOutput->SetName(input->GetName());
            auto newPoints = Points::New();
            if (!newPoints->DeepCopy(input->GetPoints())){
                return false;
            }
            unstructuredOutput->SetPoints(newPoints);

            auto newAttributeSet = AttributeSet::New();
            if (!newAttributeSet->DeepCopy(input->GetAttributeSet())){
                return false;
            }
            unstructuredOutput->SetAttributeSet(newAttributeSet);

            output = unstructuredOutput;
            break;
        }
        case IG_POINT_SET:{
            auto input = DynamicCast<PointSet>(dataObject);
            if (input == nullptr){return false;}

            auto pointSetOutput = PointSet::New();

            pointSetOutput->SetName(input->GetName());
            auto newPoints = Points::New();
            if (!newPoints->DeepCopy(input->GetPoints())){
                return false;
            }
            pointSetOutput->SetPoints(newPoints);

            auto newAttributeSet = AttributeSet::New();
            if (!newAttributeSet->DeepCopy(input->GetAttributeSet())){
                return false;
            }
            pointSetOutput->SetAttributeSet(newAttributeSet);

            output = pointSetOutput;
            break;
        }

        default:
            return false;
    }

    if (output == nullptr){
        return false;
    }

    for (IGsize i = 0; i < output->GetNumberOfPoints(); ++i){
        Point p = output->GetPoint(i);

        float x = p[0];
        float y = p[1];
        float z = p[2];
        float newX =
            m_Matrix[0][0] * x +
            m_Matrix[0][1] * y +
            m_Matrix[0][2] * z +
            m_Matrix[0][3];
        float newY =
            m_Matrix[1][0] * x +
            m_Matrix[1][1] * y +
            m_Matrix[1][2] * z +
            m_Matrix[1][3];
        float newZ =
            m_Matrix[2][0] * x +
            m_Matrix[2][1] * y +
            m_Matrix[2][2] * z +
            m_Matrix[2][3];
        float newW =
            m_Matrix[3][0] * x +
            m_Matrix[3][1] * y +
            m_Matrix[3][2] * z +
            m_Matrix[3][3];

        if (std::abs(newW) > 1e-6f){
            newX /= newW;
            newY /= newW;
            newZ /= newW;
        }

        Point newPoint(newX, newY, newZ);
        output->SetPoint(i, newPoint);
    }

    // 处理vector和normal属性
    auto attributeSet = output->GetAttributeSet();
    if (attributeSet != nullptr){

        const float L00 = m_Matrix[0][0];
        const float L01 = m_Matrix[0][1];
        const float L02 = m_Matrix[0][2];
        const float L10 = m_Matrix[1][0];
        const float L11 = m_Matrix[1][1];
        const float L12 = m_Matrix[1][2];
        const float L20 = m_Matrix[2][0];
        const float L21 = m_Matrix[2][1];
        const float L22 = m_Matrix[2][2];

        const float det =L00 * (L11 * L22 - L12 * L21)- L01 * (L10 * L22 - L12 * L20)+ L02 * (L10 * L21 - L11 * L20);
        const bool invertible = std::abs(det) > 1e-8f;
        float InvT[3][3]{};

        if (invertible){

            // L^-1
            float inv[3][3];

            inv[0][0] =  (L11 * L22 - L12 * L21) / det;
            inv[0][1] = -(L01 * L22 - L02 * L21) / det;
            inv[0][2] =  (L01 * L12 - L02 * L11) / det;

            inv[1][0] = -(L10 * L22 - L12 * L20) / det;
            inv[1][1] =  (L00 * L22 - L02 * L20) / det;
            inv[1][2] = -(L00 * L12 - L02 * L10) / det;

            inv[2][0] =  (L10 * L21 - L11 * L20) / det;
            inv[2][1] = -(L00 * L21 - L01 * L20) / det;
            inv[2][2] =  (L00 * L11 - L01 * L10) / det;

            // (L^-1)^T
            for (int i = 0; i < 3; ++i){
                for (int j = 0; j < 3; ++j){
                    InvT[i][j] = inv[j][i];
                }
            }
        }

        // 遍历所有属性
        for (IGsize i = 0;i < attributeSet->GetNumberOfAttributes();++i){

            auto& attribute = attributeSet->GetAttribute(i);
            if (attribute.IsDeleted()){continue;}
            auto array = attribute.GetPointer();
            if (array == nullptr){continue;}

            const IGenum type = attribute.GetType();
            if (type == IG_VECTOR){

                if (array->GetDimension() < 3){continue;}

                for (IGsize j = 0;j < array->GetNumberOfElements();++j){

                    std::vector<double> value(array->GetDimension());
                    array->GetElement(j, value);
                    if (value.size() < 3){continue;}

                    const double x = value[0];
                    const double y = value[1];
                    const double z = value[2];
                    value[0] =L00 * x +L01 * y +L02 * z;
                    value[1] =L10 * x +L11 * y +L12 * z;
                    value[2] =L20 * x +L21 * y +L22 * z;

                    array->SetElement(j, value.data());
                }
                attribute.UpdateAllDataRange();
            }else if (type == IG_NORMAL){

                if (!invertible){continue;}
                if (array->GetDimension() < 3){continue;}

                for (IGsize j = 0;j < array->GetNumberOfElements();++j){

                    std::vector<double> value(array->GetDimension());
                    array->GetElement(j, value);

                    if (value.size() < 3){continue;}

                    const double x = value[0];
                    const double y = value[1];
                    const double z = value[2];
                    double nx =InvT[0][0] * x +InvT[0][1] * y +InvT[0][2] * z;
                    double ny =InvT[1][0] * x +InvT[1][1] * y +InvT[1][2] * z;
                    double nz =InvT[2][0] * x +InvT[2][1] * y +InvT[2][2] * z;

                    // Normal归一化
                    double length =std::sqrt(nx * nx +ny * ny +nz * nz);
                    if (length > 1e-12){nx /= length;ny /= length;nz /= length;}
                    value[0] = nx;
                    value[1] = ny;
                    value[2] = nz;

                    array->SetElement(j, value.data());
                }
                attribute.UpdateAllDataRange();
            }
        }
    }

    this->SetOutput(0, output);
    return true;
}


IGAME_NAMESPACE_END
