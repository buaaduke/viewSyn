#include "GLWidgetAllImgs.h"
#include <gl/GLU.h>
#include <glm/gtx/transform.hpp>
#include <iostream>
#include <QKeyEvent>

int GLWidgetAllImgs:: printOglError(char *file, int line)
{
    GLenum glErr;
    int    retCode = 0;
    glErr = glGetError();
    if (glErr != GL_NO_ERROR)
    {
        printf("glError in file %s @ line %d: %s\n",
			     file, line, gluErrorString(glErr));
        retCode = 1;
    }
    return retCode;
}

GLWidgetAllImgs::GLWidgetAllImgs(std::vector<image> **allIms, QGLWidget *sharedWidget, const QList<GLWidget*> &imageQGLWidgets)
	: QGLWidget((QWidget*)NULL, sharedWidget), _allIms(allIms), _imageQGLWidgets(imageQGLWidgets), _virtualImg(NULL)
{
	upDateParam();	
}

void GLWidgetAllImgs :: upDateParam()
{
	computeBoundingBox();

	_nearPlane = _allCamRadius * 0.1;
	_farPlane = _allCamRadius * 500;	// this will work for all the cases
	_fieldOfView = 90;	// this can be changed by mouse

	_objCenterPos = _allCamCenter;
	//_optCenterPos = _allCamCenter + glm::vec3(0., 0., -1.5* _allCamRadius);
	_optCenterPos = _allCamCenter - ((_viewingDir * _allCamRadius) * 5.0f);
	std::cout<<" allCamRadius: " << _allCamRadius << std::endl;
	_virtualModelViewMatrix = glm::lookAt( _optCenterPos, 
		_objCenterPos, glm::vec3(0.0, 1.0, 0.0));
}


void GLWidgetAllImgs :: initializeGL(){	
	glClearColor(0.5, 0.5, 0.5, 1.);
	glColor4f(1., 0., 0., 1.);	
}



void GLWidgetAllImgs::mousePressEvent(QMouseEvent *event)
{
	_mouseX = event->x();
	_mouseY = event->y();
}

void GLWidgetAllImgs::mouseMoveEvent(QMouseEvent *event)
{
	int deltaX =  event->x() - _mouseX;
	int deltaY =  event->y() - _mouseY;
	std::cout<< "mouseX: " << _mouseX << " mouseY: " << _mouseY << std::endl;
	std::cout<< "deltaX: " << deltaX << " deltaY: " << deltaY << std::endl;

	float s = static_cast<float>( std::max(this->width(), this->height()));
	float rangeX = static_cast<float>(deltaX) / (s + 0.00001);
	float rangeY = static_cast<float>(deltaY) / (s + 0.00001);
	std::cout<< "rangeX: " << rangeX << " rangeY: " << rangeY << std::endl;
	//std::cout<< "deltaX: " << deltaX << " deltaY: " << deltaY << std::endl; 
	if (event->buttons() & Qt::LeftButton) {
		glm::mat4x4 inverseVirtualModelViewMatrix = glm::inverse(_virtualModelViewMatrix);
		glm::vec4 dir =   inverseVirtualModelViewMatrix *  glm::vec4(deltaY, deltaX, 0.0f, 0.0f);
		glm::normalize(dir);
		float mag = sqrt(pow(rangeX,2) + pow(rangeY,2)) * 180;
		//glm::vec3 aaa = glm::vec3(0., 0., -1.5* _allCamRadius);
		_virtualModelViewMatrix =  _virtualModelViewMatrix * glm::translate(_objCenterPos) * glm::rotate(mag, dir.x, dir.y, dir.z) * glm::translate(-_objCenterPos);	
	}
	else if (event->buttons() & Qt::RightButton){
		// do the translation		
		glm::mat4x4 inverseProjectionMatrix = glm::inverse(_virtualProjectionMatrix);
		// TPM = P((P^-1)TP)M
		_virtualModelViewMatrix = inverseProjectionMatrix * glm::translate(rangeX, -rangeY, 0.0f) * _virtualProjectionMatrix *  _virtualModelViewMatrix;
		//_virtualProjectionMatrix = glm::translate(rangeX, -rangeY, 0.0f) * _virtualProjectionMatrix;
	}
	makeCurrent();
	display();
	swapBuffers();

	_mouseX = event->x();
	_mouseY = event->y();
}

