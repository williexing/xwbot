package org.xw;

import android.app.Activity;
import android.os.Bundle;
import android.widget.Button;

public class GobeeActivity extends Activity {

	String jid;
	String pwd;
	GobeeMeSystem system;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		Bundle extras = getIntent().getExtras();
		if (extras != null) {
			jid = extras.getString("jid");
			pwd = extras.getString("upwd");
		}
		setContentView(R.layout.roster_main);
		this.gobeeConnect(jid, pwd);
	}

	private void gobeeConnect(String jid2, String pwd2) {
		system = new GobeeMeSystem(this);
		system.gobeeStart(jid, pwd);
	}

}
