//
//  Gobee_GLViewController.h
//  gles2_test3
//
//  Created by User GOBEE on 3/7/12.
//  Copyright (c) 2012 CrossLine Media, Ltd.. All rights reserved.
//

#include "gl_common.h"
#include <xwlib/x_obj.h>

#import <GLKit/GLKit.h>

@interface Gobee_GLViewController : GLKViewController <GLKViewDelegate, GLKViewControllerDelegate>
{
    GLuint _program;
    GLKMatrix4 _modelViewProjectionMatrix;
    GLKMatrix3 _normalMatrix;
    float _rotation;
    GLuint _vertexArray;
    GLuint _vertexBuffer;
    GLKView *glView;
}

@property (strong, nonatomic) EAGLContext *context;
@property (strong, nonatomic) GLKBaseEffect *effect;
@property (nonatomic) x_object *dataSrcXobj;
@property (nonatomic) x_object *hidSrcXobj;
@property (atomic) img_plane_buffer *YUVbuf;
@property (nonatomic) gl_rendering_context_t *renderCtx;

- (void *)initContext;
- (void)setupGL;
- (void)tearDownGL;
- (BOOL)loadShaders;
- (void)addControlView;
- (void)removeAllSubviews;
- (void)initInternals;

@end
