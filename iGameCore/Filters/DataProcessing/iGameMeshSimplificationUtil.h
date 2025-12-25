#pragma once

#include <cmath>
#include <vector>
#include <Eigen/Core>
#include <Eigen/Dense>

template<class T>
class TVector {
public:
    union {
        struct {
            T x, y, z;
        };
        T xyz[3];
    };
    
    TVector(){}
    TVector(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}

    T Dot(const TVector<T>& v) const { return x * v.x + y * v.y + z * v.z; }
    TVector Cross(const TVector<T>& v) const {
        return TVector<T>{y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x};
    }
    TVector operator-(const TVector<T>& v) const { return TVector<T>{x - v.x, y - v.y, z - v.z}; }
    TVector operator+(const TVector<T>& v) const { return TVector<T>{x + v.x, y + v.y, z + v.z}; }
    TVector operator*(T s) const { return TVector<T>{x * s, y * s, z * s}; }
    T& operator[](const size_t index) { return xyz[index]; }
    const T& operator[](const size_t index) const { return xyz[index]; }

    double Length() const { return std::sqrt(x * x + y * y + z * z); }

    double Normalize() {
        double len = Length();
        if (len > 0) {
            x /= len;
            y /= len;
            z /= len;
        }
        return len;
    }
};

using FVector = TVector<double>;


class FQuadric {
public:
    double a00, a11, a22;
    double a10, a20, a21; // a10=xy, a20=xz, a21=yz
    double b0, b1, b2, c; // 一次项系数 b 和常数项 c
    double w;             // 权重/面积

    FQuadric() { }

    void Zero() {
        a00 = a11 = a22 = 0;
        a10 = a20 = a21 = 0;
        b0 = b1 = b2 = c = 0;
        w = 0;
    }

    FQuadric& operator+=(const FQuadric& q) {
        a00 += q.a00;
        a11 += q.a11;
        a22 += q.a22;
        a10 += q.a10;
        a20 += q.a20;
        a21 += q.a21;
        b0 += q.b0;
        b1 += q.b1;
        b2 += q.b2;
        c += q.c;
        w += q.w;
        return *this;
    }

    FQuadric operator+(const FQuadric& q) const {
        FQuadric res = *this;
        res += q;
        return res;
    }

    FQuadric operator*(double s) const {
        FQuadric res = *this;
        res.a00 *= s;
        res.a11 *= s;
        res.a22 *= s;
        res.a10 *= s;
        res.a20 *= s;
        res.a21 *= s;
        res.b0 *= s;
        res.b1 *= s;
        res.b2 *= s;
        res.c *= s;
        res.w *= s;
        return res;
    }

    // Cost = v^T A v + 2 b^T v + c
    double Evaluate(const FVector& v) const {
        double rx = b0;
        double ry = b1;
        double rz = b2;

        rx += a10 * v.y;
        ry += a21 * v.z;
        rz += a20 * v.x;

        rx *= 2;
        ry *= 2;
        rz *= 2;

        rx += a00 * v.x;
        ry += a11 * v.y;
        rz += a22 * v.z;

        double r = c;
        r += rx * v.x;
        r += ry * v.y;
        r += rz * v.z;

        return r;
    }

    bool Optimize(FVector& v) const {
        Eigen::Matrix3d A;
        A << a00, a10, a20, 
             a10, a11, a21, 
             a20, a21, a22;
        Eigen::Vector3d b(-b0, -b1, -b2);

        Eigen::FullPivLU<Eigen::MatrixXd> lu(A);
        if (lu.isInvertible()) {
            Eigen::Vector3d x = lu.solve(b);
            v.x = x.x();
            v.y = x.y();
            v.z = x.z();
            return true;
        }
        return false;
    }

    // 从平面方程 ax+by+cz+d=0 构建几何 FQuadric (pp^T)
    // 还需要传入权重 w (通常是面积)
    static FQuadric FromPlane(double a, double b, double c, double d, double w) {
        FQuadric Q;
        // 缩放系数
        // 原始公式 Q = (n n^T)
        // 带权重 Q = w * (n n^T)

        double aw = a * w;
        double bw = b * w;
        double cw = c * w;
        double dw = d * w;

        Q.a00 = a * aw;
        Q.a11 = b * bw;
        Q.a22 = c * cw;

        Q.a10 = a * bw; // xy
        Q.a20 = a * cw; // xz
        Q.a21 = b * cw; // yz

        Q.b0 = a * dw;
        Q.b1 = b * dw;
        Q.b2 = c * dw;

        Q.c = d * dw;
        Q.w = w;

        return Q;
    }

    static FQuadric FromPlane(double a, double b, double c, double d) {
        FQuadric Q;

        Q.a00 = a * a;
        Q.a11 = b * b;
        Q.a22 = c * c;

        Q.a10 = a * b; // xy
        Q.a20 = a * c; // xz
        Q.a21 = b * c; // yz

        Q.b0 = a * d;
        Q.b1 = b * d;
        Q.b2 = c * d;

        Q.c = d * d;
        return Q;
    }

