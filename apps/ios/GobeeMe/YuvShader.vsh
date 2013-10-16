attribute vec4 aPosition;
attribute vec2 aTextureCoord;
varying vec2 vTextureCoord;

void main(void)
{
	gl_Position = aPosition;
	vTextureCoord = aTextureCoord;
}
