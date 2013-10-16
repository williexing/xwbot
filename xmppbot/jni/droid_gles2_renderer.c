#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "oglrndrr.h"

void
x_vdisplay_java_notify_geometry(virtual_display_t *d, int v1, int v2, int v3,
    int v4, int v5, int v6);

static int32_t _id;
static GLuint _textureIds[3]; // Texture id of Y,U and V texture.
static GLuint _program;
static GLuint _vPositionHandle;
static GLsizei _textureWidth;
static GLsizei _textureHeight;
static int viewportWidth;
static int viewportHeight;

/** yuv frame buffer */
static img_plane_buffer ycbcr;
static img_plane_buffer *YUVbuf = NULL;

static GLfloat _vertices[20] =
  {
  // X, Y, Z, U, V
      -1, -1, 0, 0, 1, // Bottom Left
      1, -1, 0, 1, 1, //Bottom Right
      1, 1, 0, 1, 0, //Top Right
      -1, 1, 0, 0, 0 }; //Top Left

static const char g_indices[] =
  { 0, 3, 2, 0, 2, 1 };

static const char g_vertextShader[] =
  { "attribute vec4 aPosition;\n"
      "attribute vec2 aTextureCoord;\n"
      "varying vec2 vTextureCoord;\n"
      "void main(void) {\n"
      "  gl_Position = aPosition;\n"
      "  vTextureCoord = aTextureCoord;\n"
      "}\n" };

// The fragment shader.
// Do YUV to RGB565 conversion.
static const char g_fragmentShader[] =
  { "precision mediump float;\n"
      "uniform sampler2D Ytex;\n"
      "uniform sampler2D Utex,Vtex;\n"
      "varying vec2 vTextureCoord;\n"
      "void main(void) {\n"

      "  float nx,ny,r,g,b,y,u,v;\n"
      "  mediump vec4 txl,ux,vx;"

      "  nx=vTextureCoord[0];\n"
      "  ny=vTextureCoord[1];\n"
      "  y=texture2D(Ytex,vec2(nx,ny)).r;\n"
      "  u=texture2D(Utex,vec2(nx,ny)).r;\n"
      "  v=texture2D(Vtex,vec2(nx,ny)).r;\n"

      "  y=1.1643*(y-0.0625);\n"
      "  u=u-0.5;\n"
      "  v=v-0.5;\n"

      "  r=y+1.5958*v;\n"
      "  g=y-0.39173*u-0.81290*v;\n"
      "  b=y+2.017*u;\n"

      "  gl_FragColor=vec4(r,g,b,1.0);\n"
      "}\n" };

static void
checkGlError(const char* op)
{
  GLint error;
  for (error = glGetError(); error; error = glGetError())
    {
      __android_log_print(ANDROID_LOG_WARN, "[GLES2/JNI]",
          "[DROID-DISPLAY][GLES2/JNI] after %s() glError (0x%x)\n", op, error);
    }
}

static GLuint
loadShader(GLenum shaderType, const char* pSource)
{
  GLuint shader = glCreateShader(shaderType);

  checkGlError("glCreateShader");

  if (shader)
    {
      glShaderSource(shader, 1, &pSource, NULL);
      checkGlError("glShaderSource");

      glCompileShader(shader);
      checkGlError("glCompileShader");

      GLint compiled = 0;
      glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
      checkGlError("glGetShaderiv");

      if (!compiled)
        {
          GLint infoLen = 0;
          glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
          checkGlError("glGetShaderiv");

          if (infoLen)
            {
              char* buf = (char*) x_malloc(infoLen);
              if (buf)
                {
                  glGetShaderInfoLog(shader, infoLen, NULL, buf);
                  __android_log_print(ANDROID_LOG_WARN, "[GLES2/JNI]",
                      "[DROID-DISPLAY][GLES2/JNI] %s: Could not compile shader %d: %s",
                      __FUNCTION__, shaderType, buf);
                  x_free(buf);
                }
              glDeleteShader(shader);
              shader = 0;
            }
        }
    }

  return shader;
}

