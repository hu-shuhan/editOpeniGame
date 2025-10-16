#include "Convert\iGameConvertToVolumeMesh.h"
#include "iGameVolumeMeshMetrics.h"


IGAME_NAMESPACE_BEGIN

VolumeMeshMetrics::VolumeMeshMetrics() {
    this->SetNumberOfInputs(1);
    this->SetNumberOfOutputs(1);
    m_Cells = nullptr;
    m_Points = nullptr;
}

VolumeMeshMetrics::~VolumeMeshMetrics() {
    m_Cells = nullptr;
    m_Points = nullptr;
}


bool VolumeMeshMetrics::Execute() {
    if (m_Inputs->GetNumberOfElements() == 0) { return false; }

    auto input = m_Inputs->GetElement(0);
    if (!input) { return false; }
    m_Cells = nullptr;
    switch (input->GetDataObjectType()) 
    {
        case IG_VOLUME_MESH:
            m_Cells = (DynamicCast<VolumeMesh>(input))->GetCells();
            m_Points = (DynamicCast<VolumeMesh>(input))->GetPoints();
            break;
        case IG_UNSTRUCTURED_MESH: 
        {
            auto mesh = DynamicCast<UnstructuredMesh>(input);
            auto converter = ConvertToVolumeMesh::New();
            converter->SetInput(mesh);
            bool result = converter->Execute();
            if (result) {
                m_Cells = converter->GetVolumeMesh()->GetCells();
                m_Points = converter->GetVolumeMesh()->GetPoints();
            }
            break;
        }
        default:
            igDebug("请输入体网格进行质量检测");
            break;
    }
    if (!m_Cells) {
        igDebug("没有体网格单元");
        return false;

    }
    igIndex vhs[IGAME_CELL_MAX_SIZE] = {0}; //存储每个面的顶点索引数组
    igIndex vNum = 0;                       //每个面的顶点数量

    igIndex cellNum = m_Cells->GetNumberOfCells(); //总面数

    iGame::DoubleArray::Pointer metricArray = iGame::DoubleArray::New();

    metricArray->SetDimension(1);
    metricArray->Reserve(cellNum);
    iGame::DoubleArray::Pointer dataRange = iGame::DoubleArray::New();
    dataRange->AddValue(0.0);
    dataRange->AddValue(1.0);
    dataRange->AddValue(0.0);
    dataRange->AddValue(1.0);
    for (igIndex i = 0; i < cellNum; i++) {
        vNum = m_Cells->GetCellIds(i, vhs);             // 获取顶点索引
        double metric = this->ComputeMetric(vNum, vhs); // 计算质量指标
        metricArray->AddValue(metric);                  // 存储结果
    }


    auto output = input;
    output->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, metricArray);
    this->SetOutput(output);
    return true;
}


double VolumeMeshMetrics::ComputeMetric(igIndex vNum, igIndex* vhs) {


    std::vector<iGame::Point> points;
    for (igIndex i = 0; i < vNum; i++) { 
        points.push_back(m_Points->GetPoint(vhs[i])); 
    }
    //Point p[100];
    //for (igIndex i = 0; i < vNum; i++) {
    //	p[i]=m_Points->GetPoint(vhs[0]);
    //}
    //double len= (p[1]-p[0]).norm();
    //return ComputeA(vNum, vhs);

    // 根据顶点数量选择计算方法
    if (vNum == 4) {
        // 四面体
        switch (m_Metric) {
            case TET_EDGE_RATIO:
                return ComputeTetEdgeRatio(points);
                break;
            case TET_VOLUME:
                return ComputeTetVolume(points);
                break;
            case TET_ASPECT_RATIO:
                return GetAspectRatioOfCell(points);
                break;
            case TET_JACOBIAN:
                return GetJacobianOfCell(points);
                break;
            case TET_COLLAPSE_RATIO:
                return GetCollapseRatioOfCell(points);
                break;
            case TET_VOL_SKEW:
                return GetVolSkewOfCell(points);
                break;
            case TET_MIN_ANGLE:
                return GetMinInternalAnglesOfCell(points);
                break;
            case TET_EQUIANGLE_SKEWNESS:
                return GetEquiangleSkewnessOfCell(points);
                break;
            case TET_INRADIUS:
                return GetInradiusOfCell(points);
                break;
            case TET_CIRCUMRADIUS:
                return GetCircumradiusOfCell(points);
                break;
            case TET_VOL_ASPECT_RATIO:
                return GetVolAspectRatioOfCell(points);
                break;
            default:
                break;
        }
    } else if (vNum == 8) {
        // 六面体
        switch (m_Metric) {
            case HEX_VOLUME:
                return ComputeHexVolume(points);
                break;
            case HEX_TAPER:
                return ComputeHexTaper(points);
                break;
            case HEX_JACOBIAN:
                return ComputeHexJacobian(points);
                break;
            case HEX_EDGE_RATIO:
                return ComputeHexEdgeRatio(points);
                break;
            case HEX_MAX_EDGE_RATIO:
                return ComputeHexMaxEdgeRatio(points);
                break;
            case HEX_SKEW:
                return ComputeHexSkew(points);
                break;
            case HEX_STRETCH:
                return ComputeHexStretch(points);
                break;
            case HEX_DIAGONAL:
                return ComputeHexDiagonal(points);
                break;
            case HEX_RELATIVE_SIZE_SQUARED:
                return 0;
                break;
            default:
                break;
        }
    }

}


