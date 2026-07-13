#include "iGameGP_Bezier_IO.h"

#include <Common/GeometryKernel.h>
#include <MeshKernel/Mesh.h>
#include <functional>
#include <vector>
#include <omp.h>
using namespace std;

bool BezierIO::read2BezierOnly(const std::string& _filename) {
    int cube[4][4][4] = {0};
    int to_face[6][4][4] = {0};
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            for (int k = 0; k < 4; ++k) cube[i][j][k] = i * 16 + j * 4 + k;

    for (int j = 0; j < 4; ++j) {
        for (int k = 0; k < 4; ++k) {
            to_face[0][j][k] = cube[0][j][k];
            to_face[1][j][k] = cube[3][j][k];
            to_face[2][j][k] = cube[j][0][k];
            to_face[3][j][k] = cube[j][3][k];
            to_face[4][j][k] = cube[j][k][0];
            to_face[5][j][k] = cube[j][k][3];
        }
    }

    vector<vector<vector<vector<int>>>> control_point;
    vector<V3f> global_V3f;
    vector<int> global_V3f_count;
    function<int(double, double, double)> addVectex = [&](double x, double y,
                                                          double z) {
        V3f point(x, y, z);
        for (int i = 0; i < global_V3f.size(); ++i) {
            if (distance(point, global_V3f[i]) <= 1e-5) {
                ++global_V3f_count[i];
                return i;
            }
        }
        global_V3f.emplace_back(point);
        global_V3f_count.emplace_back(1);
        return (int) global_V3f.size() - 1;
    };

    std::ifstream iff(_filename.c_str(), std::ios::in);
    if (!iff.good()) {
        std::cerr << "Error: Could not open file " << _filename
                  << " for writing!" << std::endl;
        iff.close();
        return false;
    }
    std::string line;
    bool is_number = false;
    int soild_num = 0;
    int control_num = 0;
    vector<vector<vector<int>>> oneSoild(
            4, vector<vector<int>>(4, vector<int>(4)));
    while (std::getline(iff, line)) {
        vector<string> ss;
        if (std::isdigit(line[0]) || line[0] == '-') {
            is_number = true;
            int left = 0;
            for (int i = 0; i < line.length(); ++i) {
                if (line[i] == ' ') {
                    ss.emplace_back(line.substr(left, i - left));
                    left = i + 1;
                }
            }
            ss.emplace_back(line.substr(left, line.length() - left));
            cout << "point " << control_num << endl;
            double value[3] = {0};
            for (int i = 0; i < ss.size(); i++) {
                if (ss[i].length() == 0) continue;
                if (std::isdigit(ss[i][0]) || ss[i][0] == '-') {
                    value[i] = std::stod(ss[i]);
                    cout << value[i] << " ";
                }
            }
            int vid = addVectex(value[0], value[1], value[2]);
            int level_num = control_num / 16;
            int row_num = (control_num % 16) / 4;
            int column_num = control_num % 4;
            oneSoild[level_num][row_num][column_num] = vid;
            std::cout << std::endl;
            control_num++;
        } else {
            if (is_number) {
                ++soild_num;
                control_point.emplace_back(oneSoild);
                control_num = 0;
                cout << soild_num << endl;
                is_number = false;
            }
        }
    }

    vector<vector<vector<Vec>>> faces_control_points;
    vector<vector<vector<Vec>>> faces_control_points_dir;
    for (int soild_i = 0; soild_i < soild_num; ++soild_i) {
        for (int face_i = 0; face_i < 6; ++face_i) {
            vector<vector<Vec>> oneface_control_points(4, vector<Vec>(4));
            vector<vector<Vec>> oneface_control_points_dir(4, vector<Vec>(4));
            bool is_boundary = false;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    int control_num = to_face[face_i][i][j];
                    int level_num = control_num / 16;
                    int row_num = (control_num % 16) / 4;
                    int column_num = control_num % 4;
                    int vid = control_point[soild_i][level_num][row_num]
                                           [column_num];
                    oneface_control_points[i][j] =
                            Vec(global_V3f[vid].x, global_V3f[vid].y,
                                global_V3f[vid].z);
                    if (global_V3f_count[vid] == 1) is_boundary = true;
                }
            }
            if (is_boundary) {
                faces_control_points.emplace_back(oneface_control_points);
            }
        }
    }
    cad_restruction(faces_control_points);
}

bool BezierIO::cad_restruction(vector<vector<vector<Vec>>>& control_points) {
    int udegree = 3;
    int vdegree = 3;
    vector<double> uknots{0, 1};
    vector<double> vknots{0, 1};
    vector<int> umultis{4, 4};
    vector<int> vmultis{4, 4};

    vector<CBSplineSurface> muti_bezier_batch;

    for (int i = 0; i < control_points.size(); i++) {
        const auto& single_patch = control_points[i];
        vector<vector<CPoint>> control_surface;
        for (int j = 0; j < 4; ++j) {
            vector<CPoint> control_line;
            for (int k = 0; k < 4; ++k) {
                control_line.push_back(CPoint(single_patch[j][k].x(),
                                              single_patch[j][k].y(),
                                              single_patch[j][k].z()));
            }
            control_surface.push_back(control_line);
        }
        CBSplineSurface bsplineSurface(udegree, vdegree, control_surface,
                                       uknots, vknots, umultis, vmultis);
        muti_bezier_batch.emplace_back(bsplineSurface);
    }
    return true;
}