static GLuint
createProgram(virtual_display_t *d, const char* pVertexSource,
    const char* pFragmentSource)
{
  GLuint vertexShader = loadShader(0x8B31 /*GL_VERTEX_SHADER*/, pVertexSource);
  if (!vertexShader)
    {
      return 0;
    }

  GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
  if (!pixelShader)
    {
      return 0;
    }

  GLuint program = glCreateProgram();
  if (program)
    {
      __android_log_print(ANDROID_LOG_WARN, "[GLES2/JNI]",
          "[DROID-DISPLAY][GLES2/JNI] Applying vertex Shader\n");

      glAttachShader(program, vertexShader);
      checkGlError("glAttachShader");

      __android_log_print(ANDROID_LOG_WARN, "[GLES2/JNI]",
          "[DROID-DISPLAY][GLES2/JNI] Applying pixel Shader\n");

      glAttachShader(program, pixelShader);
      checkGlError("glAttachShader");

      glLinkProgram(program);
      GLint linkStatus = GL_FALSE;

      glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
      if (linkStatus != GL_TRUE)
        {
          GLint bufLength = 0;
          glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);

          if (bufLength)
            {
              char* buf = (char*) x_malloc(bufLength);
              if (buf)
                {
                  glGetProgramInfoLog(program, bufLength, NULL, buf);
                  __android_log_print(ANDROID_LOG_WARN, "[GLES2/JNI]",
                      "[DROID-DISPLAY][GLES2/JNI] %s: Could not link program: %s",
                      __FUNCTION__, buf);
                  x_free(buf);
                }
            }
          glDeleteProgram(program);
          program = 0;
        }
    }
  return program;
}

#if 1
static int
SetCoordinates(int zOrder, const float left, const float top, const float right,
    const float bottom)
  {
    __android_log_print(ANDROID_LOG_WARN, "[GLES2/JNI]", "[GLES2/JNI] %s: ENTER",
        __FUNCTION__);

    if ((top > 1 || top < 0) || (right > 1 || right < 0)
        || (bottom > 1 || bottom < 0) || (left > 1 || left < 0))
      {
        __android_log_print(ANDROID_LOG_WARN, "[GLES2/JNI]",
            "[GLES2/JNI] %s: Wrong coordinates", __FUNCTION__);
        return -1;
      }

    // Bottom Left
    _vertices[0] = (left * 2) - 1;
    _vertices[1] = -1 * (2 * bottom) + 1;
    _vertices[2] = zOrder;

    //Bottom Right
    _vertices[5] = (right * 2) - 1;
    _vertices[6] = -1 * (2 * bottom) + 1;
    _vertices[7] = zOrder;

    //Top Right
    _vertices[10] = (right * 2) - 1;
    _vertices[11] = -1 * (2 * top) + 1;
    _vertices[12] = zOrder;

    //Top Left
    _vertices[15] = (left * 2) - 1;
    _vertices[16] = -1 * (2 * top) + 1;
    _vertices[17] = zOrder;

    return 0;
  }
#endif

static void
SetupTextures(virtual_display_t *d, img_plane_buffer *yuv)
{
  double ratio;
  int vw = 0;
  int vh = 0;
  GLuint currentTextureId;
  const GLsizei width = (GLsizei) (*yuv)[0].width;
  const GLsizei height = (GLsizei) (*yuv)[0].height;
  const char *buffer = (*yuv)[0].data;
  const char *uComponent = (*yuv)[1].data;
  const char *vComponent = (*yuv)[2].data;

  __android_log_print(ANDROID_LOG_WARN, "[GLES2/JNI]",
      "[DROID-DISPLAY][GLES2/JNI] %s: "
      "width %d, height %d\n", __FUNCTION__, width, height);

  glGenTextures(3, _textureIds); //Generate  the Y, U and V texture
  currentTextureId = _textureIds[0]; // Y
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, currentTextureId);

  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE,
      GL_UNSIGNED_BYTE, (const GLvoid*) buffer);

  currentTextureId = _textureIds[1]; // U
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, currentTextureId);

  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0,
      GL_LUMINANCE, GL_UNSIGNED_BYTE, (const GLvoid*) uComponent);

  currentTextureId = _textureIds[2]; // V
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, currentTextureId);

  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0,
      GL_LUMINANCE, GL_UNSIGNED_BYTE, (const GLvoid*) vComponent);
  checkGlError("SetupTextures");

  _textureWidth = width;
  _textureHeight = height;

//  update viewport ratios
  ratio = (double) _textureHeight / (double) _textureWidth;
  vh = (ratio * (double) viewportWidth);

  if (vh > viewportHeight)
    {
      vw = ((double) viewportHeight / ratio);
      vh = viewportHeight;
    }
  else
    {
      vw = viewportWidth;
    }

  d->left_top_x = (viewportWidth - vw) / 2;
  d->left_top_y = (viewportHeight - vh) / 2;
  d->viewportWidth = vw;
  d->viewportHeight = vh;
  d->frameWidth = _textureWidth;
  d->frameHeight = _textureHeight;

  glViewport((viewportWidth - vw) / 2, (viewportHeight - vh) / 2, vw, vh);
  checkGlError("glViewport");

  __android_log_print(ANDROID_LOG_WARN, "[GLES2/JNI]",
      "[GLES2/JNI] %s: Using viewport: %dx%d, ratio = %f", __FUNCTION__,
      viewportWidth, viewportHeight, ratio);

}

