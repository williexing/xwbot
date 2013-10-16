/**
 * 
 */
package org.xw;

import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.Enumeration;
import java.util.HashMap;

import android.app.Activity;
import android.content.Intent;
import android.util.Log;

/**
 * @author artemka
 * 
 */
public class GobeeMeSystem {

	static HashMap<String, Long> hidMap 
		= new HashMap<String, Long>();
	
	Activity context;
	long _bus;

	public GobeeMeSystem(Activity gobeeActivity) {
		context = gobeeActivity;
	}

	private native long jni_bus_init(String jid, String pw);

	private native void jni_bus_connect(long nbus);

	private native void jni_bus_add_ip(long nbus, String ip);

	public boolean gobeeStart(final String j, final String p) {

		new Thread() {
			public void run() {
				try {
					_bus = jni_bus_init(j, p);
					jni_bus_connect(_bus);
				} catch (Exception e) {
					Log.d("[xwIP]", "[xwIP] Error connecting bus");
				}
			}
		}.start();

		try {
			updateInterfaces();
		} catch (Exception e) {
			Log.d("[xwIP]", "[xwIP] Cannot update interfaces");
		}

		return true;
	}

	public void openGobeeDisplay(final long n_oid) {
		context.runOnUiThread(new Runnable() {

			@Override
			public void run() {
				Log.d("[DROID-DISPLAY-j]",
						"[DROID-DISPLAY-j] Starting display Activity");
				Intent intent = new Intent(context, GobeeDisplayActivity.class);
				intent.putExtra("n_oid", Long.toString(n_oid));
				context.startActivity(intent);
			}

		});
	}

	public void registerHid4Sid(final String sid, final long n_oid) 
	{
		Log.d("[Gobee]", "[Gobee] " +
				"registerHid4Sid(): sid=" + sid
				+ " xobj=" + new Long(n_oid).toString());
		hidMap.put(sid,new Long(n_oid));
	}

	public static long getHid4Sid(final String sid) 
	{
		Log.d("[Gobee]", "[Gobee] getHid4Sid(): sid=" + sid);
		try
		{
			return hidMap.get(sid).longValue();
		}
		catch(Exception e)
		{
			return 0;
		}
	}

	public void updateInterfaces() throws SocketException {
		String sLocalIP;
		String sInterfaceName;

		for (Enumeration<NetworkInterface> en = NetworkInterface
				.getNetworkInterfaces(); en.hasMoreElements();) {

			NetworkInterface intf = en.nextElement();
			// Iterate over all IP addresses in each network interface.

			for (Enumeration<InetAddress> enumIPAddr = intf.getInetAddresses(); enumIPAddr
					.hasMoreElements();) {
				InetAddress iNetAddress = enumIPAddr.nextElement();

				// Loop back address (127.0.0.1) doesn't count as an in-use
				// IP address.
				if (!iNetAddress.isLoopbackAddress()) {
					sLocalIP = iNetAddress.getHostAddress().toString();
					sInterfaceName = intf.getName();
					if (sLocalIP.indexOf('%') >= 0
							|| sLocalIP.indexOf(':') >= 0)
						continue;
					Log.d("[xwIP]", "[xwIP] adding ip: " + sLocalIP);
					jni_bus_add_ip(_bus, sLocalIP);
				}
			}

		}
	}

	public static void initJni() {
		try {
			Log.d("[Gobee]", "[Gobee] Loading natives 1.");

			System.loadLibrary("ev");
			System.loadLibrary("xofactory");
			System.loadLibrary("xw");

			System.loadLibrary("xw++");

			System.loadLibrary("xwcrypt");
			System.loadLibrary("xstun");
			System.loadLibrary("tls");
			System.loadLibrary("sessions");

			Log.d("[Gobee]", "[Gobee] Loading natives 2.");
			System.loadLibrary("theoradec");
			System.loadLibrary("theoraenc");

			Log.d("[Gobee]", "[Gobee] Loading natives 3.");
			System.loadLibrary("xparser");

			Log.d("[Gobee]", "[Gobee] Loading natives 4.");
			System.loadLibrary("auth_plain");
			System.loadLibrary("bind");
			System.loadLibrary("commands");
			System.loadLibrary("discovery");
			System.loadLibrary("evtdomain");
			System.loadLibrary("xbus");

			System.loadLibrary("features");
			System.loadLibrary("ibshell");
			System.loadLibrary("iq");
			System.loadLibrary("jingle");
			System.loadLibrary("message");
			System.loadLibrary("presence");
			System.loadLibrary("procsys");
			System.loadLibrary("pubapi");
			System.loadLibrary("sasl");
			System.loadLibrary("stream");
			System.loadLibrary("xmppbot");
			System.loadLibrary("_x_theora");
			System.loadLibrary("_x_speex");
			System.loadLibrary("_x_hid");
			System.loadLibrary("xvpx");
			System.loadLibrary("xwjni");
		} catch (UnsatisfiedLinkError e) {
			System.err.println("Native code library failed to load.\n" + e);
			Log.d("[Gobee]", "[Gobee] ERROR! " + e);
		}

	}
}
