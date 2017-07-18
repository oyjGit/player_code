#include "Logs.h"
#include "VideoRendererGles.h"
#include <stdlib.h>

enum MoveDirection
{
	MOVE_DIRECTION_NONE  = 0,
	MOVE_DIRECTION_LEFT  = 1,
	MOVE_DIRECTION_RIGHT = 2,
	MOVE_DIRECTION_UP    = 4,
	MOVE_DIRECTION_DOWN  = 8
};


#ifdef TEST_MODEL_MATRIX
const char VideoRendererOpenGles20::g_vertextShader[] = {
	"attribute vec4 position;"
	"attribute vec2 texCoord;" 
	"varying vec2 tc;"
	"uniform mat4 modelView;"
	"void main()"
	"{"
		"gl_Position = position * modelView;"
		"tc = texCoord;"
	"}" };

#else

const char VideoRendererOpenGles20::g_vertextShader[] = {
      "attribute vec4 position;"
	  "attribute vec2 texCoord;" 
	  "varying vec2 tc;"
      "void main()"
      "{"
          "gl_Position = position;"
	      "tc = texCoord;"
	   "}" };


#endif

const char VideoRendererOpenGles20::g_fragmentShader[] = {
	"precision mediump float;"
	"varying lowp vec2 tc;"
	"uniform sampler2D SamplerY;"
	"uniform sampler2D SamplerU;"
	"uniform sampler2D SamplerV;"
	"void main()"
	"{"
		"mediump vec3 yuv;"
		"lowp vec3 rgb;"
		"yuv.x = texture2D(SamplerY, tc).r;"
		"yuv.y = texture2D(SamplerU, tc).r - 0.5;"
		"yuv.z = texture2D(SamplerV, tc).r - 0.5;"
		"rgb = mat3( 1,   1,   1,"
			"0, -0.39465,  2.03211,"
			"1.13983, -0.58060, 0) * yuv;"
		"gl_FragColor = vec4(rgb, 1);"
	"}" };



GLfloat VideoRendererOpenGles20::g_squareVertices[] =
{
	-1.0f, -1.0f,
	1.0f,  -1.0f,
	-1.0f,  1.0f,
	1.0f,   1.0f
};

GLfloat VideoRendererOpenGles20::g_squareTexCoords[] =
{
	0.0f,  1.0f,
	1.0f,  1.0f,
	0.0f,  0.0f,
	1.0f,  0.0f
};




VideoRendererOpenGles20::VideoRendererOpenGles20()
	:m_hasCreateTexture(false),
	 m_shaderProgram(0),
	 m_vertexShader(0),
	 m_fragmentShader(0),
	 m_viewportX(0),
	 m_viewportY(0),
	 m_viewportWidth(0),
	 m_viewportHeight(0),
	 m_textureWidth(0),
	 m_textureHeight(0),
	 m_hasChangeVertices(false)
{
#ifdef TEST_MODEL_MATRIX
	ksMatrixLoadIdentity(&m_modelViewMatrix);
#endif
}

VideoRendererOpenGles20::~VideoRendererOpenGles20()
{
	destroryRenderer();
}

bool VideoRendererOpenGles20::checkGlError(const char* op)
{
    GLint error;
	bool hasError = false;
    for (error = glGetError(); error; error = glGetError())
	{
		JPLAYER_LOG_ERROR("after %s() glError (0x%x)\n", op, error);
		hasError = true;
    }
	return hasError;
}

GLuint VideoRendererOpenGles20::buildShader(const char* source, GLenum shaderType) 
{
	GLuint shaderHandle = glCreateShader(shaderType);
    if (!shaderHandle)
	{
		return 0;
	}

	glShaderSource(shaderHandle, 1, &source, 0);
    glCompileShader(shaderHandle);

    GLint compiled = 0;
    glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compiled);
    if (compiled)
	{
		return shaderHandle;
	}

    GLint infoLen = 0;
    glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &infoLen);
    if (infoLen)
	{
        char* buf = (char*) malloc(infoLen+1);
        if (buf) 
		{
            glGetShaderInfoLog(shaderHandle, infoLen, &infoLen, buf);
            JPLAYER_LOG_ERROR("Could not compile shader %d:\n%s\n", shaderType, buf);
            free(buf);
        }
    }
    glDeleteShader(shaderHandle);
    return 0;
}

