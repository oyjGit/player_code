#include <stdio.h>


///屏幕坐标系和GL坐标系转换
class GLPointUtil
{
public:
	inline bool setScreenSize(float screenWidth,float screenHeight)
	{
		if (screenWidth == 0.0 || screenHeight == 0.0)
		{
			return false;
		}

		m_screenWidth  = screenWidth;
		m_screenHeight = screenHeight;
		m_ratio        = 1.0;
		return true;
	}


	/*
	* Convert x to openGL
	* @param  x Screen x offset top left
	* @return Screen x offset top left in OpenGL
	*/
	inline float toGLX(float x)
	{
		return -1.0f * m_ratio + toGLWidth(x);
	}

	/*
	* Convert y to openGL y
	* @param y  Screen y offset top left
	* @return Screen y offset top left in OpenGL
	*/
	inline float toGLY(float y)
	{
		return 1.0f - toGLHeight(y);
	}


	/*
	* Convert width to openGL width
	* @param  width
	* @return Width in openGL
	*/
	inline float toGLWidth(float width) 
	{
		return 2.0f * (width / m_screenWidth) * m_ratio;
	}

	/*
	* Convert height to openGL height
	* @param height
	* @return Height in openGL
	*/
	inline float toGLHeight(float height) 
	{
		return 2.0f * (height / m_screenHeight);
	}

	/*
	* Convert x to screen x
	* @param glX openGL x
	* @return screen x
	*/
	inline float toScreenX(float glX) 
	{
		return toScreenWidth(glX - (-1 * m_ratio));
	}

	/*
	* Convert y to screent y
	* @param glY  openGL y
	* @return screen y
	*/
	inline float toScreenY(float glY) 
	{
		return toScreenHeight(1.0f - glY);
	}

	/*
	* Convert glWidth to screen width
	* @param  glWidth
	* @return Width in screen
	*/
	inline float toScreenWidth(float glWidth) 
	{
		return (glWidth * m_screenWidth) / (2.0f * m_ratio);
	}

	/*
	* Convert height to screen height
	* @param glHeight
	* @return Height in screen
	*/
	inline float toScreenHeight(float glHeight) 
	{
		return (glHeight * m_screenHeight) / 2.0f;
	}


private:
	float m_screenWidth;
	float m_screenHeight;
	float m_ratio;
};


