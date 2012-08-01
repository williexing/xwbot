#include <jni.h>
#include <android/log.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#undef DEBUG_PRFX
#define DEBUG_PRFX "[ xwjni ] "
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include "oglrndrr.h"

typedef struct
{
  float Position[3];
  float Color[4];
  float TexCoord[2]; // New
} Vertex;

#if 1
#define TEX_COORD_MAX   1

const Vertex Vertices[] =
  {
  // Front
        {
          { 1, -1, 0 },
          { 1, 0, 0, 1 },
          { TEX_COORD_MAX, 0 } },
        {
          { 1, 1, 0 },
          { 0, 1, 0, 1 },
          { TEX_COORD_MAX, TEX_COORD_MAX } },
        {
          { -1, 1, 0 },
          { 0, 0, 1, 1 },
          { 0, TEX_COORD_MAX } },
        {
          { -1, -1, 0 },
          { 0, 0, 0, 1 },
          { 0, 0 } },
      // Back
        {
          { 1, 1, -2 },
          { 1, 0, 0, 1 },
          { TEX_COORD_MAX, 0 } },
        {
          { -1, -1, -2 },
          { 0, 1, 0, 1 },
          { TEX_COORD_MAX, TEX_COORD_MAX } },
        {
          { 1, -1, -2 },
          { 0, 0, 1, 1 },
          { 0, TEX_COORD_MAX } },
        {
          { -1, 1, -2 },
          { 0, 0, 0, 1 },
          { 0, 0 } },
      // Left
        {
          { -1, -1, 0 },
          { 1, 0, 0, 1 },
          { TEX_COORD_MAX, 0 } },
        {
          { -1, 1, 0 },
          { 0, 1, 0, 1 },
          { TEX_COORD_MAX, TEX_COORD_MAX } },
        {
          { -1, 1, -2 },
          { 0, 0, 1, 1 },
          { 0, TEX_COORD_MAX } },
        {
          { -1, -1, -2 },
          { 0, 0, 0, 1 },
          { 0, 0 } },
      // Right
        {
          { 1, -1, -2 },
          { 1, 0, 0, 1 },
          { TEX_COORD_MAX, 0 } },
        {
          { 1, 1, -2 },
          { 0, 1, 0, 1 },
          { TEX_COORD_MAX, TEX_COORD_MAX } },
        {
          { 1, 1, 0 },
          { 0, 0, 1, 1 },
          { 0, TEX_COORD_MAX } },
        {
          { 1, -1, 0 },
          { 0, 0, 0, 1 },
          { 0, 0 } },
      // Top
        {
          { 1, 1, 0 },
          { 1, 0, 0, 1 },
          { TEX_COORD_MAX, 0 } },
        {
          { 1, 1, -2 },
          { 0, 1, 0, 1 },
          { TEX_COORD_MAX, TEX_COORD_MAX } },
        {
          { -1, 1, -2 },
          { 0, 0, 1, 1 },
          { 0, TEX_COORD_MAX } },
        {
          { -1, 1, 0 },
          { 0, 0, 0, 1 },
          { 0, 0 } },
      // Bottom
        {
          { 1, -1, -2 },
          { 1, 0, 0, 1 },
          { TEX_COORD_MAX, 0 } },
        {
          { 1, -1, 0 },
          { 0, 1, 0, 1 },
          { TEX_COORD_MAX, TEX_COORD_MAX } },
        {
          { -1, -1, 0 },
          { 0, 0, 1, 1 },
          { 0, TEX_COORD_MAX } },
        {
          { -1, -1, -2 },
          { 0, 0, 0, 1 },
          { 0, 0 } } };

const GLubyte Indices[] =
  {
  // Front
      0, 1, 2, 2, 3, 0,
      // Back
      4, 5, 6, 4, 5, 7,
      // Left
      8, 9, 10, 10, 11, 8,
      // Right
      12, 13, 14, 14, 15, 12,
      // Top
      16, 17, 18, 18, 19, 16,
      // Bottom
      20, 21, 22, 22, 23, 20 };