static void
UpdateTextures(virtual_display_t *d, img_plane_buffer *yuv)
{
  GLuint currentTextureId;
  const GLsizei width = (GLsizei) (*yuv)[0].width;
  const GLsizei height = (GLsizei) (*yuv)[0].height;
  const char *buffer = (*yuv)[0].data;
  const char *uComponent = (*yuv)[1].data;
  const char *vComponent = (*yuv)[2].data;

  __android_log_print(ANDROID_LOG_WARN, "[EGL/JNI]", "[DROID-DISPLAY]%s: ENTER", __FUNCTION__);

  currentTextureId = _textureIds[0]; // Y
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, currentTextureId);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_LUMINANCE,
      GL_UNSIGNED_BYTE, (const GLvoid*) buffer);

  currentTextureId = _textureIds[1]; // U
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, currentTextureId);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_LUMINANCE,
      GL_UNSIGNED_BYTE, (const GLvoid*) uComponent);

  currentTextureId = _textureIds[2]; // V
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, currentTextureId);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_LUMINANCE,
      GL_UNSIGNED_BYTE, (const GLvoid*) vComponent);
//  checkGlError("UpdateTextures");

}

int
x_gles2_init(virtual_display_t *d, int width, int height)
{
  int maxTextureImageUnits[2];
  int maxTextureSize[2];

  d->displayWidth = viewportWidth = width;
  d->displayHeight = viewportHeight = height;

  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, maxTextureImageUnits);
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, maxTextureSize);

  __android_log_print(ANDROID_LOG_WARN, "[GLES2/JNI]",
      "[DROID-DISPLAY][GLES2/JNI] %s: number of textures %d, size %d", __FUNCTION__,
      (int) maxTextureImageUnits[0], (int) maxTextureSize[0]);

  _program = createProgram(d, g_vertextShader, g_fragmentShader);
  if (!_program)
    {
      __android_log_print(ANDROID_LOG_WARN, "[GLES2/JNI]",
          "[DROID-DISPLAY][GLES2/JNI] %s: Could not create program", __FUNCTION__);
      return -1;
    }

  int positionHandle = glGetAttribLocation(_program, "aPosition");
  checkGlError("glGetAttribLocation aPosition");
  if (positionHandle == -1)
    {
      __android_log_print(ANDROID_LOG_WARN, "[GLES2/JNI]",
          "[DROID-DISPLAY][GLES2/JNI] %s: Could not get aPosition handle", __FUNCTION__);
      return -1;
    }
  int textureHandle = glGetAttribLocation(_program, "aTextureCoord");
  checkGlError("glGetAttribLocation aTextureCoord");
  if (textureHandle == -1)
    {
      __android_log_print(ANDROID_LOG_WARN, "[GLES2/JNI]",
          "[DROID-DISPLAY][GLES2/JNI] %s: Could not get aTextureCoord handle", __FUNCTION__);
      return -1;
    }

  // set the vertices array in the shader
  // _vertices contains 4 vertices with 5 coordinates. 3 for (xyz) for the vertices and 2 for the texture
  glVertexAttribPointer(positionHandle, 3, GL_FLOAT, 0, 5 * sizeof(GLfloat),
      _vertices);

  glEnableVertexAttribArray(positionHandle);

  // set the texture coordinate array in the shader
  // _vertices contains 4 vertices with 5 coordinates. 3 for (xyz) for the vertices and 2 for the texture
  glVertexAttribPointer(textureHandle, 2, GL_FLOAT, 0, 5 * sizeof(GLfloat),
      &_vertices[3]);
  glEnableVertexAttribArray(textureHandle);

  glUseProgram(_program);
  int i = glGetUniformLocation(_program, "Ytex");
  glUniform1i(i, 0); /* Bind Ytex to texture unit 0 */

  i = glGetUniformLocation(_program, "Utex");
  glUniform1i(i, 1); /* Bind Utex to texture unit 1 */

  i = glGetUniformLocation(_program, "Vtex");
  checkGlError("glGetUniformLocation");
  glUniform1i(i, 2); /* Bind Vtex to texture unit 2 */
  checkGlError("glUniform1i");

//  glViewport(0, 0, width, height);
//  checkGlError("glViewport");
  return 0;
}

