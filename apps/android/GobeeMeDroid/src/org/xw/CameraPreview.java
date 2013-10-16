package org.xw;

import java.io.IOException;

import android.content.Context;
import android.graphics.PixelFormat;
import android.hardware.Camera;
import android.hardware.Camera.PreviewCallback;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class CameraPreview extends SurfaceView implements
		SurfaceHolder.Callback, PreviewCallback {

	public static final int WIDTH = 320;
	public static final int HEIGHT = 240;
	
	private SurfaceHolder mHolder;
	private Camera mCamera;

	java.nio.ByteBuffer bbuf[] = new java.nio.ByteBuffer[3];

	public native int _n_setup_frame(int w, int h, int type);

	public native int _n_push_frame(byte[] buf);

	public CameraPreview(Context context, Camera camera) {
		super(context);
		mCamera = camera;

		// Install a SurfaceHolder.Callback so we get notified when the
		// underlying surface is created and destroyed.
		mHolder = getHolder();
		mHolder.addCallback(this);
		// deprecated setting, but required on Android versions prior to 3.0
		mHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
	}

	public void surfaceCreated(SurfaceHolder holder) {
		// The Surface has been created, now tell the camera where to draw the
		// preview.
		try {
			mCamera.setPreviewDisplay(holder);
			mCamera.startPreview();
		} catch (IOException e) {
			Log.d("CAM", "Error setting camera preview: " + e.getMessage());
		}
	}

	public void surfaceDestroyed(SurfaceHolder holder) {
		// empty. Take care of releasing the Camera preview in your activity.
	}

	public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
		// If your preview can change or rotate, take care of those events here.
		// Make sure to stop the preview before resizing or reformatting it.

		if (mHolder.getSurface() == null) {
			// preview surface does not exist
			return;
		}

		// stop preview before making changes
		try {
			mCamera.stopPreview();
		} catch (Exception e) {
			// ignore: tried to stop a non-existent preview
		}

		// set preview size and make any resize, rotate or
		// reformatting changes here

		// start preview with new settings
		try {
			mCamera.setPreviewDisplay(mHolder);

			Camera.Parameters parameters = mCamera.getParameters();
			java.util.List<Integer> formats = parameters
					.getSupportedPreviewFormats();
			
			for (int i = 0; i < formats.size(); i++) {
				Log.d("CAM", "Supported format: " + formats.get(i).toString());
			}

			PixelFormat pf = new PixelFormat();
			PixelFormat.getPixelFormatInfo(
					parameters.getPreviewFormat(), pf);

			Log.d("CAM",
					"Actual format is "
				+ String.valueOf(parameters.getPreviewFormat())
				+ ", bytesPP=" + String.valueOf(pf.bytesPerPixel));

			parameters.setPreviewSize(
					CameraPreview.WIDTH, CameraPreview.HEIGHT);
			mCamera.setParameters(parameters);

			parameters = mCamera.getParameters();
			Camera.Size psiz = parameters.getPreviewSize();

			Log.d("CAM", "Set preview size: " + String.valueOf(psiz.width)
					+ "x" + String.valueOf(psiz.height));

			// bbuf[0] = java.nio.ByteBuffer.allocateDirect(640*480*4);
			bbuf[0] = java.nio.ByteBuffer
					.allocate(psiz.width * psiz.height * 4);

			Log.d("CAM",
					"ByteBuffer: NULL? " + String.valueOf((bbuf[0] == null))
							+ " capacity=" + String.valueOf(bbuf[0].capacity()));

			// bbuf[1] = java.nio.ByteBuffer.allocateDirect(640*480*4);
			// bbuf[2] = java.nio.ByteBuffer.allocateDirect(640*480*4);

			mCamera.addCallbackBuffer(bbuf[0].array());
			// mCamera.addCallbackBuffer(bbuf[1].array());
			// mCamera.addCallbackBuffer(bbuf[2].array());

			Log.d("CAM", "ByteBuffer added");

			_n_setup_frame(psiz.width, psiz.height, 0);

			mCamera.setPreviewCallbackWithBuffer(this);
			mCamera.startPreview();

		} catch (Exception e) {
			Log.e("CAM", "Error starting camera preview: " + e.toString());
		}
	}

	@Override
	public void onPreviewFrame(byte[] data, Camera camera) {
		// Log.d("CAM", "Frame captured");
		_n_push_frame(data);
		camera.addCallbackBuffer(data);
	}
}