void GLWidgetAllImgs::wheelEvent(QWheelEvent * event)
{
	bool ctrlPressed = event->modifiers() == Qt::ControlModifier;
	int numDegrees = event->delta();
		
	if(numDegrees > 0 && ctrlPressed )
		_camMinDistance *= 1.01f;
	else if(numDegrees < 0 && ctrlPressed && _camMinDistance > 0.02 )
		_camMinDistance /= 1.01f;
	else if(numDegrees > 0 && !ctrlPressed && _fieldOfView > 0.02)
	{
		_fieldOfView /= 1.01f; // zoom in
		_virtualProjectionMatrix = glm::perspective( _fieldOfView, _aspectRatio, _nearPlane, _farPlane);	
	}
	else
	{
		_fieldOfView *= 1.01f; // zoom out
		_virtualProjectionMatrix = glm::perspective( _fieldOfView, _aspectRatio, _nearPlane, _farPlane);
	}
	makeCurrent();
	display();
	swapBuffers();
}

void GLWidgetAllImgs::keyPressEvent(QKeyEvent* event) {

	switch(event->key()) {
	case Qt::Key_Escape:
		close();
		break;
	
	default:
		event->ignore();
		break;
	}
}

void GLWidgetAllImgs::updateVirtualView_slot(virtualImage virImg)
{
	if(_virtualImg == NULL)
		_virtualImg = new virtualImage();

	*_virtualImg = virImg;
	updateGL();
}


void GLWidgetAllImgs :: resizeGL(int w, int h){
	glViewport(0, 0, w, h);
	_aspectRatio = static_cast<float>(w)/ static_cast<float>(h);
	_windowWidth = w;
	_windowHeight = h;	

	_virtualProjectionMatrix = glm::perspective( _fieldOfView, _aspectRatio, _nearPlane, _farPlane);
}

void GLWidgetAllImgs::display()
{
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	glLoadMatrixf(&(_virtualProjectionMatrix[0][0]));

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glLoadMatrixf(&(_virtualModelViewMatrix[0][0]));
	
	for(size_t i = 0; i< (*_allIms)->size(); i++)
	{		
		drawOneCam( (**_allIms)[i], _imageQGLWidgets[i]); 
	}
	drawCoordinate();
	drawObjectScope();

	if(_virtualImg != NULL)
		drawOneCam( *_virtualImg);

	printOglError(__FILE__, __LINE__);
}

// make a slot to receive virtual image info, and then reDisplay

void GLWidgetAllImgs :: drawObjectScope()
{
	float xmin = -0.054568f; 	float ymin =  0.001728f; 	float zmin = -0.042945f;
	float xmax = 0.047855f;		float ymax = 0.161892f;		float zmax = 0.032236f;

	glBegin(GL_LINE_STRIP);
		glVertex3f( xmin, ymin, zmin);
		glVertex3f( xmax, ymin, zmin);
		glVertex3f( xmax, ymax, zmin);
		glVertex3f( xmin, ymax, zmin);
		glVertex3f( xmin, ymin, zmin);
	//--------------	
		glVertex3f( xmin, ymin, zmax);
		glVertex3f( xmax, ymin, zmax);
		glVertex3f( xmax, ymax, zmax);
		glVertex3f( xmin, ymax, zmax);
		glVertex3f( xmin, ymin, zmax);
	glEnd();
	glBegin(GL_LINES);
		glVertex3f( xmax, ymin, zmin);
		glVertex3f( xmax, ymin, zmax);
	//--------------
		glVertex3f( xmax, ymax, zmin);
		glVertex3f( xmax, ymax, zmax);
	//--------------	
		glVertex3f( xmin, ymax, zmin);
		glVertex3f( xmin, ymax, zmax);
	glEnd();
}

void GLWidgetAllImgs :: paintGL(){
	display();
}	

void GLWidgetAllImgs::drawCoordinate()
{
	glBegin(GL_LINES);
	// x axis
		glColor3f(1., 0., 0.);
		glVertex3f(0.,0.,0.); glVertex3f(_camMinDistance,0.,0.);
	// y axis	
		glColor3f(0., 1., 0.);
		glVertex3f(0.,0.,0.); glVertex3f(0., _camMinDistance,0.);
	// z axis	
		glColor3f(0., 0., 1.);
		glVertex3f(0.,0.,0.); glVertex3f(0.,0.,_camMinDistance);
	glEnd();
}