/*
	* 计算某一个四面体单元的最小/最大长度
*/
std::vector<double> VolumeMeshMetrics::GetMinAndMaxLenOfCell(const std::vector<iGame::Point>& points) {
    double minLength = DBL_MAX, maxLength = 0.0;

    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            double length = (points[i] - points[j]).norm();
            minLength = std::min(minLength, length);
            maxLength = std::max(maxLength, length);
        }
    }

    std::vector<double> result;
    result.push_back(minLength);
    result.push_back(maxLength);
    return result;
}

/*
	* 计算某一个四面体单元的边长比 最长边/最短边
	* 正四面体：1
	*/
double VolumeMeshMetrics::ComputeTetEdgeRatio(const std::vector<iGame::Point>& points) {
    std::vector<double> result = GetMinAndMaxLenOfCell(points);
    return result[1] / result[0];
}

/*
* 计算某一个四面体单元的体积
*/
double VolumeMeshMetrics::ComputeTetVolume(const std::vector<iGame::Point>& points) {
    std::vector<iGame::Point> L(3);
    L[0] = points[1] - points[0];
    L[1] = points[2] - points[0];
    L[2] = points[3] - points[0];
    double volume = std::abs(L[0].cross(L[1]).dot(L[2]) / 6.0);

    return volume;
}

/*
* 计算某一个面的面积 
*/
double VolumeMeshMetrics::GetAreaOfFace(iGame::Point v1, iGame::Point v2, iGame::Point v3) {
    return ((v2 - v1).cross(v3 - v1)).norm() / 2.0;
}

/*
* 计算某一个四面体单元的面积 该点对应面的面积 
*/
std::vector<double> VolumeMeshMetrics::GetAreaOfCell(const std::vector<iGame::Point>& points) {
   
    std::vector<double> areas;

    double area = GetAreaOfFace(points[1], points[2], points[3]);
    areas.push_back(area);

    area = GetAreaOfFace(points[0], points[2], points[3]);
    areas.push_back(area);

    area = GetAreaOfFace(points[0], points[1], points[3]);
    areas.push_back(area);

    area = GetAreaOfFace(points[0], points[1], points[2]);
    areas.push_back(area);

    return areas;
}

/*
* 计算某一个四面体单元的内切球的半径  3*volume/sum(area)
*/
double VolumeMeshMetrics::GetInradiusOfCell(const std::vector<iGame::Point>& points) {
    std::vector<double> areas = GetAreaOfCell(points);
    double volume = ComputeTetVolume(points);
    double sumArea = 0;
    for (double area: areas) { sumArea += area; }
    return 3.0 * volume / sumArea;
}

	/*
	* 计算某一个四面体单元的外接球的球心
	*/
iGame::Point VolumeMeshMetrics::GetOuterCircle(const std::vector<iGame::Point>& points) {

    Eigen::Matrix3f m3;
    

    iGame::Point p1 = (points[1] - points[0]) * 2.0;
    iGame::Point p2 = (points[2] - points[1]) * 2.0;
    iGame::Point p3 = (points[3] - points[2]) * 2.0;
    double d1 = points[1] * points[1] - points[0] * points[0];
    double d2 = points[2] * points[2] - points[1] * points[1];
    double d3 = points[3] * points[3] - points[2] * points[2];
    m3 << p1[0], p1[1], p1[2], p2[0], p2[1], p2[2], p3[0], p3[1], p3[2];

    

    Eigen::Vector3f vc3(d1, d2, d3);
    Eigen::Matrix3f m3IN = m3.inverse();

    vc3 = m3IN * vc3;
    //return iGame::Point(center.x(), center.y(), center.z());
    return iGame::Point(vc3(0), vc3(1), vc3(2));

}

/*
	* 计算某一个四面体单元的外接球的半径
	*/
double VolumeMeshMetrics::GetCircumradiusOfCell(const std::vector<iGame::Point>& points) {
    iGame::Point outcircle = GetOuterCircle(points);

    return (points[0] - outcircle).norm();
}

	/*
	* 计算某个四面体的纵横比   最长边/( 2*sqrt(6)*内切半径)   
	* 接受范围：[1,3]
	* 最好：1
	*/
double VolumeMeshMetrics::GetAspectRatioOfCell(const std::vector<iGame::Point>& points) {
    double r = GetInradiusOfCell(points);
    double maxLength = GetMinAndMaxLenOfCell(points)[1];

    double coff = 2.0 * sqrt(6);
    double aspectRatio = maxLength / coff / r;
    return aspectRatio;
}

/*
	* 计算某个四面体的雅可比行列式的结果 为 6倍的体积
	* 接受范围：[0,DBL MAX]
	* 单位长度正四面体：sqrt(2)/2
    */
double VolumeMeshMetrics::GetJacobianOfCell(const std::vector<iGame::Point>& points) {
    return ComputeTetVolume(points) * 6.0;
}