void VideoRendererOpenGles20::destroryProgram()
{
	if (m_shaderProgram != 0)
	{
		glDetachShader(m_shaderProgram, m_vertexShader);
		glDetachShader(m_shaderProgram, m_fragmentShader);
		glDeleteProgram(m_shaderProgram);
		m_shaderProgram = 0;
	}
}

GLuint VideoRendererOpenGles20::createProgram(const char* vertexShaderSource, 
											  const char* fragmentShaderSource) 
{
	m_vertexShader = buildShader(vertexShaderSource, GL_VERTEX_SHADER);
	if(!m_vertexShader) 
	{
		JPLAYER_LOG_ERROR("Could not buildShader GL_VERTEX_SHADER\n");
		return 0;
	}

	m_fragmentShader = buildShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
	if(!m_fragmentShader)
	{
		glDeleteShader(m_vertexShader);
		JPLAYER_LOG_ERROR("Could not buildShader GL_FRAGMENT_SHADER\n");
		return 0;
	}

	GLuint programHandle = glCreateProgram();
	if (!programHandle) 
	{
		glDeleteShader(m_vertexShader);
		glDeleteShader(m_fragmentShader);
		JPLAYER_LOG_ERROR("Could not glCreateProgram\n");
		return 0;
	}

	glAttachShader(programHandle, m_vertexShader);
	//checkGlError("glAttachShader vertexShader");

	glAttachShader(programHandle, m_fragmentShader);
	checkGlError("glAttachShader fragmentShader");

	glBindAttribLocation(programHandle, ATTRIB_VERTEX, "position");
	glBindAttribLocation(programHandle, ATTRIB_TEXCOORD, "texCoord");

	glLinkProgram(programHandle);

	GLint linkStatus = GL_FALSE;
	glGetProgramiv(programHandle, GL_LINK_STATUS, &linkStatus);
	if (linkStatus == GL_FALSE)
	{
		GLint bufLength = 0;
		glGetProgramiv(programHandle, GL_INFO_LOG_LENGTH, &bufLength);
		if (bufLength) 
		{
			char* buf = (char*) malloc(bufLength);
			if (buf) 
			{
				glGetProgramInfoLog(programHandle, bufLength, NULL, buf);
				JPLAYER_LOG_ERROR("Could not link program:\n%s\n", buf);
				free(buf);
			}
		}
		
		glDetachShader(programHandle, m_vertexShader);
		glDeleteShader(m_vertexShader);

		glDetachShader(programHandle, m_fragmentShader);
		glDeleteShader(m_fragmentShader);

		glDeleteProgram(programHandle);

		JPLAYER_LOG_ERROR("Could not glLinkProgram\n");
		return 0;
	}

	glDeleteShader(m_vertexShader);
	glDeleteShader(m_fragmentShader);

	glUseProgram(programHandle);
	return programHandle;
}


void VideoRendererOpenGles20::createTexture()
{
	if (m_hasCreateTexture)
	{
		destroryTexture();
	}


	glGenTextures(NUM_UNIFORMS, m_videoTexture);

	glBindTexture(GL_TEXTURE_2D, m_videoTexture[UNIFORM_Y]);		//Y Panel
	///如果贴图小的话，那我们需要使用放大函数进行放大操作
	///模糊一点，应该使用GL_LINEAR 清晰使用GL_NEAREST
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
	///在贴图过多时，使用压缩函数进行缩小
	///模糊一点，应该使用GL_LINEAR 清晰使用GL_NEAREST
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	checkGlError("glTexParameteri");
	///GL_CLAMP_TO_EDGE表示OpenGL只画图片一次，剩下的部分将使用图片最后一行像素重复
	///GL_REPEAT意味着OpenGL应该重复纹理超过1.0的部分
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	checkGlError("glTexParameteri");
	///GL_CLAMP_TO_EDGE表示OpenGL只画图片一次，剩下的部分将使用图片最后一行像素重复
	///GL_REPEAT意味着OpenGL应该重复纹理超过1.0的部分
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	checkGlError("glTexParameteri");


	glBindTexture(GL_TEXTURE_2D, m_videoTexture[UNIFORM_U]);		//U Panel
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);		
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


	glBindTexture(GL_TEXTURE_2D, m_videoTexture[UNIFORM_V]);		//V Panel		
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);		
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	checkGlError("createTexture");


	JPLAYER_LOG_INFO("videoTexture0:%d videoTexture1:%d videoTexture2:%d\n",
		m_videoTexture[0],m_videoTexture[1],m_videoTexture[2]);

	m_hasCreateTexture = true;
}

