attribute vec4 position;
attribute vec2 texCoord;
varying vec2 tc;
void main()
{
	gl_Position = position ;
	tc = texCoord;
}

