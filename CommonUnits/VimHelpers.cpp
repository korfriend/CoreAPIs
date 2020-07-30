#include "VimHelpers.h"

//#include <string>
#include <math.h>

//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtx/transform.hpp>
//#include <glm/gtc/constants.hpp>
//#include <glm/glm.hpp>
//#include <glm/gtc/type_ptr.hpp>

using namespace glm;

#define __WINDOWS
#ifdef __WINDOWS
#include <windows.h>
//#include <intrin.h>
#endif

namespace vmmath {

#define c_out(out, in, T) *(T*)out = T(in.x / in.w, in.y / in.w, in.z / in.w);
	// row major math...

	// glm::mat2x2 [col][row]
	// ==> (0~3)
	// [0][0] (0), [1][0] (2)
	// [0][1] (1). [1][1] (3)
	// legacy_mat2x2 _m
	// ==> (0~3)
	// _m11 (0), _m21 (2)
	// _m12 (1), _m22 (3)

	inline void TransformPoint(d3p pos_out, const_d3p pos_in, const_d44p mat)
	{
		const dvec3& _pos_in = *(const dvec3*)pos_in;
		const dmat4x4& _mat = *(dmat4x4*)mat;
		dvec4 _pos_out = dvec4(_pos_in, 1.) * _mat;
		c_out(pos_out, _pos_out, dvec3);
	}

	inline void TransformVector(d3p vec_out, const_d3p vec_in, const_d44p mat)
	{
		const dvec3& _vec_in = *(const dvec3*)vec_in;
		const dmat4x4& _mat = *(dmat4x4*)mat;

		const double* _d = glm::value_ptr(_mat);
		//double d33[9] = { d[0], d[1], d[2], d[4], d[5], d[6], d[8], d[9], d[10] };
		double d33[9] = { _d[0], _d[4], _d[8], _d[1], _d[5], _d[9], _d[2], _d[6], _d[10] };
		dmat3x3 _mat33 = glm::make_mat3x3(d33);

		*(dvec3*)vec_out = _mat33 * _vec_in;
	}

	inline void MatrixMultiply(d44p mat, const_d44p matl, const_d44p matr)
	{
		const dmat4x4& _matl = *(dmat4x4*)matl;
		const dmat4x4& _matr = *(dmat4x4*)matr;
		*(dmat4x4*)mat = _matl * _matr;
	}

	inline void AddVector(d3p vec, const_d3p vec1, const_d3p vec2)
	{
		const dvec3& _vec1 = *(dvec3*)vec1;
		const dvec3& _vec2 = *(dvec3*)vec2;
		*(dvec3*)vec = _vec1 + _vec2;
	}

	inline void SubstractVector(d3p vec, const_d3p vec1, const_d3p vec2)
	{
		const dvec3& _vec1 = *(dvec3*)vec1;
		const dvec3& _vec2 = *(dvec3*)vec2;
		*(dvec3*)vec = _vec1 - _vec2;
	}

	inline double LengthVector(const_d3p vec)
	{
		const dvec3& _vec = *(dvec3*)vec;
		return glm::length(_vec);
	}
	inline double LengthVectorSq(const_d3p vec)
	{
		const dvec3& _vec = *(dvec3*)vec;
		return _vec.x * _vec.x + _vec.y * _vec.y + _vec.z * _vec.z;
	}

	void NormalizeVector(d3p vec_out, const_d3p vec_in)
	{
		double l = LengthVector(vec_in);
		if (l <= DBL_EPSILON) *(dvec3*)vec_out = dvec3(0);
		else *(dvec3*)vec_out = *(dvec3*)vec_in / l;
	}

	inline double DotVector(const_d3p vec1, const_d3p vec2)
	{
		const dvec3& _vec1 = *(dvec3*)vec1;
		const dvec3& _vec2 = *(dvec3*)vec2;
		return glm::dot(_vec1, _vec2);
	}

	inline void CrossDotVector(d3p vec, const_d3p vec1, const_d3p vec2)
	{
		const dvec3& _vec1 = *(dvec3*)vec1;
		const dvec3& _vec2 = *(dvec3*)vec2;
		*(dvec3*)vec = glm::cross(_vec1, _vec2);
	}

