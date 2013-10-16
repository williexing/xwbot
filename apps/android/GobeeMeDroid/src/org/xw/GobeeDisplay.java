package org.xw;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import org.xw.R.string;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.Renderer;
import android.os.Bundle;
import android.util.Log;
import android.util.*;
import android.view.MotionEvent;

public class GobeeDisplay extends GLSurfaceView implements Renderer {

	long _lhid = 0; // hid dev reference 
	
	private native void _n_emit_render_frame(int[] buf, int w, int h);

	private native void _n_emit_setup_display(int w, int h);

	private native void _n_emit_new_display();
	
	private native void _n_emit_hid_event(long hid, int x, 
			int y, int buttons, int keymod);
	
	public GobeeDisplay(Context context /*, AttributeSet attrs*/) {
		super(context /*, attrs */ );
		Log.println(Log.INFO, "[EGL]", "[QQQ GL] OpenGLRenderer()");
		setEGLContextClientVersion(2);
		setRenderer(this);
	}

	public void onSurfaceCreated(GL10 unused, EGLConfig config) {
		Log.println(Log.INFO, "[EGL]", "[QQQ GL] onSurfaceCreated()");
		_n_emit_new_display();
	}

	public void onDrawFrame(GL10 unused) {
//		Log.println(Log.INFO, "[EGL]", "[QQQ GL] onDrawFrame()");
		try {
			_n_emit_render_frame(null, 100, 100);
		} catch (Exception e) {
			Log.println(Log.INFO, "[EGL]", e.toString());
		}
	}

	public void onSurfaceChanged(GL10 unused, int width, int height) {
		Log.println(Log.INFO, "[EGL]",
				"[EGL] onSurfaceChanged() => " + String.valueOf(width) + "x"
						+ String.valueOf(height));
		_n_emit_setup_display(width, height);
		Log.println(Log.INFO, "[EGL]",
				"[EGL] onSurfaceChanged() OK!");
	}

	@Override
	public boolean onTouchEvent(MotionEvent me) {
		if (_lhid != 0 || (_lhid = 
				GobeeMeSystem.getHid4Sid(new String("fixme"))) != 0)
		{
			Log.println(Log.INFO, "[EGL]", "Motion event: " + me.toString());
			_n_emit_hid_event(_lhid, 1, (int)me.getX(), 
					(int)me.getY(), (int)me.getPressure());
		}
		else
		{
			Log.println(Log.INFO, "[EGL]", "Invalid hid device\n");
		}
		return true;
	}

	public void onPause() {
		super.onPause();
		Log.e("[xw_ogl]", "onResume()");
	}

	public void onResume() {
		super.onResume();
		Log.e("[xw_ogl]", "onResume()");
	}

}