    // 从 3x3 张量 M 构建属性 FQuadric
    // 对应 QEM 形式:
    // [ A   -Ap ]
    // [-p^TA p^TAp]
    // 其中 A = J^T M J (已经算好的 3x3 矩阵)
    static FQuadric FromMetricTensor(const double M[3][3], const FVector& p) {
        FQuadric Q;
        Q.w = 0; // 属性 FQuadric 通常不带面积权重，或者由外部加权控制

        // A 部分 (3x3)
        // M[0][0]->a00, M[1][1]->a11, M[2][2]->a22
        // M[0][1]->a10 (xy), M[0][2]->a20 (xz), M[1][2]->a21 (yz)
        Q.a00 = M[0][0];
        Q.a11 = M[1][1];
        Q.a22 = M[2][2];
        Q.a10 = M[0][1];
        Q.a20 = M[0][2];
        Q.a21 = M[1][2];

        // 计算 b = -Ap
        // 由于 A 对称:
        // (Ap)_x = a00*px + a10*py + a20*pz
        // (Ap)_y = a10*px + a11*py + a21*pz
        // (Ap)_z = a20*px + a21*py + a22*pz

        double Apx = Q.a00 * p.x + Q.a10 * p.y + Q.a20 * p.z;
        double Apy = Q.a10 * p.x + Q.a11 * p.y + Q.a21 * p.z;
        double Apz = Q.a20 * p.x + Q.a21 * p.y + Q.a22 * p.z;

        Q.b0 = -Apx;
        Q.b1 = -Apy;
        Q.b2 = -Apz;

        // 计算 c = p^T A p = p^T (Ap)
        Q.c = p.x * Apx + p.y * Apy + p.z * Apz;

        return Q;
    }
};


struct FVertexAdjacency 
{
    // 三角形中一个顶点的对偶边
    struct Edge {
        int next;
        int prev;
    };

    std::vector<int> Offsets;
    std::vector<Edge> Data;

    int Num(int id) const { return Offsets[id + 1] - Offsets[id]; }
    int Begin(int id) const { return Offsets[id]; }
    int End(int id) const { return Offsets[id + 1]; }
};

struct FCollapseNode 
{
    int v1;
    int v2;

    FVector target;
    float cost;
};

struct Edge {
    int v1, v2;
    bool operator==(const Edge& other) const { return v1 == other.v1 && v2 == other.v2; }
};

struct EdgeHasher {
    size_t operator()(const Edge& p) const {
        const size_t h1 = std::hash<int>()(p.v1);
        const size_t h2 = std::hash<int>()(p.v2);
        // Improved hash mixing
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};

class FEdgeHash {
public:
    typedef Edge key_t;
    typedef int val_t;
    typedef EdgeHasher hash_t;
    struct node {
        key_t key;
        val_t val;
        node* next;
        node() : key(key_t{}), val(val_t{}), next(nullptr) {}
        node(key_t key, val_t val) : key(key), val(val), next(nullptr) {}
    };

    size_t count;
    size_t capacity;
    node** data;

    // Optimization: Memory Pool
    std::vector<node*> blocks;
    node* freeList;
    static const size_t BLOCK_SIZE = 1024;
    size_t currentBlockIndex;

public:
    FEdgeHash(int initCapacity = 16)
        : count(0), capacity(initCapacity), freeList(nullptr), currentBlockIndex(BLOCK_SIZE) {
        data = new node*[capacity];
        memset(data, 0, capacity * sizeof(node*));
    }

    ~FEdgeHash() {
        delete[] data;
        // Efficient cleanup: just delete the blocks
        for (auto block: blocks) { delete[] block; }
    }

    // Allocate from pool or free list
    node* Allocate(key_t k, val_t v) {
        if (freeList) {
            node* p = freeList;
            freeList = freeList->next;
            p->key = k;
            p->val = v;
            p->next = nullptr;
            return p;
        }
        if (currentBlockIndex >= BLOCK_SIZE) {
            blocks.push_back(new node[BLOCK_SIZE]);
            currentBlockIndex = 0;
        }
        node* p = &blocks.back()[currentBlockIndex++];
        p->key = k;
        p->val = v;
        p->next = nullptr;
        return p;
    }

    // Return to free list
    void Deallocate(node* p) {
        p->next = freeList;
        freeList = p;
    }

    size_t GetIndex(key_t key) const {
        size_t code = hash_t()(key);
        return code & (capacity - 1);
    }

    bool addUnique(key_t key, val_t value) {
        if (count >= capacity * 0.75) { Resize(); }

        size_t index = GetIndex(key);
        node* prev = nullptr;
        node* curr = data[index];

        while (curr) {
            if (curr->key == key) {
                return true;
            }
            prev = curr;
            curr = curr->next;
        }

        // Not found: Add new node from pool
        node* p = Allocate(key, value);
        p->next = data[index];
        data[index] = p;
        count++;
        return false;
    }

    // 第一次添加，第二次删除并返回true，所以遇到非流形会有重复边
    bool addOrRemove(key_t key, val_t value, val_t& oldValue) {
        if (count >= capacity * 0.75) { Resize(); }

        size_t index = GetIndex(key);
        node* prev = nullptr;
        node* curr = data[index];

        while (curr) {
            if (curr->key == key) {
                // Found: Remove and return to free list
                oldValue = curr->val;
                if (prev) prev->next = curr->next;
                else
                    data[index] = curr->next;

                Deallocate(curr);
                count--;
                return true;
            }
            prev = curr;
            curr = curr->next;
        }

        // Not found: Add new node from pool
        node* p = Allocate(key, value);
        p->next = data[index];
        data[index] = p;
        count++;
        return false;
    }

private:
    void Resize() {
        capacity *= 2;
        node** old_data = data;
        data = new node*[capacity];
        for (int i = 0; i < capacity; i++) { data[i] = nullptr; }
        for (int i = 0; i < capacity / 2; i++) {
            if (old_data[i]) {
                node* cur = old_data[i];
                while (cur) {
                    node* nxt = cur->next;
                    Insert(cur);
                    cur = nxt;
                }
            }
        }
    }
    void Insert(node* p) {
        size_t index = GetIndex(p->key);
        p->next = data[index];
        data[index] = p;
    }
};