	inline void MatrixWS2CS(d44p mat, const_d3p pos_eye, const_d3p vec_up, const_d3p vec_view)
	{
		const dvec3& _pos_eye = *(dvec3*)pos_eye;
		const dvec3& _vec_up = *(dvec3*)vec_up;
		const dvec3& _vec_view = *(dvec3*)vec_view;

		dvec3 d3VecAxisZ = -_vec_view;
		d3VecAxisZ = glm::normalize(d3VecAxisZ);

		dvec3 d3VecAxisX = glm::cross(_vec_up, d3VecAxisZ);
		d3VecAxisX = glm::normalize(d3VecAxisX);

		dvec3 d3VecAxisY = glm::cross(d3VecAxisZ, d3VecAxisX);
		d3VecAxisY = glm::normalize(d3VecAxisY);

		dmat4x4& _mat = *(dmat4x4*)mat;

		_mat[0][0] = d3VecAxisX.x;
		_mat[1][0] = d3VecAxisY.x;
		_mat[2][0] = d3VecAxisZ.x;
		_mat[3][0] = 0;
		_mat[0][1] = d3VecAxisX.y;
		_mat[1][1] = d3VecAxisY.y;
		_mat[2][1] = d3VecAxisZ.y;
		_mat[3][1] = 0;
		_mat[0][2] = d3VecAxisX.z;
		_mat[1][2] = d3VecAxisY.z;
		_mat[2][2] = d3VecAxisZ.z;
		_mat[3][2] = 0;
		_mat[0][3] = -glm::dot(d3VecAxisX, _pos_eye);
		_mat[1][3] = -glm::dot(d3VecAxisY, _pos_eye);
		_mat[2][3] = -glm::dot(d3VecAxisZ, _pos_eye);
		_mat[3][3] = 1;
	}

	inline void MatrixOrthogonalCS2PS(d44p mat, const double w, const double h, const double _near, const double _far)
	{
		//dmat4x4 _mat = glm::orthoRH(-w / 2., w / 2., -h / 2., h / 2., _near, _far);
		//*(dmat4x4*)mat = glm::transpose(_mat);
		// GL 과 다르다. 
		dmat4x4& _mat = *(dmat4x4*)mat;
		_mat[0][0] = 2. / w;
		_mat[1][0] = 0;
		_mat[2][0] = 0;
		_mat[3][0] = 0;
		_mat[0][1] = 0;
		_mat[1][1] = 2. / h;
		_mat[2][1] = 0;
		_mat[3][1] = 0;
		_mat[0][2] = 0;
		_mat[1][2] = 0;
		_mat[2][2] = 1. / (_near - _far);
		_mat[3][2] = 0;
		_mat[0][3] = 0;
		_mat[1][3] = 0;
		_mat[2][3] = _near / (_near - _far);
		_mat[3][3] = 1.;
	}

	inline void MatrixPerspectiveCS2PS(d44p mat, const double fovy, const double aspect_ratio, const double _near, const double _far)
	{
		//const double h = 1.0;
		//const double w = aspect_ratio * h;
		//dmat4x4 _mat = glm::perspectiveFovRH(fovy, w, h, _near, _far);
		//*(dmat4x4*)mat = glm::transpose(_mat);
		double yScale = 1.0 / tan(fovy / 2.0);
		double xScale = yScale / aspect_ratio;

		dmat4x4& _mat = *(dmat4x4*)mat;
		_mat[0][0] = (float)xScale;
		_mat[1][0] = 0;
		_mat[2][0] = 0;
		_mat[3][0] = 0;
		_mat[0][1] = 0;
		_mat[1][1] = (float)yScale;
		_mat[2][1] = 0;
		_mat[3][1] = 0;
		_mat[0][2] = 0;
		_mat[1][2] = 0;
		_mat[2][2] = _far / (_near - _far);
		_mat[3][2] = -1;
		_mat[0][3] = 0;
		_mat[1][3] = 0;
		_mat[2][3] = _near * _far / (_near - _far);
		_mat[3][3] = 0;
	}

	inline void MatrixPS2SS(d44p mat, const double w, const double h)
	{
		dmat4x4 matTranslate, matScale, matTransform, matTranslateSampleModel;
		matTranslate = glm::translate(dvec3(1., -1., 0.));
		matScale = glm::scale(dvec3(w*0.5, h*0.5, 1.));
		matTranslateSampleModel = glm::translate(dvec3(-0.5, 0.5, 0.));

		matTranslate = glm::transpose(matTranslate);
		//matScale = glm::transpose(matScale);
		matTranslateSampleModel = glm::transpose(matTranslateSampleModel);

		*mat = (matTranslate * matScale) * matTranslateSampleModel;
		//[row][column] (legacy)
		//[col][row] (vismtv...glm...)
		(*mat)[1][0] *= -1.;
		(*mat)[1][1] *= -1.;
		(*mat)[1][2] *= -1.;
		(*mat)[1][3] *= -1.;
	}

