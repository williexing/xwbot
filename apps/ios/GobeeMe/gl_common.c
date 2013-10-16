//
//  gl_common.c
//  gles2_test3
//
//  Created by User GOBEE on 3/2/12.
//  Copyright (c) 2012 CrossLine Media, Ltd.. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

#include "gl_common.h"

static gl_rendering_context_t glCtx = {
    ._vertices =
    {
        // X, Y, Z, U, V
        -1, -1, 0, 0, 1, // Bottom Left
        1, -1, 0, 1, 1, //Bottom Right
        1, 1, 0, 1, 0, //Top Right
        -1, 1, 0, 0, 0 }, //Top Left
    .g_indices = { 0, 3, 2, 0, 2, 1, },
    .skipframe = 0,
    .yuv_fd = -1,
    .yuv_frame_size = 0,
    .yuv_frame_width = 0,
    .yuv_frame_height = 0,
    .yuv_mmap_p = 0,
    .yuv_mmap_end_p = 0,
    .yuv_cur_frame_p = 0,
    .yuv_sequence_len = 0};

static gl_rendering_context_t *__glCtx_p = &glCtx;


gl_rendering_context_t *
gl_context_alloc(void)
{
  return __glCtx_p;
}

void gl_context_free(gl_rendering_context_t *gl_ctx)
{
  return;
}

////////////////////////////////////
////////////////////////////////////
////////////////////////////////////
////////////////////////////////////

#if 0
static unsigned char
clamp(int d)
{
	if (d < 0)
		return 0;
	
	if (d > 255)
		return 255;
	
	return (unsigned char) d;
}