void GLWidgetAllImgs::drawOneCam(const virtualImage &img)
{
	glm::vec3 optCenterPos = img._optCenterPos;
	glm::vec3 lookAtPos = img._lookAtPos;
	glm::vec3 upDir = img._upDir;

	glm::vec3 camLookAtDir = glm::normalize(lookAtPos - optCenterPos);
	glm::vec3 camUpDir = glm::normalize(upDir);
	glm::vec3 camLeftDir = glm::cross(camUpDir, camLookAtDir);

	float scaleFocalLength = img._glmK[0][0] / static_cast<float>(img._image.cols);
	float scaleImageHeight = img._image.rows/ static_cast<float>(img._image.cols);

	float cameraSizeScale = _camMinDistance;
	//	p1 ---- p2
	//	p4 ---- p3
	glm::vec3 p1 = optCenterPos + camLookAtDir * (cameraSizeScale ) + (camUpDir * scaleImageHeight + camLeftDir) * cameraSizeScale/2.0f;
	glm::vec3 p2 = optCenterPos + camLookAtDir * (cameraSizeScale ) + (camUpDir * scaleImageHeight - camLeftDir) * cameraSizeScale/2.0f;
	glm::vec3 p3 = optCenterPos + camLookAtDir * (cameraSizeScale ) + (-camUpDir * scaleImageHeight - camLeftDir) * cameraSizeScale/2.0f;
	glm::vec3 p4 = optCenterPos + camLookAtDir * (cameraSizeScale ) + (-camUpDir * scaleImageHeight + camLeftDir) * cameraSizeScale/2.0f;


	float currentColor[4];
	glGetFloatv(GL_CURRENT_COLOR,currentColor);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBegin(GL_LINE_LOOP);	
		glVertex3f(p1.x, p1.y,p1.z); 
		glVertex3f(p2.x, p2.y,p2.z);
		glVertex3f(p3.x, p3.y,p3.z);
		glVertex3f(p4.x, p4.y,p4.z);		
	glEnd();
	glBegin(GL_LINES);
		glVertex3f(optCenterPos.x, optCenterPos.y,optCenterPos.z);
		glVertex3f(p1.x, p1.y, p1.z);
		glVertex3f(optCenterPos.x, optCenterPos.y,optCenterPos.z);
		glVertex3f(p2.x, p2.y, p2.z);
		glVertex3f(optCenterPos.x, optCenterPos.y,optCenterPos.z);
		glVertex3f(p3.x, p3.y, p3.z);
		glVertex3f(optCenterPos.x, optCenterPos.y,optCenterPos.z);
		glVertex3f(p4.x, p4.y, p4.z);
	glEnd();

	// draw near planes and far planes
	glBegin(GL_LINE_LOOP);
		glVertex3fv( &(img.nearPlane.leftBottom[0]));
		glVertex3fv( &(img.nearPlane.leftTop[0]));
		glVertex3fv( &(img.nearPlane.rightTop[0]));
		glVertex3fv( &(img.nearPlane.rightBottom[0]));
	glEnd();
	glBegin(GL_LINE_LOOP);
		glVertex3fv( &(img.farPlane.leftBottom[0]));
		glVertex3fv( &(img.farPlane.leftTop[0]));
		glVertex3fv( &(img.farPlane.rightTop[0]));
		glVertex3fv( &(img.farPlane.rightBottom[0]));
	glEnd();
	glBegin(GL_LINES);
		glVertex3fv( &(img.nearPlane.leftBottom[0])); glVertex3fv( &(img.farPlane.leftBottom[0]));
		glVertex3fv( &(img.nearPlane.leftTop[0])); glVertex3fv( &(img.farPlane.leftTop[0]));
		glVertex3fv( &(img.nearPlane.rightTop[0])); glVertex3fv( &(img.farPlane.rightTop[0]));
		glVertex3fv( &(img.nearPlane.rightBottom[0])); glVertex3fv( &(img.farPlane.rightBottom[0]));
	glEnd();

	glColor4f(currentColor[0], currentColor[1], currentColor[2], currentColor[3]);

	


}