	inline void MatrixRotationAxis(d44p mat, const_d3p vec_axis, const double angle_rad)
	{
		const dvec3& _vec_axis = *(dvec3*)vec_axis;
		dmat4x4 _mat = glm::rotate(angle_rad, _vec_axis);
		*(dmat4x4*)mat = glm::transpose(_mat);
	}

	inline void MatrixScaling(d44p mat, const_d3p scale_factors)
	{
		*(dmat4x4*)mat = glm::scale(*(const dvec3*)scale_factors);
	}

	inline void MatrixTranslation(d44p mat, const_d3p vec_trl)
	{
		dmat4x4 _mat = glm::translate(*(const dvec3*)vec_trl);
		*(dmat4x4*)mat = glm::transpose(_mat);
	}

	inline void MatrixInverse(d44p mat, const_d44p mat_in)
	{
		const dmat4x4& _mat = *(dmat4x4*)mat_in;
		*(dmat4x4*)mat = glm::inverse(_mat);
	}

	inline void fTransformPoint(f3p pos_out, const_f3p pos_in, const_f44p mat)
	{
		fvec3& p_in = *(fvec3*)pos_in;

		const fmat4x4& _mat = *(fmat4x4*)mat;
		fvec4 _pos_out = fvec4(p_in, 1.) * _mat;
		c_out(pos_out, _pos_out, fvec3);
	}
	inline void fTransformVector(f3p vec_out, const_f3p vec_in, const_f44p mat)
	{
		const fvec3& _vec_in = *(const fvec3*)vec_in;
		const fmat4x4& _mat = *(fmat4x4*)mat;

		const float* _f = glm::value_ptr(_mat);
		//double f33[9] = { _f[0], _f[1], _f[2], _f[4], _f[5], _f[6], _f[8], _f[9], _f[10] };
		double f33[9] = { _f[0], _f[4], _f[8], _f[1], _f[5], _f[9], _f[2], _f[6], _f[10] };
		fmat3x3 _mat33 = glm::make_mat3x3(f33);

		*(fvec3*)vec_out = _mat33 * _vec_in;
	}
	inline float fLengthVector(const_f3p vec)
	{
		const fvec3& _vec = *(fvec3*)vec;
		return glm::length(_vec);
	}
	inline float fLengthVectorSq(const_f3p vec)
	{
		const fvec3& _vec = *(fvec3*)vec;
		return _vec.x * _vec.x + _vec.y * _vec.y + _vec.z * _vec.z;
	}
	inline void fNormalizeVector(f3p vec_out, const_f3p vec_in)
	{
		float l = fLengthVector(vec_in);
		if (l <= DBL_EPSILON) *(fvec3*)vec_out = fvec3(0);
		else *(fvec3*)vec_out = *(fvec3*)vec_in / l;
	}
	inline float fDotVector(const_f3p vec1, const_f3p vec2)
	{
		const fvec3& _vec1 = *(fvec3*)vec1;
		const fvec3& _vec2 = *(fvec3*)vec2;
		return glm::dot(_vec1, _vec2);
	}
	inline void fCrossDotVector(f3p vec, const_f3p vec1, const_f3p vec2)
	{
		const fvec3& _vec1 = *(fvec3*)vec1;
		const fvec3& _vec2 = *(fvec3*)vec2;
		*(fvec3*)vec = glm::cross(_vec1, _vec2);
	}