void VideoRendererOpenGles20::destroryTexture()
{
	if (!m_hasCreateTexture)
	{
		return;
	}

	glDeleteTextures(NUM_UNIFORMS, m_videoTexture);
	m_hasCreateTexture = false;
}

void VideoRendererOpenGles20::destroryRenderer()
{
	///删除之前的纹理数组
	destroryTexture();

	/////删除之前的程序
	destroryProgram();
}


int VideoRendererOpenGles20::createRenderer(int viewWidth,int viewHeight)
{
	if(m_shaderProgram != 0)
	{
		destroryRenderer();
	}

	if (m_shaderProgram == 0)
	{
		///创建新程序
		m_shaderProgram = createProgram(g_vertextShader, g_fragmentShader);
		if (m_shaderProgram == 0)
		{
			return -1;
		}

		if ( !m_hasCreateTexture )
		{
			createTexture();
		}


		JPLAYER_LOG_INFO("m_shaderProgram:%d m_vertexShader:%d m_fragmentShader:%d\n",
			m_shaderProgram,m_vertexShader,m_fragmentShader);
	}

	

	m_viewportX = 0;
	m_viewportY = 0;
	m_viewportWidth = viewWidth;
	m_viewportHeight = viewHeight;


#ifdef TEST_MODEL_MATRIX
	matrixLoadIdentity();
#endif

#ifdef TEST_TEXT_MATRIX
	textureMatrixLoadIdentity();
#endif

	return m_shaderProgram;
}

bool VideoRendererOpenGles20::setViewport(int x,int y,int width,int height)
{
	m_viewportX = x;
	m_viewportY = y;
	m_viewportWidth = width;
	m_viewportHeight = height;

	return true;
}


void VideoRendererOpenGles20::bindTexture(GLuint texture, 
										  unsigned char *buffer, 
										  GLuint w , 
										  GLuint h)
{
	///纹理ID使用glBindTexture方式进行绑定，后面将使用ID来调用纹理
	glBindTexture ( GL_TEXTURE_2D, texture );
	checkGlError("glBindTexture");

	///2D纹理
	///texImage2D(GL10.GL_TEXTURE_2D,0, bitmap, 0);
	glTexImage2D ( GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, 
		GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer);


	checkGlError("glTexImage2D");
}


bool VideoRendererOpenGles20::renderYUV420Frame(unsigned char* yuvFrame,
												int videoWidth,
												int videoHeight)
{
	//comn::AutoCritSec lock(m_csDraw);

	if (m_shaderProgram == 0)
	{
		return false;
	}

	if(videoWidth%4 == 0)
		glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
	else if(videoWidth%8 == 0)
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	else
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	checkVideoFormat(videoWidth,videoHeight);
	

	glViewport(m_viewportX, m_viewportY, m_viewportWidth, m_viewportHeight);
	
	bindTexture(m_videoTexture[UNIFORM_Y], 
		yuvFrame, 
		videoWidth, 
		videoHeight);
	bindTexture(m_videoTexture[UNIFORM_U], 
		yuvFrame + videoWidth*videoHeight, 
		videoWidth/2, 
		videoHeight/2);
	bindTexture(m_videoTexture[UNIFORM_V], 
		yuvFrame + videoWidth*videoHeight*5/4, 
		videoWidth/2, 
		videoHeight/2);


	glClearColor(0.0f, 0.0f, 0.0f, 1.0f );
	glClear(GL_COLOR_BUFFER_BIT);
	

	GLint tex_y = glGetUniformLocation(m_shaderProgram, "SamplerY");
	if (checkGlError("SamplerY")) 
	{
		return false;
	}
	GLint tex_u = glGetUniformLocation(m_shaderProgram, "SamplerU");
	if (checkGlError("SamplerU")) 
	{
		return false;
	}
	GLint tex_v = glGetUniformLocation(m_shaderProgram, "SamplerV");
	if (checkGlError("SamplerV")) 
	{
		return false;
	}

#ifdef TEST_MODEL_MATRIX
	GLint tex_m = glGetUniformLocation(m_shaderProgram, "modelView");
#endif

	// Update attribute values.
	glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, g_squareVertices);
	glEnableVertexAttribArray(ATTRIB_VERTEX);

	glVertexAttribPointer(ATTRIB_TEXCOORD, 2, GL_FLOAT, 0, 0, g_squareTexCoords);
	glEnableVertexAttribArray(ATTRIB_TEXCOORD);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_videoTexture[UNIFORM_Y]);
	glUniform1i(tex_y, UNIFORM_Y);
	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_videoTexture[UNIFORM_U]);
	glUniform1i(tex_u, UNIFORM_U);
	
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_videoTexture[UNIFORM_V]);
	glUniform1i(tex_v, UNIFORM_V);
	

