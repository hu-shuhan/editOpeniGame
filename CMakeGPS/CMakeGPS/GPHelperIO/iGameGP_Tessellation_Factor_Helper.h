#pragma once

#include <glm/glm.hpp>

namespace gpbezier
{
    class TessellationFactorHelper
    {
    public:
        double A, B;
        double tauX, tauY;
        int Width, Height;

        glm::mat4 proj;

        inline void setProjMatrix(glm::mat4 _proj)
        {
            proj = _proj;
        }

        inline void calculateAB()
        {
            assert(proj.length() != 0);

            glm::vec4 v = glm::vec4(1.0);
            glm::vec4 v_prime = proj * v;
            assert(v_prime.w != 0);
            v_prime = v_prime / v_prime.w;

            A = -1.0 * v_prime.x;
            B = 1.0 * v_prime.y;

            A = abs(A);
            B = abs(B);
        }

        inline double getA() const
        {
            return A;
        }

        inline double getB() const
        {
            return B;
        }

        inline void setWidthHeight(int width, int height)
        {
            Width = width;
            Height = height;
        }

        inline void calculateTau()
        {
            assert(Width != 0);
            assert(Height != 0);

            tauX = 1.0 / static_cast<double>(Width);
            tauY = 1.0 / static_cast<double>(Height);
        }

        inline double getTauX() const { return tauX; };
        inline double getTauY() const { return tauY; };
    };
}