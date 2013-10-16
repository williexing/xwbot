/*
 * x_gl.c
 *
 *  Created on: Nov 13, 2011
 *      Author: artemka
 */

#include <stdlib.h>
#include <math.h>

#undef DEBUG_PRFX
#include <x_config.h>
#if TRACE_VIRTUAL_DIPSLAY_ON
#define TRACE_LEVEL 2
#define DEBUG_PRFX "[VIRT-DISPLAY] "
#endif

#include <xwlib/x_types.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
//#include <GL/gl.h>
//#include <GL/glext.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <gdk/gdk.h>
#include <gdk/gdkpixbuf.h>

#include <theora/theora.h>
#include <theora/theoradec.h>
#include <theora/theoraenc.h>

/*------*/
Display *x_dpy;
Window win;
EGLSurface egl_surf;
EGLContext egl_ctx;
EGLDisplay egl_dpy;
EGLint egl_major, egl_minor, es_ver;
EGLConfig config;
/*------*/

#define PI_OVER_360 0.0087266

static GLuint _textureIds[3]; // Texture id of Y,U and V texture.
static GLuint _program;
static GLsizei _textureWidth;
static GLsizei _textureHeight;
static int viewportWidth;
static int viewportHeight;

/** yuv frame buffer */
static th_ycbcr_buffer YUVbuf;

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
      "void main() {\n"
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
      //"  y = v;\n"+
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
      // printf("after %s() glError (0x%x)\n", op, error);
    }
}

static GLuint
loadShader(GLenum shaderType, const char* pSource)
{
  GLuint shader = glCreateShader(shaderType);
  if (shader)
    {
      glShaderSource(shader, 1, &pSource, NULL);
      glCompileShader(shader);
      GLint compiled = 0;
      glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
      if (!compiled)
        {
          GLint infoLen = 0;
          glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
          if (infoLen)
            {
              char* buf = (char*) malloc(infoLen);
              if (buf)
                {
                  glGetShaderInfoLog(shader, infoLen, NULL, buf);
                  printf("%s: Could not compile shader %d: %s", __FUNCTION__,
                      shaderType, buf);
                  free(buf);
                }
              glDeleteShader(shader);
              shader = 0;
            }
        }
    }
  return shader;
}

static GLuint
createProgram(const char* pVertexSource, const char* pFragmentSource)
{
  GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
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
      glAttachShader(program, vertexShader);
      checkGlError("glAttachShader");
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
              char* buf = (char*) malloc(bufLength);
              if (buf)
                {
                  glGetProgramInfoLog(program, bufLength, NULL, buf);
                  printf("%s: Could not link program: %s", __FUNCTION__, buf);
                  free(buf);
                }
            }
          glDeleteProgram(program);
          program = 0;
        }
    }
  return program;
}

static int
x_gles2_init(int width, int height)
{
  int maxTextureImageUnits[2];
  int maxTextureSize[2];

  viewportWidth = width;
  viewportHeight = height;

  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, maxTextureImageUnits);
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, maxTextureSize);

  printf("%s: number of textures %d, size %d\n", __FUNCTION__,
      (int) maxTextureImageUnits[0], (int) maxTextureSize[0]);

  _program = createProgram(g_vertextShader, g_fragmentShader);
  if (!_program)
    {
      printf("%s: Could not create program\n", __FUNCTION__);
      return -1;
    }

  int positionHandle = glGetAttribLocation(_program, "aPosition");
  checkGlError("glGetAttribLocation aPosition");
  if (positionHandle == -1)
    {
      printf("%s: Could not get aPosition handle\n", __FUNCTION__);
      return -1;
    }
  int textureHandle = glGetAttribLocation(_program, "aTextureCoord");
  checkGlError("glGetAttribLocation aTextureCoord");
  if (textureHandle == -1)
    {
      printf("%s: Could not get aTextureCoord handle\n", __FUNCTION__);
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

  glViewport(0, 0, width, height);
  checkGlError("glViewport");
  return 0;
}

static void
SetupTextures(th_ycbcr_buffer yuv)
{
  GLuint currentTextureId;
  const GLsizei width = (GLsizei) (yuv)[0].width;
  const GLsizei height = (GLsizei) (yuv)[0].height;
  const char *buffer = (yuv)[0].data;
  const char *uComponent = (yuv)[1].data;
  const char *vComponent = (yuv)[2].data;

  printf("%s: width %d, height %d\n", __FUNCTION__, width, height);
  if (!buffer)
    return;

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
}

static void
UpdateTextures(th_ycbcr_buffer yuv)
{
  GLuint currentTextureId;
  const GLsizei width = (GLsizei) (yuv)[0].width;
  const GLsizei height = (GLsizei) (yuv)[0].height;
  const char *buffer = (yuv)[0].data;
  const char *uComponent = (yuv)[1].data;
  const char *vComponent = (yuv)[2].data;

  /*
   printf("%s Y(%p), U(%p), V(%p)\n", __FUNCTION__, (yuv)[0].data, (yuv)[1].data,
   (yuv)[2].data);
   */
  if (!buffer)
    return;

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
  checkGlError("UpdateTextures");

}

