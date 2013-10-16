precision mediump float;
uniform sampler2D Ytex;
varying vec2 vTextureCoord;

void main(void)
{
	float nx,ny,r,g,b;
	
    nx=vTextureCoord[0];
    ny=vTextureCoord[1];
    
	r=texture2D(Ytex,vec2(nx,ny)).r;
	g=texture2D(Ytex,vec2(nx,ny)).g;
	b=texture2D(Ytex,vec2(nx,ny)).b;
    
	gl_FragColor=vec4(r,g,b,1.0);
}
