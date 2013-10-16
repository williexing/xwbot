//
//  Gobee_GLViewController.m
//  gles2_test3
//
//  Created by User GOBEE on 3/7/12.
//  Copyright (c) 2012 CrossLine Media, Ltd.. All rights reserved.
//
#import <Foundation/NSSet.h>
#import <UIKit/UIEvent.h>
#import "Gobee_GLViewController.h"
#include "eagl_common.h"

void n_emit_hid_event(x_object *thiz, int tip, int x, int y, int buttons);


@implementation Gobee_GLViewController
{
  th_ycbcr_buffer *frame;
  UIViewController *glControlsController;
  int controlsState;
}

@synthesize context = _context;
@synthesize effect = _effect;
@synthesize renderCtx = _renderCtx;

- (void *)initContext {
    self->_renderCtx = gl_context_alloc();
  return (void *)self->_renderCtx;
}


- (void)__glkitViewPressed:(UIGestureRecognizer *)sender
{
  CGPoint tapPoint = [sender locationInView: self.view];
  int x = (int) tapPoint.x;
  int y = (int) tapPoint.y;
  int buttons = 0;
  
  printf("glkitViewPressed... long\n");
#if 0
  if (!controlsState)
    [self addControlView];
  else
    [self removeAllSubviews];
  controlsState =!controlsState;
#endif
  
  x = x * self->_renderCtx->_textureWidth / self.view.bounds.size.width;
  y = y * self->_renderCtx->_textureHeight / self.view.bounds.size.height;
  
  if (!self.hidSrcXobj) return;
  
  if (sender.state == UIGestureRecognizerStateBegan)
  {
    buttons = 0x1;
  }
  else if (sender.state == UIGestureRecognizerStateEnded)
  {
    buttons = 0x0;
  }
  else
  {
    return;
  }
  
  n_emit_hid_event(self.hidSrcXobj, 1, x, y, buttons);
}


- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
  if (!self.hidSrcXobj) return;

  UITouch *touch = [touches anyObject];
  CGPoint point = [touch locationInView:self.view];
  
  int x = point.x * self->_renderCtx->_textureWidth / self.view.bounds.size.width;
  int y = point.y * self->_renderCtx->_textureHeight / self.view.bounds.size.height;
      
  n_emit_hid_event(self.hidSrcXobj, 1, x, y, 0x1);
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
  if (!self.hidSrcXobj) return;
  
  UITouch *touch = [touches anyObject];
  CGPoint point = [touch locationInView:self.view];
  
  int x = point.x * self->_renderCtx->_textureWidth / self.view.bounds.size.width;
  int y = point.y * self->_renderCtx->_textureHeight / self.view.bounds.size.height;
  
  n_emit_hid_event(self.hidSrcXobj, 1, x, y, 0x1);
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
  if (!self.hidSrcXobj) return;
  
  UITouch *touch = [touches anyObject];
  CGPoint point = [touch locationInView:self.view];
  
  int x = point.x * self->_renderCtx->_textureWidth / self.view.bounds.size.width;
  int y = point.y * self->_renderCtx->_textureHeight / self.view.bounds.size.height;
  
  n_emit_hid_event(self.hidSrcXobj, 1, x, y, 0x0);
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
}


- (void)addControlView
{

  UIButton *btn = [UIButton buttonWithType:UIButtonTypeRoundedRect];
  [btn setTitle:@"Stop Call" forState:UIControlStateNormal];
  btn.frame = CGRectMake(80.0, 350.0, 160.0, 40.0);
  [self.view addSubview:btn];

  printf("OK\n");
}

- (void)removeAllSubviews
{
  for(UIView *view in self.view.subviews)
  {
    [view removeFromSuperview];
  }
}

- (void)initInternals
{
  UITapGestureRecognizer *swipeTap;
  GLKView *view = (GLKView *)self.view;
  view.context = self.context;
  view.drawableDepthFormat = GLKViewDrawableDepthFormat16;
  
//  swipeTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(__glkitViewPressed:)];
//  [self.view addGestureRecognizer:swipeTap];
  
  [self setupGL];
}

- (void)viewDidLoad
{
  [super viewDidLoad];
//  [self initInternals];
}

- (void)viewDidAppear: (BOOL)animated
{
  [super viewDidAppear:animated];

  self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
  
  if (!self.context) {
    NSLog(@"Failed to create ES context");
  }

  [self initInternals];
}

- (void)viewDidUnload
{
  [self tearDownGL];
  
  if ([EAGLContext currentContext] == self.context) {
    [EAGLContext setCurrentContext:nil];
  }
	self.context = nil;
  
  [super viewDidUnload];
}

- (void)didReceiveMemoryWarning
{
  [super didReceiveMemoryWarning];
  // Release any cached data, images, etc. that aren't in use.
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  // Return YES for supported orientations
  if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
    return (interfaceOrientation != UIInterfaceOrientationPortraitUpsideDown);
  } else {
    return YES;
  }
}

- (void)setupGL
{
  const char *path = getYuSequencePath();
  [EAGLContext setCurrentContext:self.context];
    
  [self loadShaders];
  
  self.effect = [[GLKBaseEffect alloc] init];
  self.effect.light0.enabled = GL_TRUE;
  self.effect.light0.diffuseColor = GLKVector4Make(1.0f, 0.4f, 0.4f, 1.0f);
  
}

- (void)tearDownGL
{
  [EAGLContext setCurrentContext:self.context];
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{
  UI_Render(self->_renderCtx, self->_YUVbuf);
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
}

#pragma mark -  OpenGL ES 2 shader compilation

- (BOOL)loadShaders
{
  NSString *vsh;
  NSString *fsh;
  
  NSString *vertShaderPathname, *fragShaderPathname;
  
  // Create and compile vertex shader.
  vertShaderPathname = [[NSBundle mainBundle] pathForResource:@"YuvShader" ofType:@"vsh"];
  NSLog(@"SHADER PATH: %@\n", vertShaderPathname);
  
  vsh = [NSString stringWithContentsOfFile:vertShaderPathname];
  
  // Create and compile fragment shader.
  fragShaderPathname = [[NSBundle mainBundle] pathForResource:@"YuvShader" ofType:@"fsh"];
  fsh = [NSString stringWithContentsOfFile:fragShaderPathname];
  
  
  loadCompileLinkShaders(self->_renderCtx, [vsh cStringUsingEncoding:[NSString defaultCStringEncoding]], [fsh cStringUsingEncoding:[NSString defaultCStringEncoding]]);
  
  return YES;
}


@end
