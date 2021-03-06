#include "image.h"

image::image(std::string fileName, double * K, double *R, double *T)
{		

	_K = cv::Mat(3,3,CV_64F, K).clone();
	_R = cv::Mat(3,3,CV_64F, R).clone();
	_T = cv::Mat(3,1,CV_64F, T).clone();
	//_T = -_R * _C;
	_C = -_R.t() * _T;
		
	_proj.create(3,4,CV_64F);
	for(int i = 0; i< 3; i++)
		// + 0 is necessary. See: http://opencv.willowgarage.com/documentation/cpp/core_basic_structures.html#Mat::row	
		_proj.col(i) = _R.col(i) + 0;	
	_proj.col(3) = _T + 0;

	_proj = _K * _proj;		
	_image = cv::imread(fileName);
	_imageName = fileName;

	// class member for opengl
	for(int i = 0; i < 9; i ++)
	{
		(&_glmK[0][0])[i] = static_cast<float>(K[i]);
		(&_glmR[0][0])[i] = static_cast<float>(R[i]);
	}
	// due to difference in storage (colmn major)
	_glmK = glm::transpose(_glmK); _glmR = glm::transpose(_glmR);
	// -------------------------------------------------------------
	_glmT[0] = static_cast<float>(T[0]); _glmT[1] = static_cast<float>(T[1]); _glmT[2] = static_cast<float>(T[2]);
	_glmC = -glm::transpose(_glmR) * _glmT;

	setModelViewMatrix();
	setProjMatrix();
}


cv::Mat image::calculateFundMatrix(const image &im)
{
	// calculate the fundermental matrix
	cv::Mat e2 = _proj.colRange(cv::Range(0,3)) * im._C + _proj.col(3);	// epipole in camera 2

	double e2_aux[9] = {0, -e2.at<double>(2,0), e2.at<double>(1,0), e2.at<double>(2,0), 0, -e2.at<double>(0,0), -e2.at<double>(1,0), e2.at<double>(0,0), 0 };
	cv::Mat e2x = cv::Mat(3,3,CV_64F, e2_aux); 	

	cv::Mat fundMatrix = e2x * _proj * im._proj.inv(cv::DECOMP_SVD);		
	return fundMatrix;
}

void image::setModelViewMatrix()
{
	//glm::vec3 viewDir = glm::vec3(0.0f,0.0f,1.0f);
	//viewDir = glm::transpose(_glmR) * viewDir ;	
	_viewDir = glm::transpose(_glmR) * glm::vec3(0.0f, 0.0f, 1.0f);

	//_optCenterPos = -1* glm::transpose(_glmR) * _glmT;	
	_optCenterPos = _glmC;
	_lookAtPos = _optCenterPos + _viewDir;

	_upDir = glm::vec3(0.0f,-1.0f,0.0f);
	_upDir = glm::normalize(glm::transpose(_glmR) * _upDir);
	
	_modelViewMatrix = glm::lookAt(_optCenterPos,_lookAtPos, _upDir);
	
}

void image::setProjMatrix()
{
	float near1 = 0.1f;  float far1 = 200.0;
	
	float bottom = -( ((float) _image.rows  - _glmK[2][1])/_glmK[1][1] ) * near1 ;	// focal length is in matrix K
	float top    = ( _glmK[2][1]/_glmK[1][1] ) * near1 ;
	float left   = -( _glmK[2][0]/_glmK[0][0] ) * near1 ;
	float right	 = ( ((float)_image.cols - _glmK[2][0])/_glmK[0][0] ) * near1;
	//float bottom = -( ((float) _image.rows  - _glmK[1][2])/_glmK[1][1] ) * near1 ;	// focal length is in matrix K
	//float top    = ( _glmK[1][2]/_glmK[1][1] ) * near1 ;
	//float left   = -( _glmK[0][2]/_glmK[0][0] ) * near1 ;
	//float right	 = ( ((float)_image.cols - _glmK[0][2])/_glmK[0][0] ) * near1;


	_projMatrix = glm::frustum(left,right,bottom,top,near1,far1);
}





