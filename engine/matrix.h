#pragma once

#include "lib.h"
#include "vector.h"

//******************************************************************************
// Matrix2 - Two-Dimensional Matrix
//******************************************************************************
class matrix2
{
public:
	float x1, x2;
	float y1, y2;

public:
	matrix2()
	{
		x1 = 1.0f; x2 = 0.0f;
		y1 = 0.0f; y2 = 1.0f;
	}
	matrix2(float a1, float a2,
			float b1, float b2)
	{
		x1 = a1; x2 = a2;
		y1 = b1; y2 = b2;
	}
	float det() const
	{
		return x1 * y2 - x2 * y1;
	}
};

//******************************************************************************
// Matrix3 - Three-Dimensional Matrix
//******************************************************************************
class matrix3
{
public:
	float x1, x2, x3;
	float y1, y2, y3;
	float z1, z2, z3;

public:
	matrix3()
	{
		x1 = 1.0f; x2 = 0.0f; x3 = 0.0f;
		y1 = 0.0f; y2 = 1.0f; y3 = 0.0f;
		z1 = 0.0f; z2 = 0.0f; z3 = 1.0f;
	}
	matrix3(float a1, float a2, float a3,
			float b1, float b2, float b3,
			float c1, float c2, float c3)
	{
		x1 = a1; x2 = a2; x3 = a3;
		y1 = b1; y2 = b2; y3 = b3;
		z1 = c1; z2 = c2; z3 = c3;
	}

	vector3 operator*(const vector3 B) const
	{
		vector3 v;
		v.x = x1 * B.x + x2 * B.y + x3 * B.z;
		v.y = y1 * B.x + y2 * B.y + y3 * B.z;
		v.z = z1 * B.x + z2 * B.y + z3 * B.z;
		return v;
	}

	matrix3 transpose_get() const
	{
		matrix3 m = *this;
		m.transpose();
		return m;
	}

	void transpose()
	{
		*this = matrix3(x1, y1, z1,
			            x2, y2, z2,
			            x3, y3, z3);
	}

	// determinent
	float   det() const
	{
		return x1*y2*z3 - x1*y3*z2 + x2*y3*z1 - x2*y1*z3 + x3*y1*z2 - x3*y2*z1;
	}

	matrix3 inv() const
	{
		matrix3 m;
		float d = det();
		m.x1 = matrix2(y2, y3, z2, z3).det() / d;
		m.x2 = matrix2(x3, x2, z3, z2).det() / d;
		m.x3 = matrix2(x2, x3, y2, y3).det() / d;
		m.y1 = matrix2(y3, y1, z3, z1).det() / d;
		m.y2 = matrix2(x1, x3, z1, z3).det() / d;
		m.y3 = matrix2(x3, x1, y3, y1).det() / d;
		m.z1 = matrix2(y1, y2, z1, z2).det() / d;
		m.z2 = matrix2(x2, x1, z2, z1).det() / d;
		m.z3 = matrix2(x1, x2, y1, y2).det() / d;

		return m;
	}
};

//******************************************************************************
// column major 4x4 Matrix
//******************************************************************************
class matrix4
{
public:
	float x1, x2, x3, x4;
	float y1, y2, y3, y4;
	float z1, z2, z3, z4;
	float w1, w2, w3, w4;

public:
	matrix4()
	{
		x1 = 1.0f; x2 = 0.0f; x3 = 0.0f; x4 = 0.0f;
		y1 = 0.0f; y2 = 1.0f; y3 = 0.0f; y4 = 0.0f;
		z1 = 0.0f; z2 = 0.0f; z3 = 1.0f; z4 = 0.0f;
		w1 = 0.0f; w2 = 0.0f; w3 = 0.0f; w4 = 1.0f;
	}
	matrix4(float in[16])
	{
		x1 = in[0];  x2 = in[1];  x3 = in[2];  x4 = in[3];
		y1 = in[4];  y2 = in[5];  y3 = in[6];  y4 = in[7];
		z1 = in[8];  z2 = in[9];  z3 = in[10]; z4 = in[11];
		w1 = in[12]; w2 = in[13]; w3 = in[14]; w4 = in[15];
	}
	matrix4(float a1, float a2, float a3, float a4,
			float b1, float b2, float b3, float b4,
			float c1, float c2, float c3, float c4,
			float d1, float d2, float d3, float d4)
	{
		x1 = a1; x2 = a2; x3 = a3; x4 = a4;
		y1 = b1; y2 = b2; y3 = b3; y4 = b4;
		z1 = c1; z2 = c2; z3 = c3; z4 = c4;
		w1 = d1; w2 = d2; w3 = d3; w4 = d4;
	}