#ifdef TEST_MODEL_MATRIX
	glUniformMatrix4fv(tex_m, 1, GL_FALSE, (GLfloat*)&(m_modelViewMatrix.m[0][0])); 
	checkGlError("glUniformMatrix4fv");
#endif

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	return true;
}

void VideoRendererOpenGles20::clearDisplay()
{
	glViewport(m_viewportX, m_viewportY, m_viewportWidth, m_viewportHeight);
	glClearColor(0.0,0.0,0.0,0.0); 
	glClear(GL_COLOR_BUFFER_BIT); 
	glFlush();//
}


void VideoRendererOpenGles20::checkVideoFormat(int videoWidth,int videoHeight)
{
	if ( (m_textureHeight == 0) || (m_textureWidth == 0))
	{
		m_textureHeight = videoHeight;
		m_textureWidth = videoWidth;
	}
	
	if (   (m_textureHeight != videoHeight)
		|| (m_textureWidth  != videoWidth) )
	{
		createRenderer(m_viewportWidth,m_viewportHeight);

		m_textureHeight = videoHeight;
		m_textureWidth  = videoWidth;

		JPLAYER_LOG_INFO("checkVideoFormat,videoWidth:%d,videoHeight:%d\n",
			videoWidth,videoHeight);
	}
}


#ifdef TEST_MODEL_MATRIX
void VideoRendererOpenGles20::matrixLoadIdentity()
{
	ksMatrixLoadIdentity(&m_modelViewMatrix);
}

void VideoRendererOpenGles20::matrixScale(GLfloat sx, GLfloat sy)
{
	ksScale(&m_modelViewMatrix,sx,sy,0);
}

void VideoRendererOpenGles20::matrixTranslate(GLfloat tx, GLfloat ty)
{
	
	//m_viewportX = tx;
	//m_viewportY = ty;

	//KSMatrix4 tmpModelViewMatrix;
	//ksMatrixLoadIdentity(&tmpModelViewMatrix);
	ksTranslate(&m_modelViewMatrix,tx, ty, 0);
	//ksMatrixMultiply(&m_modelViewMatrix, &tmpModelViewMatrix,&m_modelViewMatrix);
}
#endif

#ifdef TEST_TEXT_MATRIX

void VideoRendererOpenGles20::resetVertices()
{
	g_squareVertices[0] = -1.0f;
	g_squareVertices[2] = 1.0f;
	g_squareVertices[4] = -1.0f;
	g_squareVertices[6] = 1.0f;

	g_squareVertices[1] = -1.0f;
	g_squareVertices[3] = -1.0f;
	g_squareVertices[5] = 1.0f;
	g_squareVertices[7] = 1.0f;

	m_hasChangeVertices = false;
}

