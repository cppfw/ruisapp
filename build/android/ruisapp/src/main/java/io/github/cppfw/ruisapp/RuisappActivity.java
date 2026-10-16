package io.github.cppfw.ruisapp;

import android.app.NativeActivity;
import android.app.Service;
import android.content.Context;
import android.graphics.Insets;
import android.graphics.Rect;
import android.os.Build;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;

import java.io.IOException;


public class RuisappActivity extends NativeActivity {
	{
		System.loadLibrary("c++_shared");
		System.loadLibrary("ruisapp_application");
	}

	public static final String LOGTAG = "ruisapp java side";

	private int curKeyDevice;
	private KeyCharacterMap curCharMap;

	public int resolveKeyUnicode(int deviceID, int metaState, int keyCode){
		Log.d(LOGTAG, "resolveKeyUnicode(): invoked, keyCode = " + keyCode);
		if(this.curCharMap == null || curKeyDevice != deviceID){
			try{
				this.curCharMap = KeyCharacterMap.load(deviceID);
				this.curKeyDevice = deviceID;
			}catch(Exception e){
				Log.e(LOGTAG, "resolveKeyUnicode(): Could not load KeyCharacterMap");
				return 0;//could not load char map, thus could not resolve character
			}
		}

		return this.curCharMap.get(keyCode, metaState);
	}

