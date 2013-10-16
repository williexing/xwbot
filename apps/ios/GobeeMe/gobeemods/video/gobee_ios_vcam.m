//
//  gobee_ios_vcam.m
//  GobeeMe
//
//  Copyright (c) 2013 CrossLine Media, Ltd.. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <AVFoundation/AVCaptureOutput.h>

#include <x_obj.h>

#pragma mark - capture

@interface myIosCameraController :  NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>

- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection;

//- (UIImage *) imageFromSampleBuffer:(CMSampleBufferRef) sampleBuffer;

- (void) setVcamTarget: (x_object *)xobj;

@end


@implementation myIosCameraController

{
  x_object *xobj_cam_out;
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
fromConnection:(AVCaptureConnection *)connection
{
  CVImageBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
  CVPixelBufferLockBaseAddress(pixelBuffer, 0);
  
  unsigned char *rawPixelBase = (unsigned char *)CVPixelBufferGetBaseAddress(pixelBuffer);
  int bytesPerRow = CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0);
  
  char *lumaBuffer = (char*)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 0);

  if (self->xobj_cam_out)
    _WRITE(self->xobj_cam_out, lumaBuffer, bytesPerRow*288*3/2, NULL);

  CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
}

- (void) setVcamTarget: (x_object *)xobj
{
  xobj_cam_out = xobj;
}

@end

// Create and configure a capture session and start it running
void
create_camera (const char *sid, x_object *xobj)
{
  NSError *error = nil;
  AVCaptureSession *session = [[AVCaptureSession alloc] init];
  myIosCameraController *camCtl = [[myIosCameraController alloc] init];
  
  [camCtl setVcamTarget:xobj];
  
  session.sessionPreset = AVCaptureSessionPreset352x288;
  
  AVCaptureDevice *camera = [AVCaptureDevice
                             defaultDeviceWithMediaType:AVMediaTypeVideo];
  
  AVCaptureDeviceInput *input = [AVCaptureDeviceInput deviceInputWithDevice:camera
                                                                      error:&error];
  if (!input)
  {
    NSLog(@"PANIC: no media input");
  }
  
  [session addInput:input];
  
  AVCaptureVideoDataOutput *output = [[AVCaptureVideoDataOutput alloc] init];
  output.minFrameDuration = CMTimeMake(1,15);
  output.videoSettings =
      [NSDictionary dictionaryWithObject:
       [NSNumber numberWithInt:kCVPixelFormatType_420YpCbCr8BiPlanarFullRange]
                                forKey:(id)kCVPixelBufferPixelFormatTypeKey];

  [session addOutput:output];
  
  dispatch_queue_t queue = dispatch_queue_create("myQueue", NULL);
  [output setSampleBufferDelegate:camCtl queue:queue];
  dispatch_release(queue);
  
  [session startRunning];
}