static void
rgb2yuv(unsigned char *rgb, th_ycbcr_buffer *ycbcr, int w, int h)
{
	int x;
	int y;
	int r;
	int g;
	int b;
	
#if 0
	union
	{
		unsigned int raw;
		unsigned char rgba[4];
	} color;
#endif
	
	ENTER;
	for (y = 0; y < h; y++)
    {
		for (x = 0; x < w; x++)
        {
			r = rgb[(y * w + x) * 4 + 0];
			g = rgb[(y * w + x) * 4 + 1];
			b = rgb[(y * w + x) * 4 + 2];
			
			(*ycbcr)[0].data[y * w + x] = 
            clamp(((66 * r + 129 * g + 25 * b + 128) >> 8) + 16);
			(*ycbcr)[1].data[(x >> 1) + (y >> 1) * ((*ycbcr)[1].stride)] = 
            clamp(((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128);
			(*ycbcr)[2].data[(x >> 1) + (y >> 1) * ((*ycbcr)[2].stride)] = 
            clamp(((112 * r - 94 * g - 18 * b + 128) >> 8) + 128);
			
        }
    }
	EXIT;
}

void
setup_yuv(gl_rendering_context_t *glCtx_p, int w, int h, int __do_alloc__)
{
	size_t yuvsiz;
    
	ENTER;
	/* submit data to encoder */
	glCtx_p->ycbcr[0].width = w; // (w + 15) & ~15;
	glCtx_p->ycbcr[0].height = h; //(h + 15) & ~15;
	glCtx_p->ycbcr[0].stride = glCtx_p->ycbcr[0].width;
	
	glCtx_p->ycbcr[1].width = glCtx_p->ycbcr[0].width >> 1;
	glCtx_p->ycbcr[1].height = glCtx_p->ycbcr[0].height >> 1;
	glCtx_p->ycbcr[1].stride = glCtx_p->ycbcr[1].width;
	
	glCtx_p->ycbcr[2].width = glCtx_p->ycbcr[1].width;
	glCtx_p->ycbcr[2].height = glCtx_p->ycbcr[1].height;
	glCtx_p->ycbcr[2].stride = glCtx_p->ycbcr[1].stride;
	
    if (__do_alloc__)
    {
        yuvsiz = glCtx_p->ycbcr[0].width * glCtx_p->ycbcr[0].height
        + (glCtx_p->ycbcr[1].width * glCtx_p->ycbcr[1].height) * 2;
        
        glCtx_p->ycbcr[0].data = (unsigned char*) malloc(yuvsiz);
        glCtx_p->ycbcr[1].data = glCtx_p->ycbcr[0].data + glCtx_p->ycbcr[0].width * glCtx_p->ycbcr[0].height;
        glCtx_p->ycbcr[2].data = glCtx_p->ycbcr[1].data + glCtx_p->ycbcr[1].width * glCtx_p->ycbcr[1].height;
	}
	
	glCtx_p->YUVbuf = &glCtx_p->ycbcr;
    // set frame values
    glCtx_p->yuv_frame_size = w*h*3/2;
	EXIT;
}

void 
yuv_next_frame (gl_rendering_context_t *glCtx_p, th_ycbcr_buffer *yuv_buffer)
{
    if (!glCtx_p->yuv_cur_frame_p)
    {
        EXIT;
        return;
    }
    
    (*yuv_buffer)[0].data = glCtx_p->yuv_cur_frame_p;
    (*yuv_buffer)[1].data = (*yuv_buffer)[0].data + (*yuv_buffer)[0].width * (*yuv_buffer)[0].height;
    (*yuv_buffer)[2].data = (*yuv_buffer)[1].data + (*yuv_buffer)[1].width * (*yuv_buffer)[1].height;
    
    glCtx_p->yuv_cur_frame_p += glCtx_p->yuv_frame_size;
    if (glCtx_p->yuv_cur_frame_p >= glCtx_p->yuv_mmap_end_p)
        glCtx_p->yuv_cur_frame_p = glCtx_p->yuv_mmap_p;
}
#endif //0

int
x_gles2_init(gl_rendering_context_t *glCtx_p, GLuint _prog)
{
	int maxTextureImageUnits[2];
	int maxTextureSize[2];
	
	ENTER;
	
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureImageUnits[0]);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize[0]);
	
	//NSLog(@"%@: number of textures %d, size %d\n", __FUNCTION__,
    //   (int) maxTextureImageUnits[0], (int) maxTextureSize[0]);
	
	int positionHandle = glGetAttribLocation(_prog, "aPosition");
    
	if (positionHandle == -1)
    {
		//NSLog(@"%@: Could not get aPosition handle\n", __FUNCTION__);
		return -1;
    }
	int textureHandle = glGetAttribLocation(_prog, "aTextureCoord");
    
	if (textureHandle == -1)
    {
		//NSLog(@"%@: Could not get aTextureCoord handle\n", __FUNCTION__);
		return -1;
    }
	
	// set the vertices array in the shader
	// _vertices contains 4 vertices with 5 coordinates. 3 for (xyz) for the vertices and 2 for the texture
	glVertexAttribPointer(positionHandle, 3, GL_FLOAT, 0, 5 * sizeof(GLfloat),
						  glCtx_p->_vertices);
	
	glEnableVertexAttribArray(positionHandle);
	
	// set the texture coordinate array in the shader
	// _vertices contains 4 vertices with 5 coordinates. 3 for (xyz) for the vertices and 2 for the texture
	glVertexAttribPointer(textureHandle, 2, GL_FLOAT, 0, 5 * sizeof(GLfloat),
						  &glCtx_p->_vertices[3]);
	glEnableVertexAttribArray(textureHandle);
	
	glUseProgram(_prog);
	int i = glGetUniformLocation(_prog, "Ytex");
	glUniform1i(i, 0); /* Bind Ytex to texture unit 0 */
	
#ifndef GOBEE_CF_TEST
    
	i = glGetUniformLocation(_prog, "Utex");
	glUniform1i(i, 1); /* Bind Utex to texture unit 1 */
	
	i = glGetUniformLocation(_prog, "Vtex");
	glUniform1i(i, 2); /* Bind Vtex to texture unit 2 */
    
#endif //GOBEE_CF_TEST
    
	EXIT;
	return 0;
}