	inline void fMatrixWS2CS(f44p mat, const_f3p pos_eye, const_f3p vec_up, const_f3p vec_view)
	{
		//const fvec3& _pos_eye = *(fvec3*)pos_eye;
		//const fvec3& _vec_up = *(fvec3*)vec_up;
		//const fvec3& _vec_view = *(fvec3*)vec_view;
		//fmat4x4 _mat = glm::lookAtRH(_pos_eye, _pos_eye + _vec_view, _vec_up);
		//*(fmat4x4*)mat = glm::transpose(_mat);

		const fvec3& _pos_eye = *(fvec3*)pos_eye;
		const fvec3& _vec_up = *(fvec3*)vec_up;
		const fvec3& _vec_view = *(fvec3*)vec_view;

		fvec3 f3VecAxisZ = -_vec_view;
		f3VecAxisZ = glm::normalize(f3VecAxisZ);

		fvec3 f3VecAxisX = glm::cross(_vec_up, f3VecAxisZ);
		f3VecAxisX = glm::normalize(f3VecAxisX);

		fvec3 f3VecAxisY = glm::cross(f3VecAxisZ, f3VecAxisX);
		f3VecAxisY = glm::normalize(f3VecAxisY);

		fmat4x4& _mat = *(fmat4x4*)mat;

		_mat[0][0] = f3VecAxisX.x;
		_mat[1][0] = f3VecAxisY.x;
		_mat[2][0] = f3VecAxisZ.x;
		_mat[3][0] = 0;
		_mat[0][1] = f3VecAxisX.y;
		_mat[1][1] = f3VecAxisY.y;
		_mat[2][1] = f3VecAxisZ.y;
		_mat[3][1] = 0;
		_mat[0][2] = f3VecAxisX.z;
		_mat[1][2] = f3VecAxisY.z;
		_mat[2][2] = f3VecAxisZ.z;
		_mat[3][2] = 0;
		_mat[0][3] = -glm::dot(f3VecAxisX, _pos_eye);
		_mat[1][3] = -glm::dot(f3VecAxisY, _pos_eye);
		_mat[2][3] = -glm::dot(f3VecAxisZ, _pos_eye);
		_mat[3][3] = 1;
	}
	inline void fMatrixOrthogonalCS2PS(f44p mat, const float w, const float h, const float _near, const float _far)
	{
		//fmat4x4 _mat = glm::orthoRH(-w / 2.f, w / 2.f, -h / 2.f, h / 2.f, _near, _far);
		//*(fmat4x4*)mat = glm::transpose(_mat);
		fmat4x4& _mat = *(fmat4x4*)mat;
		_mat[0][0] = 2.f / w;
		_mat[1][0] = 0;
		_mat[2][0] = 0;
		_mat[3][0] = 0;
		_mat[0][1] = 0;
		_mat[1][1] = 2.f / h;
		_mat[2][1] = 0;
		_mat[3][1] = 0;
		_mat[0][2] = 0;
		_mat[1][2] = 0;
		_mat[2][2] = 1.f / (_near - _far);
		_mat[3][2] = 0;
		_mat[0][3] = 0;
		_mat[1][3] = 0;
		_mat[2][3] = _near / (_near - _far);
		_mat[3][3] = 1.f;
	}
	inline void fMatrixPerspectiveCS2PS(f44p mat, const float fovy, const float aspect_ratio, const float _near, const float _far)
	{
		//const float h = 1.0f;
		//const float w = aspect_ratio * h;
		//fmat4x4 _mat = glm::perspectiveFovRH(fovy, w, h, _near, _far);
		//*(fmat4x4*)mat = glm::transpose(_mat);

		double yScale = 1.0 / tan(fovy / 2.0);
		double xScale = yScale / aspect_ratio;

		fmat4x4& _mat = *(fmat4x4*)mat;
		_mat[0][0] = (float)xScale;
		_mat[1][0] = 0;
		_mat[2][0] = 0;
		_mat[3][0] = 0;
		_mat[0][1] = 0;
		_mat[1][1] = (float)yScale;
		_mat[2][1] = 0;
		_mat[3][1] = 0;
		_mat[0][2] = 0;
		_mat[1][2] = 0;
		_mat[2][2] = _far / (_near - _far);
		_mat[3][2] = -1.f;
		_mat[0][3] = 0;
		_mat[1][3] = 0;
		_mat[2][3] = _near * _far / (_near - _far);
		_mat[3][3] = 0;
	}
	inline void fMatrixPS2SS(f44p mat, const float w, const float h)
	{
		fmat4x4 matTranslate, matScale, matTranslateSampleModel;
		matTranslate = glm::translate(fvec3(1.f, -1.f, 0.f));
		matScale = glm::scale(fvec3(w*0.5f, h*0.5f, 1.f));
		matTranslateSampleModel = glm::translate(fvec3(-0.5f, 0.5f, 0.f));

		matTranslate = glm::transpose(matTranslate);
		//matScale = glm::transpose(matScale);
		matTranslateSampleModel = glm::transpose(matTranslateSampleModel);

		*mat = (matTranslate * matScale) * matTranslateSampleModel;
		//[row][column] (legacy)
		//[col][row] (vismtv)
		(*mat)[1][0] *= -1.;
		(*mat)[1][1] *= -1.;
		(*mat)[1][2] *= -1.;
		(*mat)[1][3] *= -1.;
	}
	inline void fMatrixRotationAxis(f44p mat, const_f3p vec_axis, const float angle_rad)
	{
		const fvec3& _vec_axis = *(fvec3*)vec_axis;
		fmat4x4 _mat = glm::rotate(angle_rad, _vec_axis);
		*(fmat4x4*)mat = glm::transpose(_mat);
	}
	inline void fMatrixScaling(f44p mat, const_f3p scale_factors)
	{
		*(fmat4x4*)mat = glm::scale(*(const fvec3*)scale_factors);
	}
	inline void fMatrixTranslation(f44p mat, const_f3p vec_trl)
	{
		fmat4x4 _mat = glm::translate(*(const fvec3*)vec_trl);
		*(fmat4x4*)mat = glm::transpose(_mat);
	}
	inline void fMatrixInverse(f44p mat, const_f44p mat_in)
	{
		const fmat4x4& _mat = *(fmat4x4*)mat_in;
		*(fmat4x4*)mat = glm::inverse(_mat);
	}
}