/*
	* 计算某个四面体的某点的塌陷率
	*/
double VolumeMeshMetrics::GetCollapseRatioOfVertex(iGame::Point v1, iGame::Point v2, iGame::Point v3,
                                                          double volume) {
    double area = ((v2 - v1).cross(v3 - v1)).norm();
    double high = volume / area * 6;

    double maxLength = std::max((v2 - v1).norm(), (v3 - v1).norm());
    maxLength = std::max(maxLength, (v3 - v2).norm());

    return high / maxLength;
}

/*
	* 计算某个四面体的塌陷率   某点所在高 除以 该点对应的面的最长边。
	* 返回四个点的中最小塌陷率
	* 接受范围：[0.1,DBL MAX]
	* 最好：sqrt(6)/3
	*/
double VolumeMeshMetrics::GetCollapseRatioOfCell(const std::vector<iGame::Point>& points) {
    double volume = ComputeTetVolume(points);

    double CollapseRatio = GetCollapseRatioOfVertex(points[1], points[2], points[3], volume);

    CollapseRatio = std::min(CollapseRatio, GetCollapseRatioOfVertex(points[0], points[2], points[3], volume));
    CollapseRatio = std::min(CollapseRatio, GetCollapseRatioOfVertex(points[1], points[0], points[3], volume));
    CollapseRatio = std::min(CollapseRatio, GetCollapseRatioOfVertex(points[1], points[2], points[0], volume));
    return CollapseRatio;
}

/*
	*得到某个四面体体积歪斜度 (外接球的正四面体的体积-体积)/外接球的正四面体的体积
	* 接受范围：[0,1]
	* 最好：0
	*/
double VolumeMeshMetrics::GetVolSkewOfCell(const std::vector<iGame::Point>& points){
    double circumRadius = GetCircumradiusOfCell(points);
    double a = circumRadius * 4.0 / sqrt(6.0);
    double volume = ComputeTetVolume(points);

    double regularVolume = a * a * a * sqrt(2.0) / 12.0;
    return (regularVolume - volume) / regularVolume;
}

/*
	*得到某个四面体的某一点歪斜度   某个节点到对边中点的线段 与 另外两条边中点连接的线段之间 的较小的角
	*/
double VolumeMeshMetrics::GetSkewnessOfVertex(iGame::Point v0, iGame::Point v1, iGame::Point v2) {

    iGame::Point m12 = (v1 + v2) / 2;
    iGame::Point m01 = (v0 + v1) / 2;
    iGame::Point m02 = (v0 + v2) / 2;
    double cosa = (v0 - m01).normalized() * (m01 - m02).normalized();
    double a = acos(cosa) * 180 / PI;
    return std::min(a, 180 - a);
}

/*
	*得到某个四面体的某个面的歪斜度   为三个点的歪斜度中的最大值  某个节点到对边中点的线段 与 另外两条边中点连接的线段之间 的较小的角
	* 接受范围:(0,90]
	* 最好：90
	*/
double VolumeMeshMetrics::GetSkewnessOfFace(const std::vector<iGame::Point>& points) {
    double skewOfFace = 0;

    skewOfFace = std::max(GetSkewnessOfVertex(points[0], points[1], points[2]), skewOfFace);
    skewOfFace = std::max(GetSkewnessOfVertex(points[1], points[0], points[2]), skewOfFace);
    skewOfFace = std::max(GetSkewnessOfVertex(points[2], points[1], points[0]), skewOfFace);
    return skewOfFace;
}

/*
	*得到某个四面体的某个面的的v0的内角
	*/
double VolumeMeshMetrics::GetInternalAnglesOfVertex(iGame::Point v0, iGame::Point v1, iGame::Point v2) {

    double cosa = (v1 - v0).normalized() * (v2 - v0).normalized();
    double angle = acos(cosa) * 180.0 / PI;

    return angle;
}

/*
	*得到某个四面体的某个面的三个内角 
	*/
std::vector<double> VolumeMeshMetrics::GetInternalAnglesOfFace(const std::vector<iGame::Point>& points) {
    std::vector<double> angles;
    angles.push_back(GetInternalAnglesOfVertex(points[0], points[1], points[2]));
    angles.push_back(GetInternalAnglesOfVertex(points[1], points[0], points[2]));
    angles.push_back(GetInternalAnglesOfVertex(points[2], points[1], points[0]));
    return angles;
}


/*
* 得到某个四面体的所有面的内角  
*/
std::vector<std::vector<double>> VolumeMeshMetrics::GetInternalAnglesOfCell(const std::vector<iGame::Point>& points) {
    std::vector<std::vector<double>> angles;

    // 四面体的四个面
    std::vector<std::vector<iGame::Point>> faces = {
            {points[1], points[2], points[3]}, // 排除points[0]的面
            {points[0], points[2], points[3]}, // 排除points[1]的面
            {points[0], points[1], points[3]}, // ...
            {points[0], points[1], points[2]}  
    };

    for (const auto& face: faces) { angles.push_back(GetInternalAnglesOfFace(face)); }

    return angles;
}
/*
    *得到某个四面体的最小内角，角度制
    */