	public float getDotsPerInch(){
		Display d = ((WindowManager)getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
		DisplayMetrics m = new DisplayMetrics();
		d.getMetrics(m);
		Log.d(LOGTAG, "getDotsPerInch(): xdpi = " + m.xdpi + " ydpi = " + m.ydpi + " density = " + m.density);
		return (m.xdpi + m.ydpi) / 2.0f;
	}

	public float getPixelsPerDp(){
		Display d = ((WindowManager)getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
		DisplayMetrics m = new DisplayMetrics();
		d.getMetrics(m);
		Log.d(LOGTAG, "getPixelsPerDp(): density = " + m.density);
		return m.density;
	}

	public int[] getScreenResolution(){
		DisplayMetrics metrics = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getRealMetrics(metrics);
		int width = metrics.widthPixels;
		int height = metrics.heightPixels;
		Log.d(LOGTAG, "getScreenResolution(): width = " + width + ", height = " + height);
		return new int[] {width, height};
	}

	/**
	 * Returns the current content insets in pixels, in ruis::sides order
	 * {left, top, right, bottom} - the per-side max of the system bar insets and the
	 * on-screen keyboard (IME) insets, i.e. the region beyond which the content is not
	 * visible. Queried from the root window insets.
	 * Called by the native side when the window content rect changes (initial layout,
	 * rotation, ...), as a fallback for the WindowInsets listener (which is the primary
	 * source of inset updates).
	 */
	public int[] getInsets(){
		android.view.WindowInsets in = this.getWindow().getDecorView().getRootWindowInsets();
		if(in == null){
			return new int[] {0, 0, 0, 0};
		}
		int[] ret = this.computeInsets(in);
		Log.d(LOGTAG, "getInsets(): " + ret[0] + ", " + ret[1] + ", " + ret[2] + ", " + ret[3]);
		return ret;
	}

	/**
	 * Enters or exits immersive fullscreen mode.
	 * When entering, both the status bar and the navigation bar are hidden,
	 * when exiting, they are shown back.
	 * In immersive mode, a swipe from the screen edge transiently shows the
	 * bars, they hide themselves again shortly after.
	 * On API 30+ the modern WindowInsetsController API is used, on earlier
	 * versions the deprecated but still functional setSystemUiVisibility()
	 * immersive flags (available since API 19, minSdk is 21).
	 */
	public void setFullscreen(boolean fullscreen){
		if(fullscreen){
			if(Build.VERSION.SDK_INT >= 30){
				WindowInsetsController c = this.getWindow().getInsetsController();
				if(c != null){
					c.hide(WindowInsets.Type.statusBars() | WindowInsets.Type.navigationBars());
					c.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
					return;
				}
			}
			int flags = View.SYSTEM_UI_FLAG_LAYOUT_STABLE
				| View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
				| View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
				| View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
				| View.SYSTEM_UI_FLAG_FULLSCREEN
				| View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
			this.getWindow().getDecorView().setSystemUiVisibility(flags);
		}else{
			if(Build.VERSION.SDK_INT >= 30){
				WindowInsetsController c = this.getWindow().getInsetsController();
				if(c != null){
					c.show(WindowInsets.Type.statusBars() | WindowInsets.Type.navigationBars());
					return;
				}
			}
			this.getWindow().getDecorView().setSystemUiVisibility(0);
		}
	}

	private static native void handleCharacterStringInput(String str);

	// Native method invoked by reportInsets() to pass the content insets (in pixels,
	// in ruis::sides order {left, top, right, bottom}) to the native side. The insets
	// are the per-side max of the system bar insets and the on-screen keyboard (IME)
	// insets, combined on this (Java) side because both come from a single WindowInsets
	// object, which keeps the native side simple and makes it re-layout the content
	// once per insets change instead of twice.
	private static native void onInsetsChanged(int left, int top, int right, int bottom);

	/**
	 * Computes the content insets (in pixels) from a WindowInsets object, in
	 * ruis::sides order {left, top, right, bottom}: the per-side max of the system bars
	 * (status bar and navigation bar) insets and the on-screen keyboard (input method)
	 * insets, i.e. the region beyond which the content is not visible.
	 * On API 30+ both inset types are read from the WindowInsets object; on earlier
	 * versions the deprecated system-window insets are used for the bars (they are the
	 * bars only, as the IME is not reported there in the non edge-to-edge mode) and the
	 * keyboard is detected via the visible display frame.
	 */
	private int[] computeInsets(WindowInsets insets){
		int[] bars;
		int[] ime;
		if(Build.VERSION.SDK_INT >= 30){
			Insets b = insets.getInsets(WindowInsets.Type.statusBars() | WindowInsets.Type.navigationBars());
			bars = new int[] {b.left, b.top, b.right, b.bottom};
			Insets i = insets.getInsets(WindowInsets.Type.ime());
			ime = new int[] {i.left, i.top, i.right, i.bottom};
		}else{
			bars = new int[] {
				insets.getSystemWindowInsetLeft(),
				insets.getSystemWindowInsetTop(),
				insets.getSystemWindowInsetRight(),
				insets.getSystemWindowInsetBottom()
			};
			// Pre-API 30: the WindowInsets object does not expose the IME inset, so
			// detect the soft keyboard via the visible display frame (the region of the
			// decor view not covered by the system UI / the keyboard).
			View decorView = this.getWindow().getDecorView();
			Rect visible = new Rect();
			decorView.getWindowVisibleDisplayFrame(visible);
			int keyboardBottom = Math.max(0, decorView.getHeight() - visible.bottom);
			ime = new int[] {0, 0, 0, keyboardBottom};
		}
		return new int[] {
			Math.max(bars[0], ime[0]),
			Math.max(bars[1], ime[1]),
			Math.max(bars[2], ime[2]),
			Math.max(bars[3], ime[3])
		};
	}

	/**
	 * Computes the content insets from a WindowInsets object and reports them to the
	 * native side via a single onInsetsChanged() call.
	 * Reported from the WindowInsets listener because in edge-to-edge mode hiding or
	 * showing the bars (e.g. when entering or leaving immersive fullscreen) does not
	 * change the window content rectangle, and showing the keyboard does not resize
	 * the window either, so the ANativeActivity content-rect callback reflects neither.
	 * Moreover, insets are dispatched asynchronously, so querying getRootWindowInsets()
	 * synchronously right after hiding the bars would report the stale (pre-hide) values.
	 */
	private void reportInsets(WindowInsets insets){
		int[] in = this.computeInsets(insets);
		Log.d(LOGTAG, "reportInsets(): " + in[0] + ", " + in[1] + ", " + in[2] + ", " + in[3]);
		onInsetsChanged(in[0], in[1], in[2], in[3]);
	}

	@Override
	public boolean dispatchKeyEvent(KeyEvent event) {
		Log.d(LOGTAG, "dispatchKeyEvent(): invoked");
		if(event.getAction() == KeyEvent.ACTION_MULTIPLE && event.getKeyCode() == KeyEvent.KEYCODE_UNKNOWN){
			Log.d(LOGTAG, "dispatchKeyEvent(): multiple characters are being delivered, chars = " + event.getCharacters());
			//Multiple characters are being delivered. NativeActivity currently does not
			//support passing multiple characters from KeyEvent to native code, so we need to do it here.
			this.handleCharacterStringInput(event.getCharacters());
			return true;
		}
		return super.dispatchKeyEvent(event);
	}

	// Override the Back button handler to prevent activity from automatically closing by Back key.
	@Override
	public void onBackPressed() {
	}

	private InputMethodManager imm;

	@Override
	protected void onCreate(Bundle savedInstanceState){
		super.onCreate(savedInstanceState);

		this.imm = (InputMethodManager)this.getSystemService(Service.INPUT_METHOD_SERVICE);

		// Report the window insets to the native side. In edge-to-edge mode
		// (targetSdk 35+) the window is neither resized when the system bars are
		// hidden/shown (e.g. on entering/leaving immersive fullscreen) nor when the
		// on-screen keyboard (IME) is shown/hidden, so the ANativeActivity content-rect
		// callback reflects neither; the WindowInsets listener is the reliable way to
		// observe both the system bar insets and the IME insets.
		View decorView = this.getWindow().getDecorView();
		decorView.setOnApplyWindowInsetsListener(new View.OnApplyWindowInsetsListener(){
			@Override
			public WindowInsets onApplyWindowInsets(View view, WindowInsets insets){
				// A single WindowInsets object describes all the window inset types at
				// once - the system bars, the IME, the display cutout, etc. - so both the
				// system bar and the IME insets are extracted from it and reported to the
				// native side in a single call.
				RuisappActivity.this.reportInsets(insets);
				// The returned value is what gets applied to the view and dispatched further
				// down the view hierarchy. We only observe the insets without consuming or
				// modifying them, so we return the very same object to keep the default
				// behaviour intact for the rest of the hierarchy (returning null or a
				// consumed/stripped object would stop the insets from reaching the children).
				return insets;
			}
		});
	}

	public void showVirtualKeyboard(){
		this.imm.showSoftInput(this.getWindow().getDecorView(), InputMethodManager.SHOW_FORCED);
		Rect r = new Rect();
		this.getWindow().getDecorView().getWindowVisibleDisplayFrame(r);
		Log.d(LOGTAG, "showVirtualKeyboard(): visible rect = " + r.left + ", " + r.right + ", " + r.top + ", " + r.bottom);
	}

	public void hideVirtualKeyboard(){
		this.imm.hideSoftInputFromWindow(this.getWindow().getDecorView().getWindowToken(), InputMethodManager.HIDE_IMPLICIT_ONLY);
	}

	public String[] listDirContents(String path){
		try {
			String[] list = getAssets().list(path);

			for(int i = 0; i != list.length; ++i){
				if(getAssets().list(path + "/" + list[i]).length > 0){
					//this is a directory
					list[i] += "/";
				}
			}

//			Log.d(LOGTAG, "file list = " + list);
			return list;
		}catch (IOException e){
			Log.d(LOGTAG, e.getMessage());
		}
		return null;
	}

	public String getStorageDir(){
		return this.getFilesDir().getPath();
	}
}