int __attribute__((visibility("default")))
Render(th_ycbcr_buffer yuv)
{
  size_t yuvsiz;

  /*
   printf("%s Y(%p), U(%p), V(%p)\n", __FUNCTION__, (yuv)[0].data, (yuv)[1].data,
   (yuv)[2].data);
   */

  (YUVbuf)[0].width = (yuv)[0].stride;
  (YUVbuf)[0].height = (yuv)[0].height;
  (YUVbuf)[0].stride = (yuv)[0].stride;
  (YUVbuf)[0].data = (yuv)[0].data;

  (YUVbuf)[1].width = (yuv)[1].stride;
  (YUVbuf)[1].height = (yuv)[1].height;
  (YUVbuf)[1].stride = (yuv)[1].stride;
  (YUVbuf)[1].data = (yuv)[1].data;

  (YUVbuf)[2].width = (yuv)[2].stride;
  (YUVbuf)[2].height = (yuv)[2].height;
  (YUVbuf)[2].stride = (yuv)[2].stride;
  (YUVbuf)[2].data = (yuv)[2].data;

  return 0;

}

static int
__gl_Render(th_ycbcr_buffer yuv)
{
  if (!yuv[0].data)
    return -1;
  GLsizei _w = (GLsizei) (yuv)[0].width;
  GLsizei _h = (GLsizei) (yuv)[0].height;

  glUseProgram(_program);
  checkGlError("glUseProgram");

  if (_textureWidth != _w || _textureHeight != _h)
    {
      printf("%s: setup textures\n", __FUNCTION__);
      SetupTextures(yuv);
    }
  else
    {
      UpdateTextures(yuv);
    }

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, g_indices);
  checkGlError("glDrawArrays");
  return 0;
}

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
rgb2yuv(unsigned char *rgb, th_ycbcr_buffer *_yuv, int w, int h)
{
  int x;
  int y;
  int r;
  int g;
  int b;

  union
  {
    unsigned int raw;
    unsigned char rgba[4];
  } color;

  for (y = 0; y < h; y++)
    {
      for (x = 0; x < w; x++)
        {
          r = rgb[(y * w + x) * 4 + 0];
          g = rgb[(y * w + x) * 4 + 1];
          b = rgb[(y * w + x) * 4 + 2];

          (*_yuv)[0].data[y * w + x] = clamp(
              ((66 * r + 129 * g + 25 * b + 128) >> 8) + 16);
          (*_yuv)[1].data[(x >> 1) + (y >> 1) * ((*_yuv)[1].stride)] = clamp(
              ((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128);
          (*_yuv)[2].data[(x >> 1) + (y >> 1) * ((*_yuv)[2].stride)] = clamp(
              ((112 * r - 94 * g - 18 * b + 128) >> 8) + 128);

        }
    }
}

static void
setup_yuv(uint32_t *pixels, int w, int h)
{
  size_t yuvsiz;

  printf("%s: id %dx%d\n", __FUNCTION__, w, h);

  /* submit data to encoder */
  YUVbuf[0].width = w; // (w + 15) & ~15;
  YUVbuf[0].height = h; //(h + 15) & ~15;
  YUVbuf[0].stride = YUVbuf[0].width;

  YUVbuf[1].width = YUVbuf[0].width >> 1;
  YUVbuf[1].height = YUVbuf[0].height >> 1;
  YUVbuf[1].stride = YUVbuf[1].width;

  YUVbuf[2].width = YUVbuf[1].width;
  YUVbuf[2].height = YUVbuf[1].height;
  YUVbuf[2].stride = YUVbuf[1].stride;

  yuvsiz = YUVbuf[0].width * YUVbuf[0].height
      + (YUVbuf[1].width * YUVbuf[1].height) * 2;

  YUVbuf[0].data = (unsigned char*) malloc(yuvsiz);
  YUVbuf[1].data = YUVbuf[0].data + YUVbuf[0].width * YUVbuf[0].height;
  YUVbuf[2].data = YUVbuf[1].data + YUVbuf[1].width * YUVbuf[1].height;

  rgb2yuv((unsigned char *) pixels, &YUVbuf, w, h);

}

////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

/*
 * Print a list of extensions, with word-wrapping.
 */
static void
print_extension_list(const char *ext)
{
  const char indentString[] = "    ";
  const int indent = 4;
  const int max = 79;
  int width, i, j;

  if (!ext || !ext[0])
    return;

  width = indent;
  printf("%s", indentString);
  i = j = 0;
  while (1)
    {
      if (ext[j] == ' ' || ext[j] == 0)
        {
          /* found end of an extension name */
          const int len = j - i;
          if (width + len > max)
            {
              /* start a new line */
              printf("\n");
              width = indent;
              printf("%s", indentString);
            }
          /* print the extension name between ext[i] and ext[j] */
          while (i < j)
            {
              printf("%c", ext[i]);
              i++;
            }
          /* either we're all done, or we'll continue with next extension */
          width += len + 1;
          if (ext[j] == 0)
            {
              break;
            }
          else
            {
              i++;
              j++;
              if (ext[j] == 0)
                break;
              printf(", ");
              width += 2;
            }
        }
      j++;
    }
  printf("\n");
}

static void
info(EGLDisplay egl_dpy)
{
  const char *s;

  s = eglQueryString(egl_dpy, EGL_VERSION);
  printf("EGL_VERSION = %s\n", s);

  s = eglQueryString(egl_dpy, EGL_VENDOR);
  printf("EGL_VENDOR = %s\n", s);

  s = eglQueryString(egl_dpy, EGL_EXTENSIONS);
  printf("EGL_EXTENSIONS = %s\n", s);

  s = eglQueryString(egl_dpy, EGL_CLIENT_APIS);
  printf("EGL_CLIENT_APIS = %s\n", s);

  printf("GL_VERSION: %s\n", (char *) glGetString(GL_VERSION));
  printf("GL_RENDERER: %s\n", (char *) glGetString(GL_RENDERER));
  printf("GL_EXTENSIONS:\n");
  print_extension_list((char *) glGetString(GL_EXTENSIONS));
}

static void
xgl_egl_init(GtkWidget *widget, void *data)
{
  EGLint num_configs;

  EGLint attribs[] =
    { EGL_RENDERABLE_TYPE, 0x0, EGL_RED_SIZE, 1, EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1, EGL_NONE };
  EGLint ctx_attribs[] =
    { EGL_CONTEXT_CLIENT_VERSION, 0, EGL_NONE };

  x_dpy = GDK_WINDOW_XDISPLAY(widget->window);
  win = GDK_WINDOW_XWINDOW(widget->window);

  if (!x_dpy)
    {
      TRACE("Error: GDK_WINDOW_XDISPLAY() 0x%p\n", (void *) x_dpy);
      exit(-1);
    }

  TRACE("GDK_WINDOW_XDISPLAY() %p(%p)\n", (void *) x_dpy, (void *) win);

  egl_dpy = eglGetDisplay(x_dpy);
  if (!egl_dpy)
    {
      printf("Error: eglGetDisplay() failed\n");
      exit(-1);
    }

  if (!eglInitialize(egl_dpy, &egl_major, &egl_minor))
    {
      printf("Error: eglInitialize() failed\n");
      exit(-1);
    }

  attribs[1] = EGL_OPENGL_ES2_BIT;
  ctx_attribs[1] = 2;

  if (!eglChooseConfig(egl_dpy, attribs, &config, 1, &num_configs))
    {
      printf("Error: couldn't get an EGL visual config\n");
      exit(1);
    }

  eglBindAPI(EGL_OPENGL_ES_API);

  egl_ctx = eglCreateContext(egl_dpy, config, EGL_NO_CONTEXT, ctx_attribs);
  if (!egl_ctx)
    {
      printf("Error: eglCreateContext failed\n");
      exit(-1);
    }

  egl_surf = eglCreateWindowSurface(egl_dpy, config, win, NULL);

  if (!egl_surf)
    {
      printf("Error: eglCreateWindowSurface failed\n");
      exit(-1);
    }

  /*XMapWindow(x_dpy, win);*/
  if (!eglMakeCurrent(egl_dpy, egl_surf, egl_surf, egl_ctx))
    {
      printf("Error: eglMakeCurrent() failed\n");
      return;
    }

  info(x_dpy);

}

int
xgl_start(GtkWidget *widget, void *data)
{
  GdkPixbuf *pbuf;
  uint32_t * pixels;
  int w;
  int h;

  ENTER;

  /**
   * @todo Loading pixbuf here causes
   * awful memory leaks.
   */
  pbuf = gdk_pixbuf_new_from_file("logo.png", NULL);
  pixels = (uint32_t *) gdk_pixbuf_get_pixels(pbuf);

  w = gdk_pixbuf_get_width(pbuf);
  h = gdk_pixbuf_get_height(pbuf);

  setup_yuv(pixels, w, h);

  xgl_egl_init(widget, data);

  EXIT;

  return 1;
}

int
xgl_render_scene(GtkWidget *widget, GdkEvent *_event, void *data)
{

  __gl_Render(YUVbuf);
  eglWaitGL();
  eglSwapBuffers(egl_dpy, egl_surf);

  return 0;
}

int
xgl_configure(GtkWidget *widget, GdkEvent *ev, void *data)
{
  ENTER;

  x_gles2_init(ev->configure.width, ev->configure.height);

  EXIT;
  return 0;
}