int findSpan(int n, int degree, double param,
             const vector<double>& knotVector) {
    if (param == knotVector[n]) return n - 1;  
    int low = degree, high = n, mid;
    while (low <= high) {
        mid = (low + high) / 2;
        if (param >= knotVector[mid] && param < knotVector[mid + 1]) return mid;
        else if (param < knotVector[mid])
            high = mid - 1;
        else
            low = mid + 1;
    }
    return -1;  
}


vector<double> computeBSplineBasis(double param, int degree,
                                   const vector<double>& knotVector) {
    int n = knotVector.size() - degree - 1;
    vector<double> N(n, 0.0);

    int span = findSpan(n, degree, param, knotVector);
    if (span == -1) return N;

    vector<double> left(degree + 1), right(degree + 1);
    N[span] = 1.0;  

    for (int j = 1; j <= degree; ++j) {
        left[j] = param - knotVector[span + 1 - j];
        right[j] = knotVector[span + j] - param;

        double saved = 0.0;
        for (int r = 0; r < j; ++r) {
            double temp = N[span - r] / (right[r + 1] + left[j - r]);
            N[span - r] = saved + right[r + 1] * temp;
            saved = left[j - r] * temp;
        }
        N[span - j] = saved;
    }

    return N;
}


vector<CBSplineSurface>
BezierIO::ReadSolid2Surface(const std::string& _filename, bool bCheckCCW) {
    int cube[4][4][4] = {0};
    int to_face[6][4][4] = {0};
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            for (int k = 0; k < 4; ++k) cube[i][j][k] = i * 16 + j * 4 + k;

    int to_face_next[6][4][4] = {0};

    for (int j = 0; j < 4; ++j) {
        for (int k = 0; k < 4; ++k) {

            to_face[0][j][k] = cube[0][j][k];
            to_face[1][3 - j][k] = cube[3][j][k];

            to_face[2][3 - j][k] = cube[j][0][k];
            to_face[3][j][k] = cube[j][3][k];

            to_face[4][j][k] = cube[j][k][0];
            to_face[5][3 - j][k] = cube[j][k][3];

            to_face_next[0][j][k] = cube[1][j][k]; 
            to_face_next[1][3 - j][k] = cube[2][j][k];

            to_face_next[2][3 - j][k] = cube[j][1][k];
            to_face_next[3][j][k] = cube[j][2][k];

            to_face_next[4][j][k] = cube[j][k][1];
            to_face_next[5][3 - j][k] = cube[j][k][2];

        }
    }

    vector<vector<vector<vector<int>>>> control_point;
    vector<vector<vector<vector<int>>>> scalar_point;
    vector<V3f> global_V3f;
    vector<int> global_V3f_count;

    vector<bool> global_V3f_flag;

    vector<V3f> global_V3f_unique;
    vector<bool> global_V3f_unique_added;
    vector<int> global_V3f_unique_count;


    function<int(double, double, double)> addVectex = [&](double x, double y,
                                                          double z) {
        V3f point(x, y, z);
        for (int i = 0; i < global_V3f.size(); ++i) {
            if (distance(point, global_V3f[i]) <= 1e-5) {
                ++global_V3f_count[i]; 
                return i;
            }
        }
        global_V3f.emplace_back(point);
        global_V3f_count.emplace_back(1);
        return (int) global_V3f.size() - 1;
    };

    vector<V3f> global_scalar_V3f;
    vector<int> global_scalar_V3f_count;
    function<int(double, double, double)> addScalar = [&](double x, double y,
                                                          double z) {
        V3f point(x, y, z);
        for (int i = 0; i < global_scalar_V3f.size(); ++i) {
            if (distance(point, global_scalar_V3f[i]) <= 1e-5) {
                ++global_scalar_V3f_count[i]; 
                return i;
            }
        }
        global_scalar_V3f.emplace_back(point);
        global_scalar_V3f_count.emplace_back(1);
        return (int) global_scalar_V3f.size() - 1;
    };


    std::ifstream iff(_filename.c_str(), std::ios::in);
    if (!iff.good()) {
        std::cerr << "Error: Could not open file " << _filename
                  << " for writing!" << std::endl;
        iff.close();
        return {};
    }
    std::string line;
    bool is_number = false;
    int solid_num = 0;
    int control_num = 0;
    vector<vector<vector<int>>> oneSoild(
            4, vector<vector<int>>(4, vector<int>(4)));
    vector<vector<vector<int>>> oneSoildScalar(
            4, vector<vector<int>>(4, vector<int>(4)));
    int num = 1;
    while (std::getline(iff, line)) {

        if (num == 65) {
            if (is_number) {
                ++solid_num;
                control_point.emplace_back(oneSoild);
                control_num = 0;
                is_number = false;
            }
        }
        if (num == 129) {
            if (is_number) {
                scalar_point.emplace_back(oneSoildScalar);
                control_num = 0;
                num = 1;
                is_number = false;
            }
        }
        if (num < 65) {
            vector<string> ss;
            if (std::isdigit(line[0]) || line[0] == '-') {
                is_number = true;
                int left = 0;
                for (int i = 0; i < line.length(); ++i) {
                    if (line[i] == ' ') {
                        ss.emplace_back(line.substr(left, i - left));
                        left = i + 1;
                    }
                }
                ss.emplace_back(line.substr(left, line.length() - left));
                double value[3] = {0};
                for (int i = 0; i < ss.size(); i++) {
                    if (ss[i].length() == 0) continue;
                    if (std::isdigit(ss[i][0]) || ss[i][0] == '-') {
                        value[i] = std::stod(ss[i]);
                    }
                }
                int vid = addVectex(value[0], value[1], value[2]);
                int level_num = control_num / 16;
                int row_num = (control_num % 16) / 4;
                int column_num = control_num % 4;
                oneSoild[level_num][row_num][column_num] = vid;
                control_num++;
                num++;
            }
        } else {
            vector<string> ss;
            if (std::isdigit(line[0]) || line[0] == '-') {
                is_number = true;
                int left = 0;
                for (int i = 0; i < line.length(); ++i) {
                    if (line[i] == ' ') {
                        ss.emplace_back(line.substr(left, i - left));
                        left = i + 1;
                    }
                }
                ss.emplace_back(line.substr(left, line.length() - left));
                double value[3] = {0};
                for (int i = 0; i < ss.size(); i++) {
                    if (ss[i].length() == 0) continue;
                    if (std::isdigit(ss[i][0]) || ss[i][0] == '-') {
                        value[i] = std::stod(ss[i]);
                    }
                }
                int vid = addScalar(value[0], value[1], value[2]);
                int level_num = control_num / 16;
                int row_num = (control_num % 16) / 4;
                int column_num = control_num % 4;
                oneSoildScalar[level_num][row_num][column_num] = vid;
                control_num++;
                num++;
            }
        }
    }

    global_V3f_flag = vector<bool>(global_V3f.size(), false);

    vector<vector<vector<Vec>>> faces_control_points;
    vector<vector<vector<Vec>>> faces_control_points_dir;

    vector<unordered_map<int, vector<vector<Vec>>>>
            faces_control_points_hashArray(solid_num);
    vector<unordered_map<int, vector<vector<Vec>>>>
            next_faces_control_points_hashArray(solid_num);

    vector<vector<vector<Vec>>> faces_scalar_points;
    vector<vector<vector<Vec>>> faces_scalar_dir;

    vector<unordered_map<int, vector<vector<Vec>>>>
            faces_scalar_points_hashArray(solid_num);
    vector<unordered_map<int, vector<vector<Vec>>>>
            next_faces_scalar_points_hashArray(solid_num);

    for (int solid = 0; solid < solid_num; ++solid) {
        for (int face_i = 0; face_i < 6; ++face_i) {
            vector<vector<Vec>> oneface_control_points(4, vector<Vec>(4));
            vector<vector<Vec>> next_oneface_control_points(4, vector<Vec>(4));

            vector<vector<Vec>> oneface_scalar_points(4, vector<Vec>(4));
            vector<vector<Vec>> next_oneface_scalar_points(4, vector<Vec>(4));

            bool is_boundary = false;
            bool is_not_added = false;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    int control_num = to_face[face_i][i][j];
                    int level_num = control_num / 16;
                    int row_num = (control_num % 16) / 4;
                    int column_num = control_num % 4;
                    int vid = control_point[solid][level_num][row_num]
                                           [column_num];
                    oneface_control_points[i][j] =
                            Vec(global_V3f[vid].x, global_V3f[vid].y,
                                global_V3f[vid].z);

                    int vid2 =
                            scalar_point[solid][level_num][row_num][column_num];
                    oneface_scalar_points[i][j] = Vec(global_scalar_V3f[vid2].x,
                                                      global_scalar_V3f[vid].y,
                                                      global_scalar_V3f[vid].z);

                    if (bCheckCCW) {
                        int next_control_num = to_face_next[face_i][i][j];
                        int next_level_num = next_control_num / 16;
                        int next_row_num = (next_control_num % 16) / 4;
                        int next_column_num = next_control_num % 4;
                        int next_vid =
                                control_point[solid][next_level_num]
                                             [next_row_num][next_column_num];
                        next_oneface_control_points[i][j] = Vec(
                                global_V3f[next_vid].x, global_V3f[next_vid].y,
                                global_V3f[next_vid].z);

                        int next_vid2 =
                                scalar_point[solid][next_level_num]
                                            [next_row_num][next_column_num];
                        next_oneface_scalar_points[i][j] =
                                Vec(global_V3f[next_vid2].x,
                                    global_V3f[next_vid2].y,
                                    global_V3f[next_vid2].z);
                    }

                    if (global_V3f_count[vid] == 1) { is_boundary = true; }

                    if (global_V3f_flag[vid] == false) { is_not_added = true; }

                    global_V3f_flag[vid] = true;
                }
            }
            if (is_boundary) {

                faces_control_points.emplace_back(oneface_control_points);

                faces_scalar_points.emplace_back(oneface_scalar_points);

                if (bCheckCCW) {
                    faces_control_points_hashArray[solid][face_i] =
                            oneface_control_points;
                    next_faces_control_points_hashArray[solid][face_i] =
                            next_oneface_control_points;

                    faces_scalar_points_hashArray[solid][face_i] =
                            oneface_scalar_points;
                    next_faces_scalar_points_hashArray[solid][face_i] =
                            next_oneface_scalar_points;
                }
            }
        }
    }

    int udegree = 3;
    int vdegree = 3;
    vector<double> uknots{0, 1};
    vector<double> vknots{0, 1};
    vector<int> umultis{4, 4};
    vector<int> vmultis{4, 4};

    vector<CBSplineSurface> muti_bezier_batch;

    if (bCheckCCW) {
        uint32_t ccw_count = 0;
        uint32_t cw_count = 0;

        for (int solid = 0; solid < solid_num; ++solid) {
            auto& hashDetail = faces_control_points_hashArray[solid];
            auto& nextHashDetail = next_faces_control_points_hashArray[solid];

            auto& hashScalarDetail = faces_scalar_points_hashArray[solid];
            auto& nextScalarHashDetail =
                    next_faces_scalar_points_hashArray[solid];

            auto hashScalarIter = hashScalarDetail.begin();

            for (auto hashIter = hashDetail.begin();
                 hashIter != hashDetail.end(); ++hashIter) {
                int face_id = hashIter->first;

                auto& Patch = hashIter->second;
                assert(nextHashDetail.count(face_id) > 0);
                auto& nextPatch = nextHashDetail[face_id];

                vector<vector<CPoint>> control_surface;
                vector<vector<CPoint>> next_control_surface;

                for (int j = 0; j < 4; ++j) {
                    vector<CPoint> control_line;
                    vector<CPoint> next_control_line;

                    for (int k = 0; k < 4; ++k) {
                        control_line.push_back(CPoint(Patch[j][k].x(),
                                                      Patch[j][k].y(),
                                                      Patch[j][k].z()));
                        next_control_line.push_back(
                                CPoint(nextPatch[j][k].x(), nextPatch[j][k].y(),
                                       nextPatch[j][k].z()));
                    }
                    control_surface.push_back(control_line);
                    next_control_surface.push_back(next_control_line);
                }

                CBSplineSurface Surface(udegree, vdegree, control_surface,
                                        uknots, vknots, umultis, vmultis);
                CBSplineSurface nextSurface(udegree, vdegree,
                                            next_control_surface, uknots,
                                            vknots, umultis, vmultis);

                double u = 0.5;
                double v = 0.5;

                CVector normal{};


                ccw_count++;
                muti_bezier_batch.push_back(Surface);
            }
        }

    } else {
        for (int i = 0; i < faces_control_points.size(); i++) {
            const auto& single_patch = faces_control_points[i];
            const auto& scalar_patch = faces_scalar_points[i];

            vector<vector<CPoint>> control_surface;
            vector<vector<CPoint>> scalar_surface;
            for (int j = 0; j < 4; ++j) {
                vector<CPoint> control_line;
                vector<CPoint> scalar_line;

                for (int k = 0; k < 4; ++k) {
                    control_line.push_back(CPoint(single_patch[j][k].x(),
                                                  single_patch[j][k].y(),
                                                  single_patch[j][k].z()));
                    scalar_line.push_back(CPoint(scalar_patch[j][k].x(),
                                                 scalar_patch[j][k].y(),
                                                 scalar_patch[j][k].z()));
                }
                control_surface.push_back(control_line);
                scalar_surface.push_back(scalar_line);
            }
            CBSplineSurface bsplineSurface(udegree, vdegree, scalar_surface,
                                           control_surface, uknots, vknots,
                                           umultis, vmultis);
            muti_bezier_batch.emplace_back(bsplineSurface);
        }
    }

    return muti_bezier_batch;
}


