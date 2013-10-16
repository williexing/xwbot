//
//  Gobee_AppDelegate.m
//  gles2_test3
//
//  Created by User GOBEE on 3/2/12.
//  Copyright (c) 2012 CrossLine Media, Ltd.. All rights reserved.
//

#include <xwlib/x_obj.h>

#import "Gobee_AppDelegate.h"

//extern void x_mic_init(void);
//extern void x_virtdisplay_init(void);
//extern x_object *gobee_init(int argc, char **argv);
//extern void gobee_connect(x_object *bus);

static Gobee_AppDelegate *appInstance = NULL;

void display_new (x_object *xoid, img_plane_buffer *yuvbuf, void **pctx)
{
  dispatch_async(dispatch_get_main_queue(), ^{
    [appInstance openGlWindow:xoid framebuf:yuvbuf uiCtx:pctx];
  });
}

void register_vcam_with_sid (const char *sid, const x_object *xoid)
{
//  dispatch_async(dispatch_get_main_queue(), ^{
//    [appInstance openGlWindow:xoid framebuf:yuvbuf uiCtx:pctx];
//  });
}

@implementation Gobee_AppDelegate
{
  x_object *xbus;
}
@synthesize window = _window;
@synthesize view;
@synthesize viewController = _viewController;
@synthesize glViewController;
@synthesize runButton;

@synthesize jidctl;
@synthesize pwctl;

-(void) __xbusStart:(NSObject *)obj
{
//  gobee_connect(xbus);
}

-(void) xbusStart
{
//  char *argv[] = {"gobeeme","username@qqq.org/ios","passwd"};
//  x_mic_init();
//  x_virtdisplay_init();
  
//  xbus= gobee_init(3,argv);
//  [NSThread detachNewThreadSelector:@selector(__xbusStart:) toTarget:self withObject:nil];
}

-(IBAction)openGlWindow:(x_object *)xobj framebuf: (img_plane_buffer *)yuvbuf uiCtx: (void **)uiCtx
{
  NSLog(@"%s",__FUNCTION__);
  
  EAGLContext * context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
  
  self.glViewController =
  [[Gobee_GLViewController alloc] initWithNibName:nil bundle:nil];
  
  self.glViewController.delegate = self.glViewController;
  self.glViewController.preferredFramesPerSecond = 15;
  
  GLKView *gview = [[GLKView alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
  gview.context = context;
  gview.delegate = self.glViewController;
  self.glViewController.view = gview;
  
  
  if (xobj)
  {
    self.glViewController.dataSrcXobj = xobj;
  }
  
  if (yuvbuf)
  {
    self.glViewController.YUVbuf = yuvbuf;
  }
  
  if (uiCtx)
  {
    *uiCtx = [glViewController initContext];
  }
  
  self.window.rootViewController = self.glViewController;
  
  self.window.backgroundColor = [UIColor whiteColor];
  [self.window makeKeyAndVisible];
  
}


- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
  appInstance = self;
  self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];

  UIView *uiview = [[UIView alloc] initWithFrame:self.window.frame ];
  
  [self.window addSubview:uiview];
  [self.window makeKeyAndVisible];
  [self xbusStart];
  
  return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
  /*
   Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
   Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
   */
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
  /*
   Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
   If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
   */
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
  /*
   Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
   */
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
  /*
   Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
   */
}

- (void)applicationWillTerminate:(UIApplication *)application
{
  /*
   Called when the application is about to terminate.
   Save data if appropriate.
   See also applicationDidEnterBackground:.
   */
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  [textField resignFirstResponder];
  return NO;
}

@end
