#include "Convert/iGameConvertToSurfaceMeshFilter.h"
#include "iGameSurfaceMeshMetricsFilter.h"

IGAME_NAMESPACE_BEGIN

SurfaceMeshMetricsFilter::SurfaceMeshMetricsFilter() {
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
    m_Faces = nullptr;
    m_Points = nullptr;
}

SurfaceMeshMetricsFilter::~SurfaceMeshMetricsFilter() {
    m_Faces = nullptr;
    m_Points = nullptr;
}


bool SurfaceMeshMetricsFilter::Execute() {
    if (m_Inputs->GetNumberOfElements() == 0) { return false; }

    auto input = m_Inputs->GetElement(0);
    if (!input) { return false; }
    m_Faces = nullptr;
    switch (input->GetDataObjectType()) {
        case IG_SURFACE_MESH:
            m_Faces = (DynamicCast<SurfaceMesh>(input))->GetFaces();
            m_Points = (DynamicCast<SurfaceMesh>(input))->GetPoints();
            break;
        case IG_UNSTRUCTURED_MESH: {
            auto mesh = DynamicCast<UnstructuredMesh>(input);
            auto converter = ConvertToSurfaceMeshFilter::New();
            converter->SetInput(mesh);
            bool result = converter->Execute();
            if (result) {
                m_Faces = converter->GetSurfaceMesh()->GetFaces();
                m_Points = converter->GetSurfaceMesh()->GetPoints();
            }
            break;
        }
        default:
            igDebug("请输入表面网格进行质量检测");
            break;
    }
    if (!m_Faces) {
        igDebug("没有表面单元");
        return false;
    }
    igIndex vhs[IGAME_CELL_MAX_SIZE] = {0}; //存储每个面的顶点索引数组
    igIndex vNum = 0;                       //每个面的顶点数量

    igIndex faceNum = m_Faces->GetNumberOfCells(); //总面数

    DoubleArray::Pointer metricArray = DoubleArray::New();
    metricArray->SetName("Metric" + m_Metric);

    metricArray->SetDimension(1);
    metricArray->Reserve(faceNum);
    DoubleArray::Pointer dataRange = DoubleArray::New();
    dataRange->AddValue(0.0);
    dataRange->AddValue(1.0);
    dataRange->AddValue(0.0);
    dataRange->AddValue(1.0);
    for (igIndex i = 0; i < faceNum; i++) {
        vNum = m_Faces->GetCellIds(i, vhs); //获取第i个面的顶点索引
        double metric = this->ComputeMetric(vNum, vhs);
        metricArray->AddValue(metric);
    }

    auto output = input;

    output->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, metricArray);
    //output->GetAttributeSet()->AddAttribute(IG_SCALAR,IG_CELL,metricArray,dataRange);

    this->SetOutput(output);

    return true;
}


double SurfaceMeshMetricsFilter::ComputeMetric(igIndex vNum, igIndex* vhs) {
    std::vector<Point> points;
    for (igIndex i = 0; i < vNum; i++) { points.push_back(m_Points->GetPoint(vhs[i])); }
    //Point p[100];
    //for (igIndex i = 0; i < vNum; i++) {
    //	p[i]=m_Points->GetPoint(vhs[0]);
    //}
    //double len= (p[1]-p[0]).norm();
    //return ComputeA(vNum, vhs);

    // 根据顶点数量选择计算方法
    if (vNum == 3) {
        // 三角形
        switch (m_Metric) {
            case FACE_AREA:
                return ComputeTriangleArea(points);
                break;
            case MAX_ANGLE:
                return ComputeTriangleMaxAngle(points);
                break;
            case MIN_ANGLE:
                return ComputeTriangleMinAngle(points);
                break;
            case JACOBIAN:
                return ComputeTriangleJacobian(points);
                break;
            case ASPECT_RATIO:
                return ComputeTriangleAspectRatio(points);
                break;
            case EDGE_RATIO:
                return ComputeTriangleEdgeRatio(points);
                break;
            /*case AUTO_QUALITY:
                return ComputeTriangleMeshQuality(points);
                break;*/
            default:
                break;
        }
    } else if (vNum == 4) {
        // 四边形
        switch (m_Metric) {
            case FACE_AREA:
                return ComputeQuadArea(points);
                break;
            case MAX_ANGLE:
                return ComputeQuadMaxAngle(points);
                break;
            case MIN_ANGLE:
                return ComputeQuadMinAngle(points);
                break;
            case JACOBIAN:
                return ComputeQuadJacobian(points);
                break;
            case ASPECT_RATIO:
                return ComputeQuadAspectRatio(points);
                break;
            case EDGE_RATIO:
                return ComputeQuadEdgeRatio(points);
                break;
            case WARPAGE:
                return ComputeQuadWarpage(points);
                break;
            case TAPER:
                return ComputeQuadTaper(points);
                break;
            case SKEW:
                return ComputeQuadSkew(points);
                break;
            //case AUTO_QUALITY:
            //    return ComputeQuadSkew(points); // 默认用歪斜度
            //    break;
            default:
                break;
        }
    } else {
    }

    //switch (m_Metric)
    //{
    //   case TRIANGLE_AREA:
    //       return ComputeTriangleArea(points);
    //	break;
    //   case MAX_ANGLE:
    //       return ComputeMaxAngle(points);
    //	break;
    //default:
    //	break;
    //}
    return 0.0;
}
//计算顶点内角
double SurfaceMeshMetricsFilter::GetInternalAnglesOfVertex(Point v0, Point v1, Point v2) {

    double cosa = (v1 - v0).norm() * (v2 - v0).norm();
    double angle = acos(cosa) * 180.0 / PI;

    return angle;
}


