//
//  main.m
//  gles2_test3
//
//  Created by User GOBEE on 3/2/12.
//  Copyright (c) 2012 CrossLine Media, Ltd.. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "Gobee_AppDelegate.h"

extern void x_gobee_init(void);

int main(int argc, char *argv[])
{
    @autoreleasepool {
      x_gobee_init();
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([Gobee_AppDelegate class]));
//        return UIApplicationMain(argc, argv, nil, nil);
    }
}
	