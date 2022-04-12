#include "Assignment1.h"
#include <iostream>

static void printMat(const Eigen::Matrix4d &mat)
{
	std::cout << " matrix:" << std::endl;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
			std::cout << mat(j, i) << " ";
		std::cout << std::endl;
	}
}

Assignment1::Assignment1()
{
	iteration_num = 20;
	time = 0;
	coeffs = Eigen::Vector4f(3.24, 0.44, 0.02, -1.05);
}

// Assignment1::Assignment1(float angle ,float relationWH, float near, float far) : Scene(angle,relationWH,near,far)
//{
// }

void Assignment1::Init()
{
	iteration_num = 1;
	chosen_coeff_index = 0;
	x_translate = 0.0f;
	y_translate = 0.0f;
	zoom = 1.0f;
	AddShader("C:/Users/ipism/source/repos/Computer_Graphics/tutorial/shaders/newtonShader");
	AddShape(Plane, -1, TRIANGLES, 0);
	SetShapeShader(0, 0);
	SetShapeStatic(0);
	roots = FindCubicRoots();
}

void Assignment1::Update(const Eigen::Matrix4f &Proj, const Eigen::Matrix4f &View, const Eigen::Matrix4f &Model, unsigned int shaderIndx, unsigned int shapeIndx)
{
	Shader *s = shaders[shaderIndx];
	s->SetUniform4f("coeffs", coeffs[0], coeffs[1], coeffs[2], coeffs[3]);
	s->SetUniform4f("root1", roots[0].real(), roots[0].imag(), 0, 0);
	s->SetUniform4f("root2", roots[1].real(), roots[1].imag(), 0, 0);
	s->SetUniform4f("root3", roots[2].real(), roots[2].imag(), 0, 0);
	s->SetUniform4f("translate", x_translate, y_translate, 0, 0);
	s->SetUniform1f("zoom", zoom);
	s->SetUniform1i("iteration_num", iteration_num);
	s->Bind();
	s->SetUniformMat4f("Proj", Proj);
	s->SetUniformMat4f("View", View);
	s->SetUniformMat4f("Model", Model);
	s->Unbind();
}

void Assignment1::WhenRotate() {}

void Assignment1::WhenTranslate() {
	x_translate -= xrel / 200.0f;
	y_translate += yrel / 200.0f;
}

void Assignment1::Animate()
{
	if (isActive)
	{
		time += 0.01f;
	}
}

void Assignment1::ScaleAllShapes(float amt, int viewportIndx)
{
	for (int i = 1; i < data_list.size(); i++)
	{
		if (data_list[i]->Is2Render(viewportIndx))
		{
			data_list[i]->MyScale(Eigen::Vector3d(amt, amt, amt));
		}
	}
}

void Assignment1::Zoom(float zrel)
{
	zoom /= 1 - zrel * 0.01;
}

Eigen::Vector3cf Assignment1::FindCubicRoots()
{
	Eigen::Vector2cf reduceCoeffs = Eigen::Vector2cf::Zero();
	Eigen::Vector3cf roots;
	std::complex<float> bOver3a = (coeffs[1] / coeffs[0]) / 3.0f;
	reduceCoeffs[0] = coeffs[2] / coeffs[0] - 3.0f * bOver3a * bOver3a;
	reduceCoeffs[1] = coeffs[2] / coeffs[0] * bOver3a - coeffs[3] / coeffs[0] - 2.0f * bOver3a * bOver3a * bOver3a;
	// std::cout << "reduced\n"
	// 		  << reduceCoeffs << std::endl;
	if (reduceCoeffs.norm() > 0.000001)
	{
		roots = FindRootsOfReduceEquation(reduceCoeffs);
		roots[0] -= bOver3a;
		roots[1] -= bOver3a;
		roots[2] -= bOver3a;
	}
	else
	{
		roots[0] = -1.0f * bOver3a;
		roots[1] = std::complex<float>(std::cosf(3.14159f / 3.0f), std::sinf(3.14159f / 3.0f)) * bOver3a;
		roots[2] = std::complex<float>(std::cosf(2.0f * 3.14159f / 3.0f), std::sinf(2 * 3.14159f / 3.0f)) * bOver3a;
	}

	return roots;
}

std::complex<float> Assignment1::NewtonCubicRoot(std::complex<float> num)
{
	std::complex<float> root = num;
	const int iter = 9;
	bool isSmall = false;
	if (std::abs(num) < 1e-3)
	{
		if (std::abs(num) == 0)
			return num;
		isSmall = true;
		num = num * 1e6f;
		root = num;
	}
	else if (std::abs(num) < 0.9f)
		root = 1;
	for (int k = 0; k < iter; k++)
	{
		root = (2.0f * root * root * root + num) / root / root / 3.0f;
	}
	if (isSmall)
		root = root / 100.0f;
	return root;
}

Eigen::Vector3cf Assignment1::FindRootsOfReduceEquation(Eigen::Vector2cf reduceCoeffs)
{
	Eigen::Vector3cf roots = Eigen::Vector3cf::Zero();
	std::complex<float> sqroot = std::sqrt(reduceCoeffs[0] * reduceCoeffs[0] * reduceCoeffs[0] / 27.0f + reduceCoeffs[1] * reduceCoeffs[1] / 4.0f);
	std::complex<float> p = NewtonCubicRoot(reduceCoeffs[1] / 2.0f + sqroot);
	std::complex<float> n = NewtonCubicRoot(reduceCoeffs[1] / 2.0f - sqroot);
	roots[0] = p + n;
	roots[1] = p * std::complex<float>(std::cosf(2.0f * 3.14159f / 3.0f), std::sinf(2 * 3.14159f / 3.0f)) - n * std::complex<float>(std::cosf(1.0f * 3.14159f / 3.0f), std::sinf(1 * 3.14159f / 3.0f));
	roots[2] = -p * std::complex<float>(std::cosf(1.0f * 3.14159f / 3.0f), std::sinf(1 * 3.14159f / 3.0f)) + n * std::complex<float>(std::cosf(2.0f * 3.14159f / 3.0f), std::sinf(2 * 3.14159f / 3.0f));
	return roots;
}

float Assignment1::UpdatePosition(float xpos, float ypos)
{
	xrel = xold - xpos;
	yrel = yold - ypos;
	xold = xpos;
	yold = ypos;
	return yrel;
}

Assignment1::~Assignment1(void) {}