// 三角形_面积
double SurfaceMeshMetricsFilter::ComputeTriangleArea(const std::vector<Point>& points) {
    Point edge1 = points[1] - points[0];
    Point edge2 = points[2] - points[0];
    Point cross = edge1.cross(edge2);
    return cross.norm() / 2.0;
}


double SurfaceMeshMetricsFilter::ComputeTriangleMaxAngle(const std::vector<Point>& points) {

    double angle1 = GetInternalAnglesOfVertex(points[0], points[1], points[2]);
    double angle2 = GetInternalAnglesOfVertex(points[1], points[0], points[2]);
    double angle3 = GetInternalAnglesOfVertex(points[2], points[1], points[0]);

    return std::max({angle1, angle2, angle3});
}

double SurfaceMeshMetricsFilter::ComputeTriangleMinAngle(const std::vector<Point>& points) {
    double angle1 = GetInternalAnglesOfVertex(points[0], points[1], points[2]);
    double angle2 = GetInternalAnglesOfVertex(points[1], points[0], points[2]);
    double angle3 = GetInternalAnglesOfVertex(points[2], points[1], points[0]);

    return std::min({angle1, angle2, angle3});
}

// 雅可比, range : [0, MAX], acceptable range [0, MAX], unit cube : 1
double SurfaceMeshMetricsFilter::ComputeTriangleJacobian(const std::vector<Point>& points) {
    //be caution about order
    Point edge0 = points[1] - points[0];
    Point edge1 = points[2] - points[0];
    Point edge2 = points[2] - points[1];

    Point first = edge1 - edge0;
    Point second = edge2 - edge0;

    Point cross = first.cross(second);
    double jacobian = cross.norm();

    double max_edge_length_product;
    max_edge_length_product =
            std::max({edge0.norm() * edge1.norm(), edge1.norm() * edge2.norm(), edge0.norm() * edge2.norm()});


    jacobian *= (2.0 / sqrt(3.0));
    jacobian /= max_edge_length_product;

    return jacobian;
}

// 纵横比 , range : [1, MAX], acceptable range [1, 1.3], unit cube : 1
double SurfaceMeshMetricsFilter::ComputeTriangleAspectRatio(const std::vector<Point>& points) {
    double a = (points[1] - points[0]).norm();
    double b = (points[2] - points[1]).norm();
    double c = (points[0] - points[2]).norm();
    double area = ComputeTriangleArea(points);

    return (std::max({a, b, c})) * (a + b + c) / (4.0 * sqrt(3.0) * area);
}

//长宽比
double SurfaceMeshMetricsFilter::ComputeTriangleEdgeRatio(const std::vector<Point>& points) {
    double a = (points[1] - points[0]).norm();
    double b = (points[2] - points[1]).norm();
    double c = (points[0] - points[2]).norm();

    return std::max({a, b, c}) / std::min({a, b, c});
}

