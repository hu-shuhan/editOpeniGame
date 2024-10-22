/**
 * @class   iGameNurbsReader
 * @brief   iGameNurbsReader's brief
 */


#include "iGameNurbsReader.h"
#include <iGameFileReader.h>
#include <regex>
#include <tinyxml2.h>

#include "Nurbs/MultiGeo.h"


IGAME_NAMESPACE_BEGIN


bool NurbsReader::Parsing() {

    MultiGeo geometry;

    // read xml file information
    {
        auto* item = root->FirstChildElement();
        const char* type = item->Value();
        std::string s_type = type;
        const char* delimiters = " \n";


        for (auto* item = root->FirstChildElement(type); item;
             item = item->NextSiblingElement(type)) {

            int num = s_type == "curve"
                              ? 1
                              : (s_type == "surface"
                                         ? 2
                                         : (s_type == "volume" ? 3 : 0));
            if (num == 0) {
                printf("xml tag error\n");
                exit(0);
            }

            std::vector<int> degree(num), cpt(num);
            std::vector<double> weights;
            std::vector<std::vector<double>> knots(num);
            std::vector<std::vector<double>> points;

            std::regex pattern(" ");

            std::string value = item->FirstChildElement("degree")->GetText();

            char* data_p = const_cast<char*>(value.data());
            while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t')
                data_p++;
            std::vector<std::string> s_degree;
            char* token = strtok(const_cast<char*>(value.c_str()), delimiters);
            while (token != nullptr) {
                int i = mAtoi(token);
                token = strtok(nullptr, delimiters);
                s_degree.push_back(std::to_string(i));
            }

            assert(static_cast<int>(s_degree.size()) == num);
            for (int i = 0; i < num; ++i) degree[i] = std::stoi(s_degree[i]);

            int cptNum = 1;
            value = item->FirstChildElement("number")->GetText();
            //std::vector<std::string> s_number(
            //        std::sregex_token_iterator(value.begin(), value.end(), pattern,
            //                                   -1),
            //        std::sregex_token_iterator());
            //if (s_number.back().empty()) s_number.pop_back();
            //if ((*s_number.begin()).empty())
            //    s_number = std::vector<std::string>(s_number.begin() + 1,
            //                                        s_number.end());
            //

            data_p = const_cast<char*>(value.data());
            while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t')
                data_p++;
            std::vector<std::string> s_number;
            token = strtok(const_cast<char*>(value.c_str()), delimiters);
            while (token != nullptr) {
                int i = mAtoi(token);
                token = strtok(nullptr, delimiters);
                s_number.push_back(std::to_string(i));
            }

            assert(static_cast<int>(s_number.size()) == num);
            for (int i = 0; i < num; ++i)
                cpt[i] = std::stoi(s_number[i]), cptNum *= cpt[i];

            tinyxml2::XMLElement* knot_item = nullptr;
            for (int i = 0; i < num; ++i) {
                knot_item = knot_item ? knot_item->NextSiblingElement("knots")
                                      : item->FirstChildElement("knots");
                value = knot_item->GetText();

                char* data_p = const_cast<char*>(value.data());
                while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t')
                    data_p++;
                std::vector<std::string> s_knot;
                char* token =
                        strtok(const_cast<char*>(value.c_str()), delimiters);
                while (token != nullptr) {
                    float f = mAtof(token);
                    token = strtok(nullptr, delimiters);
                    s_knot.push_back(std::to_string(f));
                }

                //std::vector<std::string> s_knot(
                //        std::sregex_token_iterator(value.begin(), value.end(),
                //                                   pattern, -1),
                //        std::sregex_token_iterator());
                //if (s_knot.back() == "") s_knot.pop_back();
                //if (*s_knot.begin() == "")
                //    s_knot = std::vector<std::string>(s_knot.begin() + 1,
                //                                      s_knot.end());

                assert(static_cast<int>(s_knot.size()) ==
                       cpt[i] + degree[i] + 1);
                for (int j = 0; j < s_knot.size(); ++j)
                    knots[i].push_back(std::stod(s_knot[j]));

                double interval = knots[i].back() - knots[i].front();
                double begin = knots[i].front();
                for (int j = 0; j < knots[i].size(); ++j)
                    knots[i][j] = (knots[i][j] - begin) / interval;
            }

            value = item->FirstChildElement("weights")->GetText();
            data_p = const_cast<char*>(value.data());
            while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t')
                data_p++;
            std::vector<std::string> s_weight;
            token = strtok(const_cast<char*>(value.c_str()), delimiters);
            while (token != nullptr) {
                float f = mAtof(token);
                token = strtok(nullptr, delimiters);
                s_weight.push_back(std::to_string(f));
            }
            //std::vector<std::string> s_weight(
            //        std::sregex_token_iterator(value.begin(), value.end(),
            //                                   pattern, -1),
            //        std::sregex_token_iterator());
            //if (s_weight.back() == "") s_weight.pop_back();
            //if (*s_weight.begin() == "")
            //    s_weight = std::vector<std::string>(s_weight.begin() + 1,
            //                                        s_weight.end());
            assert(static_cast<int>(s_weight.size()) == cptNum);
            for (int i = 0; i < cptNum; ++i)
                weights.emplace_back(std::stod(s_weight[i]));

            value = item->FirstChildElement("points")->GetText();

            data_p = const_cast<char*>(value.data());
            while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t')
                data_p++;
            std::vector<std::string> s_points;
            token = strtok(const_cast<char*>(value.c_str()), delimiters);
            while (token != nullptr) {
                float f = mAtof(token);
                token = strtok(nullptr, delimiters);
                s_points.push_back(std::to_string(f));
            }
            //std::vector<std::string> s_points(
            //        std::sregex_token_iterator(value.begin(), value.end(), pattern,
            //                                   -1),
            //        std::sregex_token_iterator());
            //if (s_points.back() == "") s_points.pop_back();
            //if (*s_points.begin() == "")
            //    s_points = std::vector<std::string>(s_points.begin() + 1,
            //                                        s_points.end());
            assert(static_cast<int>(s_points.size()) == cptNum * 3);

            for (int i = 0; i < cptNum; ++i)
                points.push_back({std::stof(s_points[i * 3]),
                                  std::stof(s_points[i * 3 + 1]),
                                  std::stof(s_points[i * 3 + 2])});

            std::shared_ptr<Geo> patch;
            if (num == 1)
                patch = std::make_shared<Curve>(degree[0], points, knots[0],
                                                weights);
            else if (num == 2)
                patch = std::make_shared<Surface>(degree[0], degree[1], points,
                                                  knots[0], knots[1], weights);
            else
                patch = std::make_shared<Volume>(degree[0], degree[1],
                                                 degree[2], points, knots[0],
                                                 knots[1], knots[2], weights);

            geometry.addPatch(patch);
        }
        std::regex pat(" ");
        auto bdyEle = root->FirstChildElement("boundary");
        std::vector<std::array<int, 2>> boundary;
        if (bdyEle && bdyEle->GetText() != 0) {
            std::string s_bdy = bdyEle->GetText();
            char* data_p = const_cast<char*>(s_bdy.data());
            while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t')
                data_p++;
            std::vector<std::string> bdy;
            char* token = strtok(const_cast<char*>(s_bdy.c_str()), delimiters);
            while (token != nullptr) {
                int i = mAtoi(token);
                token = strtok(nullptr, delimiters);
                bdy.push_back(std::to_string(i));
            }
            //std::vector<std::string> bdy(
            //        std::sregex_token_iterator(s_bdy.begin(), s_bdy.end(), pat,
            //                                   -1),
            //        std::sregex_token_iterator());
            for (int i = 0; i < bdy.size() / 2; ++i)
                boundary.push_back(
                        {std::stoi(bdy[2 * i]), std::stoi(bdy[2 * i + 1])});
        } else if (s_type == "volume") {
            printf("error!!! Volume need boundary to draw\n");
        }
        geometry.setBoundaryInfo(boundary);
    }

    // get max gap
    double init_max_gap = 0.01;
    double centerX = 0;
    double centerY = 0;
    double centerZ = 0;
    {
        auto minX = std::numeric_limits<double>::max();
        auto maxX = -std::numeric_limits<double>::max();
        auto minY = std::numeric_limits<double>::max();
        auto maxY = -std::numeric_limits<double>::max();
        auto minZ = std::numeric_limits<double>::max();
        auto maxZ = -std::numeric_limits<double>::max();
        for (auto& patch: geometry.m_Geometry) {
            auto& cc = patch->m_ControlPoints;
            for (auto& geo_point: cc) {
                minX = std::min(minX, geo_point[0]),
                maxX = std::max(maxX, geo_point[0]);
                minY = std::min(minY, geo_point[1]),
                maxY = std::max(maxY, geo_point[1]);
                minZ = std::min(minZ, geo_point[2]),
                maxZ = std::max(maxZ, geo_point[2]);
            }
        }

        centerX = (minX + maxX) / 2;
        centerY = (minY + maxY) / 2;
        centerZ = (minZ + maxZ) / 2;

        init_max_gap = fmax(maxX - minX, maxY - minY);
        init_max_gap = fmax(init_max_gap, maxZ - minZ);
    }


    // collect data
    {
        Points::Pointer points = m_Data.GetPoints();
        CellArray::Pointer faces = m_Data.GetFaces();

        /// Todo Curve VOLUME
        if (geometry.m_Geometry.size() == 0) return false;

        enum MeshType { NURBSCURVE, NURBSSURFACE, NURBSVOLUME };

        MeshType meshType;

        if ((geometry.m_Geometry)[0]->type == Geo::Surface) {
            meshType = MeshType::NURBSSURFACE;
        } else if ((geometry.m_Geometry)[0]->type == Geo::Curve) {
            meshType = MeshType::NURBSCURVE;
        } else if ((geometry.m_Geometry)[0]->type == Geo::Volume) {
            meshType = MeshType::NURBSVOLUME;
        }

        int control_face_cnt = 0;
        long long all_sample_face_cnt = 0;
        //曲线
        int control_interval_cnt = 0;
        long long all_sample_interval_cnt = 0;
        //曲体
        int control_grid_cnt = 0;
        long long all_sample_grid_cnt = 0;


        if (meshType == NURBSSURFACE) {
            int SAMPLE_NUM = (1000 / geometry.m_Geometry.size());
            SAMPLE_NUM = std::max(SAMPLE_NUM, 1);
            SAMPLE_NUM = std::min(SAMPLE_NUM, 100);

            // 1. 传入控制顶点
            for (auto& patch: geometry.m_Geometry) {

                int u_control_cnt = patch->m_Basis[0].getControlSize();
                int v_control_cnt = patch->m_Basis[1].getControlSize();

                for (int u_id = 0; u_id < u_control_cnt; ++u_id) {
                    for (int v_id = 0; v_id < v_control_cnt; ++v_id) {
                        auto v = patch->m_ControlPoints[u_id +
                                                        v_id * u_control_cnt];
                        points->AddPoint((v[0] - centerX) / init_max_gap * 2,
                                         (v[1] - centerY) / init_max_gap * 2,
                                         (v[2] - centerZ) / init_max_gap * 2);
                    }
                }
                control_face_cnt = (u_control_cnt) * (v_control_cnt - 1);
            }

            // 2. 绘制控制顶点连线
            for (auto& patch: geometry.m_Geometry) {

                int u_control_cnt = patch->m_Basis[0].getControlSize();
                int v_control_cnt = patch->m_Basis[1].getControlSize();

                for (int u_id = 0; u_id < u_control_cnt; ++u_id) {
                    for (int v_id = 0; v_id < v_control_cnt - 1; ++v_id) {
                        faces->AddCellId2(u_id * u_control_cnt + v_id,
                                          u_id * u_control_cnt + v_id + 1);
                        faces->AddCellId2(v_id * u_control_cnt + u_id,
                                          v_id * u_control_cnt + u_id +
                                                  u_control_cnt);
                    }
                }
            }

            // 3.传入离散化面片
            for (auto& patch: geometry.m_Geometry) {
                double sample_gap = 1.f / SAMPLE_NUM;
                for (int u_sample = 0; u_sample < SAMPLE_NUM; ++u_sample) {
                    for (int v_sample = 0; v_sample < SAMPLE_NUM; ++v_sample) {
                        all_sample_face_cnt++;

                        std::vector<double> v_0{u_sample * sample_gap,
                                                v_sample * sample_gap};
                        auto v0 = patch->getPointAtParam(v_0);

                        std::vector<double> v_1{(u_sample + 1) * sample_gap,
                                                v_sample * sample_gap};
                        auto v1 = patch->getPointAtParam(v_1);

                        std::vector<double> v_2{(u_sample + 1) * sample_gap,
                                                (v_sample + 1) * sample_gap};
                        auto v2 = patch->getPointAtParam(v_2);

                        std::vector<double> v_3{u_sample * sample_gap,
                                                (v_sample + 1) * sample_gap};
                        auto v3 = patch->getPointAtParam(v_3);

                        std::vector<std::vector<double>*> tempV{&v0, &v1, &v2,
                                                                &v3};
                        auto offset = points->GetNumberOfPoints();
                        for (auto v: tempV) {
                            points->AddPoint(
                                    ((*v)[0] - centerX) / init_max_gap * 2,
                                    ((*v)[1] - centerY) / init_max_gap * 2,
                                    ((*v)[2] - centerZ) / init_max_gap * 2);
                        }
                        faces->AddCellId3(offset, offset + 1, offset + 2);
                        faces->AddCellId3(offset, offset + 2, offset + 3);
                    }
                }
            }

            std::cout << control_face_cnt << " " << all_sample_face_cnt
                      << std::endl;
        } else if (meshType == NURBSCURVE) {
            /// TODO:
            int SAMPLE_NUM = (1000 / geometry.m_Geometry.size());
            SAMPLE_NUM = std::max(SAMPLE_NUM, 1);
            SAMPLE_NUM = std::min(SAMPLE_NUM, 100);

            // 1. 传入控制顶点
            for (auto& patch: geometry.m_Geometry) {
                int u_control_cnt = patch->m_Basis[0].getControlSize();

                for (int u_id = 0; u_id < u_control_cnt; ++u_id) {
                    auto v = patch->m_ControlPoints[u_id];
                    points->AddPoint((v[0] - centerX) / init_max_gap * 2,
                                     (v[1] - centerY) / init_max_gap * 2,
                                     (v[2] - centerZ) / init_max_gap * 2);
                }
                control_interval_cnt++;
            }

            // 2. 绘制控制顶点连线
            for (auto& patch: geometry.m_Geometry) {
                int u_control_cnt = patch->m_Basis[0].getControlSize();
                for (int u_id = 0; u_id < u_control_cnt - 1; ++u_id) {
                    faces->AddCellId2(u_id, u_id + 1);
                }
            }

            // 3. 离散化的采样区间（线段）绘制
            for (auto& patch: geometry.m_Geometry) {
                double sample_gap = 1.f / SAMPLE_NUM;

                auto offset = points->GetNumberOfPoints();
                for (int u_sample = 0; u_sample < SAMPLE_NUM; ++u_sample) {
                    all_sample_interval_cnt++;

                    std::vector<double> v_0{u_sample * sample_gap};
                    auto v0 = patch->getPointAtParam(v_0);

                    std::vector<std::vector<double>*> tempV{&v0};
                    for (auto v: tempV) {
                        points->AddPoint(((*v)[0] - centerX) / init_max_gap * 2,
                                         ((*v)[1] - centerY) / init_max_gap * 2,
                                         ((*v)[2] - centerZ) / init_max_gap *
                                                 2);
                    }

                    if (u_sample != SAMPLE_NUM - 1) {
                        faces->AddCellId2(offset + u_sample,
                                          offset + u_sample + 1);
                    }
                }
            }

            std::cout << control_interval_cnt << " " << all_sample_interval_cnt
                      << std::endl;
        } else if (meshType == NURBSVOLUME) {

            // 1. 传入控制顶点 & 传入控制顶点连线
            for (auto patch: geometry.m_Geometry) {
                int u_control_cnt = patch->m_Basis[0].getControlSize();
                int v_control_cnt = patch->m_Basis[1].getControlSize();
                int w_control_cnt = patch->m_Basis[2].getControlSize();
                //创建三维数组存储
                int uvCnt = u_control_cnt * v_control_cnt, uCnt = u_control_cnt;

                for (int u_id = 0; u_id < u_control_cnt - 1; ++u_id) {
                    for (int v_id = 0; v_id < v_control_cnt - 1; ++v_id) {
                        for (int w_id = 0; w_id < w_control_cnt - 1; ++w_id) {
                            control_grid_cnt++;
                            std::vector<int> cube_id(8);
                            cube_id[0] = uvCnt * w_id + uCnt * v_id + u_id;
                            cube_id[1] = uvCnt * w_id + uCnt * v_id + u_id + 1;
                            cube_id[2] =
                                    uvCnt * (w_id + 1) + uCnt * v_id + u_id + 1;
                            cube_id[3] =
                                    uvCnt * (w_id + 1) + uCnt * v_id + u_id;
                            cube_id[4] =
                                    uvCnt * w_id + uCnt * (v_id + 1) + u_id;
                            cube_id[5] =
                                    uvCnt * w_id + uCnt * (v_id + 1) + u_id + 1;
                            cube_id[6] = uvCnt * (w_id + 1) +
                                         uCnt * (v_id + 1) + u_id + 1;
                            cube_id[7] = uvCnt * (w_id + 1) +
                                         uCnt * (v_id + 1) + u_id;

                            std::vector<std::vector<int>> edges = {
                                    {0, 1}, {1, 2}, {2, 3}, {3, 0},
                                    {0, 4}, {1, 5}, {2, 6}, {3, 7},
                                    {4, 5}, {5, 6}, {6, 7}, {7, 4},
                            };

                            auto offset = points->GetNumberOfPoints();
                            for (auto edge: edges) {
                                faces->AddCellId2(offset + edge[0],
                                                  offset + edge[1]);
                                for (auto vid: edge) {
                                    auto v = patch->m_ControlPoints
                                                     [cube_id[vid]];
                                    points->AddPoint(
                                            (v[0] - centerX) / init_max_gap * 2,
                                            (v[1] - centerY) / init_max_gap * 2,
                                            (v[2] - centerZ) / init_max_gap *
                                                    2);
                                }
                            }
                        }
                    }
                }
            }

            // 3.传入离散化面片
            int SAMPLE_NUM = (1000 / geometry.m_Geometry.size());
            SAMPLE_NUM = std::max(SAMPLE_NUM, 1);
            SAMPLE_NUM = std::min(SAMPLE_NUM, 100);
            double sample_gap = 1.f / SAMPLE_NUM;

            for (auto arr: geometry.m_Boundary) {
                int patch_id = arr[0];
                auto currentPatch = geometry.m_Geometry[patch_id];
                if (arr[1] == 0) {
                    //u=0oru=1,固定u的大小
                    for (int v_sample = 0; v_sample < SAMPLE_NUM; ++v_sample) {
                        for (int w_sample = 0; w_sample < SAMPLE_NUM;
                             ++w_sample) {
                            all_sample_grid_cnt++;
                            //用离散采样点获得绘制点
                            std::vector<double> v_0{0, w_sample * sample_gap,
                                                    v_sample * sample_gap, 0};
                            auto v0 = currentPatch->getPointAtParam(v_0);

                            std::vector<double> v_1{0,
                                                    (w_sample + 1) * sample_gap,
                                                    v_sample * sample_gap};
                            auto v1 = currentPatch->getPointAtParam(v_1);

                            std::vector<double> v_2{
                                    0, (w_sample + 1) * sample_gap,
                                    (v_sample + 1) * sample_gap};
                            auto v2 = currentPatch->getPointAtParam(v_2);

                            std::vector<double> v_3{0, w_sample * sample_gap,
                                                    (v_sample + 1) *
                                                            sample_gap};
                            auto v3 = currentPatch->getPointAtParam(v_3);


                            std::vector<std::vector<double>*> tempV{&v0, &v1,
                                                                    &v2, &v3};
                            auto offset = points->GetNumberOfPoints();
                            for (auto v: tempV) {
                                points->AddPoint(
                                        ((*v)[0] - centerX) / init_max_gap * 2,
                                        ((*v)[1] - centerY) / init_max_gap * 2,
                                        ((*v)[2] - centerZ) / init_max_gap * 2);
                            }
                            faces->AddCellId3(offset, offset + 1, offset + 2);
                            faces->AddCellId3(offset, offset + 2, offset + 3);
                        }
                    }
                } else if (arr[1] == 1) {
                    for (int v_sample = 0; v_sample < SAMPLE_NUM; ++v_sample) {
                        for (int w_sample = 0; w_sample < SAMPLE_NUM;
                             ++w_sample) {
                            all_sample_grid_cnt++;

                            //用离散采样点获得绘制点
                            std::vector<double> v_0{1, w_sample * sample_gap,
                                                    v_sample * sample_gap, 0};
                            auto v0 = currentPatch->getPointAtParam(v_0);

                            std::vector<double> v_1{1,
                                                    (w_sample + 1) * sample_gap,
                                                    v_sample * sample_gap};
                            auto v1 = currentPatch->getPointAtParam(v_1);

                            std::vector<double> v_2{
                                    1, (w_sample + 1) * sample_gap,
                                    (v_sample + 1) * sample_gap};
                            auto v2 = currentPatch->getPointAtParam(v_2);

                            std::vector<double> v_3{1, w_sample * sample_gap,
                                                    (v_sample + 1) *
                                                            sample_gap};
                            auto v3 = currentPatch->getPointAtParam(v_3);


                            std::vector<std::vector<double>*> tempV{&v0, &v1,
                                                                    &v2, &v3};
                            auto offset = points->GetNumberOfPoints();
                            for (auto v: tempV) {
                                points->AddPoint(
                                        ((*v)[0] - centerX) / init_max_gap * 2,
                                        ((*v)[1] - centerY) / init_max_gap * 2,
                                        ((*v)[2] - centerZ) / init_max_gap * 2);
                            }
                            faces->AddCellId3(offset, offset + 1, offset + 2);
                            faces->AddCellId3(offset, offset + 2, offset + 3);
                        }
                    }
                } else if (arr[1] == 2) {
                    for (int w_sample = 0; w_sample < SAMPLE_NUM; ++w_sample) {
                        for (int u_sample = 0; u_sample < SAMPLE_NUM;
                             ++u_sample) {
                            all_sample_grid_cnt++;

                            //用离散采样点获得绘制点
                            std::vector<double> v_0{u_sample * sample_gap, 0,
                                                    w_sample * sample_gap};
                            auto v0 = currentPatch->getPointAtParam(v_0);

                            std::vector<double> v_1{(u_sample + 1) * sample_gap,
                                                    0, w_sample * sample_gap};
                            auto v1 = currentPatch->getPointAtParam(v_1);

                            std::vector<double> v_2{
                                    (u_sample + 1) * sample_gap, 0,
                                    (w_sample + 1) * sample_gap};
                            auto v2 = currentPatch->getPointAtParam(v_2);

                            std::vector<double> v_3{u_sample * sample_gap, 0,
                                                    (w_sample + 1) *
                                                            sample_gap};
                            auto v3 = currentPatch->getPointAtParam(v_3);


                            std::vector<std::vector<double>*> tempV{&v0, &v1,
                                                                    &v2, &v3};
                            auto offset = points->GetNumberOfPoints();
                            for (auto v: tempV) {
                                points->AddPoint(
                                        ((*v)[0] - centerX) / init_max_gap * 2,
                                        ((*v)[1] - centerY) / init_max_gap * 2,
                                        ((*v)[2] - centerZ) / init_max_gap * 2);
                            }
                            faces->AddCellId3(offset, offset + 1, offset + 2);
                            faces->AddCellId3(offset, offset + 2, offset + 3);
                        }
                    }
                } else if (arr[1] == 3) {
                    for (int w_sample = 0; w_sample < SAMPLE_NUM; ++w_sample) {
                        for (int u_sample = 0; u_sample < SAMPLE_NUM;
                             ++u_sample) {
                            all_sample_grid_cnt++;

                            //用离散采样点获得绘制点
                            std::vector<double> v_0{u_sample * sample_gap, 1,
                                                    w_sample * sample_gap};
                            auto v0 = currentPatch->getPointAtParam(v_0);

                            std::vector<double> v_1{(u_sample + 1) * sample_gap,
                                                    1, w_sample * sample_gap};
                            auto v1 = currentPatch->getPointAtParam(v_1);

                            std::vector<double> v_2{
                                    (u_sample + 1) * sample_gap, 1,
                                    (w_sample + 1) * sample_gap};
                            auto v2 = currentPatch->getPointAtParam(v_2);

                            std::vector<double> v_3{u_sample * sample_gap, 1,
                                                    (w_sample + 1) *
                                                            sample_gap};
                            auto v3 = currentPatch->getPointAtParam(v_3);


                            std::vector<std::vector<double>*> tempV{&v0, &v1,
                                                                    &v2, &v3};
                            auto offset = points->GetNumberOfPoints();
                            for (auto v: tempV) {
                                points->AddPoint(
                                        ((*v)[0] - centerX) / init_max_gap * 2,
                                        ((*v)[1] - centerY) / init_max_gap * 2,
                                        ((*v)[2] - centerZ) / init_max_gap * 2);
                            }
                            faces->AddCellId3(offset, offset + 1, offset + 2);
                            faces->AddCellId3(offset, offset + 2, offset + 3);
                        }
                    }
                } else if (arr[1] == 4) {
                    for (int v_sample = 0; v_sample < SAMPLE_NUM; ++v_sample) {
                        for (int u_sample = 0; u_sample < SAMPLE_NUM;
                             ++u_sample) {
                            all_sample_grid_cnt++;

                            //用离散采样点获得绘制点
                            std::vector<double> v_0{u_sample * sample_gap,
                                                    v_sample * sample_gap, 0};
                            auto v0 = currentPatch->getPointAtParam(v_0);

                            std::vector<double> v_1{(u_sample + 1) * sample_gap,
                                                    v_sample * sample_gap, 0};
                            auto v1 = currentPatch->getPointAtParam(v_1);

                            std::vector<double> v_2{(u_sample + 1) * sample_gap,
                                                    (v_sample + 1) * sample_gap,
                                                    0};
                            auto v2 = currentPatch->getPointAtParam(v_2);

                            std::vector<double> v_3{u_sample * sample_gap,
                                                    (v_sample + 1) * sample_gap,
                                                    0};
                            auto v3 = currentPatch->getPointAtParam(v_3);


                            std::vector<std::vector<double>*> tempV{&v0, &v1,
                                                                    &v2, &v3};
                            auto offset = points->GetNumberOfPoints();
                            for (auto v: tempV) {
                                points->AddPoint(
                                        ((*v)[0] - centerX) / init_max_gap * 2,
                                        ((*v)[1] - centerY) / init_max_gap * 2,
                                        ((*v)[2] - centerZ) / init_max_gap * 2);
                            }
                            faces->AddCellId3(offset, offset + 1, offset + 2);
                            faces->AddCellId3(offset, offset + 2, offset + 3);
                        }
                    }

                } else if (arr[1] == 5) {
                    for (int w_sample = 0; w_sample < SAMPLE_NUM; ++w_sample) {
                        for (int u_sample = 0; u_sample < SAMPLE_NUM;
                             ++u_sample) {
                            all_sample_grid_cnt++;

                            //用离散采样点获得绘制点
                            std::vector<double> v_0{u_sample * sample_gap,
                                                    w_sample * sample_gap, 1};
                            auto v0 = currentPatch->getPointAtParam(v_0);

                            std::vector<double> v_1{(u_sample + 1) * sample_gap,
                                                    w_sample * sample_gap, 1};
                            auto v1 = currentPatch->getPointAtParam(v_1);

                            std::vector<double> v_2{(u_sample + 1) * sample_gap,
                                                    (w_sample + 1) * sample_gap,
                                                    1};
                            auto v2 = currentPatch->getPointAtParam(v_2);

                            std::vector<double> v_3{u_sample * sample_gap,
                                                    (w_sample + 1) * sample_gap,
                                                    1};
                            auto v3 = currentPatch->getPointAtParam(v_3);


                            std::vector<std::vector<double>*> tempV{&v0, &v1,
                                                                    &v2, &v3};
                            auto offset = points->GetNumberOfPoints();
                            for (auto v: tempV) {
                                points->AddPoint(
                                        ((*v)[0] - centerX) / init_max_gap * 2,
                                        ((*v)[1] - centerY) / init_max_gap * 2,
                                        ((*v)[2] - centerZ) / init_max_gap * 2);
                            }
                            faces->AddCellId3(offset, offset + 1, offset + 2);
                            faces->AddCellId3(offset, offset + 2, offset + 3);
                        }
                    }
                }
            }
        }
    }


    return true;
}
bool NurbsReader::CreateDataObject() {
    SurfaceMesh::Pointer mesh = SurfaceMesh::New();

    mesh->SetPoints(m_Data.GetPoints());
    mesh->SetFaces(m_Data.GetFaces());
    mesh->SetViewStyle(IG_SURFACE);
    //mesh->SetViewStyle(IG_POINTS | IG_WIREFRAME | IG_SURFACE);

    m_Output = mesh;

    //if (m_nurbs_type == 0) {
    //
    //} else if (m_nurbs_type == 1) {
    //
    //} else if (m_nurbs_type == 2) {
    //}

    return true;
}

IGAME_NAMESPACE_END