	static matrix4 identity() { return matrix4(); }

	vector4 operator*(const vector4& B) const
	{
		vector4 v;
		v.x = x1 * B.x + x2 * B.y + x3 * B.z + x4 * B.w;
		v.y = y1 * B.x + y2 * B.y + y3 * B.z + y4 * B.w;
		v.z = z1 * B.x + z2 * B.y + z3 * B.z + z4 * B.w;
		v.w = w1 * B.x + w2 * B.y + w3 * B.z + w4 * B.w;
		return v;
	}
	vector4 operator*(const vector3& B) const
	{
		return operator*(vector4({B.x,B.y,B.z,1.0f}));
	}
	void operator*=(float v)
	{
		x1 *= v; x2 *= v; x3 *= v; x4 *= v;
		y1 *= v; y2 *= v; y3 *= v; y4 *= v;
		z1 *= v; z2 *= v; z3 *= v; z4 *= v;
		w1 *= v; w2 *= v; w3 *= v; w4 *= v;
	}

	matrix4 operator*(const matrix4& b) const
	{
		matrix4 m; 
		m.x1 = x1*b.x1 + x2*b.y1 + x3*b.z1 + x4*b.w1;
		m.x2 = x1*b.x2 + x2*b.y2 + x3*b.z2 + x4*b.w2;
		m.x3 = x1*b.x3 + x2*b.y3 + x3*b.z3 + x4*b.w3;
		m.x4 = x1*b.x4 + x2*b.y4 + x3*b.z4 + x4*b.w4;

		m.y1 = y1*b.x1 + y2*b.y1 + y3*b.z1 + y4*b.w1;
		m.y2 = y1*b.x2 + y2*b.y2 + y3*b.z2 + y4*b.w2;
		m.y3 = y1*b.x3 + y2*b.y3 + y3*b.z3 + y4*b.w3;
		m.y4 = y1*b.x4 + y2*b.y4 + y3*b.z4 + y4*b.w4;

		m.z1 = z1*b.x1 + z2*b.y1 + z3*b.z1 + z4*b.w1;
		m.z2 = z1*b.x2 + z2*b.y2 + z3*b.z2 + z4*b.w2;
		m.z3 = z1*b.x3 + z2*b.y3 + z3*b.z3 + z4*b.w3;
		m.z4 = z1*b.x4 + z2*b.y4 + z3*b.z4 + z4*b.w4;

		m.w1 = w1*b.x1 + w2*b.y1 + w3*b.z1 + w4*b.w1;
		m.w2 = w1*b.x2 + w2*b.y2 + w3*b.z2 + w4*b.w2;
		m.w3 = w1*b.x3 + w2*b.y3 + w3*b.z3 + w4*b.w3;
		m.w4 = w1*b.x4 + w2*b.y4 + w3*b.z4 + w4*b.w4;
		return m;
	}
	matrix4 operator+(const matrix4& in) const
	{
		matrix4 m;
		m.x1 = x1 + in.x1;
		m.x2 = x2 + in.x2;
		m.x3 = x3 + in.x3;
		m.x4 = x4 + in.x4;
		m.y1 = y1 + in.y1;
		m.y2 = y2 + in.y2;
		m.y3 = y3 + in.y3;
		m.y4 = y4 + in.y4;
		m.z1 = z1 + in.z1;
		m.z2 = z2 + in.z2;
		m.z3 = z3 + in.z3;
		m.z4 = z4 + in.z4;
		m.w1 = w1 + in.w1;
		m.w2 = w2 + in.w2;
		m.w3 = w3 + in.w3;
		m.w4 = w4 + in.w4;
		return m;
	}

