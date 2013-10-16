//
//  gl_common.h
//  gles2_test3
//
//  Created by User GOBEE on 3/2/12.
//  Copyright (c) 2012 CrossLine Media, Ltd.. All rights reserved.
//

#ifndef gles2_test3_gl_common_h
#define gles2_test3_gl_common_h

#define DO_TRACE
#ifdef DO_TRACE
#define ENTER do {	printf("%s: Enter\n", __FUNCTION__); } while(0)
#define EXIT do {	printf("%s: Exit\n", __FUNCTION__); } while(0)
#else
#define ENTER do {} while(0)
#define EXIT do {} while(0)
#endif

typedef struct
{
	int width;
	int height;
	int stride;
	unsigned char *data;
}
yuv_plane;
typedef yuv_plane th_ycbcr_buffer[3];

typedef yuv_plane img_plane;
typedef yuv_plane img_plane_buffer[3];

typedef struct gl_rendering_context
{
    GLfloat _vertices[20];
    const char g_indices[6];
    GLuint _textureIds[3]; // Texture id of Y,U and V texture.
    GLsizei _textureWidth;
    GLsizei _textureHeight;
    int viewportWidth;
    int viewportHeight;
    GLuint _program;
    
#define SKIP_FRAME_FLAG 0x1
    int skipframe;
    
//    /** yuv frame buffer */
//    img_plane_buffer ycbcr;
//    img_plane_buffer *YUVbuf;
//    
    /* yuv sequence file variables */
#define YUV_SEQ_WIDTH 1280
#define YUV_SEQ_HEIGHT 720
    
    int yuv_fd;
    size_t yuv_frame_size;
    size_t yuv_frame_width;
    size_t yuv_frame_height;
    void *yuv_mmap_p;
    void *yuv_mmap_end_p;
    void *yuv_cur_frame_p;
    size_t yuv_sequence_len;
} gl_rendering_context_t;

gl_rendering_context_t *gl_context_alloc(void);
void gl_context_free(gl_rendering_context_t *gl_ctx);
int loadCompileLinkShaders(gl_rendering_context_t *glCtx_p, const char *vertShaderSource, const char *fragShaderSource);

void setup_yuv(gl_rendering_context_t *glCtx_p, int w, int h, int __do_alloc__);

void UI_Render (void *UI_ctx, img_plane_buffer *frame);

#endif
