#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <algorithm>

#define _USE_MATH_DEFINES
#include <math.h>

struct __CameraState
{
	glm::dvec3 d3PosCamera;
	glm::dvec3 v3VecView;
	glm::dvec3 v3VecUp;
	double dNearPlaneDistFromCamera;
	double dFarPlaneDistFromCamera;
	bool bIsPerspective;

	glm::dmat4x4 matSS2WS;
	glm::dmat4x4 matWS2SS;

	// Zoom Ratio 관련
	double dImageWidth;
	double dImageHeight;
	double dFovY;

	// Image Plane Ratio 관련
	double dFittingWidth;
	double dFittingHeight;
	double dFittingFovY;

	//2D Image Translation Offset 관련
	double dTrLength;
	double dTrAngleSS;
};

class __arc_ball
{
private:
	glm::dvec3 m_d3PosArcballCenter;   // Rotation Center
	double m_dRadius;   // Arcball Radius
	double m_dActivatedRadius;  // Min(m_dRadius, Center2EyeLength - );

	glm::dvec3 m_d3PosOnSurfaceStart;

	// Statics 
	glm::dmat4x4 m_matSS2WS;
	glm::dvec3 m_v3VecView;
	glm::dvec3 m_d3PosCamera;

	__CameraState m_vxCamStateSetInStart;

	bool m_bIsPerspective = false;
	double m_dNearPlaneDistFromCamera = 0.1;

	bool m_bIsTrackBall = true; // Otherwise, Plane Coordinate
	bool m_bIsStateSet = false;
	bool m_bIsStartArcball = false;
	bool m_bIsFixRotationAxis = false;
	glm::dvec3 m_v3VecRotationAxis;

	double m_dTrackballSensitive = 10;

	glm::dvec3 ComputeTargetPoint(const double dPointX, const double dPointY)
	{
		glm::dvec3 d3PosOnSurface = glm::dvec3();

		// ==> Make Function
		// Get Near Plane's Position
		glm::dvec3 d3PosPointSS = glm::dvec3(dPointX, dPointY, 0);
		glm::dvec4 d3PosPointWS_h = m_matSS2WS * glm::dvec4(d3PosPointSS, 1.);
		glm::dvec3 d3PosPointWS = d3PosPointWS_h / d3PosPointWS_h.w;

		glm::dvec3 v3VecRayDir = m_v3VecView;

		if (m_bIsTrackBall)
		{
			// Use Planar Coordinate
			if (!m_bIsPerspective && m_dNearPlaneDistFromCamera < 0.1/*double.Epsilon*/)
			{
				d3PosPointWS = d3PosPointWS + v3VecRayDir * 0.1;   // <== Think
			}

			d3PosOnSurface = d3PosPointWS;
		}
		else
		{
			// Use Sphere Coordinate
			if (m_bIsPerspective)
			{
				v3VecRayDir = d3PosPointWS - m_d3PosCamera;
			}

			// Center as B, Ray as A + tv
			// B = m_d3PosArcballCenter
			// A = d3PosPointWS, v = v3VecRayDir

			// 1st compute A - B = (a`, b`, c`)
			glm::dvec3 v3BA = d3PosPointWS - m_d3PosArcballCenter;
			// 2nd compute v*v = a^2 + b^2 + c^2
			double dDotVV = glm::dot(v3VecRayDir, v3VecRayDir);
			// 3rd compute (A - B)*v = a`a + b`b + c`c
			double dDotBAV = glm::dot(v3BA, v3VecRayDir);
			// if there's cross then, 4th compute sqrt for min t
			double dDet = dDotBAV * dDotBAV - dDotVV * (glm::dot(v3BA, v3BA) - m_dActivatedRadius * m_dActivatedRadius);
			double dT;

			if (dDet >= 0)
			{
				dT = -(dDotBAV + sqrt(dDet)) / dDotVV;
			}
			else
			{
				dT = -dDotBAV / dDotVV;
			}

			d3PosOnSurface = d3PosPointWS + v3VecRayDir * dT;
		}

		return d3PosOnSurface;
	}
public:
	bool __is_set_stage;
	double __start_x, __start_y;

	__arc_ball() { __is_set_stage = false; };
	~__arc_ball() {};

	void SetArcBallMovingStyle(const bool bIsTrackBall)
	{
		m_bIsTrackBall = bIsTrackBall;
	}

	glm::dvec3 GetCenterStage() { return m_d3PosArcballCenter; };

	void FitArcballToSphere(const glm::dvec3& d3PosArcballCenter, const double dRadius)
	{
		m_d3PosArcballCenter = d3PosArcballCenter;
		m_dRadius = dRadius;
		m_bIsStateSet = true;
	}

