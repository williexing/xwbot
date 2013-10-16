//
//  eagl_common.m
//  gles2_test3
//
//  Created by User GOBEE on 3/2/12.
//  Copyright (c) 2012 CrossLine Media, Ltd.. All rights reserved.
//

#include <stdio.h>

#include "eagl_common.h"

static NSString *
getDocumentDirectory()
{
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    return [paths objectAtIndex:0];
}


const char * 
getYuSequencePath (void)
{
    NSString *fname;
    NSString *dirname;
    const char *c_fname = NULL;
    
    dirname = getDocumentDirectory();
    NSLog(@"Looking yuv sequence in '%@'...",dirname);
    
    fname = [dirname stringByAppendingPathComponent:(NSString *) @"speakhead_1280x720.yuv"];
    if ([[NSFileManager defaultManager] fileExistsAtPath:fname])
    {
        NSLog(@"File exists at path '%@'!\n",fname);
    }
    else
    {
        NSLog(@"File not found: '%@'\n",fname);
    }
    
    c_fname = [ fname cStringUsingEncoding:NSUTF8StringEncoding ];
    
YUV_ERR:
    return c_fname;
    
}


UIImage *
loadLogoImage(void)
{
	NSString *logPath;
	UIImage *img;
	CGImageRef cgImage;
	uint32_t *pixels;
	NSUInteger __width;
	NSUInteger __height;
	CGColorSpaceRef colorSpace;
	
	colorSpace= CGColorSpaceCreateDeviceRGB();
	
	
	logPath = [[NSBundle mainBundle] pathForResource:@"images/Rest" ofType:@"png"];
	img = [UIImage imageWithContentsOfFile: logPath];
    
	cgImage = img.CGImage;
    
	__width = CGImageGetWidth(cgImage);
	__height = CGImageGetHeight(cgImage);
	
//	pixels = (uint32_t *)malloc(__width * __height * 4);
    
	CGContextRef context = CGBitmapContextCreate((unsigned char *)pixels, __width, __height, 8, __width *4, colorSpace, kCGImageAlphaPremultipliedLast |
												 kCGBitmapByteOrder32Big);
	
	CGColorSpaceRelease(colorSpace);
	CGContextDrawImage(context, CGRectMake(0, 0, __width, __height), cgImage);	
	CGContextRelease(context);
	
//	free(pixels);
	
	return img;
}