#else
const Vertex Vertices[] =
  {
      {
          { 1, -1, 1},
          { 1, 0, 0, 1}},
      {
          { 1, 1, 1},
          { 1, 0, 0, 1}},
      {
          { -1, 1, 1},
          { 0, 1, 0, 1}},
      {
          { -1, -1, 1},
          { 0, 1, 0, 1}},
      {
          { 1, -1, -1},
          { 1, 0, 0, 1}},
      {
          { 1, 1, -1},
          { 1, 0, 0, 1}},
      {
          { -1, 1, -1},
          { 0, 1, 0, 1}},
      {
          { -1, -1, -1},
          { 0, 1, 0, 1}}};

const GLubyte Indices[] =
  {
    // Front
    0, 1, 2, 2, 3, 0,
    // Back
    4, 5, 6, 6, 7, 4,
    // Left
    2, 3, 7, 7, 6, 2,
    // Right
    0, 4, 5, 5, 1, 0,
    // Top
    6, 2, 1, 1, 5, 6,
    // Bottom
    0, 3, 7, 7, 4, 0};
#endif

GLuint _colorRenderBuffer;
GLuint _projectionUniform;
GLuint _modelUniform;
GLuint _viewUniform;
GLint prog;

GLuint _floorTexture;
/*GLuint _fishTexture;*/
GLuint _texCoordSlot;
GLuint _textureUniform;

GLfloat pMatrix[16] =
  { 0 };
GLfloat mMatrix[16] =
  { 0 };
GLfloat vMatrix[16] =
  { 0 };

static GLuint _positionSlot;
static GLuint _colorSlot;

static const char
    *vertexShaderCode =
        "attribute vec4 Position; \
attribute vec4 SourceColor; \
varying vec4 DestinationColor; \
uniform mat4 Projection; \
uniform mat4 Model; \
uniform mat4 View; \
attribute vec2 TexCoordIn; \
varying vec2 TexCoordOut; \
\
void main(void) { \
    DestinationColor = SourceColor;  \
    vec4 world_pos = Model * Position; \
    vec4 view_pos = View * world_pos; \
    gl_Position = Projection * view_pos; \
    TexCoordOut = TexCoordIn; \
}";

static const char
    *fragmentShaderCode =
        "varying lowp vec4 DestinationColor; \
varying lowp vec2 TexCoordOut; \
uniform sampler2D Texture; \
\
void main(void) { \
   gl_FragColor = /* DestinationColor * */ texture2D(Texture, TexCoordOut); \
   /* gl_FragColor = vec4 (1.0,0,0,1.0); */ \
}";

#define PI_OVER_360 0.0087266

void
update_projection_matrix(GLfloat *m, GLfloat fov, GLfloat aspect,
    GLfloat znear, GLfloat zfar)
{
  GLfloat d = tan(fov * PI_OVER_360);
  GLfloat d_a = d / aspect;
  GLfloat q = (znear + zfar) / (znear - zfar);
  GLfloat qn = (znear * zfar) / (znear - zfar);

  m[0] = d; // d_a;
  m[1] = 0;
  m[2] = 0;
  m[3] = 0;

  m[4] = 0;
  m[5] = d;
  m[6] = 0;
  m[7] = 0;

  m[8] = 0;
  m[9] = 0;
  m[10] = q;
  m[11] = -1;

  m[12] = 0;
  m[13] = 0;
  m[14] = qn / 2;
  m[15] = 0;
}

void
update_model_matrix(GLfloat *m, GLfloat a, GLfloat x, GLfloat y, GLfloat z)
{
  GLfloat _cos0 = cos(a);
  GLfloat _sin0 = sin(a);
  GLfloat _k1 = (1 - _cos0);
  m[0] = _k1 + _cos0;
  m[1] = _sin0;
  m[2] = _k1;
  m[3] = 0;

  m[4] = -_sin0;
  m[5] = _cos0;
  m[6] = _sin0;
  m[7] = 0;

  m[8] = _k1;
  m[9] = -_sin0;
  m[10] = _k1 + _cos0;
  m[11] = 0;

  m[12] = x;
  m[13] = y;
  m[14] = z;
  m[15] = 1;
}

