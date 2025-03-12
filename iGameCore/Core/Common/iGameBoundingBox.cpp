#include "iGameBoundingBox.h"

IGAME_NAMESPACE_BEGIN


BoundingBox::BoundingBox() { setNull(); }
BoundingBox::BoundingBox(const Vector3d& min, const Vector3d& max) : min(min), max(max) {}
BoundingBox::BoundingBox(const double min[3], const double max[3]) : min(min), max(max) {}
BoundingBox::BoundingBox(const Vector3d& center, double radius) {
    min = center - Vector3d(radius, radius, radius);
    max = center + Vector3d(radius, radius, radius);
}

void BoundingBox::reset() { setNull(); }

void BoundingBox::set(const double p[3]) {
    min[0] = max[0] = p[0];
    min[1] = max[1] = p[1];
    min[2] = max[2] = p[2];
}

void BoundingBox::set(const Vector3d& p) { min = max = p; }

void BoundingBox::setNull() {
    min[0] = 1;
    max[0] = -1;
    min[1] = 1;
    max[1] = -1;
    min[2] = 1;
    max[2] = -1;
}

void BoundingBox::add(const BoundingBox& b) {
    if (b.isNull()) return;
    if (isNull()) *this = b;
    else {
        if (min[0] > b.min[0]) min[0] = b.min[0];
        if (min[1] > b.min[1]) min[1] = b.min[1];
        if (min[2] > b.min[2]) min[2] = b.min[2];

        if (max[0] < b.max[0]) max[0] = b.max[0];
        if (max[1] < b.max[1]) max[1] = b.max[1];
        if (max[2] < b.max[2]) max[2] = b.max[2];
    }
}

void BoundingBox::add(const Vector3d& p) {
    if (isNull()) set(p);
    else {
        if (min[0] > p[0]) min[0] = p[0];
        if (min[1] > p[1]) min[1] = p[1];
        if (min[2] > p[2]) min[2] = p[2];

        if (max[0] < p[0]) max[0] = p[0];
        if (max[1] < p[1]) max[1] = p[1];
        if (max[2] < p[2]) max[2] = p[2];
    }
}

void BoundingBox::add(const double p[3]) {
    if (isNull()) set(p);
    else {
        if (min[0] > p[0]) min[0] = p[0];
        if (min[1] > p[1]) min[1] = p[1];
        if (min[2] > p[2]) min[2] = p[2];

        if (max[0] < p[0]) max[0] = p[0];
        if (max[1] < p[1]) max[1] = p[1];
        if (max[2] < p[2]) max[2] = p[2];
    }
}

void BoundingBox::add(const Vector3d& p, const double& radius) {
    if (isNull()) set(p);
    else {
        min[0] = std::min(min[0], p[0] - radius);
        min[1] = std::min(min[1], p[1] - radius);
        min[2] = std::min(min[2], p[2] - radius);

        max[0] = std::max(max[0], p[0] + radius);
        max[1] = std::max(max[1], p[1] + radius);
        max[2] = std::max(max[2], p[2] + radius);
    }
}

void BoundingBox::intersect(const BoundingBox& b) {
    if (min[0] < b.min[0]) min[0] = b.min[0];
    if (min[1] < b.min[1]) min[1] = b.min[1];
    if (min[2] < b.min[2]) min[2] = b.min[2];

    if (max[0] > b.max[0]) max[0] = b.max[0];
    if (max[1] > b.max[1]) max[1] = b.max[1];
    if (max[2] > b.max[2]) max[2] = b.max[2];

    if (min[0] > max[0] || min[1] > max[1] || min[2] > max[2]) setNull();
}

void BoundingBox::combine(const BoundingBox& b) {
    if (b.isNull()) { return; }
    if (this->isNull()) {
        *this = b;
        return;
    }

    if (min[0] > b.min[0]) min[0] = b.min[0];
    if (min[1] > b.min[1]) min[1] = b.min[1];
    if (min[2] > b.min[2]) min[2] = b.min[2];

    if (max[0] < b.max[0]) max[0] = b.max[0];
    if (max[1] < b.max[1]) max[1] = b.max[1];
    if (max[2] < b.max[2]) max[2] = b.max[2];
}

void BoundingBox::translate(const Vector3d& p) {
    min += p;
    max += p;
}

bool BoundingBox::isIn(const Vector3d& p) const {
    return (min[0] <= p[0] && p[0] <= max[0] && min[1] <= p[1] && p[1] <= max[1] && min[2] <= p[2] &&
            p[2] <= max[2]);
}

bool BoundingBox::isInEx(const Vector3d& p) const {
    return (min[0] <= p[0] && p[0] < max[0] && min[1] <= p[1] && p[1] < max[1] && min[2] <= p[2] && p[2] < max[2]);
}

bool BoundingBox::collide(const BoundingBox& b) const {
    return b.min[0] < max[0] && b.max[0] > min[0] && b.min[1] < max[1] && b.max[1] > min[1] && b.min[2] < max[2] &&
            b.max[2] > min[2];
}

bool BoundingBox::isNull() const { return min[0] > max[0] || min[1] > max[1] || min[2] > max[2]; }

bool BoundingBox::isEmpty() const { return min == max; }

double BoundingBox::diag() const { return (max - min).norm(); }

double BoundingBox::squaredDiag() const { return (max - min).squaredNorm(); }

Vector3d BoundingBox::center() const { return (min + max) / 2; }

Vector3d BoundingBox::diagVector() const { return (max - min); }
IGAME_NAMESPACE_END