void VideoRendererOpenGles20::textureMatrixLoadIdentity()
{
	//comn::AutoCritSec lock(m_csDraw);

	g_squareTexCoords[0] = 0.0f;
	g_squareTexCoords[2] = 1.0f;
	g_squareTexCoords[4] = 0.0f;
	g_squareTexCoords[6] = 1.0f;

	g_squareTexCoords[1] = 1.0f;
	g_squareTexCoords[3] = 1.0f;
	g_squareTexCoords[5] = 0.0f;
	g_squareTexCoords[7] = 0.0f;


	//resetVertices();

}


void VideoRendererOpenGles20::textureMatrixScaleTranslate(GLfloat sx, GLfloat sy,
														  GLfloat tx, GLfloat ty)
{
	//comn::AutoCritSec lock(m_csDraw);

	itextureMatrixScale(sx,sy);

	itextureMatrixTranslate(tx,ty);
	
}

void VideoRendererOpenGles20::textureMatrixScale(GLfloat x, GLfloat y)
{
	//comn::AutoCritSec lock(m_csDraw);

	itextureMatrixScale(x,y);
}

void VideoRendererOpenGles20::textureMatrixTranslate(GLfloat x, GLfloat y)
{
	//comn::AutoCritSec lock(m_csDraw);

	itextureMatrixTranslate(x,y);
}



int VideoRendererOpenGles20::checkMove(float mx, float my)
{
	int move = MOVE_DIRECTION_NONE;
	GLfloat tmp = 1.0;

	tmp = g_squareTexCoords[0] + mx;  
	if (tmp > 1.0  || tmp < 0.0)
	{
		if ((move & MOVE_DIRECTION_RIGHT) == 0)
		{
			move += MOVE_DIRECTION_RIGHT;

			//JPLAYER_LOG_INFO("MOVE_DIRECTION_RIGHT mx:%f\n",mx);
		}
	}
	tmp = g_squareTexCoords[4] + mx;  
	if (tmp > 1.0  || tmp < 0.0)
	{
		if ((move & MOVE_DIRECTION_RIGHT) == 0)
		{
			move += MOVE_DIRECTION_RIGHT;

			//JPLAYER_LOG_INFO("MOVE_DIRECTION_RIGHT mx:%f\n",mx);
		}
	}
	tmp = g_squareTexCoords[2] + mx;  
	if (tmp > 1.0  || tmp < 0.0)
	{
		if ((move & MOVE_DIRECTION_LEFT) == 0)
		{
			move += MOVE_DIRECTION_LEFT;

			//JPLAYER_LOG_INFO("MOVE_DIRECTION_LEFT mx:%f\n",mx);
		}
	}
	tmp = g_squareTexCoords[6] + mx;  
	if (tmp > 1.0  || tmp < 0.0)
	{
		if ((move & MOVE_DIRECTION_LEFT) == 0)
		{
			move += MOVE_DIRECTION_LEFT;

			//JPLAYER_LOG_INFO("MOVE_DIRECTION_LEFT mx:%f\n",mx);
		}
	}

	tmp = g_squareTexCoords[1] + my;  
	if (tmp > 1.0  || tmp < 0.0)
	{
		if ((move & MOVE_DIRECTION_UP) == 0)
		{
			move += MOVE_DIRECTION_UP;

			//JPLAYER_LOG_INFO("MOVE_DIRECTION_UP my:%f\n",my);
		}
	}
	tmp = g_squareTexCoords[3] + my;  
	if (tmp > 1.0  || tmp < 0.0)
	{
		if ((move & MOVE_DIRECTION_UP) == 0)
		{
			move += MOVE_DIRECTION_UP;

			//JPLAYER_LOG_INFO("MOVE_DIRECTION_UP my:%f\n",my);
		}
	}
	tmp = g_squareTexCoords[5] + my;  
	if (tmp > 1.0  || tmp < 0.0)
	{
		if ((move & MOVE_DIRECTION_DOWN) == 0)
		{
			move += MOVE_DIRECTION_DOWN;

			//JPLAYER_LOG_INFO("MOVE_DIRECTION_DOWN my:%f\n",my);
		}
	}
	tmp = g_squareTexCoords[7] + my;  
	if (tmp > 1.0  || tmp < 0.0)
	{
		if ((move & MOVE_DIRECTION_DOWN) == 0)
		{
			move += MOVE_DIRECTION_DOWN;

			//JPLAYER_LOG_INFO("MOVE_DIRECTION_DOWN my:%f\n",my);
		}
	}

	return move;
}