double VolumeMeshMetrics::GetMinInternalAnglesOfCell(const std::vector<iGame::Point>& points) {
    double min_angle = DBL_MAX;

    // 计算四个面的最小内角
    std::vector<std::vector<iGame::Point>> faces = {{points[1], points[2], points[3]},
                                                    {points[0], points[2], points[3]},
                                                    {points[0], points[1], points[3]},
                                                    {points[0], points[1], points[2]}};

    for (const auto& face: faces) {
        auto angles = GetInternalAnglesOfFace(face);
        for (double a: angles) { min_angle = std::min(min_angle, a); }
    }

    return min_angle;
}

	/*
	* 计算某个四面体v0所在的高的长度
	*/
double VolumeMeshMetrics::GetHighOfVertex(iGame::Point v0, iGame::Point v1, iGame::Point v2, iGame::Point v3) {
    iGame::Point normOfFace = ((v2 - v1).cross(v3 - v1)).normalized();
    return std::abs(normOfFace * (v0 - v1));
}

/*
	* 计算某个四面体的体长宽比   最长边/最短高
	* 单位正四面体：sqrt(3)/2
	*/
double VolumeMeshMetrics::GetVolAspectRatioOfCell(const std::vector<iGame::Point>& points) {
    double maxLength = GetMinAndMaxLenOfCell(points)[1];

    double minHigh = GetHighOfVertex(points[0], points[1], points[2], points[3]);
    minHigh = std::min(GetHighOfVertex(points[1], points[0], points[2], points[3]), minHigh);
    minHigh = std::min(GetHighOfVertex(points[2], points[1], points[0], points[3]), minHigh);
    minHigh = std::min(GetHighOfVertex(points[3], points[1], points[2], points[0]), minHigh);
    return maxLength / minHigh;
}

	/*
	* 计算所有四面体的等角斜率  max( (Qmax-Qe)/(180-Qe)，(Qe-Qmin)/Qe ) Qmax 最大角 Qmin最小 Qe 60（三角形）或90（四边形） acos(1/3.0)四面体
	* 计算出二面角 求其等角斜率大小
	* 再求所有面中的最大角 和最小角  算出等角斜率
	* 取两者中较大的一个
	* 
	* 最好：0
	*
	*/
double VolumeMeshMetrics::GetEquiangleSkewnessOfCell(const std::vector<iGame::Point>& points) {

    iGame::Vector3f ab = (points[1] - points[0]).normalized();
    iGame::Vector3f ac = (points[2] - points[0]).normalized();
    iGame::Vector3f ad = (points[3] - points[0]).normalized();
    iGame::Vector3f bc = (points[2] - points[1]).normalized();
    iGame::Vector3f bd = (points[3] - points[1]).normalized();
    iGame::Vector3f cd = (points[3] - points[2]).normalized();

    iGame::Vector3f abc = (bc.cross(ab)).normalized();
    iGame::Vector3f abd = (ab.cross(ad)).normalized();
    iGame::Vector3f acd = (cd.cross(ad)).normalized();
    iGame::Vector3f bcd = (bc.cross(cd)).normalized();

    //二面角
    double alpha = acos(-(abc * abd));
    double beta = acos(-(abc * acd));
    double gamma = acos(-(abc * bcd));
    double delta = acos(-(abd * acd));
    double epsilon = acos(-(abd * bcd));
    double zeta = acos(-(acd * bcd));

    double minAngle = alpha;
    minAngle = std::min(minAngle, beta);
    minAngle = std::min(minAngle, gamma);
    minAngle = std::min(minAngle, delta);
    minAngle = std::min(minAngle, epsilon);
    minAngle = std::min(minAngle, zeta);

    double maxAngle = alpha;
    maxAngle = std::min(maxAngle, beta);
    maxAngle = std::min(maxAngle, gamma);
    maxAngle = std::min(maxAngle, delta);
    maxAngle = std::min(maxAngle, epsilon);
    maxAngle = std::min(maxAngle, zeta);

    double theta = acos(1 / 3.0) * 180.0 / PI;
    minAngle *= 180.0 / PI;
    maxAngle *= 180.0 / PI;

    double dihedralMaxSkew = (maxAngle - theta) / (180.0 - theta);
    double dihedralMinSkew = (theta - minAngle) / theta;

    double maxEquiangleSkew = dihedralMaxSkew;
    maxEquiangleSkew = std::max(maxEquiangleSkew, dihedralMinSkew);

    //面上的内角
    /*double angles[12];

		angles[0] = acos(-(ab * bc));
		angles[1] = acos((bc * ac));
		angles[2] = acos((ac * ab));
		angles[3] = acos(-(ab * bd));
		angles[4] = acos((bd * ad));
		angles[5] = acos((ad * ab));
		angles[6] = acos(-(bc * cd));
		angles[7] = acos((cd * bd));
		angles[8] = acos((bd * bc));
		angles[9] = acos((ad * cd));
		angles[10] = acos(-(cd * ac));
		angles[11] = acos((ac * ad));
		
			minAngle = angles[0];
			maxAngle = angles[1];
			for (int i = 0; i < 12; i++) {
				minAngle = min(minAngle, angles[i]);
				maxAngle = max(maxAngle, angles[i]);
			}

			minAngle *= 180.0 / TET_PI;
			maxAngle *= 180.0 / TET_PI;*/

    std::vector<std::vector<double>> anglesOfFaces = GetInternalAnglesOfCell(points);
    minAngle = anglesOfFaces[0][0];
    maxAngle = anglesOfFaces[0][0];

    for (std::vector<double> angles: anglesOfFaces) {
        for (double angle: angles) {
            minAngle = std::min(angle, minAngle);
            maxAngle = std::max(angle, maxAngle);
        }
    }


    theta = 60.0;
    double maxSkew = (maxAngle - theta) / (180.0 - theta);
    double minSkew = (theta - minAngle) / theta;

    maxEquiangleSkew = std::max(maxEquiangleSkew, maxSkew);
    maxEquiangleSkew = std::max(maxEquiangleSkew, minSkew);


    return maxEquiangleSkew = std::max(maxEquiangleSkew, maxSkew);
}




