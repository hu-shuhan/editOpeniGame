/**
 * @class   iGameSplineReaderCPU
 * @brief   iGameSplineReaderCPU's brief
 */


#include "iGameSplineReaderCPU.h"

#include <cmath>
#include <iGameFileReader.h>
#include <regex>
#include <tinyxml2.h>

IGAME_NAMESPACE_BEGIN

SplineReaderCPU::SplineReaderCPU() {
    SetNumberOfOutputs(1);
    SetNumberOfInputs(0);
    SetOutput(0, m_Output);
    m_SplineType = SplineUtils::Type::SURFACE;
}

SplineReaderCPU::~SplineReaderCPU() = default;

bool SplineReaderCPU::Parsing() {
    // clear any previous patches
    m_Patchs.clear();

    const char* delimiters = " \n\t\r";

    auto parseGeometry = [&](tinyxml2::XMLElement* geometry) -> bool {
        if (!geometry) return false;
        // Basis may be nested; support both 'Basis' and 'basis'
        auto* basisRoot = geometry->FirstChildElement("Basis");
        if (!basisRoot) {
            IGAME_CORE_ERROR("[SplineReaderCPU]: <basis> not found under <geometry>.");
            return false;
        }
        // collect knot vectors from nested basis nodes
        std::vector<std::vector<double>> knots;
        auto parseKnotVectorText = [&](const char* text) {
            std::vector<double> kv;
            if (!text) return kv;
            std::string value = text;
            char* data_p = const_cast<char*>(value.data());
            while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t' || *data_p == '\r') data_p++;
            char* token = strtok(const_cast<char*>(value.c_str()), delimiters);
            while (token != nullptr) {
                float f = mAtof(token);
                token = strtok(nullptr, delimiters);
                kv.push_back(static_cast<double>(f));
            }
            return kv;
        };

        tinyxml2::XMLElement* container = nullptr;
        if (auto* b1 = basisRoot->FirstChildElement("Basis")) {
            container = b1->FirstChildElement("Basis") ? b1 : basisRoot;
        }
        if (container) {
            for (auto* b = container->FirstChildElement("Basis"); b; b = b->NextSiblingElement("Basis")) {
                auto* kvElem = b->FirstChildElement("KnotVector");
                if (!kvElem) { kvElem = b->FirstChildElement("KnotVector"); }
                if (!kvElem) { kvElem = b->FirstChildElement("KnotVector"); }
                if (kvElem) {
                    auto kv = parseKnotVectorText(kvElem->GetText());
                    if (!kv.empty()) { knots.emplace_back(std::move(kv)); }
                }
            }
        } else {
            if (auto* kvElem = basisRoot->FirstChildElement("KnotVector"); kvElem) {
                auto kv = parseKnotVectorText(kvElem->GetText());
                if (!kv.empty()) { knots.emplace_back(std::move(kv)); }
            }
        }

        const int num = static_cast<int>(knots.size());
        if (num <= 0 || num > 3) {
            IGAME_CORE_ERROR("[SplineReaderCPU]: Unsupported number of parametric directions: {}", num);
            return false;
        }
        // Determine type by first geometry; keep type for consistency
        auto inferType = [&](int n) {
            return n == 1 ? SplineUtils::Type::CURVE
                          : (n == 2 ? SplineUtils::Type::SURFACE : SplineUtils::Type::VOLUME);
        };
        SplineUtils::Type t = inferType(num);
        if (m_Patchs.empty()) { m_SplineType = t; }

        // parse weights (inside basis or geometry)
        std::vector<double> weights;
        if (auto* weightsElem = geometry->FirstChildElement("weights")) {
            std::string value = weightsElem->GetText() ? weightsElem->GetText() : "";
            char* data_p = const_cast<char*>(value.data());
            while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t' || *data_p == '\r') data_p++;
            char* token = strtok(const_cast<char*>(value.c_str()), delimiters);
            while (token != nullptr) {
                float f = mAtof(token);
                token = strtok(nullptr, delimiters);
                weights.emplace_back(static_cast<double>(f));
            }
        }

        // parse control points: prefer <coefs>/<Coefs>
        tinyxml2::XMLElement* coefs = geometry->FirstChildElement("coefs");
        if (!coefs) {
            IGAME_CORE_ERROR("[SplineReaderCPU]: Neither <coefs> found under <geometry>.");
            return false;
        }
        std::string value = coefs->GetText() ? coefs->GetText() : "";
        char* data_p = const_cast<char*>(value.data());
        while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t' || *data_p == '\r') data_p++;
        std::vector<std::string> s_points;
        char* token = strtok(const_cast<char*>(value.c_str()), delimiters);
        while (token != nullptr) {
            float f = mAtof(token);
            token = strtok(nullptr, delimiters);
            s_points.push_back(std::to_string(f));
        }

        // parse control points: prefer <coefs>/<Coefs>
        tinyxml2::XMLElement* e_scalars = geometry->FirstChildElement("scalars");
        std::vector<std::string> s_scalars;
        if (e_scalars) {
            std::string value = e_scalars->GetText() ? e_scalars->GetText() : "";
            char* data_p = const_cast<char*>(value.data());
            while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t' || *data_p == '\r') data_p++;
            char* token = strtok(const_cast<char*>(value.c_str()), delimiters);
            while (token != nullptr) {
                float f = mAtof(token);
                token = strtok(nullptr, delimiters);
                s_scalars.push_back(std::to_string(f));
            }
        }

        // infer degrees and control point counts from knot vectors (clamped open assumption)
        std::vector<int> degree(num), cpt(num);
        for (int d = 0; d < num; ++d) {
            int startMult = 1;
            const double first = knots[d].front();
            const double eps = 1e-9;
            for (size_t k = 1; k < knots[d].size(); ++k) {
                if (std::fabs(knots[d][k] - first) < eps) {
                    ++startMult;
                } else {
                    break;
                }
            }
            degree[d] = std::max(0, startMult - 1);
            cpt[d] = static_cast<int>(knots[d].size()) - degree[d] - 1;
            if (cpt[d] <= 0) {
                IGAME_CORE_ERROR("[SplineReaderCPU]: Invalid control point count inferred from knots.");
                return false;
            }
        }
        int cptNum = 1;
        for (int d = 0; d < num; ++d) { cptNum *= cpt[d]; }
        // default or fix weights length
        if (weights.empty()) {
            weights.assign(cptNum, 1.0);
        } else if ((int) weights.size() < cptNum) {
            IGAME_CORE_WARN("[SplineReaderCPU]: weights size ({}) < expected ({}), padding with 1.0.",
                            (int) weights.size(), cptNum);
            weights.resize(cptNum, 1.0);
        } else if ((int) weights.size() > cptNum) {
            IGAME_CORE_WARN("[SplineReaderCPU]: weights size ({}) > expected ({}), truncating.", (int) weights.size(),
                            cptNum);
            weights.resize(cptNum);
        }
        if ((int) s_points.size() != cptNum * 3) {
            IGAME_CORE_ERROR("[SplineReaderCPU]: control points count ({}) does not match expected ({}).",
                             (int) s_points.size(), cptNum * 3);
            return false;
        }

        std::vector<std::vector<double>> points;
        points.reserve(cptNum);
        for (int i = 0; i < cptNum; ++i) {
            points.push_back(
                    {std::stof(s_points[i * 3]), std::stof(s_points[i * 3 + 1]), std::stof(s_points[i * 3 + 2])});
        }

        std::vector<SplineUtils::Scalar> scalars;
        int dimension = s_scalars.size() / cptNum;
        if (s_scalars.size()) {
            scalars.reserve(cptNum);
            for (int i = 0; i < cptNum; ++i) {
                SplineUtils::Scalar scalar;
                for (int j = 0; j < dimension; j++) { scalar.push_back(std::stof(s_scalars[i * dimension + j])); }
                scalars.push_back(scalar);
            }
        }

        // normalize knots to [0,1]
        for (int d = 0; d < num; ++d) {
            const double interval = knots[d].back() - knots[d].front();
            const double begin = knots[d].front();
            if (std::fabs(interval) > 0) {
                for (size_t j = 0; j < knots[d].size(); ++j) knots[d][j] = (knots[d][j] - begin) / interval;
            }
        }

        std::shared_ptr<SplineUtils::Geo> patch;
        if (num == 1) {
            patch = std::make_shared<SplineUtils::Curve>(degree[0], points, knots[0], weights, scalars);
        } else if (num == 2) {
            patch = std::make_shared<SplineUtils::Surface>(degree[0], degree[1], points, knots[0], knots[1], weights,
                                                           scalars);
        } else {
            patch = std::make_shared<SplineUtils::Volume>(degree[0], degree[1], degree[2], points, knots[0], knots[1],
                                                          knots[2], weights, scalars);
        }

        m_Patchs.push_back(patch);
        return true;
    };

    int parsedCount = 0;
    for (auto* g = root->FirstChildElement("Geometry"); g; g = g->NextSiblingElement("Geometry")) {
        if (!parseGeometry(g)) { return false; }
        ++parsedCount;
    }
    if (parsedCount == 0) {
        for (auto* g = root->FirstChildElement("Geometry"); g; g = g->NextSiblingElement("Geometry")) {
            if (!parseGeometry(g)) { return false; }
            ++parsedCount;
        }
    }

    // Optional: handle <multipatch><patches> selection/duplication by index
    if (auto* mp = root->FirstChildElement("MultiPatch")) {
        if (auto* p = mp->FirstChildElement("patches")) {
            std::string value = p->GetText() ? p->GetText() : "";
            char* data_p = const_cast<char*>(value.data());
            while (*data_p == '\n' || *data_p == ' ' || *data_p == '\t' || *data_p == '\r') data_p++;
            std::vector<int> indices;
            char* token = strtok(const_cast<char*>(value.c_str()), delimiters);
            while (token != nullptr) {
                indices.push_back(mAtoi(token));
                token = strtok(nullptr, delimiters);
            }
            if (!indices.empty() && !m_Patchs.empty()) {
                std::vector<SplineUtils::Geometry> reordered;
                reordered.reserve(indices.size());
                for (int idx: indices) {
                    if (idx >= 0 && idx < (int) m_Patchs.size()) { reordered.push_back(m_Patchs[idx]); }
                }
                // if (!reordered.empty()) { m_Patchs.swap(reordered); }
            }
        }
        // Ignore <interfaces> and <boundary> for now per user instruction.
    }

    return true;
}
bool SplineReaderCPU::CreateDataObject() {
    SplineGeometry::Pointer sp = SplineGeometry::New();

    sp->SetType(m_SplineType);
    sp->SetPatch(m_Patchs);
    sp->SetViewStyle(m_SplineType == SplineUtils::Type::CURVE ? IG_WIREFRAME : IG_SURFACE);
    sp->SetSurfaceRenderForVolume(m_SurfaceRenderForVolume);

    if (m_Patchs[0]->m_ControlScalars.size()) {
        FloatArray::Pointer scalarArray = FloatArray::New();
        scalarArray->SetDimension(m_Patchs[0]->m_ControlScalars[0].size());
        scalarArray->SetName("scalar");
        sp->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, scalarArray);
    }

    m_Output = sp;
    return true;
}

IGAME_NAMESPACE_END
