package org.xw;

import android.app.Activity;
import android.hardware.Camera;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.FrameLayout;

public class GobeeDisplayActivity extends Activity {
	long n_oid;
	GobeeDisplay glView;
	View curView;
	private CameraPreview cam_capt;

	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		Log.d("[DROID-DISPLAY-j]",
				"[DROID-DISPLAY-j] Starting display Activity");

		Bundle extras = getIntent().getExtras();
		Log.d("[DROID-DISPLAY-j]", "[DROID-DISPLAY-j] Getting params");

		if (extras != null) {
			n_oid = Integer.valueOf(extras.getString("n_oid")).longValue();
			Log.d("[DROID-DISPLAY-j]",
					"[DROID-DISPLAY-j] n_oid=" + extras.getString("n_oid"));
		}
		setupView();
	}

	public void setupView() {
		FrameLayout fl = new FrameLayout(this);

		this.glView = new GobeeDisplay(this);

		this.curView = fl;

		LayoutParams params = new LayoutParams(LayoutParams.FILL_PARENT,
				LayoutParams.FILL_PARENT);

		fl.setLayoutParams(params);

		this.glView.setLayoutParams(params);
		this.glView.setZOrderOnTop(false);

		fl.addView(this.glView, 0);

		this.cam_capt = new CameraPreview(this, Camera.open());
		this.cam_capt.setZOrderOnTop(true);
		fl.addView(this.cam_capt, 200, 150);

		this.setContentView(this.curView);

	}

}