//六面体


// 体积
double VolumeMeshMetrics::ComputeHexVolume(const std::vector<iGame::Point>& points) {
    auto P_0 = points[0];
    auto P_1 = points[1];
    auto P_2 = points[2];
    auto P_3 = points[3];
    auto P_4 = points[4];
    auto P_5 = points[5];
    auto P_6 = points[6];
    auto P_7 = points[7];

    auto X_1 = (P_1 - P_0) + (P_2 - P_3) + (P_5 - P_4) + (P_6 - P_7);
    auto X_2 = (P_3 - P_0) + (P_2 - P_1) + (P_7 - P_4) + (P_6 - P_5);
    auto X_3 = (P_4 - P_0) + (P_5 - P_1) + (P_6 - P_2) + (P_7 - P_3);

    return X_1.dot(X_2.cross(X_3)) / 64.0;
}

// 锥度, range : [0, MAX], acceptable range [0, 0.5], unit cube : 0
double VolumeMeshMetrics::ComputeHexTaper(const std::vector<iGame::Point>& points) {
    auto P_0 = points[0];
    auto P_1 = points[1];
    auto P_2 = points[2];
    auto P_3 = points[3];
    auto P_4 = points[4];
    auto P_5 = points[5];
    auto P_6 = points[6];
    auto P_7 = points[7];

    auto X_1 = (P_1 - P_0) + (P_2 - P_3) + (P_5 - P_4) + (P_6 - P_7);
    auto X_2 = (P_3 - P_0) + (P_2 - P_1) + (P_7 - P_4) + (P_6 - P_5);
    auto X_3 = (P_4 - P_0) + (P_5 - P_1) + (P_6 - P_2) + (P_7 - P_3);

    auto X_12 = (P_2 - P_3) - (P_1 - P_0) + (P_6 - P_7) - (P_5 - P_4);
    auto X_13 = (P_5 - P_1) - (P_4 - P_0) + (P_6 - P_2) - (P_7 - P_3);
    auto X_23 = (P_7 - P_4) - (P_3 - P_0) + (P_6 - P_5) - (P_2 - P_1);

    auto T_12 = X_12.norm() / ((std::min)(X_1.norm(), X_2.norm()));
    auto T_13 = X_13.norm() / ((std::min)(X_1.norm(), X_3.norm()));
    auto T_23 = X_23.norm() / ((std::min)(X_2.norm(), X_3.norm()));

    return (std::max)({T_12, T_13, T_23});
}

// 雅可比, range : [0, MAX], acceptable range [0, MAX], unit cube : 1
double VolumeMeshMetrics::ComputeHexJacobian(const std::vector<iGame::Point>& points) {
    //be caution about order

    auto P_0 = points[0];
    auto P_1 = points[1];
    auto P_2 = points[2];
    auto P_3 = points[3];
    auto P_4 = points[4];
    auto P_5 = points[5];
    auto P_6 = points[6];
    auto P_7 = points[7];

    float jacobian = (std::numeric_limits<float>::max)();
    float current_jacobian = 0.0;

    iGame::Vector3f L_0, L_2, L_3;

    L_0 = P_1 - P_0;
    L_2 = P_3 - P_0;
    L_3 = P_4 - P_0;

    current_jacobian = (L_2.cross(L_3)).dot(L_0);
    if (current_jacobian < jacobian) jacobian = current_jacobian;

    L_0 = P_2 - P_1;
    L_2 = P_0 - P_1;
    L_3 = P_5 - P_1;

    current_jacobian = (L_2.cross(L_3)).dot(L_0);
    if (current_jacobian < jacobian) jacobian = current_jacobian;

    L_0 = P_3 - P_2;
    L_2 = P_1 - P_2;
    L_3 = P_6 - P_2;

    current_jacobian = (L_2.cross(L_3)).dot(L_0);
    if (current_jacobian < jacobian) jacobian = current_jacobian;

    L_0 = P_0 - P_3;
    L_2 = P_2 - P_3;
    L_3 = P_7 - P_3;

    current_jacobian = (L_2.cross(L_3)).dot(L_0);
    if (current_jacobian < jacobian) jacobian = current_jacobian;

    L_0 = P_7 - P_4;
    L_2 = P_5 - P_4;
    L_3 = P_0 - P_4;

    current_jacobian = (L_2.cross(L_3)).dot(L_0);
    if (current_jacobian < jacobian) jacobian = current_jacobian;

    L_0 = P_4 - P_5;
    L_2 = P_6 - P_5;
    L_3 = P_1 - P_5;

    current_jacobian = (L_2.cross(L_3)).dot(L_0);
    if (current_jacobian < jacobian) jacobian = current_jacobian;

    L_0 = P_5 - P_6;
    L_2 = P_7 - P_6;
    L_3 = P_2 - P_6;

    current_jacobian = (L_2.cross(L_3)).dot(L_0);
    if (current_jacobian < jacobian) jacobian = current_jacobian;

    L_0 = P_6 - P_7;
    L_2 = P_4 - P_7;
    L_3 = P_3 - P_7;

    current_jacobian = (L_2.cross(L_3)).dot(L_0);
    if (current_jacobian < jacobian) jacobian = current_jacobian;

    return jacobian;
}