vector<CBSplineSurface> BezierIO::ReadSolid(const std::string& _filename,
                                            bool bCheckCCW, int isoNum) {
    int cube[4][4][4] = {0};
    int to_face[6][4][4] = {0};
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            for (int k = 0; k < 4; ++k) cube[i][j][k] = i * 16 + j * 4 + k;

    int to_face_next[6][4][4] = {0};

    for (int j = 0; j < 4; ++j) {
        for (int k = 0; k < 4; ++k) {

            to_face[0][j][k] = cube[0][j][k];
            to_face[1][3 - j][k] = cube[3][j][k];

            to_face[2][3 - j][k] = cube[j][0][k];
            to_face[3][j][k] = cube[j][3][k];

            to_face[4][j][k] = cube[j][k][0];
            to_face[5][3 - j][k] = cube[j][k][3];

            to_face_next[0][j][k] = cube[1][j][k]; 
            to_face_next[1][3 - j][k] = cube[2][j][k];

            to_face_next[2][3 - j][k] = cube[j][1][k];
            to_face_next[3][j][k] = cube[j][2][k];

            to_face_next[4][j][k] = cube[j][k][1];
            to_face_next[5][3 - j][k] = cube[j][k][2];

        }
    }

    vector<vector<vector<vector<int>>>> control_point;
    vector<vector<vector<vector<int>>>> scalar_point;
    vector<V3f> global_V3f;
    vector<int> global_V3f_count;

    vector<bool> global_V3f_flag;

    vector<V3f> global_V3f_unique;
    vector<bool> global_V3f_unique_added;
    vector<int> global_V3f_unique_count;


    function<int(double, double, double)> addVectex = [&](double x, double y,
                                                          double z) {
        V3f point(x, y, z);
        for (int i = 0; i < global_V3f.size(); ++i) {
            if (distance(point, global_V3f[i]) <= 1e-5) {
                ++global_V3f_count[i]; 
                return i;
            }
        }
        global_V3f.emplace_back(point);
        global_V3f_count.emplace_back(1);
        return (int) global_V3f.size() - 1;
    };

    vector<V3f> global_scalar_V3f;
    vector<int> global_scalar_V3f_count;
    function<int(double, double, double)> addScalar = [&](double x, double y,
                                                          double z) {
        V3f point(x, y, z);
        for (int i = 0; i < global_scalar_V3f.size(); ++i) {
            if (distance(point, global_scalar_V3f[i]) <= 1e-5) {
                ++global_scalar_V3f_count[i]; 
                return i;
            }
        }
        global_scalar_V3f.emplace_back(point);
        global_scalar_V3f_count.emplace_back(1);
        return (int) global_scalar_V3f.size() - 1;
    };


    std::ifstream iff(_filename.c_str(), std::ios::in);
    if (!iff.good()) {
        std::cerr << "Error: Could not open file " << _filename
                  << " for writing!" << std::endl;
        iff.close();
        return {};
    }
    std::string line;
    bool is_number = false;
    int solid_num = 0;
    int control_num = 0;
    vector<vector<vector<int>>> oneSoild(
            4, vector<vector<int>>(4, vector<int>(4)));
    vector<vector<vector<int>>> oneSoildScalar(
            4, vector<vector<int>>(4, vector<int>(4)));
    int num = 1;
    while (std::getline(iff, line)) {

        if (num == 65) {
            if (is_number) {
                ++solid_num;
                control_point.emplace_back(oneSoild);
                control_num = 0;
                is_number = false;
            }
        }
        if (num == 129) {
            if (is_number) {
                scalar_point.emplace_back(oneSoildScalar);
                control_num = 0;
                num = 1;
                is_number = false;
            }
        }
        if (num < 65) {
            vector<string> ss;
            if (std::isdigit(line[0]) || line[0] == '-') {
                is_number = true;
                int left = 0;
                for (int i = 0; i < line.length(); ++i) {
                    if (line[i] == ' ') {
                        ss.emplace_back(line.substr(left, i - left));
                        left = i + 1;
                    }
                }
                ss.emplace_back(line.substr(left, line.length() - left));
                double value[3] = {0};
                for (int i = 0; i < ss.size(); i++) {
                    if (ss[i].length() == 0) continue;
                    if (std::isdigit(ss[i][0]) || ss[i][0] == '-') {
                        value[i] = std::stod(ss[i]);
                    }
                }
                int vid = addVectex(value[0], value[1], value[2]);
                int level_num = control_num / 16;
                int row_num = (control_num % 16) / 4;
                int column_num = control_num % 4;
                oneSoild[level_num][row_num][column_num] = vid;
                control_num++;
                num++;
            }
        } else {
            vector<string> ss;
            if (std::isdigit(line[0]) || line[0] == '-') {
                is_number = true;
                int left = 0;
                for (int i = 0; i < line.length(); ++i) {
                    if (line[i] == ' ') {
                        ss.emplace_back(line.substr(left, i - left));
                        left = i + 1;
                    }
                }
                ss.emplace_back(line.substr(left, line.length() - left));
                double value[3] = {0};
                for (int i = 0; i < ss.size(); i++) {
                    if (ss[i].length() == 0) continue;
                    if (std::isdigit(ss[i][0]) || ss[i][0] == '-') {
                        value[i] = std::stod(ss[i]);
                    }
                }
                int vid = addScalar(value[0], value[1], value[2]);
                int level_num = control_num / 16;
                int row_num = (control_num % 16) / 4;
                int column_num = control_num % 4;
                oneSoildScalar[level_num][row_num][column_num] = vid;
                control_num++;
                num++;
            }
        }
    }

    global_V3f_flag = vector<bool>(global_V3f.size(), false);

    vector<vector<vector<Vec>>> faces_control_points;
    vector<vector<vector<Vec>>> faces_control_points_dir;

    vector<unordered_map<int, vector<vector<Vec>>>>
            faces_control_points_hashArray(solid_num);
    vector<unordered_map<int, vector<vector<Vec>>>>
            next_faces_control_points_hashArray(solid_num);

    vector<vector<vector<Vec>>> faces_scalar_points;
    vector<vector<vector<Vec>>> faces_scalar_dir;

    vector<unordered_map<int, vector<vector<Vec>>>>
            faces_scalar_points_hashArray(solid_num);
    vector<unordered_map<int, vector<vector<Vec>>>>
            next_faces_scalar_points_hashArray(solid_num);


    vector<double> uKnots = {0.00000, 0.00000, 0.00000, 0.00000,
                             1.00000, 1.00000, 1.00000, 1.00000};
    vector<double> vKnots = {0.00000, 0.00000, 0.00000, 0.00000,
                             1.00000, 1.00000, 1.00000, 1.00000};
    vector<double> wKnots = {0.00000, 0.00000, 0.00000, 0.00000,
                             1.00000, 1.00000, 1.00000, 1.00000};
    int degree = 3;  


    int numIsoSurfaces = isoNum;
    double step = 1.0 / (numIsoSurfaces - 1);  

    //for (int solid = 0; solid < solid_num; ++solid) {

    //    for (int isoIdx = 0; isoIdx < numIsoSurfaces; ++isoIdx) {
    //        double w = isoIdx * step;
    //        vector<double> wBasis = computeBSplineBasis(w, degree, wKnots);

    //        vector<vector<Vec>> oneface_control_points(4, vector<Vec>(4));
    //        vector<vector<Vec>> oneface_scalar_points(4, vector<Vec>(4));

    //        for (int i = 0; i < 4; ++i) {
    //            for (int j = 0; j < 4; ++j) {
    //                Vec interpolatedPoint(0, 0, 0);

    //                for (int k = 0; k < 4; ++k) {
    //                    int vid = control_point[solid][k][i][j];
    //                    Vec controlPoint =
    //                            Vec(global_V3f[vid].x, global_V3f[vid].y,
    //                                global_V3f[vid].z);
    //                    interpolatedPoint +=
    //                            controlPoint * wBasis[k];  
    //                }
    //                oneface_control_points[i][j] = interpolatedPoint;

    //                Vec interpolatedScalar(0, 0, 0);
    //                for (int k = 0; k < 4; ++k) {
    //                    int vid2 = scalar_point[solid][k][i][j];
    //                    Vec scalarPoint = Vec(global_scalar_V3f[vid2].x,
    //                                          global_scalar_V3f[vid2].y,
    //                                          global_scalar_V3f[vid2].z);
    //                    interpolatedScalar +=
    //                            scalarPoint * wBasis[k];  
    //                }
    //                oneface_scalar_points[i][j] = interpolatedScalar;
    //            }
    //        }


    //        faces_control_points.emplace_back(oneface_control_points);
    //        faces_scalar_points.emplace_back(oneface_scalar_points);
    //    }
    //}

    vector<vector<double>> wBasisArray(numIsoSurfaces);
    #pragma omp parallel for
    for (int isoIdx = 0; isoIdx < numIsoSurfaces; ++isoIdx) {
        double w = isoIdx * step;
        wBasisArray[isoIdx] = computeBSplineBasis(w, degree, wKnots);
    }

    for (int solid = 0; solid < solid_num; ++solid) {
        for (int isoIdx = 0; isoIdx < numIsoSurfaces; ++isoIdx) {
            // 直接使用预先计算好的基函数
            const vector<double>& wBasis = wBasisArray[isoIdx];

            vector<vector<Vec>> oneface_control_points(4, vector<Vec>(4));
            vector<vector<Vec>> oneface_scalar_points(4, vector<Vec>(4));

            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    Vec interpolatedPoint(0, 0, 0);

                    for (int k = 0; k < 4; ++k) {
                        int vid = control_point[solid][k][i][j];
                        Vec controlPoint = Vec(global_V3f[vid].x, global_V3f[vid].y, global_V3f[vid].z);
                        interpolatedPoint += controlPoint * wBasis[k];
                    }
                    oneface_control_points[i][j] = interpolatedPoint;

                    Vec interpolatedScalar(0, 0, 0);
                    for (int k = 0; k < 4; ++k) {
                        int vid2 = scalar_point[solid][k][i][j];
                        Vec scalarPoint =
                                Vec(global_scalar_V3f[vid2].x, global_scalar_V3f[vid2].y, global_scalar_V3f[vid2].z);
                        interpolatedScalar += scalarPoint * wBasis[k];
                    }
                    oneface_scalar_points[i][j] = interpolatedScalar;
                }
            }

            faces_control_points.emplace_back(oneface_control_points);
            faces_scalar_points.emplace_back(oneface_scalar_points);
        }
    }



    int udegree = 3;
    int vdegree = 3;
    vector<double> uknots{0, 1};
    vector<double> vknots{0, 1};
    vector<int> umultis{4, 4};
    vector<int> vmultis{4, 4};

    vector<CBSplineSurface> muti_bezier_batch;

    if (bCheckCCW) {
        uint32_t ccw_count = 0;
        uint32_t cw_count = 0;

        for (int solid = 0; solid < solid_num; ++solid) {
            auto& hashDetail = faces_control_points_hashArray[solid];
            auto& nextHashDetail = next_faces_control_points_hashArray[solid];

            auto& hashScalarDetail = faces_scalar_points_hashArray[solid];
            auto& nextScalarHashDetail =
                    next_faces_scalar_points_hashArray[solid];

            auto hashScalarIter = hashScalarDetail.begin();

            for (auto hashIter = hashDetail.begin();
                 hashIter != hashDetail.end(); ++hashIter) {
                int face_id = hashIter->first;

                auto& Patch = hashIter->second;
                assert(nextHashDetail.count(face_id) > 0);
                auto& nextPatch = nextHashDetail[face_id];

                vector<vector<CPoint>> control_surface;
                vector<vector<CPoint>> next_control_surface;

                for (int j = 0; j < 4; ++j) {
                    vector<CPoint> control_line;
                    vector<CPoint> next_control_line;

                    for (int k = 0; k < 4; ++k) {
                        control_line.push_back(CPoint(Patch[j][k].x(),
                                                      Patch[j][k].y(),
                                                      Patch[j][k].z()));
                        next_control_line.push_back(
                                CPoint(nextPatch[j][k].x(), nextPatch[j][k].y(),
                                       nextPatch[j][k].z()));
                    }
                    control_surface.push_back(control_line);
                    next_control_surface.push_back(next_control_line);
                }

                CBSplineSurface Surface(udegree, vdegree, control_surface,
                                        uknots, vknots, umultis, vmultis);
                CBSplineSurface nextSurface(udegree, vdegree,
                                            next_control_surface, uknots,
                                            vknots, umultis, vmultis);

                double u = 0.5;
                double v = 0.5;

                CVector normal{};


                ccw_count++;

                muti_bezier_batch.push_back(Surface);
            }
        }

    } else {
        for (int i = 0; i < faces_control_points.size(); i++) {
            const auto& single_patch = faces_control_points[i];
            const auto& scalar_patch = faces_scalar_points[i];

            vector<vector<CPoint>> control_surface;
            vector<vector<CPoint>> scalar_surface;
            for (int j = 0; j < 4; ++j) {
                vector<CPoint> control_line;
                vector<CPoint> scalar_line;

                for (int k = 0; k < 4; ++k) {
                    control_line.push_back(CPoint(single_patch[j][k].x(),
                                                  single_patch[j][k].y(),
                                                  single_patch[j][k].z()));
                    scalar_line.push_back(CPoint(scalar_patch[j][k].x(),
                                                 scalar_patch[j][k].y(),
                                                 scalar_patch[j][k].z()));
                }
                control_surface.push_back(control_line);
                scalar_surface.push_back(scalar_line);
            }
            CBSplineSurface bsplineSurface(udegree, vdegree, scalar_surface,
                                           control_surface, uknots, vknots,
                                           umultis, vmultis);
            muti_bezier_batch.emplace_back(bsplineSurface);
        }
    }

    return muti_bezier_batch;
}