void
update_view_matrix(GLfloat *m, GLfloat a, GLfloat x, GLfloat y, GLfloat z)
{
  m[0] = 1;
  m[1] = 0;
  m[2] = 0;
  m[3] = 0;

  m[4] = 0;
  m[5] = 1;
  m[6] = 0;
  m[7] = 0;

  m[8] = 0;
  m[9] = 0;
  m[10] = 1;
  m[11] = 0;

  m[12] = x;
  m[13] = y;
  m[14] = z;
  m[15] = 1;
}

static unsigned int width = 100;
static unsigned int height = 100;

static GLuint
_init_texture_from_file(const char *pixels)
{
  GLuint texName;

  glGenTextures(1, &texName);
  glBindTexture(GL_TEXTURE_2D, texName);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
      GL_UNSIGNED_BYTE, pixels);

  return texName;
}

static int
xGlMapShader(int type, const char *src)
{
  int shader = -1;
  shader = glCreateShader(type);
  glShaderSource(shader, 1, &src, NULL);
  glCompileShader(shader);
  return shader;
}

static void
xGlInitShapes()
{
  static char msg[512];
  GLint Index;
  GLuint vertShader;
  GLuint fragShader;
  GLuint framebuffer;
  GLfloat aspect_;

  putchar('&');

  glGenRenderbuffers(1, &_colorRenderBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, _colorRenderBuffer);

    {
      GLuint vertexBuffer;
      GLuint indexBuffer;

      glGenBuffers(1, &vertexBuffer);
      glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
      glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);

      glGenBuffers(1, &indexBuffer);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices,
          GL_STATIC_DRAW);
    }

  // initialize texturess
  glEnable(GL_CULL_FACE | GL_DEPTH_TEST | GL_TEXTURE_2D);
  prog = glCreateProgram();
  vertShader = xGlMapShader(GL_VERTEX_SHADER, vertexShaderCode);
  fragShader = xGlMapShader(GL_FRAGMENT_SHADER, fragmentShaderCode);
  glAttachShader(prog, vertShader);
  glAttachShader(prog, fragShader);
  glLinkProgram(prog);

  // check shader linker's return code
  glGetProgramInfoLog(prog, sizeof msg, NULL, msg);
  printf("info: %s\n", msg);

  // use program
  glUseProgram(prog);

  // initialize vertex coords
  _positionSlot = glGetAttribLocation(prog, "Position");
  _colorSlot = glGetAttribLocation(prog, "SourceColor");
  glEnableVertexAttribArray(_positionSlot);
  glEnableVertexAttribArray(_colorSlot);

  // make symbols references
  _projectionUniform = glGetUniformLocation(prog, "Projection");
  aspect_ = 320 / 240;
  aspect_ = 1;
  update_projection_matrix(pMatrix, 60.0f, aspect_, 4.0f, 100.0f);
  glUniformMatrix4fv(_projectionUniform, 1, 0, pMatrix);

  _modelUniform = glGetUniformLocation(prog, "Model");
  update_model_matrix(mMatrix, 0.1f, 0, 0, 0);
  glUniformMatrix4fv(_modelUniform, 1, 0, mMatrix);

  _viewUniform = glGetUniformLocation(prog, "View");
  update_view_matrix(vMatrix, 0, 0, 0, -10.0f);
  glUniformMatrix4fv(_viewUniform, 1, 0, vMatrix);

  _texCoordSlot = glGetAttribLocation(prog, "TexCoordIn");
  glEnableVertexAttribArray(_texCoordSlot);
  _textureUniform = glGetUniformLocation(prog, "Texture");

}

