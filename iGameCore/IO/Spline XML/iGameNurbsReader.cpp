/**
 * @class   iGameNurbsReader
 * @brief   iGameNurbsReader's brief
 */


#include "iGameNurbsReader.h"
#include <iGameFileReader.h>
#include <regex>
#include <tinyxml2.h>

IGAME_NAMESPACE_BEGIN

NurbsReader::NurbsReader() {
    SetNumberOfOutputs(1);
    SetNumberOfInputs(0);
    SetOutput(0, m_Output);
}

NurbsReader::~NurbsReader() { /*delete m_Geometry;*/ }

bool NurbsReader::Parsing() {
    // read xml file information
    {
        auto* item = root->FirstChildElement();
        const char* type = item->Value();
        std::string s_type = type;
        const char* delimiters = " \n";

        for (auto* item = root->FirstChildElement(type); item; item = item->NextSiblingElement(type)) {

            int num = s_type == "curve" ? 1 : (s_type == "surface" ? 2 : (s_type == "volume" ? 3 : 0));
            m_NurbsType = num;
            if (num == 0) {
                printf("xml tag error\n");
                return false;
            }

            std::vector<int> degree(num), cpt(num);
            std::vector<double> weights;
            std::vector<std::vector<double>> knots(num);
            std::vector<std::vector<double>> points;

            std::regex pattern(" ");

            std::string value = item->FirstChildElement("degree")->GetText();

            char* data_p = const_cast<char*>(value.data());
            while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t') data_p++;
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
            while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t') data_p++;
            std::vector<std::string> s_number;
            token = strtok(const_cast<char*>(value.c_str()), delimiters);
            while (token != nullptr) {
                int i = mAtoi(token);
                token = strtok(nullptr, delimiters);
                s_number.push_back(std::to_string(i));
            }

            assert(static_cast<int>(s_number.size()) == num);
            for (int i = 0; i < num; ++i) cpt[i] = std::stoi(s_number[i]), cptNum *= cpt[i];

            tinyxml2::XMLElement* knot_item = nullptr;
            for (int i = 0; i < num; ++i) {
                knot_item = knot_item ? knot_item->NextSiblingElement("knots") : item->FirstChildElement("knots");
                value = knot_item->GetText();

                char* data_p = const_cast<char*>(value.data());
                while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t') data_p++;
                std::vector<std::string> s_knot;
                char* token = strtok(const_cast<char*>(value.c_str()), delimiters);
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

                assert(static_cast<int>(s_knot.size()) == cpt[i] + degree[i] + 1);
                for (int j = 0; j < s_knot.size(); ++j) knots[i].push_back(std::stod(s_knot[j]));

                double interval = knots[i].back() - knots[i].front();
                double begin = knots[i].front();
                for (int j = 0; j < knots[i].size(); ++j) knots[i][j] = (knots[i][j] - begin) / interval;
            }

            value = item->FirstChildElement("weights")->GetText();
            data_p = const_cast<char*>(value.data());
            while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t') data_p++;
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
            for (int i = 0; i < cptNum; ++i) weights.emplace_back(std::stod(s_weight[i]));

            value = item->FirstChildElement("points")->GetText();

            data_p = const_cast<char*>(value.data());
            while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t') data_p++;
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
                points.push_back(
                        {std::stof(s_points[i * 3]), std::stof(s_points[i * 3 + 1]), std::stof(s_points[i * 3 + 2])});

            std::shared_ptr<NurbsSDK::Geo> patch;
            if (num == 1) patch = std::make_shared<NurbsSDK::Curve>(degree[0], points, knots[0], weights);
            else if (num == 2)
                patch = std::make_shared<NurbsSDK::Surface>(degree[0], degree[1], points, knots[0], knots[1], weights);
            else
                patch = std::make_shared<NurbsSDK::Volume>(degree[0], degree[1], degree[2], points, knots[0], knots[1],
                                                           knots[2], weights);

            m_Patchs.push_back(patch);
            //m_Geometry.addPatch(patch);
        }

        std::regex pat(" ");
        auto bdyEle = root->FirstChildElement("boundary");
        //std::vector<std::array<int, 2>> boundary;
        if (bdyEle && bdyEle->GetText() != 0) {
            std::string s_bdy = bdyEle->GetText();
            char* data_p = const_cast<char*>(s_bdy.data());
            while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t') data_p++;
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
                m_Boundary.push_back({std::stoi(bdy[2 * i]), std::stoi(bdy[2 * i + 1])});
        } else if (s_type == "volume") {
            printf("error!!! Volume need boundary to draw\n");
        }
        //m_Geometry.setBoundaryInfo(boundary);
    }

    return true;
}
bool NurbsReader::CreateDataObject() {
    NurbsGeometry::Pointer mesh = NurbsGeometry::New();

    mesh->SetPatch(m_Patchs);
    mesh->SetBoundary(m_Boundary);
    if (m_NurbsType == 1) {
        mesh->SetType(NurbsSDK::NurbsType::CURVE);
    } else if (m_NurbsType == 2) {
        mesh->SetType(NurbsSDK::NurbsType::SURFACE);
    } else if (m_NurbsType == 3) {
        mesh->SetType(NurbsSDK::NurbsType::VOLUME);
    }
    mesh->SetViewStyle(IG_WIREFRAME | IG_SURFACE);
    //mesh->SetViewStyle(IG_POINTS | IG_WIREFRAME | IG_SURFACE);

    m_Output = mesh;

    return true;
}

IGAME_NAMESPACE_END