static void
SetupTextures(gl_rendering_context_t *glCtx_p, th_ycbcr_buffer *yuv)
{
	GLuint currentTextureId;
    
	const GLsizei width = (GLsizei) (*yuv)[0].stride;
	const GLsizei height = (GLsizei) (*yuv)[0].height;
	const char *buffer = (const char *)(*yuv)[0].data;
	const char *uComponent = (const char *)(*yuv)[1].data;
	const char *vComponent = (const char *)(*yuv)[2].data;

	glGenTextures(3, glCtx_p->_textureIds); //Generate  the Y, U and V texture

	currentTextureId = glCtx_p->_textureIds[0]; // Y
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D,currentTextureId);
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE,
				 GL_UNSIGNED_BYTE, (const GLvoid*) buffer);
    
#ifndef  GOBEE_CF_TEST
	currentTextureId = glCtx_p->_textureIds[1]; // U
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, currentTextureId);
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0,
				 GL_LUMINANCE, GL_UNSIGNED_BYTE, (const GLvoid*) uComponent);
	
	currentTextureId = glCtx_p->_textureIds[2]; // V
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, currentTextureId);
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0,
				 GL_LUMINANCE, GL_UNSIGNED_BYTE, (const GLvoid*) vComponent);
#endif //GOBEE_CF_TEST
    
	glCtx_p->_textureWidth = width;
	glCtx_p->_textureHeight = height;
	
}

static void
UpdateTextures(gl_rendering_context_t *glCtx_p, th_ycbcr_buffer *yuv)
{
	GLuint currentTextureId;
	const GLsizei width = (GLsizei) (*yuv)[0].width;
	const GLsizei height = (GLsizei) (*yuv)[0].height;
	const char *buffer = (const char *)(*yuv)[0].data;
	const char *uComponent = (const char *)(*yuv)[1].data;
	const char *vComponent = (const char *)(*yuv)[2].data;

	currentTextureId = glCtx_p->_textureIds[0]; // Y
	glActiveTexture(GL_TEXTURE0);
    //	glBindTexture(GL_TEXTURE_2D, currentTextureId);

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_LUMINANCE,
					GL_UNSIGNED_BYTE, (const GLvoid*) buffer);

#ifndef GOBEE_CF_TEST
	currentTextureId = glCtx_p->_textureIds[1]; // U
	glActiveTexture(GL_TEXTURE1);
    //	glBindTexture(GL_TEXTURE_2D, currentTextureId);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_LUMINANCE,
					GL_UNSIGNED_BYTE, (const GLvoid*) uComponent);
	
	currentTextureId = glCtx_p->_textureIds[2]; // V
	glActiveTexture(GL_TEXTURE2);
    //	glBindTexture(GL_TEXTURE_2D, currentTextureId);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_LUMINANCE,
					GL_UNSIGNED_BYTE, (const GLvoid*) vComponent);
#endif
    
}

void
UI_Render (void *glCtx_p_, th_ycbcr_buffer *frame)
{
  gl_rendering_context_t *glCtx_p = (gl_rendering_context_t *)glCtx_p_;
    GLsizei _w;
    GLsizei _h; 
    
	if (!frame || !glCtx_p_)
    {
        EXIT;
		return;
	}
    
	_w = (GLsizei) (*frame)[0].stride;
	_h = (GLsizei) (*frame)[0].height;

//    glUseProgram(glCtx_p->_program);
	
	if (glCtx_p->_textureWidth != _w || glCtx_p->_textureHeight != _h)
    {
		printf("Setup textures\n");
		SetupTextures(glCtx_p, frame);
    }
	else
    {
//		printf("Update textures (%p)\n",frame[0]->data);
		UpdateTextures(glCtx_p, frame);
    }
	
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, (void *)glCtx_p->g_indices);
    
}