static void
xGlDrawFlat(const unsigned int *framedata)
{
  static GLfloat zn = 4.0f;
  static GLfloat fov = 60.0f;
  static GLfloat rotate = 1.0f;

  putchar('+');

  glClearColor(0, 0.6, 0.9, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(prog);

  /* send data to OpenGL */
  glVertexAttribPointer(_positionSlot, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
  glVertexAttribPointer(_colorSlot, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
      (GLvoid*) (sizeof(float) * 3));

  // update_projection_matrix(pMatrix, fov, 320 / 240, zn, 18.0f);
  // zn += 0.03f;
  // fov += 0.4f;
  // glUniformMatrix4fv(_projectionUniform, 1, 0, pMatrix);

  update_model_matrix(mMatrix, rotate, 0, 0, 0);
  rotate += 0.05f;
  glUniformMatrix4fv(_modelUniform, 1, 0, mMatrix);

  update_view_matrix(vMatrix, 0, 0, 0, -6.0f);
  glUniformMatrix4fv(_viewUniform, 1, 0, vMatrix);

  // Add inside render:, right before glDrawElements
  glVertexAttribPointer(_texCoordSlot, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
      (GLvoid*) (sizeof(float) * 7));

  // bind textures
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _floorTexture);
  glUniform1i(_textureUniform, 0);

  glDrawElements(GL_TRIANGLES, sizeof(Indices) / sizeof(Indices[0]),
      GL_UNSIGNED_BYTE, 0);
}

JNIEXPORT void
JNICALL
Java_com_xw_OpenGLRenderer_draw_1native_1scene(JNIEnv *env, jobject jobj,
    jintArray buf, jint w, jint h)
{
  jsize len;
  unsigned int *lbuf = NULL;

  len = (*env)->GetArrayLength(env, buf);

  // TODO This should not be allocated on each drawing call
  // lbuf = malloc (len * sizeof(jbyte));
  // TODO sizeof (unsigned int) should be changed to sizeof(uint32)
  lbuf = (unsigned int *) calloc(width * height, sizeof(unsigned int));

  (*env)->GetIntArrayRegion(env, buf, 0, len, lbuf);
  // load textures
  _floorTexture = _init_texture_from_file((const char *) lbuf);

  free((void *) lbuf);

  xGlDrawFlat((const unsigned int *) lbuf);

  return;
}

extern x_object *
gobee_init(int argc, char **argv);
extern int
gobee_connect(x_object *xobj);

JNIEXPORT void
JNICALL
Java_com_xw_OpenGLRenderer_on_1surface(JNIEnv *env, jobject obj)
{
  xGlInitShapes();
}

JNIEXPORT void
JNICALL
Java_com_xw_OpenGLRenderer_on_1surface_1changed(JNIEnv *env, jobject obj,
    jint w, jint h)
{
  glViewport(0, 0, w, h);
}

JNIEXPORT jlong
JNICALL
Java_com_xw_xbus_XBus_xw_1init(JNIEnv *env, jobject obj, jstring jjid,
    jstring jpwd)
{
  x_object *bus;
  char *argv[3] =
    { "xmppbot", 0, 0 };

  argv[1] = (*env)->GetStringUTFChars(env, jjid, 0);
  argv[2] = (*env)->GetStringUTFChars(env, jpwd, 0);

  bus = gobee_init(3, argv);

  (*env)->ReleaseStringUTFChars(env, jjid, argv[1]);
  (*env)->ReleaseStringUTFChars(env, jpwd, argv[2]);

  TRACE("INITIALIZED bus object 0x%p\n",bus);

  return (jlong) (void *) bus;
}

/*
 * Class:     com_xw_xbus_XBus
 * Method:    xw_connect
 * Signature: (J)J
 */
JNIEXPORT void
JNICALL
Java_com_xw_xbus_XBus_xw_1connect(JNIEnv *env, jobject obj, jlong _lbus)
{
  x_object *bus;
  ENTER;
  bus = (x_object *) (void *) _lbus;
  gobee_connect(bus);
  EXIT;
}

/*
 * Class:     com_xw_xbus_XBus
 * Method:    xw_get_roster
 * Signature: ()V
 */
JNIEXPORT void
JNICALL
Java_com_xw_xbus_XBus_xw_1get_1roster(JNIEnv *env, jobject obj, jlong _lbus)
{
  int i = 0;
  x_object *tmp;
  x_object *bus;
  x_object *roster;

  ENTER;

  TRACE("PASSED bus object 0x%x\n",_lbus);

  bus = (x_object *) (void *) _lbus;
  x_object_print_path(bus, 0);
  roster = x_object_from_path(bus, "__proc/presence");
  if (roster)
    {
      TRACE("ROSTER\n");

      for (tmp = x_object_get_child(roster, NULL); tmp; tmp
          = x_object_get_sibling(tmp))
        {
          TRACE("%d: %s\n", i++, x_object_get_name(tmp));
        }
      TRACE("END OF ROSTER\n");

    }
  else
    {
      TRACE("Cannot find ROSTER\n");
    }
  EXIT;
}

