
#include <iGameStreamTracer.h>
#include <iGameThreadPool.h>
float A[5] = {1.0 / 5.0, 3.0 / 10.0, 3.0 / 5.0, 1.0, 7.0 / 8.0};
float B[5][5] = {{1.0 / 5.0, 0, 0, 0, 0},
                 {3.0 / 40.0, 9.0 / 40.0, 0, 0, 0},
                 {3.0 / 10.0, -9.0 / 10.0, 6.0 / 5.0, 0, 0},
                 {-11.0 / 54.0, 5.0 / 2.0, -70.0 / 27.0, 35.0 / 27.0, 0},
                 {1631.0 / 55296.0, 175.0 / 512.0, 575.0 / 13824.0,
                  44275.0 / 110592.0, 253.0 / 4096.0}};
float C[6] = {37.0 / 378.0, 0, 250.0 / 621.0, 125.0 / 594.0, 0, 512.0 / 1771.0};
float DC[6] = {37.0 / 378.0 - 2825.0 / 27648.0,
               0,
               250.0 / 621.0 - 18575.0 / 48384.0,
               125.0 / 594.0 - 13525.0 / 55296.0,
               -277.0 / 14336.0,
               512.0 / 1771.0 - 1.0 / 4.0};
std::vector<Vector3f> iGameStreamTracer::seedGenerate(int control,
                                                      float proportion,
                                                      int numOfseed) { // face
    this->mesh = DynamicCast<VolumeMesh>(this->mesh);
    auto allPoints = mesh->GetPoints();
    int numOfPoints = mesh->GetNumberOfPoints();
    std::vector<Vector3f> tem;
    if (mesh == nullptr) { return tem; }
    Point first = allPoints->GetPoint(igIndex(0));
    // Point first = HexMesh->GetPoint(igIndex(0));
    float maxPosition[3] = {first[0], first[1], first[2]};
    float minPosition[3] = {first[0], first[1], first[2]};
    for (int i = 1; i < numOfPoints; i++) {
        // Point a = HexMesh->GetPoint(igIndex(i));
        Point a = allPoints->GetPoint(igIndex(i));

        for (int j = 0; j < 3; j++) {
            maxPosition[j] = (maxPosition[j] > a[j]) ? maxPosition[j] : a[j];
            minPosition[j] = (minPosition[j] < a[j]) ? minPosition[j] : a[j];
        }
    }
    // std::cout << "max is" << maxPosition[0] << " "<<maxPosition[1] << " " <<
    // maxPosition[2] << std::endl; std::cout << "min is" << minPosition[0] << " "
    // << minPosition[1] << " " << minPosition[2] << std::endl;
    float lengthX = maxPosition[0] - minPosition[0];
    float lengthY = maxPosition[1] - minPosition[1];
    float lengthZ = maxPosition[2] - minPosition[2];
    float num = sqrt(numOfseed) - 1;
    for (int i = 0; i < num; i++) {
        for (int j = 0; j < num; j++) {
            float step = 1 / num;
            switch (control) {
                case 0: {
                    tem.emplace_back(
                            Vector3f(minPosition[0] + proportion * lengthX,
                                     minPosition[1] + i * step * lengthY,
                                     minPosition[2] + j * step * lengthZ));
                    break;
                }
                case 1: {
                    tem.emplace_back(
                            Vector3f(minPosition[0] + i * step * lengthX,
                                     minPosition[1] + j * step * lengthY,
                                     minPosition[2] + proportion * lengthZ));
                    break;
                }
                case 2: {
                    tem.emplace_back(
                            Vector3f(minPosition[0] + i * step * lengthX,
                                     minPosition[1] + proportion * lengthY,
                                     minPosition[2] + j * step * lengthZ));
                    break;
                }
                default:
                    break;
            }
        }
    }
    return tem;
}
std::vector<Vector3f>
iGameStreamTracer::streamSeedGenerate(int control, float proportion,
                                      int numOfseed) { // line
    // auto HexMesh =
    // iGameHexMesh::SafeDownCast(model->RenderMeshes[0]);//Subsequent update
    // options
    this->mesh = DynamicCast<VolumeMesh>(this->mesh);
    auto allPoints = mesh->GetPoints();
    int numOfPoints = mesh->GetNumberOfPoints();
    mesh->RequestEditStatus(); // Establishing Adjacency
    std::vector<Vector3f> tem;
    // Point first = HexMesh->GetPoint(igIndex(0));
    Point first = allPoints->GetPoint(igIndex(0));
    float maxPosition[3] = {first[0], first[1], first[2]};
    float minPosition[3] = {first[0], first[1], first[2]};
    for (int i = 1; i < numOfPoints; i++) {
        // Point a = HexMesh->GetPoint(igIndex(i));
        Point a = allPoints->GetPoint(igIndex(i));
        for (int j = 0; j < 3; j++) {
            maxPosition[j] = (maxPosition[j] > a[j]) ? maxPosition[j] : a[j];
            minPosition[j] = (minPosition[j] < a[j]) ? minPosition[j] : a[j];
        }
    }
    // std::cout << "max is" << maxPosition[0] << " "<<maxPosition[1] << " " <<
    // maxPosition[2] << std::endl; std::cout << "min is" << minPosition[0] << " "
    // << minPosition[1] << " " << minPosition[2] << std::endl;
    float lengthX = maxPosition[0] - minPosition[0];
    float lengthY = maxPosition[1] - minPosition[1];
    float lengthZ = maxPosition[2] - minPosition[2];
    float num = numOfseed;
    for (int i = 0; i < num; i++) {

        float step = 1 / num;
        switch (control) {
            case 0: {
                tem.emplace_back(Vector3f(minPosition[0] + proportion * lengthX,
                                          minPosition[1] + i * step * lengthY,
                                          minPosition[2] + 0.5 * lengthZ));
                break;
            }
            case 1: {
                tem.emplace_back(
                        Vector3f(minPosition[0] + i * step * lengthX,
                                 minPosition[1] + 0.5 * lengthY,
                                 minPosition[2] + proportion * lengthZ));
                break;
            }
            case 2: {
                tem.emplace_back(Vector3f(minPosition[0] + i * step * lengthX,
                                          minPosition[1] + proportion * lengthY,
                                          minPosition[2] + 0.5 * lengthZ));
                break;
            }
            default:
                break;
        }
    }
    return tem;
}

