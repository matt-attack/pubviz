

#pragma once

#include <cmath>

struct Vec3
{
	union
	{
		struct { float x, y, z; };
		float v[3];
	};

	Vec3() {}

	Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

	inline float dot(const Vec3& lhs) const
	{
		return lhs.x * x + lhs.y * y + lhs.z * z;
	}

	inline float length() const
	{
		return std::sqrt(x * x + y * y + z * z);
	}

	inline Vec3 operator+(const Vec3& o) const
	{
		return Vec3(x + o.x, y + o.y, z + o.z);
	}

	inline Vec3 operator*(float k) const
	{
		return Vec3(x * k, y * k, z * k);
	}
};

struct Vec4
{
	union
	{
		struct { float x, y, z, w; };
		struct { Vec3 xyz; float w; };
		float v[4];
	};

	Vec4() {}

	Vec4(const float _x, const float _y, const float _z, const float _w) :
		x(_x), y(_y), z(_z), w(_w)
	{

	}

	Vec4(const Vec3& vec3, float _w) : x(vec3.x), y(vec3.y), z(vec3.z), w(_w)
	{

	}

	inline float dot(const Vec3& lhs) const
	{
		return lhs.x * x + lhs.y * y + lhs.z * z + w;
	}

	inline float dot(const Vec4& lhs) const
	{
		return lhs.x * x + lhs.y * y + lhs.z * z + lhs.w * w;
	}

	inline Vec4 addw(const float f) const
	{
		return Vec4(x, y, z, w + f);
	}

	inline Vec4 operator*(float k) const
	{ 
		return Vec4(x * k, y * k, z * k, w * k);
	}

	inline Vec4 operator+(const Vec4& o) const
	{ 
		return Vec4(x + o.x, y + o.y, z + o.z, w + o.w);
	}
};

struct Quaternion
{
	float x, y, z, w;

	Quaternion() : x(0), y(0), z(0), w(1) {}

	Quaternion(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}

	inline Quaternion operator* (const Quaternion& rkQ)
	{
		return Quaternion(
			w * rkQ.x + x * rkQ.w + y * rkQ.z - z * rkQ.y,
			w * rkQ.y + y * rkQ.w + z * rkQ.x - x * rkQ.z,
			w * rkQ.z + z * rkQ.w + x * rkQ.y - y * rkQ.x,
			w * rkQ.w - x * rkQ.x - y * rkQ.y - z * rkQ.z
		);
	}

	inline double yaw()
	{
		return asin(2.0 * x * y + 2 * z * w);
	}

	// Constructs a quaternaion from a rotation in radians around a specified axis
	static Quaternion fromAngleAxis(const Vec3& axis, float angle)
	{
		float half_angle = 0.5f * angle;
		float fsin = sin(half_angle);
		return Quaternion(fsin * axis.x, fsin * axis.y, fsin * axis.z, cos(half_angle));
	}
};

struct Matrix3x3
{
	Vec3 a, b, c;

	explicit Matrix3x3(const Quaternion& q)
	{
		convertquat(q);
	}

	void convertquat(const Quaternion& q)
	{
		float x = q.x, y = q.y, z = q.z, w = q.w,
			tx = 2 * x, ty = 2 * y, tz = 2 * z,
			txx = tx * x, tyy = ty * y, tzz = tz * z,
			txy = tx * y, txz = tx * z, tyz = ty * z,
			twx = w * tx, twy = w * ty, twz = w * tz;
		a = Vec3(1 - (tyy + tzz), txy - twz, txz + twy);
		b = Vec3(txy + twz, 1 - (txx + tzz), tyz - twx);
		c = Vec3(txz - twy, tyz + twx, 1 - (txx + tyy));
	}
};

struct Matrix3x4
{
	Vec4 a, b, c;

	explicit Matrix3x4(const Quaternion& rot, const Vec3& translation)
	{
		*this = Matrix3x4(Matrix3x3(rot), translation);
	}

	explicit Matrix3x4(const Matrix3x3(rot), const Vec3& translation) :
		a(rot.a, translation.x), b(rot.b, translation.y), c(rot.c, translation.z)
	{

	}

	explicit Matrix3x4(const Vec4& _a, const Vec4& _b, const Vec4& _c)
		: a(_a), b(_b), c(_c)
	{

	}

	inline Vec3 transform(const Vec3& o) const { return Vec3(a.dot(o), b.dot(o), c.dot(o)); }

	inline Matrix3x4 operator*(const Matrix3x4& o) const
	{
		return Matrix3x4(
			(a * o.a.x + b * o.a.y + c * o.a.z).addw(o.a.w),
			(a * o.b.x + b * o.b.y + c * o.b.z).addw(o.b.w),
			(a * o.c.x + b * o.c.y + c * o.c.z).addw(o.c.w));
	}
};

/*struct AABB
{
	float min_x, max_x, min_y, max_y, min_z, max_z;
	inline bool inside()
};*/