void VideoRendererOpenGles20::itextureMatrixScale(GLfloat x, GLfloat y)
{
	if (x == 0.0 || y == 0.0)
	{
		textureMatrixLoadIdentity();
		return;
	}
	
	GLfloat sx = 1.0/x;
	GLfloat sy = 1.0/y;

	//bool hasOver = false;

 //   GLfloat tmp = 1.0;
	//tmp = g_squareTexCoords[0] * sx;
	//if (tmp > 1.0  || tmp < 0.0)
	//{
	//	hasOver = true;
	//}
	//tmp = g_squareTexCoords[2] * sx;
	//if (tmp > 1.0 || tmp < 0.0)
	//{
	//	hasOver = true;
	//}
	//tmp = g_squareTexCoords[4] * sx;
	//if (tmp > 1.0 || tmp < 0.0)
	//{
	//	hasOver = true;
	//}
	//tmp = g_squareTexCoords[6] * sx;
	//if (tmp > 1.0 || tmp < 0.0)
	//{
	//	hasOver = true;
	//}

	//tmp = g_squareTexCoords[1] * sy;
	//if (tmp > 1.0 || tmp < 0.0)
	//{
	//	hasOver = true;
	//}
	//tmp = g_squareTexCoords[3] * sy;
	//if (tmp > 1.0 || tmp < 0.0)
	//{
	//	hasOver = true;
	//}
	//tmp = g_squareTexCoords[5] * sy;
	//if (tmp > 1.0 || tmp < 0.0)
	//{
	//	hasOver = true;
	//}
	//tmp = g_squareTexCoords[7] * sy;
	//if (tmp > 1.0 || tmp < 0.0)
	//{
	//	hasOver = true;
	//}

	//if (hasOver)
	//{
	//	return;
	//}


	g_squareTexCoords[0] *= sx;
	g_squareTexCoords[2] *= sx;
	g_squareTexCoords[4] *= sx;
	g_squareTexCoords[6] *= sx;

	g_squareTexCoords[1] *= sy;
	g_squareTexCoords[3] *= sy;
	g_squareTexCoords[5] *= sy;
	g_squareTexCoords[7] *= sy;

	/*JPLAYER_LOG_INFO("scale 0:%f,1:%f,2:%f,3:%f,4:%f,5:%f,6:%f,7:%f\n",
		g_squareTexCoords[0],
		g_squareTexCoords[1],
		g_squareTexCoords[2],
		g_squareTexCoords[3],
		g_squareTexCoords[4],
		g_squareTexCoords[5],
		g_squareTexCoords[6],
		g_squareTexCoords[7]);*/

}

void VideoRendererOpenGles20::itextureMatrixTranslate(GLfloat x, GLfloat y)
{
	
	GLfloat tx = x/m_viewportWidth;
	GLfloat ty = y/m_viewportHeight;

	int move = checkMove(tx,ty);
	
	//handleMove(move,tx,ty);
	
    ///纹理超出边界了
	/*if (move != MOVE_DIRECTION_NONE)
	{
		return;
	}*/



   ///修改过顶点，就复位
	//resetVertices();

	
	g_squareTexCoords[0] -= tx;
	g_squareTexCoords[4] -= tx;
	g_squareTexCoords[2] -= tx;
	g_squareTexCoords[6] -= tx;

	g_squareTexCoords[1] -= ty;
	g_squareTexCoords[3] -= ty;
	g_squareTexCoords[5] -= ty;
	g_squareTexCoords[7] -= ty;
	

	/*JPLAYER_LOG_INFO("Translate 0:%f,1:%f,2:%f,3:%f,4:%f,5:%f,6:%f,7:%f\n",
		g_squareTexCoords[0],
		g_squareTexCoords[1],
		g_squareTexCoords[2],
		g_squareTexCoords[3],
		g_squareTexCoords[4],
		g_squareTexCoords[5],
		g_squareTexCoords[6],
		g_squareTexCoords[7]);*/

}

#endif