namespace vmhelpers {
	inline void AllocateVoidPointer2D(void*** ptr_dst, const int array_length_2d, const int array_sizebytes_1d)
	{
		*ptr_dst = new void*[array_length_2d];
		for (int i = 0; i < array_length_2d; i++)
			(*ptr_dst)[i] = new char[array_sizebytes_1d];
	}

	inline void GetSystemMemoryInfo(double* free_bytes, double* valid_sysmem_bytes)
	{
#ifdef __WINDOWS	
		MEMORYSTATUS memState;
		GlobalMemoryStatus(&memState);
#ifdef X86DEG
		*free_bytes = (double)(memState.dwAvailVirtual);
		*valid_sysmem_bytes = (double)(memState.dwTotalPhys);
#endif
#ifdef X86REL
		*free_bytes = (double)(memState.dwAvailVirtual);
		*valid_sysmem_bytes = (double)(memState.dwTotalPhys);
#endif
#ifdef X64DEG
		*free_bytes = (double)(memState.dwAvailPhys);
		*valid_sysmem_bytes = (double)(memState.dwTotalPhys);
#endif
#ifdef X64REL
		*free_bytes = (double)(memState.dwAvailPhys);
		*valid_sysmem_bytes = (double)(memState.dwTotalPhys);
#endif
#endif
	}

