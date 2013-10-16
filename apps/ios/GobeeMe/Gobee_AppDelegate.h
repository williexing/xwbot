//
// Gobee_AppDelegate.h
//  gles2_test3
//
//  Created by User GOBEE on 3/2/12.
//  Copyright (c) 2012 CrossLine Media, Ltd.. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "Gobee_GLViewController.h"

@class Gobee_ViewController;

@interface Gobee_AppDelegate : UIResponder <UIApplicationDelegate,UITextFieldDelegate>
{
    IBOutlet UIButton *runButton;
    IBOutlet Gobee_GLViewController *glViewController;
    IBOutlet Gobee_ViewController *viewController;

    IBOutlet UITextField *jidctl;
    IBOutlet UITextField *pwctl;
}

@property (strong, nonatomic) UIWindow *window;
@property (strong, nonatomic) Gobee_ViewController *viewController;
@property (strong, nonatomic) Gobee_GLViewController *glViewController;
@property (strong, nonatomic) UIButton *runButton;
@property (strong, nonatomic) IBOutlet UIView *view;

@property (nonatomic, retain) IBOutlet UITextField *jidctl;
@property (nonatomic, retain) IBOutlet UITextField *pwctl;

-(IBAction)openGlWindow:(x_object *)xobj framebuf: (img_plane_buffer *)yuvbuf uiCtx: (void **)uiCtx;

@end