static int 
compileShader(GLuint *shader, GLenum type, const GLchar *source)
{
    GLint status;

	ENTER;
    
    if (!source)
    {
        printf("Failed to load vertex shader\n");
        return -1;
    }
    
    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);
    
#if defined(DEBUG)
    GLint logLength;
    glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetShaderInfoLog(*shader, logLength, &logLength, log);
        printf("Shader compile log:\n%s", log);
        free(log);
    }
#endif
    
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
    if (status == 0)
    {
        glDeleteShader(*shader);
        return -1;
    }
    
	EXIT;
    return 0;
}

static int
linkProgram(GLuint prog)
{
    GLint status;
    
	ENTER;
    
    glLinkProgram(prog);
    
#if defined(DEBUG)
    GLint logLength;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        printf("Program link log:\n%s", log);
        free(log);
    }
#endif
    
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status == 0)
        return -1;
    
	EXIT;
    return 0;
}

static int
validateProgram(GLuint prog)
{
    GLint logLength, status;
    
	ENTER;
    
    glValidateProgram(prog);
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        printf("Program validate log:\n%s", log);
        free(log);
    }
    
    glGetProgramiv(prog, GL_VALIDATE_STATUS, &status);
    if (status == 0)
        return -1;
    
	EXIT;
    return 0;
}


static void
checkGlError (const char *op)
{
	GLint error;
	for (error = glGetError(); error; error = glGetError())
    {
		// printf("after %s() glError (0x%x)\n", op, error);
		printf("after %s() glError (0x%x)\n", op, error);
    }
}

int 
loadCompileLinkShaders(gl_rendering_context_t *glCtx_p,
                       const char *vertShaderSource, const char *fragShaderSource)
{
    GLuint vertShader;
    GLuint fragShader;
    
	ENTER;
    
    // Create shader program
    glCtx_p->_program = glCreateProgram();
  checkGlError("glAttachShader2");
  
    // Create and compile vertex shader
    if (compileShader(&vertShader, GL_VERTEX_SHADER, vertShaderSource))
    {
		checkGlError("compileShader:vertShader");
        printf("Failed to compile vertex shader\n");
        return -1;
    }
	else {
		printf("Vertex shader OK!\n");
	}
    
    
    // Create and compile vertex shader
    if (compileShader(&fragShader, GL_FRAGMENT_SHADER, fragShaderSource))
    {
		checkGlError("compileShader:vertShader");
        printf("Failed to compile fragment shader\n");
        return -1;
    }
	else {
		printf("Fragment shader OK!\n");
	}

    // Attach vertex shader to program
    glAttachShader(glCtx_p->_program, vertShader);
  checkGlError("glAttachShader");
  
    // Attach fragment shader to program
    glAttachShader(glCtx_p->_program, fragShader);
  checkGlError("glAttachShader2");

	printf("Attached shaders: %d", glCtx_p->_program);
	
    // Link program
    if (linkProgram(glCtx_p->_program))
    {
      checkGlError("linkProgram");
     printf("Failed to link program: %d", glCtx_p->_program);
        
        if (vertShader)
        {
            glDeleteShader(vertShader);
            vertShader = 0;
        }
        if (fragShader)
        {
            glDeleteShader(fragShader);
            fragShader = 0;
        }
        if (glCtx_p->_program)
        {
            glDeleteProgram(glCtx_p->_program);
            glCtx_p->_program = 0;
        }
        
        return -1;
    }
    
	printf("GLSL Program initializing...");
	
	x_gles2_init(glCtx_p,glCtx_p->_program);
	
	printf("GLSL Program initialized");
	
    // Release vertex and fragment shaders
    if (vertShader)
        glDeleteShader(vertShader);
        if (fragShader)
            glDeleteShader(fragShader);
            
            ENTER;
    
    return 0;
}
