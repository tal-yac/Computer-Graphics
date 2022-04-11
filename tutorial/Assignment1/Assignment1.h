#pragma once
#include "igl/opengl/glfw/Viewer.h"

class Assignment1 : public igl::opengl::glfw::Viewer
{
	float time;
	Eigen::Vector3cf FindRootsOfReduceEquation(Eigen::Vector2cf reduceCoeffs);
	std::complex<float> Assignment1::NewtonCubicRoot(std::complex<float> num);

public:

	Eigen::Vector4f coeffs;
	Eigen::Vector3cf roots;
	int chosen_coeff_index;
	int iteration_num;
	int xold, yold, xrel, yrel, zrel;
	float x_translate, y_translate;
	float x, y;
	float zoom;
	Assignment1();
	//	Assignment1(float angle,float relationWH,float near, float far);
	void Init();
	void Update(const Eigen::Matrix4f& Proj, const Eigen::Matrix4f& View, const Eigen::Matrix4f& Model, unsigned int  shaderIndx, unsigned int shapeIndx);
	void WhenRotate();
	void WhenTranslate();
	void Animate() override;
	float UpdatePosition(float xpos, float ypos);
	void ScaleAllShapes(float amt, int viewportIndx);
	void Zoom(float zrel);
	Eigen::Vector3cf FindCubicRoots();

	~Assignment1(void);
};