	// inverse
	matrix4 inv() const
	{
		// get cofactors of minor matrices
		float cofactor_x1 = matrix3(y2,y3,y4,z2,z3,z4,w2,w3,w4).det();
		float cofactor_x2 = matrix3(y1,y3,y4,z1,z3,z4,w1,w3,w4).det();
		float cofactor_x3 = matrix3(y1,y2,y4,z1,z2,z4,w1,w2,w4).det();
		float cofactor_x4 = matrix3(y1,y2,y3,z1,z2,z3,w1,w2,w3).det();

		float cofactor_y1 = matrix3(x2,x3,x4,z2,z3,z4,w2,w3,w4).det();
		float cofactor_y2 = matrix3(x1,x3,x4,z1,z3,z4,w1,w3,w4).det();
		float cofactor_y3 = matrix3(x1,x2,x4,z1,z2,z4,w1,w2,w4).det();
		float cofactor_y4 = matrix3(x1,x2,x3,z1,z2,z3,w1,w2,w3).det();

		float cofactor_z1 = matrix3(x2,x3,x4,y2,y3,y4,w2,w3,w4).det();
		float cofactor_z2 = matrix3(x1,x3,x4,y1,y3,y4,w1,w3,w4).det();
		float cofactor_z3 = matrix3(x1,x2,x4,y1,y2,y4,w1,w2,w4).det();
		float cofactor_z4 = matrix3(x1,x2,x3,y1,y2,y3,w1,w2,w3).det();

		float cofactor_w1 = matrix3(x2,x3,x4,y2,y3,y4,z2,z3,z4).det();
		float cofactor_w2 = matrix3(x1,x3,x4,y1,y3,y4,z1,z3,z4).det();
		float cofactor_w3 = matrix3(x1,x2,x4,y1,y2,y4,z1,z2,z4).det();
		float cofactor_w4 = matrix3(x1,x2,x3,y1,y2,y3,z1,z2,z3).det();

		float determinant = x1*cofactor_x1 - x2*cofactor_x2 + x3*cofactor_x3 - x4*cofactor_x4;
		float inv_det = 1.0f / determinant;

		matrix4 m(
			 cofactor_x1, -cofactor_y1,  cofactor_z1, -cofactor_w1,
			-cofactor_x2,  cofactor_y2, -cofactor_z2,  cofactor_w2,
			 cofactor_x3, -cofactor_y3,  cofactor_z3, -cofactor_w3,
			-cofactor_x4,  cofactor_y4, -cofactor_z4,  cofactor_w4);

		m *= inv_det;

		return m;
	}

	// determinant
	float det() const
	{
		// logically it is doing this
		//return x1 * matrix3(y2, y3, y4, z2, z3, z4, w2, w3, w4).det()
		//	   - x2 * matrix3(y1, y3, y4, z1, z3, z4, w1, w3, w4).det()
		//	   + x3 * matrix3(y1, y2, y4, z1, z2, z4, w1, w2, w4).det()
		//	   - x4 * matrix3(y1, y2, y3, z1, z2, z3, w1, w2, w3).det();

		// but i manually unrolled it to prevent unnecessary operations
		return x1 * (y2*z3*w4 + y3*z4*w2 + y4*z2*w3 - y2*z4*w3 - y3*z2*w4 - y4*z3*w2)
		     - x2 * (y1*z4*w3 + y3*z1*w4 + y4*z3*w1 - y1*z3*w4 - y3*z4*w1 - y4*z1*w3)
		     + x3 * (y1*z2*w4 + y2*z4*w1 + y4*z1*w2 - y1*z4*w2 - y2*z1*w4 - y4*z2*w1)
		     - x4 * (y1*z3*w2 + y2*z1*w3 + y3*z2*w1 - y1*z2*w3 - y2*z3*w1 - y3*z1*w2);
			
	}

	// transpose
	matrix4 transpose_get() const
	{
		matrix4 m = *this;
		m.transpose();
		return m;
	}

	void transpose()
	{
		*this = matrix4(x1, y1, z1, w1,
			            x2, y2, z2, w2,
			            x3, y3, z3, w3,
			            x4, y4, z4, w4);
	}

	void rotate(const vector3& r)
	{
		x1 = cos(r.y)*cos(r.z);   x2 = sin(r.x)*sin(r.y)*cos(r.z) - cos(r.x)*sin(r.z);    x3 = cos(r.x)*sin(r.y)*cos(r.z) + sin(r.x)*sin(r.z);
		y1 = cos(r.y)*sin(r.z);   y2 = sin(r.x)*sin(r.y)*sin(r.z) + cos(r.x)*cos(r.z);    y3 = cos(r.x)*sin(r.y)*sin(r.z) - sin(r.x)*cos(r.z);
		z1 = -sin(r.y);           z2 = sin(r.x)*cos(r.y);                                 z3 = cos(r.x)*cos(r.y);
	}

	void set_translation(const vector3& t)
	{
		x4 = t.x;
		y4 = t.y;
		z4 = t.z;
		w4 = 1;
	}
	void translate(const vector3& t)
	{
		x4 += t.x;
		y4 += t.y;
		z4 += t.z;
		w4 = 1;
	}
	void scale(float s)
	{
		scale(s, s, s);
	}
	void scale(float x, float y,float z)
	{
		x1 *= x; x2 *= x; x3 *= x; x4 *= x;
		y1 *= y; y2 *= y; y3 *= y; y4 *= y;
		z1 *= z; z2 *= z; z3 *= z; z4 *= z;
	}
};
