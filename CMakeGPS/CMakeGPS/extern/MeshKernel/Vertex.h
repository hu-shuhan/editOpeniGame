#pragma once
#include <vector>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <cassert>
#include "Kernel.h"
#include "Handle.h"

namespace MeshKernel{  
	class iGameVertex {
	public:
		iGameVertex() :position(3, 0.0) {}
		iGameVertex(const iGameVertex& _v) :position(_v.position) {}
		iGameVertex(double x, double y, double z) :position{ x,y,z } {}

		const double x() const { return position[0]; }
		const double y() const { return position[1]; }
		const double z() const { return position[2]; }

		double& x() { return position[0]; }
		double& y() { return position[1]; }
		double& z() { return position[2]; }

		inline iGameVertex operator+(const iGameVertex& _rhs) const {                             
			return iGameVertex(x() + _rhs.x(), y() + _rhs.y(), z() + _rhs.z());
		}
		inline iGameVertex operator-(const iGameVertex& _rhs) const {                             
			return iGameVertex(x() - _rhs.x(), y() - _rhs.y(), z() - _rhs.z());
		}
		inline iGameVertex operator*(double k) const {                                        
			return iGameVertex(x() * k, y() * k, z() * k);
		}
		inline iGameVertex operator/(double k) const {                                        
			return iGameVertex(x() / k, y() / k, z() / k);
		}
		inline double operator*(const iGameVertex& _rhs) const {                              
			return double(x() * _rhs.x() + y() * _rhs.y() + z() * _rhs.z());
		}
		inline double dot(const iGameVertex& _rhs) const {
			return double(x() * _rhs.x() + y() * _rhs.y() + z() * _rhs.z());
		}
		inline iGameVertex operator%(const iGameVertex& _rhs) const {                              
			return iGameVertex(y() * _rhs.z() - z() * _rhs.y(),
				z() * _rhs.x() - x() * _rhs.z(),
				x() * _rhs.y() - y() * _rhs.x());
		}
		inline iGameVertex cross(const iGameVertex& _rhs) const  {
			return iGameVertex(y() * _rhs.z() - z() * _rhs.y(),
				z() * _rhs.x() - x() * _rhs.z(),
				x() * _rhs.y() - y() * _rhs.x());
		}

		inline iGameVertex& operator+=(const iGameVertex& _rhs) {
			position[0] += _rhs.x();
			position[1] += _rhs.y();
			position[2] += _rhs.z();
			return *this;
		}
		inline iGameVertex& operator-=(const iGameVertex& _rhs) {
			position[0] -= _rhs.x();
			position[1] -= _rhs.y();
			position[2] -= _rhs.z();
			return *this;
		}
		inline double& operator[](int i) {
			assert(i >= 0 && i < position.size());
			return position[i];
		}
		inline iGameVertex& operator*=(double k) {
			position[0] *= k;
			position[1] *= k;
			position[2] *= k;
			return *this;
		}
		inline iGameVertex& operator/=(double k) {
			assert(k != 0.f);
			position[0] /= k;
			position[1] /= k;
			position[2] /= k;
			return *this;
		}
		inline double norm() const {                                                      
			return sqrt(x() * x() + y() * y() + z() * z());
		}
		inline double norm2() const {                                                      
			return x() * x() + y() * y() + z() * z();
		}
		inline iGameVertex normalized() const {                                                 
			auto m = this->norm();
			return iGameVertex(x() / m, y() / m, z() / m);
		}
		inline iGameVertex& normalize() {                                                 
			*this = this->normalized();
			return *this;
		}

		inline double dist(const iGameVertex& _rhs) const {                                   
			return (*this - _rhs).norm();
		}
		inline void setPosition(double x, double y, double z) {
			position.resize(3);
			position[0] = x;
			position[1] = y;
			position[2] = z;
		}
		inline void setPosition(std::vector<double> Pos) {
			assert(Pos.size() == 3);
			position = Pos;
		}
		inline void setX(double x) {
			position[0] = x;
		}
		inline void setY(double y) {
			position[1] = y;
		}
		inline void setZ(double z) {
			position[2] = z;
		}
		inline void setNormal(double x, double y, double z) {
			normal.resize(3);
			normal[0] = x;
			normal[1] = y;
			normal[2] = z;
		}
		inline void setNormal(std::vector<double> N) {
			assert(N.size() == 3);
			normal = N;
		}
		inline double getNormalX() {
			if (normal.size() == 3) return normal[0];
			return 0.f;
		}
		inline double getNormalY() {
			if (normal.size() == 3) return normal[1];
			return 0.f;
		}
		inline double getNormalZ() {
			if (normal.size() == 3) return normal[2];
			return 0.f;
		}
		
		inline bool operator==(const iGameVertex& _rhs) const {
			return (x() == _rhs.x() && y() == _rhs.y() && z() == _rhs.z());
		}
		iGameVertex& operator=(const iGameVertex& _v) {
			position = _v.position; return *this;
		}
	private:
		std::vector<double> position;
		std::vector<double> normal;
	};
}

namespace std
{
	template<> struct hash<MeshKernel::iGameVertex>
	{
		size_t operator()(const MeshKernel::iGameVertex& v)const
		{
			return hash<double>()(v.x()) ^ hash<double>()(v.y()) ^ hash<double>()(v.z());
		}
	};
}