	void StartArcball(const double dPointX, const double dPointY, const __CameraState& arcballCamState, double dTackballSensitive = 10)
	{
		if (!m_bIsStateSet)
			return;
		m_bIsStartArcball = true;
		m_dTrackballSensitive = dTackballSensitive;

		__start_x = dPointX;
		__start_y = dPointY;

		// Start Setting
		m_matSS2WS = arcballCamState.matSS2WS;

		// VXCameraState
		m_vxCamStateSetInStart = arcballCamState;
		m_bIsPerspective = arcballCamState.bIsPerspective;
		m_dNearPlaneDistFromCamera = arcballCamState.dNearPlaneDistFromCamera;
		m_d3PosCamera = arcballCamState.d3PosCamera;
		m_v3VecView = arcballCamState.v3VecView;

		glm::dvec3 v3VecCam2Center = m_d3PosArcballCenter - m_d3PosCamera;
		m_dActivatedRadius = std::min(m_dRadius, (double)glm::length(v3VecCam2Center) * 0.8);

		if (arcballCamState.bIsPerspective)
		{
			if (glm::length(v3VecCam2Center) < arcballCamState.dNearPlaneDistFromCamera)
			{
				std::cout << "Arcball Sphere Center is too near to control the arcball - Error!" << std::endl;
				return;
			}
		}

		m_d3PosOnSurfaceStart = ComputeTargetPoint(dPointX, dPointY);
	}

	void FixRotationAxis(const glm::dvec3& v3VecRotationAxis)
	{
		m_bIsFixRotationAxis = true;
		m_v3VecRotationAxis = v3VecRotationAxis;
	}

	void FreeRotationAxis()
	{
		m_bIsFixRotationAxis = false;
	}

	__CameraState GetCameraStateSetInStart()
	{
		return m_vxCamStateSetInStart;
	}

	double MoveArcball(glm::dmat4x4& matRotatedWS, const double dPointX, const double dPointY, const bool bIsReverseDir)
	{
		matRotatedWS = glm::dmat4x4(); // identity

		if (!m_bIsStartArcball)
			return 0;

		glm::dvec3 d3PosOnSurfaceEnd = ComputeTargetPoint(dPointX, dPointY);

		glm::dvec3 v3VecCenter3SurfStart = m_d3PosOnSurfaceStart - m_d3PosArcballCenter;
		glm::dvec3 v3VecCenter3SurfEnd = d3PosOnSurfaceEnd - m_d3PosArcballCenter;

		v3VecCenter3SurfStart = glm::normalize(v3VecCenter3SurfStart);
		v3VecCenter3SurfEnd = glm::normalize(v3VecCenter3SurfEnd);

		glm::dvec3 v3VecRotateDir = glm::cross(v3VecCenter3SurfStart, v3VecCenter3SurfEnd);
		bool bIsInvert = false;
		if (m_bIsFixRotationAxis)
		{
			if (glm::dot(v3VecRotateDir, m_v3VecRotationAxis) >= 0)
				v3VecRotateDir = m_v3VecRotationAxis;
			else
			{
				bIsInvert = true;
				v3VecRotateDir = -m_v3VecRotationAxis;
			}
		}

		if (bIsReverseDir)
			v3VecRotateDir *= -1;
		if (glm::dot(v3VecRotateDir, v3VecRotateDir) < DBL_EPSILON)
			return 0;

		double dAngle = 0;
		if (m_bIsTrackBall)
		{
			v3VecRotateDir = glm::normalize(v3VecRotateDir);
			double dCircumference = M_PI * 2 * m_dRadius;

			glm::dvec3 v3VecStart2End = d3PosOnSurfaceEnd - m_d3PosOnSurfaceStart;

			if (m_bIsFixRotationAxis)
			{
				v3VecStart2End = v3VecStart2End - glm::dot(v3VecStart2End, v3VecRotateDir) * v3VecRotateDir;
			}

			dAngle = glm::length(v3VecStart2End) / dCircumference * m_dTrackballSensitive;
			if (m_bIsPerspective)
			{
				dAngle *= glm::length(m_d3PosCamera - m_d3PosArcballCenter) / m_dNearPlaneDistFromCamera; //500
			}
		}
		else
		{
			//dAngleDeg = Vector3D.AngleBetween(v3VecCenter3SurfStart, v3VecCenter3SurfEnd);
			dAngle = std::acos(std::max(std::min(glm::dot(v3VecCenter3SurfStart, v3VecCenter3SurfEnd), 1.0), -1.0)); // 0 to PI
		}
		if (dAngle == 0) return 0;

		glm::dmat4x4 mat_rot = glm::rotate(dAngle, v3VecRotateDir);
		glm::dmat4x4 mat_trs_1 = glm::translate(m_d3PosArcballCenter);
		glm::dmat4x4 mat_trs_0 = glm::translate(-m_d3PosArcballCenter);

		matRotatedWS = mat_trs_1 * mat_rot * mat_trs_0;

		if (bIsInvert)
			dAngle *= -1;
		return dAngle;
	}


};