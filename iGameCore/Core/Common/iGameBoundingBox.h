#ifndef iGameBoundingBox_h
#define iGameBoundingBox_h

#include "iGameVector.h"

IGAME_NAMESPACE_BEGIN
class BoundingBox {
public:
    Vector3d min;
    Vector3d max;

    BoundingBox();
    BoundingBox(const Vector3d& min, const Vector3d& max);
    BoundingBox(const double min[3], const double max[3]);
    BoundingBox(const Vector3d& center, double radius);

    inline bool operator==(const BoundingBox& p) const { return min == p.min && max == p.max; }
    inline bool operator!=(const BoundingBox& p) const { return min != p.min || max != p.max; }

    void reset();

    void set(const double p[3]);

    void set(const Vector3d& p);

    void setNull();

    void add(const BoundingBox& b);

    void add(const Vector3d& p);

    void add(const double p[3]);

    template<typename OtherType>
    void add(const OtherType& p) {
        if (isNull()) set(Vector3d(p[0], p[1], p[2]));
        else {
            if (min[0] > p[0]) min[0] = p[0];
            if (min[1] > p[1]) min[1] = p[1];
            if (min[2] > p[2]) min[2] = p[2];

            if (max[0] < p[0]) max[0] = p[0];
            if (max[1] < p[1]) max[1] = p[1];
            if (max[2] < p[2]) max[2] = p[2];
        }
    }

    void add(const Vector3d& p, const double& radius);

    void intersect(const BoundingBox& b);

    void combine(const BoundingBox& b);

    void translate(const Vector3d& p);

    bool isIn(const Vector3d& p) const;

    bool isInEx(const Vector3d& p) const;

    bool collide(const BoundingBox& b) const;

    bool isNull() const;

    bool isEmpty() const;

    double diag() const;

    double squaredDiag() const;

    Vector3d center() const;

    Vector3d diagVector() const;
};
IGAME_NAMESPACE_END
#endif