vector<CBSplineSurface>
BezierIO::ReadSolid2VolumeSurface(const std::string& _filename,
                                  bool bCheckCCW) {
    int cube[4][4][4] = {0};
    int to_face[6][4][4] = {0};
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            for (int k = 0; k < 4; ++k) cube[i][j][k] = i * 16 + j * 4 + k;

    int to_face_next[6][4][4] = {0};

    for (int j = 0; j < 4; ++j) {
        for (int k = 0; k < 4; ++k) {

            to_face[0][j][k] = cube[0][j][k];
            to_face[1][3 - j][k] = cube[3][j][k];

            to_face[2][3 - j][k] = cube[j][0][k];
            to_face[3][j][k] = cube[j][3][k];

            to_face[4][j][k] = cube[j][k][0];
            to_face[5][3 - j][k] = cube[j][k][3];

            to_face_next[0][j][k] = cube[1][j][k]; 
            to_face_next[1][3 - j][k] = cube[2][j][k];

            to_face_next[2][3 - j][k] = cube[j][1][k];
            to_face_next[3][j][k] = cube[j][2][k];

            to_face_next[4][j][k] = cube[j][k][1];
            to_face_next[5][3 - j][k] = cube[j][k][2];

        }
    }

    vector<vector<vector<vector<int>>>> control_point;
    vector<V3f> global_V3f;
    vector<int> global_V3f_count;
    vector<bool> global_V3f_flag;

    vector<V3f> global_V3f_unique;
    vector<bool> global_V3f_unique_added;
    vector<int> global_V3f_unique_count;

    function<int(double, double, double)> addVectex = [&](double x, double y,
                                                          double z) {
        V3f point(x, y, z);
        for (int i = 0; i < global_V3f.size(); ++i) {
            if (distance(point, global_V3f[i]) <= 1e-5) {
                ++global_V3f_count[i];
                return i;
            }
        }
        global_V3f.emplace_back(point);
        global_V3f_count.emplace_back(1);
        return (int) global_V3f.size() - 1;
    };

    std::ifstream iff(_filename.c_str(), std::ios::in);
    if (!iff.good()) {
        std::cerr << "Error: Could not open file " << _filename
                  << " for writing!" << std::endl;
        iff.close();
        return {};
    }
    std::string line;
    bool is_number = false;
    int solid_num = 0;
    int control_num = 0;
    vector<vector<vector<int>>> oneSoild(
            4, vector<vector<int>>(4, vector<int>(4)));
    while (std::getline(iff, line)) {
        vector<string> ss;
        if (std::isdigit(line[0]) || line[0] == '-') {
            is_number = true;
            int left = 0;
            for (int i = 0; i < line.length(); ++i) {
                if (line[i] == ' ') {
                    ss.emplace_back(line.substr(left, i - left));
                    left = i + 1;
                }
            }
            ss.emplace_back(line.substr(left, line.length() - left));
            double value[3] = {0};
            for (int i = 0; i < ss.size(); i++) {
                if (ss[i].length() == 0) continue;
                if (std::isdigit(ss[i][0]) || ss[i][0] == '-') {
                    value[i] = std::stod(ss[i]);
                }
            }
            int vid = addVectex(value[0], value[1], value[2]);
            int level_num = control_num / 16;
            int row_num = (control_num % 16) / 4;
            int column_num = control_num % 4;
            oneSoild[level_num][row_num][column_num] = vid;
            control_num++;
        } else {
            if (is_number) {
                ++solid_num;
                control_point.emplace_back(oneSoild);
                control_num = 0;
                is_number = false;
            }
        }
    }

    global_V3f_flag = vector<bool>(global_V3f.size(), false);

    vector<vector<vector<Vec>>> faces_control_points;
    vector<vector<vector<Vec>>> faces_control_points_dir;

    vector<unordered_map<int, vector<vector<Vec>>>>
            faces_control_points_hashArray(solid_num);
    vector<unordered_map<int, vector<vector<Vec>>>>
            next_faces_control_points_hashArray(solid_num);

    for (int solid = 0; solid < solid_num; ++solid) {
        for (int face_i = 0; face_i < 6; ++face_i) {
            vector<vector<Vec>> oneface_control_points(4, vector<Vec>(4));
            vector<vector<Vec>> next_oneface_control_points(4, vector<Vec>(4));
            bool is_boundary = false;
            bool is_not_added = false;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    int control_num = to_face[face_i][i][j];
                    int level_num = control_num / 16;
                    int row_num = (control_num % 16) / 4;
                    int column_num = control_num % 4;
                    int vid = control_point[solid][level_num][row_num]
                                           [column_num];
                    oneface_control_points[i][j] =
                            Vec(global_V3f[vid].x, global_V3f[vid].y,
                                global_V3f[vid].z);

                    if (bCheckCCW) {
                        int next_control_num = to_face_next[face_i][i][j];
                        int next_level_num = next_control_num / 16;
                        int next_row_num = (next_control_num % 16) / 4;
                        int next_column_num = next_control_num % 4;
                        int next_vid =
                                control_point[solid][next_level_num]
                                             [next_row_num][next_column_num];
                        next_oneface_control_points[i][j] = Vec(
                                global_V3f[next_vid].x, global_V3f[next_vid].y,
                                global_V3f[next_vid].z);
                    }

                    if (global_V3f_count[vid] == 1) { is_boundary = true; }

                    if (global_V3f_flag[vid] == false) { is_not_added = true; }

                    global_V3f_flag[vid] = true;
                }
            }
            if (is_boundary || is_not_added) {
                faces_control_points.emplace_back(oneface_control_points);

                if (bCheckCCW) {
                    faces_control_points_hashArray[solid][face_i] =
                            oneface_control_points;
                    next_faces_control_points_hashArray[solid][face_i] =
                            next_oneface_control_points;
                }
            }
        }
    }

    int udegree = 3;
    int vdegree = 3;
    vector<double> uknots{0, 1};
    vector<double> vknots{0, 1};
    vector<int> umultis{4, 4};
    vector<int> vmultis{4, 4};

    vector<CBSplineSurface> muti_bezier_batch;

    if (bCheckCCW) {

        uint32_t ccw_count = 0;
        uint32_t cw_count = 0;

        for (int solid = 0; solid < solid_num; ++solid) {
            auto& hashDetail = faces_control_points_hashArray[solid];
            auto& nextHashDetail = next_faces_control_points_hashArray[solid];

            for (auto hashIter = hashDetail.begin();
                 hashIter != hashDetail.end(); ++hashIter) {
                int face_id = hashIter->first;

                auto& Patch = hashIter->second;
                assert(nextHashDetail.count(face_id) > 0);
                auto& nextPatch = nextHashDetail[face_id];

                vector<vector<CPoint>> control_surface;
                vector<vector<CPoint>> next_control_surface;

                for (int j = 0; j < 4; ++j) {
                    vector<CPoint> control_line;
                    vector<CPoint> next_control_line;

                    for (int k = 0; k < 4; ++k) {
                        control_line.push_back(CPoint(Patch[j][k].x(),
                                                      Patch[j][k].y(),
                                                      Patch[j][k].z()));
                        next_control_line.push_back(
                                CPoint(nextPatch[j][k].x(), nextPatch[j][k].y(),
                                       nextPatch[j][k].z()));
                    }
                    control_surface.push_back(control_line);
                    next_control_surface.push_back(next_control_line);
                }

                CBSplineSurface Surface(udegree, vdegree, control_surface,
                                        uknots, vknots, umultis, vmultis);
                CBSplineSurface nextSurface(udegree, vdegree,
                                            next_control_surface, uknots,
                                            vknots, umultis, vmultis);

                double u = 0.5;
                double v = 0.5;

                CVector normal{};


                ccw_count++;

                muti_bezier_batch.push_back(Surface);
            }
        }

    } else {
        for (int i = 0; i < faces_control_points.size(); i++) {
            const auto& single_patch = faces_control_points[i];
            vector<vector<CPoint>> control_surface;
            for (int j = 0; j < 4; ++j) {
                vector<CPoint> control_line;
                for (int k = 0; k < 4; ++k) {
                    control_line.push_back(CPoint(single_patch[j][k].x(),
                                                  single_patch[j][k].y(),
                                                  single_patch[j][k].z()));
                }
                control_surface.push_back(control_line);
            }
            CBSplineSurface bsplineSurface(udegree, vdegree, control_surface,
                                           uknots, vknots, umultis, vmultis);
            muti_bezier_batch.emplace_back(bsplineSurface);
        }
    }

    return muti_bezier_batch;
}