	inline void GetCPUInstructionInfo(int* cpu_info)
	{
		//  Misc.
		bool HW_MMX;
		bool HW_x64;
		bool HW_ABM;      // Advanced Bit Manipulation
		bool HW_RDRAND;
		bool HW_BMI1;
		bool HW_BMI2;
		bool HW_ADX;
		bool HW_PREFETCHWT1;

		//  SIMD: 128-bit
		bool HW_SSE;
		bool HW_SSE2;
		bool HW_SSE3;
		bool HW_SSSE3;
		bool HW_SSE41;
		bool HW_SSE42;
		bool HW_SSE4a;
		bool HW_AES;
		bool HW_SHA;

		//  SIMD: 256-bit
		bool HW_AVX;
		bool HW_XOP;
		bool HW_FMA3;
		bool HW_FMA4;
		bool HW_AVX2;

		//  SIMD: 512-bit
		bool HW_AVX512F;    //  AVX512 Foundation
		bool HW_AVX512CD;   //  AVX512 Conflict Detection
		bool HW_AVX512PF;   //  AVX512 Prefetch
		bool HW_AVX512ER;   //  AVX512 Exponential + Reciprocal
		bool HW_AVX512VL;   //  AVX512 Vector Length Extensions
		bool HW_AVX512BW;   //  AVX512 Byte + Word
		bool HW_AVX512DQ;   //  AVX512 Doubleword + Quadword
		bool HW_AVX512IFMA; //  AVX512 Integer 52-bit Fused Multiply-Add
		bool HW_AVX512VBMI; //  AVX512 Vector Byte Manipulation Instructions

		int info[4];
		__cpuidex(info, 0, 0);
		int nIds = info[0];

		__cpuidex(info, 0x80000000, 0);
		unsigned nExIds = info[0];

		//  Detect Features
		if (nIds >= 0x00000001) {
			__cpuidex(info, 0x00000001, 0);
			HW_MMX = (info[3] & ((int)1 << 23)) != 0;
			HW_SSE = (info[3] & ((int)1 << 25)) != 0;
			HW_SSE2 = (info[3] & ((int)1 << 26)) != 0;
			HW_SSE3 = (info[2] & ((int)1 << 0)) != 0;

			HW_SSSE3 = (info[2] & ((int)1 << 9)) != 0;
			HW_SSE41 = (info[2] & ((int)1 << 19)) != 0;
			HW_SSE42 = (info[2] & ((int)1 << 20)) != 0;
			HW_AES = (info[2] & ((int)1 << 25)) != 0;

			HW_AVX = (info[2] & ((int)1 << 28)) != 0;
			HW_FMA3 = (info[2] & ((int)1 << 12)) != 0;

			HW_RDRAND = (info[2] & ((int)1 << 30)) != 0;
		}
		if (nIds >= 0x00000007) {
			__cpuidex(info, 0x00000007, 0);
			HW_AVX2 = (info[1] & ((int)1 << 5)) != 0;

			HW_BMI1 = (info[1] & ((int)1 << 3)) != 0;
			HW_BMI2 = (info[1] & ((int)1 << 8)) != 0;
			HW_ADX = (info[1] & ((int)1 << 19)) != 0;
			HW_SHA = (info[1] & ((int)1 << 29)) != 0;
			HW_PREFETCHWT1 = (info[2] & ((int)1 << 0)) != 0;

			HW_AVX512F = (info[1] & ((int)1 << 16)) != 0;
			HW_AVX512CD = (info[1] & ((int)1 << 28)) != 0;
			HW_AVX512PF = (info[1] & ((int)1 << 26)) != 0;
			HW_AVX512ER = (info[1] & ((int)1 << 27)) != 0;
			HW_AVX512VL = (info[1] & ((int)1 << 31)) != 0;
			HW_AVX512BW = (info[1] & ((int)1 << 30)) != 0;
			HW_AVX512DQ = (info[1] & ((int)1 << 17)) != 0;
			HW_AVX512IFMA = (info[1] & ((int)1 << 21)) != 0;
			HW_AVX512VBMI = (info[2] & ((int)1 << 1)) != 0;
		}
		if (nExIds >= 0x80000001) {
			__cpuidex(info, 0x80000001, 0);
			HW_x64 = (info[3] & ((int)1 << 29)) != 0;
			HW_ABM = (info[2] & ((int)1 << 5)) != 0;
			HW_SSE4a = (info[2] & ((int)1 << 6)) != 0;
			HW_FMA4 = (info[2] & ((int)1 << 16)) != 0;
			HW_XOP = (info[2] & ((int)1 << 11)) != 0;
		}
		*cpu_info =
			((int)HW_MMX) + ((int)HW_x64 << 1) + ((int)HW_ABM << 2) + ((int)HW_RDRAND << 3) + ((int)HW_BMI1 << 4) + ((int)HW_BMI2 << 5) + ((int)HW_ADX << 6) + ((int)HW_PREFETCHWT1 << 7) +
			((int)HW_SSE << 8) + ((int)HW_SSE2 << 9) + ((int)HW_SSE3 << 10) + ((int)HW_SSSE3 << 11) + ((int)HW_SSE41 << 12) + ((int)HW_SSE42 << 13) + ((int)HW_SSE4a << 14) + ((int)HW_AES << 15) +
			((int)HW_SHA << 16) + ((int)HW_AVX << 17) + ((int)HW_XOP << 18) + ((int)HW_FMA3 << 19) + ((int)HW_FMA4 << 20) + ((int)HW_AVX2 << 21) + ((int)HW_AVX512F << 22) + ((int)HW_AVX512CD << 23) +
			((int)HW_AVX512PF << 24) + ((int)HW_AVX512ER << 25) + ((int)HW_AVX512VL << 26) + ((int)HW_AVX512BW << 27) + ((int)HW_AVX512DQ << 28) + ((int)HW_AVX512IFMA << 29) + ((int)HW_AVX512VBMI << 30);
	}

	unsigned long long GetCurrentTimePack()
	{
#ifdef __WINDOWS
		// 38 bit : year, 4 bit : month, 5 bit : day, 5 bit : hour, 6 bit : minute, 6 bit : second
		_SYSTEMTIME st;
		GetSystemTime(&st);

		return st.wMilliseconds + ((unsigned long long)st.wSecond << 10) + ((unsigned long long)st.wMinute << 16) + ((unsigned long long)st.wHour << 22)
			+ ((unsigned long long)st.wDay << 27) + ((unsigned long long)st.wMonth << 32) + ((unsigned long long)st.wYear << 36);
#else
		return 0;
#endif

	}
}