// 网格质量, range : [0, 1], acceptable range [0.8, 1], unit cube : 1
double SurfaceMeshMetricsFilter::ComputeTriangleMeshQuality(const std::vector<Point>& points) {
    double area = ComputeTriangleArea(points);
    double a = (points[1] - points[0]).norm();
    double b = (points[2] - points[1]).norm();
    double c = (points[0] - points[2]).norm();

    double in_r = 2.0 * area / (a + b + c);
    double h = std::max({a, b, c});

    //double Q = 2 * sqrt(3) *area/ (in_r * h); //这个最大是3根号3啊，论文里面写错了
    double Q = 2.0 * sqrt(3.0) * area / ((a + b + c) * 0.5 * h);
    //double Q = 4 * sqrt(3) * area / (a * a + b * b + c * c);

    return Q;
}


// ==================== 四边形计算方法 ====================

// 面积 range : [0, MAX], acceptable range [0, MAX], unit cube : 1
double SurfaceMeshMetricsFilter::ComputeQuadArea(const std::vector<Point>& points) {
    double area1 = (points[1] - points[0]).cross(points[2] - points[0]).norm() / 2.0;
    double area2 = (points[2] - points[0]).cross(points[3] - points[0]).norm() / 2.0;
    double area = area1 + area2;
    return area;
}

// 最大角, range : [0, π], acceptable range [π/2, π3/4], unit cube : π/3
double SurfaceMeshMetricsFilter::ComputeQuadMaxAngle(const std::vector<Point>& points) {
    std::vector<double> angles;
    angles.push_back(GetInternalAnglesOfVertex(points[1], points[0], points[2]));
    angles.push_back(GetInternalAnglesOfVertex(points[2], points[1], points[3]));
    angles.push_back(GetInternalAnglesOfVertex(points[3], points[2], points[0]));
    angles.push_back(GetInternalAnglesOfVertex(points[0], points[3], points[1]));

    return *std::max_element(angles.begin(), angles.end());
}

// 最小角, range : [0, π], acceptable range [π/4, π/2], unit cube : π/3
double SurfaceMeshMetricsFilter::ComputeQuadMinAngle(const std::vector<Point>& points) {

    std::vector<double> angles;
    angles.push_back(GetInternalAnglesOfVertex(points[1], points[0], points[2]));
    angles.push_back(GetInternalAnglesOfVertex(points[2], points[1], points[3]));
    angles.push_back(GetInternalAnglesOfVertex(points[3], points[2], points[0]));
    angles.push_back(GetInternalAnglesOfVertex(points[0], points[3], points[1]));

    return *std::min_element(angles.begin(), angles.end());
}

// 雅可比, range : [0, MAX], acceptable range [0, MAX], unit cube : 1
double SurfaceMeshMetricsFilter::ComputeQuadJacobian(const std::vector<Point>& points) {
    Point x_1 = (points[1] - points[0]) + (points[2] - points[3]);
    Point x_2 = (points[2] - points[1]) + (points[3] - points[0]);
    Point n_c = x_1.cross(x_2).normalized();
    std::vector<double> alpha;
    alpha.emplace_back(n_c * (points[0] - points[3]).cross(points[1] - points[0]).normalized());
    alpha.emplace_back(n_c * (points[1] - points[0]).cross(points[2] - points[1]).normalized());
    alpha.emplace_back(n_c * (points[2] - points[1]).cross(points[3] - points[2]).normalized());
    alpha.emplace_back(n_c * (points[3] - points[2]).cross(points[0] - points[3]).normalized());

    return *std::min_element(alpha.begin(), alpha.end());
}

// 纵横比 , range : [1, MAX], acceptable range [1, 1.3], unit cube : 1
double SurfaceMeshMetricsFilter::ComputeQuadAspectRatio(const std::vector<Point>& points) {
    std::vector<double> v;
    double area = ComputeQuadArea(points);

    v.push_back((points[1] - points[0]).norm());
    v.push_back((points[2] - points[1]).norm());
    v.push_back((points[3] - points[2]).norm());
    v.push_back((points[0] - points[3]).norm());

    return (*std::max_element(v.begin(), v.end())) * (v[0] + v[1] + v[2] + v[3]) / (4.0 * area);
}

// 长宽比 , range : [1, MAX], acceptable range [1, 1.3], unit cube : 1
double SurfaceMeshMetricsFilter::ComputeQuadEdgeRatio(const std::vector<Point>& points) {
    std::vector<double> v;
    v.push_back((points[1] - points[0]).norm());
    v.push_back((points[2] - points[1]).norm());
    v.push_back((points[3] - points[2]).norm());
    v.push_back((points[0] - points[3]).norm());

    return *std::max_element(v.begin(), v.end()) / *std::min_element(v.begin(), v.end());
}