void GLWidgetAllImgs::drawOneCam(const image &img, GLWidget* _oneImageQGLWidgets)
{
	glm::vec3 optCenterPos = img._optCenterPos;
	glm::vec3 lookAtPos = img._lookAtPos;
	glm::vec3 upDir = img._upDir;

	glm::vec3 camLookAtDir = glm::normalize(lookAtPos - optCenterPos);
	glm::vec3 camUpDir = glm::normalize(upDir);
	glm::vec3 camLeftDir = glm::cross(camUpDir, camLookAtDir);

	float scaleFocalLength = img._glmK[0][0] / static_cast<float>(img._image.cols);
	float scaleImageHeight = img._image.rows/ static_cast<float>(img._image.cols);

	float cameraSizeScale = _camMinDistance;
	//	p1 ---- p2
	//	p4 ---- p3
	glm::vec3 p1 = optCenterPos + camLookAtDir * (cameraSizeScale ) + (camUpDir * scaleImageHeight + camLeftDir) * cameraSizeScale/2.0f;
	glm::vec3 p2 = optCenterPos + camLookAtDir * (cameraSizeScale ) + (camUpDir * scaleImageHeight - camLeftDir) * cameraSizeScale/2.0f;
	glm::vec3 p3 = optCenterPos + camLookAtDir * (cameraSizeScale ) + (-camUpDir * scaleImageHeight - camLeftDir) * cameraSizeScale/2.0f;
	glm::vec3 p4 = optCenterPos + camLookAtDir * (cameraSizeScale ) + (-camUpDir * scaleImageHeight + camLeftDir) * cameraSizeScale/2.0f;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, _oneImageQGLWidgets->_tex._textureID);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glBegin(GL_QUADS);	
		glTexCoord2f(0.0f,0.0f);
		glVertex3f(p1.x, p1.y,p1.z); 
		glTexCoord2f(1.0f,0.0f);
		glVertex3f(p2.x, p2.y,p2.z);
		glTexCoord2f(1.0f,1.0f);
		glVertex3f(p3.x, p3.y,p3.z);
		glTexCoord2f(0.0f,1.0f);
		glVertex3f(p4.x, p4.y,p4.z);		
	glEnd();
	glDisable(GL_TEXTURE_2D);

	glBegin(GL_LINES);
		glVertex3f(optCenterPos.x, optCenterPos.y,optCenterPos.z);
		glVertex3f(p1.x, p1.y, p1.z);
		glVertex3f(optCenterPos.x, optCenterPos.y,optCenterPos.z);
		glVertex3f(p2.x, p2.y, p2.z);
		glVertex3f(optCenterPos.x, optCenterPos.y,optCenterPos.z);
		glVertex3f(p3.x, p3.y, p3.z);
		glVertex3f(optCenterPos.x, optCenterPos.y,optCenterPos.z);
		glVertex3f(p4.x, p4.y, p4.z);
	glEnd();
}


void GLWidgetAllImgs:: computeBoundingBox()
{
	// find the bounding box of all cameras
	int numOfCams = (*_allIms)->size(); 
	if(numOfCams == 0) {std::cout<< "WARNING: there is no cameras" <<std::endl; exit(0);}
	
	_allCamCenter = glm::vec3(0.,0.,0.); 
	for( int i = 0; i < numOfCams; i++)
	{
		_allCamCenter += (**_allIms)[i]._glmC;
	}
	_allCamCenter = _allCamCenter / static_cast<float>(numOfCams);
	_allCamRadius = -1.0f;
	//_allCamMaxDistance = MAX_FLOAT;
	for( int i = 0; i< numOfCams; i++)
	{
		float radius = glm::distance( (**_allIms)[i]._glmC, _allCamCenter);
		if( radius > _allCamRadius)
			_allCamRadius = radius;
	}
	if(numOfCams == 1)
		_allCamRadius = 1.0f;
	//------------------------------------------------------------------------
	if(numOfCams > 1)
	{
		_camMinDistance = glm::distance((**_allIms)[0]._glmC, (**_allIms)[1]._glmC );
		for( int i = 0; i< numOfCams; i++)
			for( int j = i + 1; j<numOfCams;j++)
			{
				if( _camMinDistance > glm::distance((**_allIms)[0]._glmC, (**_allIms)[1]._glmC ))
					_camMinDistance = glm::distance((**_allIms)[0]._glmC, (**_allIms)[1]._glmC );
			}
	}
	else
	{
		_camMinDistance = 1.0f;	
	}

	_viewingDir = (** _allIms)[0]._viewDir;
	_viewingDir = glm::normalize(_viewingDir);

}