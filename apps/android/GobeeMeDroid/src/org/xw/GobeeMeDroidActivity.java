package org.xw;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;

public class GobeeMeDroidActivity extends Activity {
	/** Called when the activity is first created. */
	static {
		Log.d("[Gobee]", "[Gobee] Loading system libs");
		GobeeMeSystem.initJni();
	}

	private OnClickListener mCorkyListener = new OnClickListener() {
		public void onClick(View v) {
			EditText txt1 = (EditText) findViewById(R.id.editText1);
			EditText txt2 = (EditText) findViewById(R.id.editText2);
			
			Intent intent = new Intent(GobeeMeDroidActivity.this,
					GobeeActivity.class);
			
			intent.putExtra("jid",txt1.getText().toString());
			intent.putExtra("upwd",txt2.getText().toString());
			
			startActivity(intent);
		}
	};

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
		Button button = (Button) findViewById(R.id.button2);
		button.setOnClickListener(mCorkyListener);
	}
}