int
Render(virtual_display_t *d, img_plane_buffer yuv)
{
#if 1
  __android_log_print(ANDROID_LOG_WARN, "[GLES2/JNI]", "[GLES2/JNI] %s: siz: %dx%d, %dx%d",
      __FUNCTION__, yuv[0].width, yuv[0].height, yuv[0].stride,
      yuv[1].stride);

  ycbcr[0].data = yuv[0].data;
  ycbcr[1].data = yuv[1].data;
  ycbcr[2].data = yuv[2].data;

//  ycbcr[0].width = yuv[0].width;
  ycbcr[0].height = yuv[0].height;
  ycbcr[0].stride = yuv[0].stride;

//  ycbcr[1].width = yuv[1].width;
  ycbcr[1].height = yuv[1].height;
  ycbcr[1].stride = yuv[1].stride;

//  ycbcr[2].width = yuv[2].width;
  ycbcr[2].height = yuv[2].height;
  ycbcr[2].stride = yuv[2].stride;

  ycbcr[0].width = yuv[0].stride;
  ycbcr[1].width = yuv[1].stride;
  ycbcr[2].width = yuv[2].stride;

  yuv[0].width = yuv[0].stride;
  yuv[1].width = yuv[1].stride;
  yuv[2].width = yuv[2].stride;

  YUVbuf = &ycbcr;
#else
  __android_log_print(ANDROID_LOG_WARN, "[GLES2/JNI]", "[GLES2/JNI] %s: siz: %dx%d, %dx%d",
      __FUNCTION__, (*yuv)[0].width, (*yuv)[0].height, (*yuv)[0].stride,
      (*yuv)[1].stride);

  (*yuv)[0].width = (*yuv)[0].stride;
  (*yuv)[1].width = (*yuv)[1].stride;
  (*yuv)[2].width = (*yuv)[2].stride;
  YUVbuf = yuv;
#endif
  return 0;

}

int
__gl_Render(virtual_display_t *d)
{
  GLsizei _w;
  GLsizei _h;

  if (!YUVbuf)
    {
//      __android_log_print(ANDROID_LOG_WARN, "[DROID-DISPLAY]",
//          "%s: YUVbuf=%p", __FUNCTION__,YUVbuf);
      return -1;
    }

  _w = (GLsizei) (*YUVbuf)[0].width;
  _h = (GLsizei) (*YUVbuf)[0].height;

//  __android_log_print(ANDROID_LOG_WARN,
//      "[GLES2/JNI]", "[DROID-DISPLAY][GLES2/JNI] %s:%d",
//      __FUNCTION__, __LINE__);

  glUseProgram(_program);
  checkGlError("glUseProgram");

  if (_textureWidth != _w || _textureHeight != _h)
    {
//      __android_log_print(ANDROID_LOG_WARN,
//           "[GLES2/JNI]", "[DROID-DISPLAY][GLES2/JNI] %s:%d",
//           __FUNCTION__, __LINE__);
      SetupTextures(d,YUVbuf);
      x_vdisplay_java_notify_geometry(d, d->left_top_x, d->left_top_y,
          d->viewportWidth, d->viewportHeight, d->frameWidth,
          d->frameHeight);
    }
  else
    {
//      __android_log_print(ANDROID_LOG_WARN,
//           "[GLES2/JNI]", "[DROID-DISPLAY][GLES2/JNI] %s:%d",
//           __FUNCTION__, __LINE__);
      UpdateTextures(d,YUVbuf);
    }

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, g_indices);
  checkGlError("glDrawArrays");

  YUVbuf = 0;
  return 0;
}

#if 0
static void
translate_xy(int viewportX, int viewportY, int *textureX, int *textureY)
  {
    *textureX = viewportX * _textureWidth / viewportWidth;
    *textureY = viewportY * _textureHeight / viewportHeight;
  }

JNIEXPORT
void JNICALL
emit_touch_event(JNIEnv *env, jobject obj, jint etype, jfloat pressure,
    jfloat x_, jfloat y_)
  {
    int ix;
    int iy;
    __android_log_print(ANDROID_LOG_WARN, "[GLES2/JNI]: ",
        "[GLES2/JNI] Touch event: type(%d) x(%d):y(%d), pressure(%f)\n", etype,
        (int) x_, (int) y_, pressure);
    translate_xy((int) x_, (int) y_, &ix, &iy);
    __android_log_print(ANDROID_LOG_WARN, "[rtp_send_event]: ",
        "[GLES2/JNI] Translated touch event: type(%d) x(%d):y(%d), pressure(%f)\n",
        etype, (int) ix, (int) iy, pressure);
    send_motion_evt(etype, ix, iy);
  }
#endif