// 长宽比 , range : [1, MAX], acceptable range [1, MAX], unit cube : 1
double VolumeMeshMetrics::ComputeHexEdgeRatio(const std::vector<iGame::Point>& points) {
    auto P_0 = points[0];
    auto P_1 = points[1];
    auto P_2 = points[2];
    auto P_3 = points[3];
    auto P_4 = points[4];
    auto P_5 = points[5];
    auto P_6 = points[6];
    auto P_7 = points[7];

    std::vector<float> lengths(12, 0);
    lengths[0] = (P_1 - P_0).norm();
    lengths[1] = (P_2 - P_1).norm();
    lengths[2] = (P_3 - P_2).norm();
    lengths[3] = (P_3 - P_0).norm();
    lengths[4] = (P_4 - P_0).norm();
    lengths[5] = (P_5 - P_1).norm();
    lengths[6] = (P_6 - P_2).norm();
    lengths[7] = (P_7 - P_3).norm();
    lengths[8] = (P_5 - P_4).norm();
    lengths[9] = (P_6 - P_5).norm();
    lengths[10] = (P_7 - P_6).norm();
    lengths[11] = (P_7 - P_4).norm();

    auto max_ele = std::max_element(lengths.begin(), lengths.end());
    auto min_ele = std::min_element(lengths.begin(), lengths.end());

    return *max_ele / *min_ele;
}

// 最大长宽比 , range : [1, MAX], acceptable range [1, 1.3], unit cube : 1
double VolumeMeshMetrics::ComputeHexMaxEdgeRatio(const std::vector<iGame::Point>& points) {
    auto P_0 = points[0];
    auto P_1 = points[1];
    auto P_2 = points[2];
    auto P_3 = points[3];
    auto P_4 = points[4];
    auto P_5 = points[5];
    auto P_6 = points[6];
    auto P_7 = points[7];

    auto X_1 = (P_1 - P_0) + (P_2 - P_3) + (P_5 - P_4) + (P_6 - P_7);
    auto X_2 = (P_3 - P_0) + (P_2 - P_1) + (P_7 - P_4) + (P_6 - P_5);
    auto X_3 = (P_4 - P_0) + (P_5 - P_1) + (P_6 - P_2) + (P_7 - P_3);

    auto L_1 = X_1.norm();
    auto L_2 = X_2.norm();
    auto L_3 = X_3.norm();

    auto A_12 = (std::max)(L_1 / L_2, L_2 / L_1);
    auto A_13 = (std::max)(L_1 / L_3, L_3 / L_1);
    auto A_23 = (std::max)(L_2 / L_3, L_3 / L_2);

    return (std::max)({A_12, A_13, A_23});
}

    // 体积歪斜度/歪斜度 , range : [0, 1], acceptable range [0, 0.5], unit cube : 0
double VolumeMeshMetrics::ComputeHexSkew(const std::vector<iGame::Point>& points) {
    auto P_0 = points[0];
    auto P_1 = points[1];
    auto P_2 = points[2];
    auto P_3 = points[3];
    auto P_4 = points[4];
    auto P_5 = points[5];
    auto P_6 = points[6];
    auto P_7 = points[7];

    auto X_1 = (P_1 - P_0) + (P_2 - P_3) + (P_5 - P_4) + (P_6 - P_7);
    auto X_2 = (P_3 - P_0) + (P_2 - P_1) + (P_7 - P_4) + (P_6 - P_5);
    auto X_3 = (P_4 - P_0) + (P_5 - P_1) + (P_6 - P_2) + (P_7 - P_3);

    auto X_1_hat = X_1.normalized();
    auto X_2_hat = X_2.normalized();
    auto X_3_hat = X_3.normalized();

    auto skew_12 = std::abs(X_1_hat.dot(X_2_hat));
    auto skew_13 = std::abs(X_1_hat.dot(X_3_hat));
    auto skew_23 = std::abs(X_2_hat.dot(X_3_hat));

    return (std::max)({skew_12, skew_13, skew_23});
}

    // 伸展度 , range : [0, 1], acceptable range [0.25, 1], unit cube : 1
