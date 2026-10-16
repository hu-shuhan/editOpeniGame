#pragma once

#include <array>
#include <functional>
#include <map>
#include <string>
#include <vector>

// Portable CPU implementation of the Jacobi and Nektar++ expansions.
// Kept independent of Qt/OpenGL so file parsing and numerical evaluation can
// also be validated without a graphics context.
namespace iGame::Spectral {
using Vec3 = std::array<double, 3>;
enum class Shape { Hexahedron, Prism, Quadrilateral };
enum class Basis { Legendre, ModifiedA, OrthoA, ModifiedB };
struct Curve {
    std::vector<Vec3> points;
    std::vector<double> nodes;
    std::array<int, 4> corners{}; // local element vertex indices
    bool face = false;
};
struct Element {
    int id = 0;
    Shape shape = Shape::Hexahedron;
    std::array<Vec3, 8> vertices{};
    std::array<int, 3> modes{1, 1, 1};
    std::array<Basis, 3> basis{Basis::Legendre, Basis::Legendre, Basis::Legendre};
    bool nektar = false;
    std::map<std::string, std::vector<double>> fields;
    std::vector<Curve> curves;
    double Evaluate(const std::string& field, const Vec3& tensorPoint) const;
    Vec3 Position(const Vec3& tensorPoint) const;
};
struct Data {
    std::vector<Element> elements;
    std::vector<std::string> fields;
};
double Jacobi(int degree, int alpha, int beta, double x);
Data ReadJacobi(const void* bytes, size_t size);
Data ReadNektar(const std::string& selectedPath);
bool IsNektarFile(const std::string& path);
// subdivisions=0 selects max(2, largest polynomial degree + 1).
// Cells use OpeniGame's hex/prism/quad ordering. Each element owns its points:
// discontinuous spectral fields must not be averaged across element boundaries.
void Sample(const Data& data, int subdivisions,
            const std::function<void(const Vec3&, const std::vector<double>&)>& point,
            const std::function<void(Shape, const std::vector<size_t>&, int)>& cell);
}