std::vector<std::vector<float>> iGameStreamTracer::showStreamLineMix(
        std::vector<Vector3f> seed, std::string vectorName,
        std::vector<std::vector<float>>& streamColor, float lengOfStreamLine,
        float lengthOfStep, float terminalSpeed, int maxSteps) {
    this->mesh = DynamicCast<VolumeMesh>(this->mesh);
    streamColor.clear();
    streamColor.resize(seed.size());
    std::vector<std::vector<float>> tem(seed.size());
    if (mesh == nullptr) return tem;
    std::vector<Vector3f> _vector;
    int numOfPoints = mesh->GetNumberOfPoints();
    clock_t time1 = clock();
    auto PointData = mesh->GetAttributeSet();
    auto Points = mesh->GetPoints();
    auto Vector = PointData->GetVector(vectorName);
    int component = Vector.pointer->GetDimension();
    float MAX_STEP = 0.001, MIN_STEP = 0.0001, ERR = 0.000001;

    for (int i = 0; i < numOfPoints; i++) {
        float v[4] = {0.0f};
        Vector.pointer->GetElement(i, v);
        _vector.emplace_back(Vector3f(v[0], v[1], v[2]));
    }
    // std::cout << clock() - time1 << std::endl;
    // clock_t time1 = clock();
    ThreadPool::Pointer tp = ThreadPool::Instance();

    std::vector<std::future<void>> result(seed.size());

    auto func = [&](int i) -> void {
        // std::cout << i << " start\n";
        // auto time1 = clock();
        int steps = maxSteps;
        auto& _coord = seed[i];
        tem[i].emplace_back(_coord[0]);
        tem[i].emplace_back(_coord[1]);
        tem[i].emplace_back(_coord[2]);
        bool inside = true;
        bool flag = true;
        igIndex flag1 = -1;
        float length = 0;
        while (flag && length < lengOfStreamLine && steps-- > 0) {
            inside = false;
            flag = false;
            bool check = false;
            Vector3f k[7];
            auto temV = interpolationVectorHexWithNatural(
                    _coord, inside, flag1, vectorName, _vector, terminalSpeed);
            k[1] = lengthOfStep * temV;
            streamColor[i].emplace_back(temV[0]);
            streamColor[i].emplace_back(temV[1]);
            streamColor[i].emplace_back(temV[2]);
            if (inside) {
                flag = true;
                inside = false;
            }
            k[2] = lengthOfStep *
                   interpolationVectorMixWithMeanV(_coord + k[1] * B[0][0],
                                                   inside, flag1, vectorName,
                                                   _vector, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[3] = lengthOfStep *
                   interpolationVectorMixWithMeanV(
                           _coord + k[1] * B[1][0] + k[2] * B[1][1], inside,
                           flag1, vectorName, _vector, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[4] = lengthOfStep *
                   interpolationVectorMixWithMeanV(
                           _coord + k[1] * B[2][0] + k[2] * B[2][1] +
                                   k[3] * B[2][2],
                           inside, flag1, vectorName, _vector, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[5] = lengthOfStep *
                   interpolationVectorMixWithMeanV(
                           _coord + k[1] * B[3][0] + k[2] * B[3][1] +
                                   k[3] * B[3][2] + k[4] * B[3][3],
                           inside, flag1, vectorName, _vector, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[6] = lengthOfStep *
                   interpolationVectorMixWithMeanV(
                           _coord + k[1] * B[4][0] + k[2] * B[4][1] +
                                   k[3] * B[4][2] + k[4] * B[4][3] +
                                   k[5] * B[4][4],
                           inside, flag1, vectorName, _vector, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            if (flag) {
                Vector3f temC(0, 0, 0);
                for (int i = 0; i < 6; i++) { temC += k[i + 1] * C[i]; }
                length += temC.length();
                _coord += temC;
            }
            tem[i].emplace_back(_coord[0]);
            tem[i].emplace_back(_coord[1]);
            tem[i].emplace_back(_coord[2]);
            temV = interpolationVectorMixWithMeanV(
                    _coord, inside, flag1, vectorName, _vector, terminalSpeed);
            streamColor[i].emplace_back(temV[0]);
            streamColor[i].emplace_back(temV[1]);
            streamColor[i].emplace_back(temV[2]);
            if (flag && length < lengOfStreamLine && steps != 0) {
                tem[i].emplace_back(_coord[0]);
                tem[i].emplace_back(_coord[1]);
                tem[i].emplace_back(_coord[2]);
            }
        }
        // std::cout << "1" << std::endl;
        // std::cout << i << "end" <<clock()-time1<< std::endl;
    };

    for (int i = 0; i < seed.size(); i++) { result[i] = tp->Commit(func, i); }
    for (int i = 0; i < seed.size(); i++) { result[i].wait(); }
    return tem;
    // result[2] = tp->Commit(func, 2);
    // result[2].wait();

    //	std::cout << "time: " << clock() - time1 << std::endl;
}
std::vector<std::vector<float>> iGameStreamTracer::showStreamLineHex(
        std::vector<Vector3f> seed, std::string vectorName,
        std::vector<std::vector<float>>& streamColor, float lengOfStreamLine,
        float lengthOfStep, float terminalSpeed, int maxSteps) {
    this->mesh = DynamicCast<VolumeMesh>(this->mesh);
    streamColor.clear();
    streamColor.resize(seed.size());
    std::vector<std::vector<float>> tem(seed.size());
    if (mesh == nullptr) return tem;
    std::vector<Vector3f> _vector;
    int numOfPoints = mesh->GetNumberOfPoints();
    clock_t time1 = clock();
    auto PointData = mesh->GetAttributeSet();
    auto Points = mesh->GetPoints();
    auto Vector = PointData->GetVector(vectorName);
    int component = Vector.pointer->GetDimension();
    float MAX_STEP = 0.001, MIN_STEP = 0.0001, ERR = 0.000001;

    for (int i = 0; i < numOfPoints; i++) {
        float v[8] = {0.0f};
        Vector.pointer->GetElement(i, v);
        _vector.emplace_back(Vector3f(v[0], v[1], v[2]));
    }
    // std::cout << clock() - time1 << std::endl;
    // clock_t time1 = clock();
    ThreadPool::Pointer tp = ThreadPool::Instance();

    std::vector<std::future<void>> result(seed.size());

    auto func = [&](int i) -> void {
        // std::cout << i << " start\n";
        // auto time1 = clock();
        int steps = maxSteps;
        auto& _coord = seed[i];
        tem[i].emplace_back(_coord[0]);
        tem[i].emplace_back(_coord[1]);
        tem[i].emplace_back(_coord[2]);
        bool inside = true;
        bool flag = true;
        igIndex flag1 = -1;
        float length = 0;
        while (flag && length < lengOfStreamLine && steps-- > 0) {
            inside = false;
            flag = false;
            bool check = false;
            Vector3f k[7];
            auto temV = interpolationVectorHexWithNatural(
                    _coord, inside, flag1, vectorName, _vector, terminalSpeed);
            k[1] = lengthOfStep * temV;
            streamColor[i].emplace_back(temV[0]);
            streamColor[i].emplace_back(temV[1]);
            streamColor[i].emplace_back(temV[2]);
            if (inside) {
                flag = true;
                inside = false;
            }
            k[2] = lengthOfStep *
                   interpolationVectorHexWithNatural(_coord + k[1] * B[0][0],
                                                     inside, flag1, vectorName,
                                                     _vector, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[3] = lengthOfStep *
                   interpolationVectorHexWithNatural(
                           _coord + k[1] * B[1][0] + k[2] * B[1][1], inside,
                           flag1, vectorName, _vector, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[4] = lengthOfStep *
                   interpolationVectorHexWithNatural(
                           _coord + k[1] * B[2][0] + k[2] * B[2][1] +
                                   k[3] * B[2][2],
                           inside, flag1, vectorName, _vector, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[5] = lengthOfStep *
                   interpolationVectorHexWithNatural(
                           _coord + k[1] * B[3][0] + k[2] * B[3][1] +
                                   k[3] * B[3][2] + k[4] * B[3][3],
                           inside, flag1, vectorName, _vector, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[6] = lengthOfStep *
                   interpolationVectorHexWithNatural(
                           _coord + k[1] * B[4][0] + k[2] * B[4][1] +
                                   k[3] * B[4][2] + k[4] * B[4][3] +
                                   k[5] * B[4][4],
                           inside, flag1, vectorName, _vector, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            if (flag) {
                Vector3f temC(0, 0, 0);
                for (int i = 0; i < 6; i++) { temC += k[i + 1] * C[i]; }
                length += temC.length();
                _coord += temC;
            }
            tem[i].emplace_back(_coord[0]);
            tem[i].emplace_back(_coord[1]);
            tem[i].emplace_back(_coord[2]);
            temV = interpolationVectorHexWithNatural(
                    _coord, inside, flag1, vectorName, _vector, terminalSpeed);
            streamColor[i].emplace_back(temV[0]);
            streamColor[i].emplace_back(temV[1]);
            streamColor[i].emplace_back(temV[2]);
            if (flag && length < lengOfStreamLine && steps != 0) {
                tem[i].emplace_back(_coord[0]);
                tem[i].emplace_back(_coord[1]);
                tem[i].emplace_back(_coord[2]);
            }
        }
        // std::cout << "1" << std::endl;
        // std::cout << i << "end" <<clock()-time1<< std::endl;
    };

    for (int i = 0; i < seed.size(); i++) { result[i] = tp->Commit(func, i); }
    for (int i = 0; i < seed.size(); i++) { result[i].wait(); }

    // result[2] = tp->Commit(func, 2);
    // result[2].wait();

    //	std::cout << "time: " << clock() - time1 << std::endl;
    return tem;
}
std::vector<std::vector<float>> iGameStreamTracer::showStreamLineCellData(
        std::vector<Vector3f> seed, std::string vectorName,
        std::vector<std::vector<float>>& streamColor, float lengOfStreamLine,
        float lengthOfStep, float terminalSpeed, int maxSteps) {
    CellData2PointData(vectorName);
    this->mesh->GetAttributeSet()->TransformScalars2VectorArray();
    this->mesh = DynamicCast<VolumeMesh>(this->mesh);
    auto allPolyhedrons = mesh->GetVolumes();
    auto allPoints = mesh->GetPoints();
    auto allFaces = mesh->GetFaces();
    auto numOfFaces = mesh->GetNumberOfFaces();
    auto numOfPoints = mesh->GetNumberOfPoints();
    streamColor.clear();
    streamColor.resize(seed.size());
    std::cout << "Vector is" << vectorName << std::endl;
    std::vector<std::vector<float>> tem(seed.size());
    if (mesh == nullptr) return tem;
    std::vector<Vector3f> _vector;
    auto Vec = mesh->GetAttributeSet();
    auto vec = Vec->GetVector(0);
    int numOfCells = mesh->GetNumberOfVolumes();
    for (int i = 0; i < numOfPoints; i++) {
        float VV[3] = {0.0f};
        vec.pointer->GetElement(i, VV);
        _vector.emplace_back(VV[0], VV[1], VV[2]);
    }
    std::vector<std::vector<int>> f2c(numOfFaces);
    std::vector<std::vector<int>> c2c(numOfCells);
    igIndex vhs[256];
    igIndex fhs[256];
    igIndex vcnt, fcnt = 0;
    for (int i = 0; i < numOfCells; i++) {
        int size = mesh->GetVolumeFaceIds(i, fhs);
        for (int j = 0; j < size; j++) { 
            int temIndex = fhs[j];
            f2c[temIndex].emplace_back(i);
        }
    }
    for (int i = 0; i < numOfCells; i++) {
        int size = mesh->GetVolumeFaceIds(i, fhs);
        for (int j = 0; j < size; j++) {
            auto chs = f2c[fhs[j]];
            for (int k = 0; k < chs.size(); k++) {
                if (chs[k] != i) { c2c[i].emplace_back(chs[k]); }
            }
        }
    }

    clock_t time1 = clock();
    float MAX_STEP = 0.001, MIN_STEP = 0.0001, ERR = 0.000001;
    // std::cout << clock() - time1 << std::endl;
    // clock_t time1 = clock();
    ThreadPool* tp = ThreadPool::Instance();

    std::vector<std::future<void>> result(seed.size());

    auto func = [&](int i) -> void {
        // std::cout << i << " start\n";
        // auto time1 = clock();
        int steps = maxSteps;
        auto& _coord = seed[i];
        tem[i].emplace_back(_coord[0]);
        tem[i].emplace_back(_coord[1]);
        tem[i].emplace_back(_coord[2]);
        bool inside = true;
        bool flag = true;
        igIndex flag1 = -1;
        float length = 0;
        while (flag && length < lengOfStreamLine && steps-- > 0) {
            inside = false;
            flag = false;
            bool check = false;
            Vector3f k[7];
            auto temV = interpolationVectorMixWithMeanV(
                    _coord, inside, flag1, vectorName, _vector, terminalSpeed);
            k[1] = lengthOfStep * temV;
            streamColor[i].emplace_back(temV[0]);
            streamColor[i].emplace_back(temV[1]);
            streamColor[i].emplace_back(temV[2]);
            if (inside) {
                flag = true;
                inside = false;
            }
            k[2] = lengthOfStep *
                   interpolationVectorMixWithMeanV(_coord + k[1] * B[0][0],
                                                   inside, flag1, vectorName,
                                                   _vector, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[3] = lengthOfStep *
                   interpolationVectorMixWithMeanV(
                           _coord + k[1] * B[1][0] + k[2] * B[1][1], inside,
                           flag1, vectorName, _vector, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[4] = lengthOfStep *
                   interpolationVectorMixWithMeanV(
                           _coord + k[1] * B[2][0] + k[2] * B[2][1] +
                                   k[3] * B[2][2],
                           inside, flag1, vectorName, _vector, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[5] = lengthOfStep *
                   interpolationVectorMixWithMeanV(
                           _coord + k[1] * B[3][0] + k[2] * B[3][1] +
                                   k[3] * B[3][2] + k[4] * B[3][3],
                           inside, flag1, vectorName, _vector, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[6] = lengthOfStep *
                   interpolationVectorMixWithMeanV(
                           _coord + k[1] * B[4][0] + k[2] * B[4][1] +
                                   k[3] * B[4][2] + k[4] * B[4][3] +
                                   k[5] * B[4][4],
                           inside, flag1, vectorName, _vector, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            if (flag) {
                Vector3f temC(0, 0, 0);
                for (int i = 0; i < 6; i++) { temC += k[i + 1] * C[i]; }
                length += temC.length();
                _coord += temC;
            }
            tem[i].emplace_back(_coord[0]);
            tem[i].emplace_back(_coord[1]);
            tem[i].emplace_back(_coord[2]);
            temV = interpolationVectorMixWithMeanV(
                    _coord, inside, flag1, vectorName, _vector, terminalSpeed);
            streamColor[i].emplace_back(temV[0]);
            streamColor[i].emplace_back(temV[1]);
            streamColor[i].emplace_back(temV[2]);
            if (flag && length < lengOfStreamLine && steps != 0) {
                tem[i].emplace_back(_coord[0]);
                tem[i].emplace_back(_coord[1]);
                tem[i].emplace_back(_coord[2]);
            }
        }
        // std::cout << "1" << std::endl;
        // std::cout << i << "end" <<clock()-time1<< std::endl;
    };

    for (int i = 0; i < seed.size(); i++) { result[i] = tp->Commit(func, i); }
    for (int i = 0; i < seed.size(); i++) { result[i].wait(); }

    // result[41] = tp->Commit(func, 41);
    // result[41].wait();

    //	std::cout << "time: " << clock() - time1 << std::endl;
    return tem;
}
std::vector<std::vector<std::vector<float>>> iGameStreamTracer::showStreamFace(
        std::vector<Vector3f> seed, std::string vectorName,
        std::vector<std::vector<std::vector<float>>>& streamColor,
        float lengOfStreamLine, float lengthOfStep, float terminalSpeed,
        int maxSteps) {
    this->mesh = DynamicCast<VolumeMesh>(this->mesh);
    streamColor.clear();
    std::vector<std::vector<std::vector<float>>> tem;
    std::vector<Vector3f> seedTem = seed;
    std::cout << "temsize" << seedTem.size() << std::endl;
    if (mesh == nullptr) return tem;
    std::vector<Vector3f> _vector;
    int numOfPoints = mesh->GetNumberOfPoints();
    clock_t time1 = clock();
    auto PointData = mesh->GetAttributeSet();
    auto Vector = PointData->GetVector(vectorName);
    int component = Vector.pointer->GetDimension();
    float MAX_STEP = 0.001, MIN_STEP = 0.0001, ERR = 0.000001;
    int initial = 0;
    float* Length = new float[seedTem.size()];
    int MAX_SEED = seed.size() * 2;
    std::vector<int> tem_strip;
    for (int i = 0; i < seedTem.size(); i++) {
        Length[i] = 0;
        if (i != seedTem.size() - 1) { tem_strip.emplace_back(i); }
    }
    // mesh->streamFaceStrip.emplace_back(tem_strip);
    for (int i = 0; i < numOfPoints; i++) {
        float v[8] = {0.0f};
        Vector.pointer->GetElement(i, v);
        _vector.emplace_back(Vector3f(v[0], v[1], v[2]));
    }
    // std::cout << clock() - time1 << std::endl;
    // clock_t time1 = clock();
    ThreadPool* tp = ThreadPool::Instance();
    std::vector<std::vector<float>> temFace;
    std::vector<std::vector<float>> temColor;
    auto func = [&](int i, float* length, int index) -> void {
        // std::cout << i << " start\n";
        // auto time1 = clock();
        int steps;
        if (maxSteps >= 100) {
            steps = 100;
        } else {
            steps = maxSteps;
        }
        auto& _coord = seedTem[i];
        temFace[i].emplace_back(_coord[0]);
        temFace[i].emplace_back(_coord[1]);
        temFace[i].emplace_back(_coord[2]);
        bool inside = true;
        bool flag = true;
        igIndex flag1 = -1;
        while (flag && steps-- > 0) {
            inside = false;
            flag = false;
            bool check = false;
            Vector3f k[7];
            auto temV = interpolationVectorHexWithNatural(
                    _coord, inside, flag1, vectorName, _vector, terminalSpeed);
            k[1] = lengthOfStep * temV;
            temColor[i].emplace_back(temV[0]);
            temColor[i].emplace_back(temV[1]);
            temColor[i].emplace_back(temV[2]);
            if (inside) {
                flag = true;
                inside = false;
            }
            k[2] = lengthOfStep *
                   interpolationVectorHexWithNatural(_coord + k[1] * B[0][0],
                                                     inside, flag1, vectorName,
                                                     _vector, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[3] = lengthOfStep *
                   interpolationVectorHexWithNatural(
                           _coord + k[1] * B[1][0] + k[2] * B[1][1], inside,
                           flag1, vectorName, _vector, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[4] = lengthOfStep *
                   interpolationVectorHexWithNatural(
                           _coord + k[1] * B[2][0] + k[2] * B[2][1] +
                                   k[3] * B[2][2],
                           inside, flag1, vectorName, _vector, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[5] = lengthOfStep *
                   interpolationVectorHexWithNatural(
                           _coord + k[1] * B[3][0] + k[2] * B[3][1] +
                                   k[3] * B[3][2] + k[4] * B[3][3],
                           inside, flag1, vectorName, _vector, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            k[6] = lengthOfStep *
                   interpolationVectorHexWithNatural(
                           _coord + k[1] * B[4][0] + k[2] * B[4][1] +
                                   k[3] * B[4][2] + k[4] * B[4][3] +
                                   k[5] * B[4][4],
                           inside, flag1, vectorName, _vector, terminalSpeed);
            if (inside) {
                flag = true;
                inside = false;
            }

            if (flag) {
                Vector3f temC(0, 0, 0);
                for (int i = 0; i < 6; i++) { temC += k[i + 1] * C[i]; }
                length[i] += temC.length();
                _coord += temC;
            }
            temFace[i].emplace_back(_coord[0]);
            temFace[i].emplace_back(_coord[1]);
            temFace[i].emplace_back(_coord[2]);
            temV = interpolationVectorHexWithNatural(
                    _coord, inside, flag1, vectorName, _vector, terminalSpeed);
            temColor[i].emplace_back(temV[0]);
            temColor[i].emplace_back(temV[1]);
            temColor[i].emplace_back(temV[2]);
            if (flag && steps != 0) {
                temFace[i].emplace_back(_coord[0]);
                temFace[i].emplace_back(_coord[1]);
                temFace[i].emplace_back(_coord[2]);
            }
        }
        // std::cout << "1" << std::endl;
        // std::cout << i << "end" <<clock()-time1<< std::endl;
    };
    float* temLength = nullptr;
    temLength = new float[1];
    temLength[0] = 0;
    while (maxSteps > 0 && !seedTem.empty()) {
        int nOfSeed = seedTem.size();
        std::vector<std::future<void>> result(nOfSeed);
        temFace.clear();
        temColor.clear();
        tem_strip.clear();
        temFace.resize(nOfSeed);
        temColor.resize(nOfSeed);
        int lineNum = 0;
        for (int i = 0; i < nOfSeed; i++) {
            result[i] = tp->Commit(func, i, Length, initial);
        }
        for (int i = 0; i < nOfSeed; i++) { result[i].wait(); }
        tem.emplace_back(temFace);
        streamColor.emplace_back(temColor);
        seedTem.clear();
        delete[] temLength;
        temLength = new float[nOfSeed * 2];
        for (int i = 0; i < nOfSeed * 2; i++) { temLength[i] = 0; }
        bool* seedIsEnd = new bool[nOfSeed];
        for (int i = 0; i < nOfSeed; i++) { seedIsEnd[i] = true; }
        for (int i = 0; i < nOfSeed - 1; i++) {
            int num1 = tem[initial][i].size(),
                num2 = tem[initial][i + 1].size();
            // if (num1 == 600 && num2 == 600) {
            if (true) {
                Vector3f v00(tem[initial][i][num1 - 6],
                             tem[initial][i][num1 - 5],
                             tem[initial][i][num1 - 4]);
                Vector3f v10(tem[initial][i + 1][num2 - 6],
                             tem[initial][i + 1][num2 - 5],
                             tem[initial][i + 1][num2 - 4]);
                Vector3f v01(tem[initial][i][num1 - 3],
                             tem[initial][i][num1 - 2],
                             tem[initial][i][num1 - 1]);
                Vector3f v11(tem[initial][i + 1][num2 - 3],
                             tem[initial][i + 1][num2 - 2],
                             tem[initial][i + 1][num2 - 1]);
                float H = std::min(distance2Line(v01, v00, v10),
                                   distance2Line(v11, v00, v10));
                if ((v01 - v11).length() > 2 * H && (lineNum < MAX_SEED)) {
                    tem_strip.emplace_back(lineNum);
                    if (seedIsEnd[i]) {
                        seedTem.emplace_back(v01);
                        temLength[lineNum++] = Length[i];
                    }
                    seedTem.emplace_back((v01 + v11) / 2);
                    tem_strip.emplace_back(lineNum);
                    temLength[lineNum++] = (Length[i + 1] + Length[i]) / 2;
                    seedTem.emplace_back(v11);
                    temLength[lineNum++] = Length[i + 1];
                    seedIsEnd[i + 1] = false;
                } else {
                    tem_strip.emplace_back(lineNum);
                    if (seedIsEnd[i]) {
                        seedTem.emplace_back(v01);
                        temLength[lineNum++] = Length[i];
                    }
                }
            }
        }

        maxSteps -= 100;
        delete[] seedIsEnd;
        initial++;
        delete[] Length;
        Length = temLength;
        float checkLength = 0;
        std::cout << "temsize" << seedTem.size() << std::endl;
        for (int i = 0; i < seedTem.size(); i++) { checkLength += Length[i]; }
        if (checkLength > (lengOfStreamLine * seedTem.size())) {
            seedTem.clear();
        }
        //	mesh->streamFaceStrip.emplace_back(tem_strip);
    }
    delete[] Length;
    // result[41] = tp->Commit(func, 41);
    // result[41].wait();

    //	std::cout << "time: " << clock() - time1 << std::endl;
    return tem;
}
bool iGameStreamTracer::CellData2PointData(std::string vectorName) {
    this->mesh = DynamicCast<VolumeMesh>(this->mesh);
    auto allPolyhedrons = mesh->GetVolumes();
    auto allPoints = mesh->GetPoints();
    auto numOfPoints = mesh->GetNumberOfPoints();
    auto numOfCells = mesh->GetNumberOfVolumes();
    auto Vec = mesh->GetAttributeSet();
    Vec->TransformScalars2VectorArray();
    auto Scalars = Vec->GetAllAttributes();
    int size = Scalars->GetNumberOfElements();
    for (int i = 0; i < size; i++) {
        auto scalarDataArray = Scalars->GetElement(i);
        if (scalarDataArray.type==IG_VECTOR)
        std::cout << "type is a" << scalarDataArray.attachmentType << std::endl;
    }
    auto vec = Vec->GetVector(1);
    // how much D
    if (vec.attachmentType != IG_CELL) { 
        std::cout << vec.attachmentType << std::endl;
        return false; }
    auto vecV = vec.pointer->GetDimension();
    std::vector<Vector3f> pointVector(numOfPoints);
    std::vector<int> pointVectorNUM(
            numOfPoints); // The number of volumes to which the point belongs
    igIndex vhs[256];
    igIndex fhs[256];
    igIndex vcnt, fcnt = 0;
    for (int i = 0; i < numOfCells; i++) {
        float VV[3] = {0.0f};
        vec.pointer->GetElement(i, VV);
        Vector3f temVec(VV[0], VV[1], VV[2]);
        auto volume = mesh->GetVolume(i);
        int numOfCellPoints = volume->GetCellSize(); // The number of point
        for (int j = 0; j < numOfCellPoints; j++) {
            auto pointId = volume->GetPointId(j);
            pointVector[pointId] = pointVector[pointId] + temVec;
            pointVectorNUM[pointId]++;
        }
    }
    FloatArray::Pointer VectorData = FloatArray::New();
    VectorData->SetDimension(3);
    VectorData->SetName(vectorName);
    for (int i = 0; i < numOfPoints; i++) {
        for (int j = 0; j < vecV; j++) {
            VectorData->AddValue(pointVector[i][j] / pointVectorNUM[i]);
        }
    }
    return true;
}
float iGameStreamTracer::distance2Line(Vector3f point, Vector3f lineP1,
                                       Vector3f lineP2) {
    Vector3f pP1 = point - lineP1;
    Vector3f line = lineP2 - lineP1;
    return abs((pP1 * line) / line.length());
}
Vector3f iGameStreamTracer::interpolationVectorTri(
        Vector3f coord, bool& inside, igIndex& VolumeId, std::string vectorName,
        std::vector<Vector3f> _vector, float terminalSpeed) { // Interpolation
    this->mesh = DynamicCast<VolumeMesh>(this->mesh);
    Vector3f finnal = Vector3f(0, 0, 0);
    std::vector<igIndex> tem;
    if (VolumeId == -1) {
        // if (true) {
        int numOfPoints = mesh->GetNumberOfPoints();
        int numOfVolumes = mesh->GetNumberOfVolumes();
        for (int i = 0; i < numOfPoints; i++) {
            Point a = mesh->GetPoint(i);
            if (a == coord) { return _vector[i]; }
        }
        for (int i = 0; i < numOfVolumes; i++) { tem.emplace_back(i); }
    } else {
        igIndex link[30];
        int nearNum = mesh->GetVolumeToNeighborVolumesWithPoint(VolumeId, link);
        for (int i = 0; i < nearNum; i++) { tem.emplace_back(link[i]); }
        tem.emplace_back(VolumeId);
    }
    VolumeId = -1;
    for (auto& c: tem) {
        if (mesh == nullptr) { return finnal; }
        int contactPointNum = 0;
        auto* volume = mesh->GetVolume(c);
        for (int i = 0; i < 4; i++) {
            auto* f = volume->GetFace(i);

            auto& v0 = f->GetPoint(0);
            auto& v1 = f->GetPoint(1);
            auto& v2 = f->GetPoint(2);
            if (checkContact(coord, v0, v1, v2)) contactPointNum++;
        }
        if (contactPointNum % 2 == 1) {
            inside = true;
            VolumeId = c;
            break;
        }
    }
    if (!inside) { return finnal; }
    std::vector<Point> v(8);
    auto* volume = mesh->GetVolume(VolumeId);
    for (int i = 0; i < 4; i++) { v[i] = volume->GetPoint(i); }
    float weights[4];
    weights[0] = pointToFaceDis(coord, v[1], v[2], v[3]) /
                 pointToFaceDis(v[0], v[1], v[2], v[3]);
    weights[1] = pointToFaceDis(coord, v[0], v[2], v[3]) /
                 pointToFaceDis(v[1], v[1], v[2], v[3]);
    weights[2] = pointToFaceDis(coord, v[3], v[1], v[0]) /
                 pointToFaceDis(v[2], v[1], v[2], v[3]);
    weights[3] = pointToFaceDis(coord, v[1], v[2], v[0]) /
                 pointToFaceDis(v[3], v[1], v[2], v[3]);
    for (int i = 0; i < 4; i++) {
        finnal += _vector[volume->GetPointId(i)] * weights[i];
    }
    if (finnal.length() < terminalSpeed) { inside = false; }
    return finnal;
}
Vector3f iGameStreamTracer::interpolationVectorHexWithNatural(
        Vector3f coord, bool& inside, igIndex& VolumeId, std::string vectorName,
        std::vector<Vector3f> _vector, float terminalSpeed) { // Interpolation
    this->mesh = DynamicCast<VolumeMesh>(this->mesh);
    Vector3f finnal = Vector3f(0, 0, 0);
    std::vector<igIndex> tem;
    if (VolumeId == -1) {
        // if (true) {
        int numOfPoints = mesh->GetNumberOfPoints();
        int numOfVolumes = mesh->GetNumberOfVolumes();
        for (int i = 0; i < numOfPoints; i++) {
            Point a = mesh->GetPoint(i);
            if (a == coord) { return _vector[i]; }
        }
        for (int i = 0; i < numOfVolumes; i++) { tem.emplace_back(i); }
    } else {
        igIndex nearVolume[128];
        auto nearNum =
                mesh->GetVolumeToNeighborVolumesWithPoint(VolumeId, nearVolume);
        for (int i = 0; i < nearNum; i++) { tem.emplace_back(nearVolume[i]); }
        tem.emplace_back(VolumeId);
    }
    VolumeId = -1;
    for (auto& c: tem) {
        if (mesh == nullptr) { return finnal; }
        int contactPointNum = 0;
        // Hexahedron *volume = dynamic_cast<Hexahedron*>(mesh->GetVolume(c));
        igIndex volume[32]{};
        igIndex f[32]{};
        int size = mesh->GetVolumeFaceIds(c, volume);
        for (int i = 0; i < 6; i++) {
            int fsize = mesh->GetFacePointIds(volume[i], f);
            for (int j = 0; j < 4; j = j + 2) {
                auto& v0 = mesh->GetPoint(f[j % 4]);
                auto& v1 = mesh->GetPoint(f[(j + 1) % 4]);
                auto& v2 = mesh->GetPoint(f[(j + 2) % 4]);

                if (checkContact(coord, v0, v1, v2)) contactPointNum++;
            }
        }
        if (contactPointNum % 2 == 1) {
            inside = true;
            VolumeId = c;
            break;
        }
    }
    if (!inside) { return finnal; }
    std::vector<Point> v(8);
    igIndex volume[32]{};
    int size = mesh->GetVolumePointIds(VolumeId, volume);
    for (int i = 0; i < 8; i++) { v[i] = mesh->GetPoint(volume[i]); }
    double params[3] = {0.5, 0.5, 0.5};
    double derivs[24];
    double pcoords[3];
    int diagonals[4][2] = {{0, 6}, {1, 7}, {2, 4}, {3, 5}};
    double longestDiagonal = 0;
    double weights[8];
    for (int i = 0; i < 4; i++) {

        double d2 = (v[diagonals[i][0]] - v[diagonals[i][1]]).length();
        if (longestDiagonal < d2) { longestDiagonal = d2; }
    }
    double volumeBound = longestDiagonal * std::sqrt(longestDiagonal);
    double determinantTolerance =
            1e-20 < .00001 * volumeBound ? 1e-20 : .00001 * volumeBound;
    pcoords[0] = pcoords[1] = pcoords[2] = 0.5;
    int iteration, converged = 0;
    for (iteration = 0; !converged && (iteration < 10); iteration++) {
        InterpolationFunctions(pcoords, weights);
        InterpolationDerivs(pcoords, derivs);

        double fcol[3] = {0, 0, 0}, rcol[3] = {0, 0, 0}, scol[3] = {0, 0, 0},
               tcol[3] = {0, 0, 0};
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 3; j++) {

                fcol[j] += v[i][j] * weights[i];
                rcol[j] += v[i][j] * derivs[i];
                scol[j] += v[i][j] * derivs[i + 8];
                tcol[j] += v[i][j] * derivs[i + 16];
            }
        }
        for (int i = 0; i < 3; i++) { fcol[i] -= coord[i]; }
        double d = Determinant3x3(rcol, scol, tcol);
        if (fabs(d) < determinantTolerance) {
            inside = false;
            return finnal;
        }
        pcoords[0] = params[0] - Determinant3x3(fcol, scol, tcol) / d;
        pcoords[1] = params[1] - Determinant3x3(rcol, fcol, tcol) / d;
        pcoords[2] = params[2] - Determinant3x3(rcol, scol, fcol) / d;
        if (((fabs(pcoords[0] - params[0])) < 1.e-05) &&
            ((fabs(pcoords[1] - params[1])) < 1.e-05) &&
            ((fabs(pcoords[2] - params[2])) < 1.e-05)) {
            converged = 1;
        } else if ((fabs(pcoords[0]) > 10000) || (fabs(pcoords[1]) > 10000) ||
                   (fabs(pcoords[2]) > 10000)) {
            inside = false;
            return finnal;
        }

        //  if not converged, repeat
        else {
            params[0] = pcoords[0];
            params[1] = pcoords[1];
            params[2] = pcoords[2];
        }
    }
    if (!converged) {
        inside = false;
        return finnal;
    }
    InterpolationFunctions(pcoords, weights);
    for (int i = 0; i < 8; i++) {

        finnal += _vector[volume[i]] * weights[i];
    }
    if (finnal.length() < terminalSpeed) { inside = false; }
    // return finnal * longestDiagonal;
    return finnal;
}
Vector3f iGameStreamTracer::interpolationVectorMixWithMeanV(
        Vector3f coord, bool& inside, igIndex& VolumeId, std::string vectorName,
        std::vector<Vector3f> _vector, float terminalSpeed) { // Interpolation
    this->mesh = DynamicCast<VolumeMesh>(this->mesh);
    Vector3f finnal = Vector3f(0, 0, 0);
    std::vector<igIndex> tem;
    if (VolumeId == -1) {
        // if (true) {
        int numOfPoints = mesh->GetNumberOfPoints();
        int numOfVolumes = mesh->GetNumberOfVolumes();
        for (int i = 0; i < numOfPoints; i++) {
            Point a = mesh->GetPoint(i);
            if ((a - coord).length() <= 0.001) { return _vector[i]; }
        }
        if (ptFinder) {
            igIndex temPointId = ptFinder->FindClosestPoint(coord);
            igIndex nearVolume[128];
            int find = mesh->GetPointToNeighborVolumes(temPointId, nearVolume);
            for (int i = 0; i < find; i++) { tem.emplace_back(nearVolume[i]); }
        }

    } else {
        igIndex nearVolume[128];
        auto nearNum =
                mesh->GetVolumeToNeighborVolumesWithPoint(VolumeId, nearVolume);
        for (int i = 0; i < nearNum; i++) { tem.emplace_back(nearVolume[i]); }
        tem.emplace_back(VolumeId);
    }
    VolumeId = -1;
    for (auto& c: tem) {
        if (mesh == nullptr) { return finnal; }
        int contactPointNum = 0;
        // Hexahedron *volume = dynamic_cast<Hexahedron*>(mesh->GetVolume(c));
        igIndex volume[32]{};
        igIndex f[32]{};
        int size = mesh->GetVolumeFaceIds(c, volume);
        for (int i = 0; i < size; i++) {
            int fsize = mesh->GetFacePointIds(volume[i], f);
            for (int j = 1; j < fsize - 1; j++) {
                auto& v0 = mesh->GetPoint(f[0]);
                auto& v1 = mesh->GetPoint(f[j]);
                auto& v2 = mesh->GetPoint(f[(j + 1)]);

                if (checkContact(coord, v0, v1, v2)) contactPointNum++;
            }
        }
        if (contactPointNum % 2 == 1) {
            inside = true;
            VolumeId = c;
            break;
        }
    }
    if (!inside) { return finnal; }
    igIndex volume[32]{};
    igIndex face[32]{};
    igIndex p[32]{};
    int size = mesh->GetVolumePointIds(VolumeId, volume);
    int fsize = mesh->GetVolumeFaceIds(VolumeId, face);
    std::vector<float> weights(size);
    int MaxPolygonSize = 0;
    for (int i = 0; i < fsize; i++) {
        int fpsize = mesh->GetFacePointIds(face[i], p);
        MaxPolygonSize = std::max(fpsize, MaxPolygonSize);
    }
    weights = ComputeWeightsForPolygonMesh(volume, coord, face, MaxPolygonSize,
                                           size, fsize);
    for (int i = 0; i < size; i++) {

        finnal += _vector[volume[i]] * weights[i];
    }
    if (finnal.length() < terminalSpeed) { inside = false; }
    // return finnal * longestDiagonal;
    return finnal;
}
std::vector<float> iGameStreamTracer::ComputeWeightsForPolygonMesh(
        igIndex* PointIds, Vector3f coord, igIndex* FaceIds, int MaxPolygonSize,
        int psize, int fsize) {
    std::vector<float> weights(psize, 0);
    // 点到点源的长度
    std::vector<double> dist(psize);
    // 点到点源的单位矢量
    std::vector<Vector3f> uVec(psize);
    static constexpr double eps = 0.00000001;
    for (int pid = 0; pid < psize; ++pid) {

        auto pt = mesh->GetPoint(PointIds[pid]);
        uVec[pid] =
                Vector3f(pt[0] - coord[0], pt[1] - coord[1], pt[2] - coord[2]);
        // distance
        dist[pid] = uVec[pid].length();

		if (dist[pid] < eps)
		{
			weights[pid] = 1.0;
			return weights;
		}

		uVec[pid] /= dist[pid];
	}

	//点到多面体内点源的单位矢量
	std::vector<Vector3f> u(MaxPolygonSize);
	//点在体单元内的局部id
	std::vector<int> uIdInVolume(MaxPolygonSize);
	std::vector<double> alpha(MaxPolygonSize);
	std::vector<double> theta(MaxPolygonSize);

    int poly = 0;
    while (poly < fsize) {
        igIndex fp[32];
        int fpsize = mesh->GetFacePointIds(FaceIds[poly], fp);
        if (fpsize == 0) {
            poly++;
            continue;
        }

        for (int j = 0; j < fpsize; j++) {
            for (int i = 0; i < psize; i++) {
                int temid = fp[j];
                if (PointIds[i] == temid) u[j] = uVec[i];
                uIdInVolume[j] = i;
            }
        }

        // unit vector v.
        Vector3f v(0, 0, 0);
        double l;
        double angle;
        Vector3f temp;
        for (int j = 0; j < fpsize - 1; j++) {
            temp = u[j].cross(u[j + 1]);
            temp.normalize();

            l = (u[j] - u[j + 1]).length();
            angle = 2.0 * asin(l / 2.0);

			v[0] += 0.5 * angle * temp[0];
			v[1] += 0.5 * angle * temp[1];
			v[2] += 0.5 * angle * temp[2];
		}
		l = (u[fpsize - 1] - u[0]).length();
		angle = 2.0 * asin(l / 2.0);
		temp = u[fpsize - 1].cross(u[0]);
		temp.normalize();
		v[0] += 0.5 * angle * temp[0];
		v[1] += 0.5 * angle * temp[1];
		v[2] += 0.5 * angle * temp[2];
		double vNorm = v.length();
		v.normalize();
		if (v.dot(u[0]) < 0)
		{
			v[0] = -v[0];
			v[1] = -v[1];
			v[2] = -v[2];
		}

        Vector3f n0, n1;
        for (int j = 0; j < fpsize - 1; j++) {
            n0 = u[j].cross(v);
            n0.normalize();
            n1 = u[j + 1].cross(v);
            n1.normalize();
            l = (n0 - n1).length();
            alpha[j] = 2.0 * asin(l / 2.0);
            temp = n0.cross(n1);
            if (temp.dot(v) < 0) { alpha[j] = -alpha[j]; }
            l = (u[j] - v).length();
            theta[j] = 2.0 * asin(l / 2.0);
        }

        n0 = u[fpsize - 1].cross(v);
        n0.normalize();
        n1 = u[0].cross(v);
        n1.normalize();
        l = (n0 - n1).length();
        alpha[fpsize - 1] = 2.0 * asin(l / 2.0);
        temp = n0.cross(n1);
        if (temp.dot(v) < 0) { alpha[fpsize - 1] = -alpha[fpsize - 1]; }

		l = (u[fpsize - 1] - v).length();
		theta[fpsize - 1] = 2.0 * asin(l / 2.0);

        bool outlierFlag = false;
        for (int j = 0; j < fpsize; j++) {
            if (fabs(theta[j]) < eps) {
                outlierFlag = true;
                weights[uIdInVolume[j]] += vNorm / dist[uIdInVolume[j]];
                break;
            }
        }

        if (outlierFlag) {
            poly++;
            continue;
        }

        double sum = 0.0;
        sum += 1.0 / tan(theta[0]) *
               (tan(alpha[0] / 2.0) + tan(alpha[fpsize - 1] / 2.0));
        for (int j = 1; j < fpsize; j++) {
            sum += 1.0 / tan(theta[j]) *
                   (tan(alpha[j] / 2.0) + tan(alpha[j - 1] / 2.0));
        }

		if (fabs(sum) < eps)
		{
			weights.assign(psize, 0.0);
			for (int j = 0; j < fpsize - 1; j++)
			{
				l = (u[j] - u[j + 1]).length();
				theta[j] = 2.0 * asin(l / 2.0);
			}
			l = (u[fpsize - 1] - u[0]).length();
			theta[fpsize - 1] = 2.0 * asin(l / 2.0);

            double sumWeight;
            weights[uIdInVolume[0]] =
                    1.0 / dist[uIdInVolume[0]] *
                    (tan(theta[fpsize - 1] / 2.0) + tan(theta[0] / 2.0));
            sumWeight = weights[uIdInVolume[0]];
            for (int j = 1; j < fpsize; j++) {
                weights[uIdInVolume[j]] =
                        1.0 / dist[uIdInVolume[j]] *
                        (tan(theta[j - 1] / 2.0) + tan(theta[j] / 2.0));
                sumWeight = sumWeight + weights[uIdInVolume[j]];
            }

            if (sumWeight < eps) { return weights; }

            for (int j = 0; j < fpsize; j++) {
                weights[uIdInVolume[j]] /= sumWeight;
            }

            return weights;
        }

        // weight
        weights[uIdInVolume[0]] +=
                vNorm / sum / dist[uIdInVolume[0]] / sin(theta[0]) *
                (tan(alpha[0] / 2.0) + tan(alpha[fpsize - 1] / 2.0));
        for (int j = 1; j < fpsize; j++) {
            weights[uIdInVolume[j]] +=
                    vNorm / sum / dist[uIdInVolume[j]] / sin(theta[j]) *
                    (tan(alpha[j] / 2.0) + tan(alpha[j - 1] / 2.0));
        }

		poly++;
	}

	float sumWeight = 0;
    for (int i = 0; i < weights.size(); i++)
	{
		sumWeight += weights[i];
	}

    if (fabs(sumWeight) < eps) { return weights; }
    for (int i = 0; i < weights.size(); i++) { weights[i] /= sumWeight; }
    return weights;
}
bool iGameStreamTracer::isInside(Vector3f coord, Vector3f v0, Vector3f v1,
                                 Vector3f v2, Vector3f v3,
                                 std::vector<float>& dis) {
    int contact_num = 0;
    dis.clear();
    if (checkContact(coord, v0, v1, v3)) contact_num++;
    if (checkContact(coord, v0, v2, v3)) contact_num++;
    if (checkContact(coord, v0, v1, v2)) contact_num++;
    if (checkContact(coord, v2, v1, v3)) contact_num++;
    if (contact_num % 2 == 1) {
        dis.emplace_back(
                pointToFaceDis(coord, v1, v2, v3) /
                pointToFaceDis(Vector3f(v0[0], v0[1], v0[2]), v1, v2, v3));
        dis.emplace_back(
                pointToFaceDis(coord, v0, v2, v3) /
                pointToFaceDis(Vector3f(v1[0], v1[1], v1[2]), v0, v2, v3));
        dis.emplace_back(
                pointToFaceDis(coord, v1, v0, v3) /
                pointToFaceDis(Vector3f(v2[0], v2[1], v2[2]), v1, v0, v3));
        dis.emplace_back(
                pointToFaceDis(coord, v1, v2, v0) /
                pointToFaceDis(Vector3f(v3[0], v3[1], v3[2]), v1, v2, v0));
        return true;
    }
    return false;
}
void iGameStreamTracer::InterpolationFunctions(const double pcoords[3],
                                               double sf[8]) {
    double rm, sm, tm;

    rm = 1. - pcoords[0];
    sm = 1. - pcoords[1];
    tm = 1. - pcoords[2];

    const auto rmXsm = rm * sm;
    const auto p0Xsm = pcoords[0] * sm;
    const auto p0Xp1 = pcoords[0] * pcoords[1];
    const auto rmXp1 = rm * pcoords[1];

    sf[0] = rmXsm * tm;
    sf[1] = p0Xsm * tm;
    sf[2] = p0Xp1 * tm;
    sf[3] = rmXp1 * tm;
    sf[4] = rmXsm * pcoords[2];
    sf[5] = p0Xsm * pcoords[2];
    sf[6] = p0Xp1 * pcoords[2];
    sf[7] = rmXp1 * pcoords[2];
}
void iGameStreamTracer::InterpolationDerivs(const double pcoords[3],
                                            double derivs[24]) {
    double rm, sm, tm;

    rm = 1. - pcoords[0];
    sm = 1. - pcoords[1];
    tm = 1. - pcoords[2];

    // r-derivatives
    derivs[0] = -sm * tm;
    derivs[1] = -derivs[0];
    derivs[2] = pcoords[1] * tm;
    derivs[3] = -derivs[2];
    derivs[4] = -sm * pcoords[2];
    derivs[5] = -derivs[4];
    derivs[6] = pcoords[1] * pcoords[2];
    derivs[7] = -derivs[6];

    // s-derivatives
    derivs[8] = -rm * tm;
    derivs[9] = -pcoords[0] * tm;
    derivs[10] = -derivs[9];
    derivs[11] = -derivs[8];
    derivs[12] = -rm * pcoords[2];
    derivs[13] = -pcoords[0] * pcoords[2];
    derivs[14] = -derivs[13];
    derivs[15] = -derivs[12];

    // t-derivatives
    derivs[16] = -rm * sm;
    derivs[17] = -pcoords[0] * sm;
    derivs[18] = -pcoords[0] * pcoords[1];
    derivs[19] = -rm * pcoords[1];
    derivs[20] = -derivs[16];
    derivs[21] = -derivs[17];
    derivs[22] = -derivs[18];
    derivs[23] = -derivs[19];
}
double iGameStreamTracer::Determinant3x3(const double c1[3], const double c2[3],
                                         const double c3[3]) {
    return c1[0] * c2[1] * c3[2] + c2[0] * c3[1] * c1[2] +
           c3[0] * c1[1] * c2[2] - c1[0] * c3[1] * c2[2] -
           c2[0] * c1[1] * c3[2] - c3[0] * c2[1] * c1[2];
}
float iGameStreamTracer::pointToFaceDis(Vector3f coord, Vector3f v0,
                                        Vector3f v1, Vector3f v2) {
    Vector3f a(v0[0], v0[1], v0[2]);
    Vector3f b(v1[0], v1[1], v1[2]);
    Vector3f c(v2[0], v2[1], v2[2]);
    Vector3f ab = b - a;
    Vector3f ac = c - a;
    Vector3f normal = ab.cross(ac);
    Vector3f pa = coord - a;
    return abs((pa.dot(normal))) / normal.length();
}
bool iGameStreamTracer::checkContact(Vector3f coord, Vector3f v0, Vector3f v1,
                                     Vector3f v2) {
    Vector3f p0(v0[0], v0[1], v0[2]);
    Vector3f p1(v1[0], v1[1], v1[2]);
    Vector3f p2(v2[0], v2[1], v2[2]);
    Vector3f d(1, 1, 1);
    Vector3f e1 = p1 - p0;
    Vector3f e2 = p2 - p0;
    Vector3f s = coord - p0;
    Vector3f s1 = d.cross(e2);
    Vector3f s2 = s.cross(e1);
    float t = s2.dot(e2) / (s1.dot(e1));
    float b1 = s1.dot(s) / (s1.dot(e1));
    float b2 = s2.dot(d) / (s1.dot(e1));
    if (t > 0 && b1 > 0 && b2 > 0 && (1 - b1 - b2) > 0) return true;
    return false;
}