double VolumeMeshMetrics::ComputeHexStretch(const std::vector<iGame::Point>& points) {
    auto P_0 = points[0];
    auto P_1 = points[1];
    auto P_2 = points[2];
    auto P_3 = points[3];
    auto P_4 = points[4];
    auto P_5 = points[5];
    auto P_6 = points[6];
    auto P_7 = points[7];

    std::vector<float> edges(12, 0);
    edges[0] = (P_1 - P_0).norm();
    edges[1] = (P_2 - P_1).norm();
    edges[2] = (P_3 - P_2).norm();
    edges[3] = (P_3 - P_0).norm();
    edges[4] = (P_4 - P_0).norm();
    edges[5] = (P_5 - P_1).norm();
    edges[6] = (P_6 - P_2).norm();
    edges[7] = (P_7 - P_3).norm();
    edges[8] = (P_5 - P_4).norm();
    edges[9] = (P_6 - P_5).norm();
    edges[10] = (P_7 - P_6).norm();
    edges[11] = (P_7 - P_4).norm();

    std::vector<float> diagonals(4, 0);

    diagonals[0] = (P_6 - P_0).norm();
    diagonals[1] = (P_7 - P_1).norm();
    diagonals[2] = (P_4 - P_2).norm();
    diagonals[3] = (P_5 - P_3).norm();

    float L_min = *(std::min_element(edges.begin(), edges.end()));
    float D_max = *(std::max_element(diagonals.begin(), diagonals.end()));

    return std::sqrt(3.0f) * L_min / D_max;
}

// 对角线长度比值, range : [0, 1], acceptable range [0.65, 1], unit cube : 1
double VolumeMeshMetrics::ComputeHexDiagonal(const std::vector<iGame::Point>& points) {
    auto P_0 = points[0];
    auto P_1 = points[1];
    auto P_2 = points[2];
    auto P_3 = points[3];
    auto P_4 = points[4];
    auto P_5 = points[5];
    auto P_6 = points[6];
    auto P_7 = points[7];

    std::vector<float> diagonals(4, 0);

    diagonals[0] = (P_6 - P_0).norm();
    diagonals[1] = (P_7 - P_1).norm();
    diagonals[2] = (P_4 - P_2).norm();
    diagonals[3] = (P_5 - P_3).norm();

    auto max_ele = std::max_element(diagonals.begin(), diagonals.end());
    auto min_ele = std::min_element(diagonals.begin(), diagonals.end());

    return *min_ele / *max_ele;
}

// 相对大小平方, range : [0, 1], acceptable range [0, 1], unit cube : 依赖于平均体积
double VolumeMeshMetrics::ComputeHexRelativeSizeSquared(const std::vector<iGame::Point>& points, float average_volume) {
    auto D = ComputeHexVolume(points) / average_volume;
    auto sqr_q = (std::min)(D, 1.0f / D);

    return sqr_q * sqr_q;
}

// 六面体最小标量雅可比
double VolumeMeshMetrics::ComputeHexMinScaledJacobian(const std::vector<iGame::Point>& points) {
    double min_det = 2.f; // the value of Scaled Jacobian is the	determinant of the matrix Jacobian
    std::unordered_map<int, std::vector<int>> neighbor;
    std::vector<std::pair<int, int>> edges = {
            {0, 1}, {1, 2}, {2, 3}, {3, 0}, // 底面
            {4, 5}, {5, 6}, {6, 7}, {7, 4}, // 顶面
            {0, 4}, {1, 5}, {2, 6}, {3, 7}  // 侧面
    };
    for (const auto& edge: edges) {
        neighbor[edge.first].push_back(edge.second);
        neighbor[edge.second].push_back(edge.first);
    }

    // 对每个顶点计算雅可比矩阵行列式
    for (int i = 0; i < 8; ++i) {
        auto& v = points[i];
        auto adjvh = neighbor[i];

        if (adjvh.size() != 3) { continue; }

        auto v1 = points[adjvh[0]];
        auto v2 = points[adjvh[1]];
        auto v3 = points[adjvh[2]];

        auto center = (v1 + v2 + v3) / 3.0;
        auto vec = (center - v).normalized();

        auto vec12 = v2 - v1;
        auto vec13 = v3 - v1;
        auto normal = (vec12.cross(vec13)).normalized();
        double cosine = vec.dot(normal);

        if (cosine < 0) { std::swap(v2, v3); }

        std::vector<Vector3d> ev(3);
        auto tmp1 = v1 - v;
        ev[0] = tmp1.normalized();
        auto tmp2 = v2 - v;
        ev[1] = tmp2.normalized();
        auto tmp3 = v3 - v;
        ev[2] = tmp3.normalized();

        Eigen::Matrix3d J;
        J << ev[0][0], ev[1][0], ev[2][0], ev[0][1], ev[1][1], ev[2][1], ev[0][2], ev[1][2], ev[2][2];

        double det = J.determinant();
        min_det = std::min(min_det, det);
    }

    if (min_det < 0) { igDebug("Warning: There exist too bad hexahedron!!!"); }

    return min_det;
}