// 翘曲 , range : [0, 1], acceptable range [0, 0.7], unit cube : 0
double SurfaceMeshMetricsFilter::ComputeQuadWarpage(const std::vector<Point>& points) {

    Point n0 = ((points[0] - points[3]).cross(points[1] - points[0])).normalized();
    Point n1 = ((points[1] - points[0]).cross(points[2] - points[1])).normalized();
    Point n2 = ((points[2] - points[1]).cross(points[3] - points[2])).normalized();
    Point n3 = ((points[3] - points[2]).cross(points[0] - points[3])).normalized();

    double m1 = (n0 * n2) * (n0 * n2) * (n0 * n2);
    double m2 = (n1 * n3) * (n1 * n3) * (n1 * n3);

    return 1.0 - std::min(m1, m2);
}

// 锥度, range : [0, MAX], acceptable range [0, 0.7], unit cube : 0
double SurfaceMeshMetricsFilter::ComputeQuadTaper(const std::vector<Point>& points) {
    Point x_1 = (points[1] - points[0]) + (points[2] - points[3]);
    Point x_2 = (points[2] - points[1]) + (points[3] - points[0]);
    Point x_12 = (points[0] - points[1]) + (points[2] - points[3]);

    return x_12.norm() / std::min(x_1.norm(), x_2.norm());
}

// 歪斜度 , range : [0, 1], acceptable range [0.5, 1], unit cube : 1
double SurfaceMeshMetricsFilter::ComputeQuadSkew(const std::vector<Point>& points) {
    Point x_1 = (points[1] - points[0]) + (points[2] - points[3]);
    Point x_2 = (points[2] - points[1]) + (points[3] - points[0]);
    Point x_1_norm = x_1 / x_1.norm();
    Point x_2_norm = x_2 / x_2.norm();

    return std::abs(x_1_norm * x_2_norm);
}


/*
* 表面网格面片最小角质量计算
* 基于MeshMath.h中的get_min_angle_quality_SurfaceMesh
*/
double SurfaceMeshMetricsFilter::ComputeFaceMinAngleQuality(const std::vector<Point>& points) {
    if (points.size() < 3) { return 0.0; }

    double min_angle = 180.0;
    int n = points.size();

    // 遍历每个顶点计算内角
    for (int i = 0; i < n; ++i) {
        auto& v0 = points[i];
        auto& v1 = points[(i + 1) % n];
        auto& v2 = points[(i + 2) % n];

        auto vec10 = (v0 - v1).normalized();
        auto vec12 = (v2 - v1).normalized();
        double angle = std::acos(vec10.dot(vec12)) / M_PI * 180.0;
        min_angle = std::min(min_angle, angle);
    }

    // 根据面类型计算质量值
    double val = std::fmin(1.0, (n == 3) ? (min_angle / 60.0) : (min_angle / 90.0));
    return val;
}


/*
* 表面网格体积计算
* 基于MeshMath.h中的get_volume_surface_mesh
*/
double SurfaceMeshMetricsFilter::ComputeSurfaceVolume(const std::vector<Point>& points) {
    // 注意：这个函数通常用于整个表面网格，而不是单个面片
    // 这里实现单个面片的体积贡献计算

    if (points.size() < 3) { return 0.0; }

    double volume_contribution = 0.0;

    // 将多边形分解为三角形并计算体积贡献
    for (int i = 2; i < points.size(); ++i) {
        auto& v0 = points[0];
        auto& v1 = points[i - 1];
        auto& v2 = points[i];

        // 使用原始算法：v_tet = (v0 % v1) * v2
        double v_tet = (v0.cross(v1)).dot(v2);
        volume_contribution += v_tet;
    }

    return std::abs(volume_contribution) / 6.0;
}


/*
* 表面网格的体积
*/
double SurfaceMeshMetricsFilter::ComputeTotalSurfaceVolume(CellArray::Pointer m_Faces, Points::Pointer m_Points) {
    if (!m_Faces || !m_Points) { return 0.0; }

    double total_volume = 0.0;
    igIndex vhs[IGAME_CELL_MAX_SIZE] = {0};

    igIndex cellNum = m_Faces->GetNumberOfCells();

    for (igIndex i = 0; i < cellNum; i++) {
        igIndex vNum = m_Faces->GetCellIds(i, vhs);
        std::vector<Point> points;

        for (igIndex j = 0; j < vNum; j++) { points.push_back(m_Points->GetPoint(vhs[j])); }

        // 累加每个面片的体积贡献
        total_volume += ComputeSurfaceVolume(points);
    }

    return total_volume;
}


IGAME_NAMESPACE_END
