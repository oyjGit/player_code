#ifndef VIDEO_RENDERER_GLES_H
#define VIDEO_RENDERER_GLES_H

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>


#define TEST_TEXT_MATRIX
//#define TEST_MODEL_MATRIX

#ifdef TEST_MODEL_MATRIX
#include "GLESMath.h"
#endif



enum
{
	ATTRIB_VERTEX,
	ATTRIB_TEXCOORD,
	NUM_ATTRIBUTES
};


enum
{
	UNIFORM_Y = 0,
	UNIFORM_U,
	UNIFORM_V,
	NUM_UNIFORMS
};


class VideoRendererOpenGles20
{
public:
	VideoRendererOpenGles20();
	~VideoRendererOpenGles20();

	int  createRenderer(int viewWidth,int viewHeight);
	void destroryRenderer();

	bool setViewport(int x,int y,int width,int height);

	bool renderYUV420Frame(unsigned char* yuvFrame,int videoWidth, int videoHeight);

	void clearDisplay();

#ifdef TEST_MODEL_MATRIX
	void matrixLoadIdentity();
	void matrixScale(GLfloat sx, GLfloat sy);
	void matrixTranslate(GLfloat tx, GLfloat ty);
#endif

#ifdef TEST_TEXT_MATRIX
	void textureMatrixLoadIdentity();
	void textureMatrixScale(GLfloat sx, GLfloat sy);
	void textureMatrixTranslate(GLfloat tx, GLfloat ty);
	void textureMatrixScaleTranslate(GLfloat sx, GLfloat sy,GLfloat tx, GLfloat ty);
#endif

private:
	 bool checkGlError(const char* op);

	 GLuint buildShader(const char* source, GLenum shaderType);
	 GLuint createProgram(const char* vertexShaderSource,const char* fragmentShaderSource);
	 void   destroryProgram();

	 void createTexture();
	 void destroryTexture();
	 void bindTexture(GLuint texture, 
		 unsigned char *buffer, 
		 GLuint w , 
		 GLuint h);

	 void checkVideoFormat(int videoWidth,int videoHeight);

	 void resetVertices();
	 int checkMove(float mx, float my);

	 void itextureMatrixScale(GLfloat sx, GLfloat sy);
	 void itextureMatrixTranslate(GLfloat tx, GLfloat ty);

private:
	GLuint m_videoTexture[NUM_UNIFORMS];
	bool   m_hasCreateTexture;

	GLuint m_shaderProgram;
	GLuint m_vertexShader;
	GLuint m_fragmentShader;

	int m_viewportX;
	int m_viewportY;
	int m_viewportWidth;
	int m_viewportHeight;

	int m_textureWidth;
	int m_textureHeight;


	bool m_hasChangeVertices;

#ifdef TEST_MODEL_MATRIX
	KSMatrix4 m_modelViewMatrix;
#endif

	static const char g_vertextShader[];
	static const char g_fragmentShader[];

	static GLfloat g_squareVertices[];
	static GLfloat g_squareTexCoords[];

	//comn::CriticalSection m_csDraw;
};

#endif