// 六面体平均标量雅可比
double VolumeMeshMetrics::ComputeHexAvgScaledJacobian(const std::vector<iGame::Point>& points) {
    double sum_det = 0.0;

    // 构建邻接关系
    std::unordered_map<int, std::vector<int>> neighbor;

    std::vector<std::pair<int, int>> edges = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                                              {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};

    for (const auto& edge: edges) {
        neighbor[edge.first].push_back(edge.second);
        neighbor[edge.second].push_back(edge.first);
    }

    int valid_vertices = 0;

    // 对每个顶点计算雅可比矩阵行列式
    for (int i = 0; i < 8; ++i) {
        auto& v = points[i];
        auto adjvh = neighbor[i];

        if (adjvh.size() != 3) {
            continue; // 跳过不满足条件的顶点
        }

        valid_vertices++;

        auto v1 = points[adjvh[0]];
        auto v2 = points[adjvh[1]];
        auto v3 = points[adjvh[2]];

        // 计算中心点并确定法向
        auto center = (v1 + v2 + v3) / 3.0;
        auto vec = (center - v).normalized();

        // 调整顶点顺序以确保正确的法向
        auto vec12 = v2 - v1;
        auto vec13 = v3 - v1;
        auto normal = (vec12.cross(vec13)).normalized();
        double cosine = vec.dot(normal);

        if (cosine < 0) { std::swap(v2, v3); }

        // 构建三个边的单位向量
        std::vector<Vector3d> ev(3);
        auto tmp1 = v1 - v;
        ev[0] = tmp1.normalized();
        auto tmp2 = v2 - v;
        ev[1] = tmp2.normalized();
        auto tmp3 = v3 - v;
        ev[2] = tmp3.normalized();

        // 构建雅可比矩阵
        Eigen::Matrix3d J;
        J << ev[0][0], ev[1][0], ev[2][0], ev[0][1], ev[1][1], ev[2][1], ev[0][2], ev[1][2], ev[2][2];

        sum_det += J.determinant();
    }

    return (valid_vertices > 0) ? sum_det / valid_vertices : 0.0;
}

// 四面体替代纵横比计算方法
double VolumeMeshMetrics::ComputeTetAspectRatioAlt(const std::vector<iGame::Point>& points) {
    if (points.size() != 4) {
        igDebug("Error: It is not a tet mesh!");
        return 0.0;
    }

    // 计算最长边长度
    double len_max = 0.0;
    std::vector<std::pair<int, int>> edges = {{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}};

    for (const auto& edge: edges) {
        double length = (points[edge.first] - points[edge.second]).norm();
        len_max = std::max(len_max, length);
    }

    double height_min = DBL_MAX;

    for (int i = 0; i < 4; ++i) {
        auto& v = points[i];
        std::vector<iGame::Point> face_points;

        for (int j = 0; j < 4; ++j) {
            if (j != i) { face_points.push_back(points[j]); }
        }

        Vector3f normal =(face_points[2] - face_points[0])
                                         .cross(face_points[1] - face_points[0]);

        double d = -normal.dot(face_points[0]);
        double numerator = std::abs(normal.dot(v) + d);
        double denominator = normal.norm();

        double height = numerator / std::max(denominator, 1e-7); // 避免除以零
        height_min = std::min(height_min, height);
    }

    double aspect_ratio = len_max / std::max(height_min, 1e-7);

    return aspect_ratio;
}



//四面体体积计算 - 基于MeshMath.h中的get_volume_tetahedral_mesh
double VolumeMeshMetrics::ComputeTetVolumeAlt(const std::vector<iGame::Point>& points) {
    if (points.size() != 4) { return 0.0; }

    double volume_mesh = 0;

    std::vector<std::vector<int>> faces = {{0, 1, 2}, {0, 2, 3}, {0, 3, 1}, {1, 2, 3}};

    for (const auto& face: faces) {
        auto& v0 = points[face[0]];
        auto& v1 = points[face[1]];
        auto& v2 = points[face[2]];

        double v_tet = (v0.cross(v1)).dot(v2);
        volume_mesh += v_tet;
    }

    return std::abs(volume_mesh) / 6.0;
}

//六面体体积计算 - 基于MeshMath.h中的get_volume_hexahedral_mesh  
double VolumeMeshMetrics::ComputeHexVolumeAlt(const std::vector<iGame::Point>& points) {
    if (points.size() != 8) { return 0.0; }

    double volume_mesh = 0;

    std::vector<std::vector<std::vector<int>>> quad_faces = {
            {{0, 1, 2}, {2, 3, 0}}, // 底面
            {{4, 5, 6}, {6, 7, 4}}, // 顶面
            {{0, 1, 5}, {5, 4, 0}}, // 前面
            {{2, 3, 7}, {7, 6, 2}}, // 后面
            {{0, 3, 7}, {7, 4, 0}}, // 左面
            {{1, 2, 6}, {6, 5, 1}}  // 右面
    };

    for (const auto& quad: quad_faces) {
        for (const auto& triangle: quad) {
            auto& v0 = points[triangle[0]];
            auto& v1 = points[triangle[1]];
            auto& v2 = points[triangle[2]];

            double v_tet = (v0.cross(v1)).dot(v2);
            volume_mesh += v_tet;
        }
    }

    return std::abs(volume_mesh) / 6.0;
}
IGAME_NAMESPACE_END
