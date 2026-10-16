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
	 * Returns the system bar insets in pixels, in ruis::sides order
	 * {left, top, right, bottom}.
	 * In the edge-to-edge case (targetSdk 35+ on Android 15+) these are the
	 * system bar sizes; otherwise they are 0 (the window already excludes the
	 * bars). The deprecated getSystemWindowInset*() API is used because it is
	 * valid across the whole minSdk 24.. range, unlike the
	 * WindowInsets.Type-based replacement which requires API 30+.
	 */
	public int[] getSystemBarInsets(){
		android.view.WindowInsets in = this.getWindow().getDecorView().getRootWindowInsets();
		if(in == null){
			return new int[] {0, 0, 0, 0};
		}
		int[] ret = new int[] {
			in.getSystemWindowInsetLeft(),
			in.getSystemWindowInsetTop(),
			in.getSystemWindowInsetRight(),
			in.getSystemWindowInsetBottom()
		};
		Log.d(LOGTAG, "getSystemBarInsets(): " + ret[0] + ", " + ret[1] + ", " + ret[2] + ", " + ret[3]);
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

	// Native method invoked by reportKeyboardInsets() to pass the on-screen
	// keyboard (input method) insets (in pixels, in ruis::sides order {left, top,
	// right, bottom}) to the native side, where they are merged into the content
	// insets (taking the max with the system-bar insets on each side).
	private static native void onKeyboardInsetsChanged(int left, int top, int right, int bottom);

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

		// Report the on-screen keyboard (input method) insets to the native side. In
		// edge-to-edge mode (targetSdk 35+) the window is not resized when the IME is
		// shown, so the ANativeActivity content-rect callback does not reflect it; the
		// WindowInsets listener is the reliable way to observe IME changes.
		View decorView = this.getWindow().getDecorView();
		decorView.setOnApplyWindowInsetsListener(new View.OnApplyWindowInsetsListener(){
			@Override
			public WindowInsets onApplyWindowInsets(View view, WindowInsets insets){
				RuisappActivity.this.reportKeyboardInsets(insets);
				return insets;
			}
		});
	}

	/**
	 * Computes the on-screen keyboard (input method) insets (in pixels) and reports
	 * them to the native side, in ruis::sides order {left, top, right, bottom}.
	 * On API 30+ the IME inset is read from the WindowInsets object; on earlier
	 * versions the soft keyboard is detected via the visible display frame.
	 */
	private void reportKeyboardInsets(WindowInsets insets){
		int left = 0, top = 0, right = 0, bottom = 0;
		if(Build.VERSION.SDK_INT >= 30){
			Insets ime = insets.getInsets(WindowInsets.Type.ime());
			left = ime.left;
			top = ime.top;
			right = ime.right;
			bottom = ime.bottom;
		}else{
			// Pre-API 30: the WindowInsets object does not expose the IME inset, so
			// detect the soft keyboard via the visible display frame (the region of the
			// decor view not covered by the system UI / the keyboard).
			View decorView = this.getWindow().getDecorView();
			Rect visible = new Rect();
			decorView.getWindowVisibleDisplayFrame(visible);
			bottom = Math.max(0, decorView.getHeight() - visible.bottom);
		}
		Log.d(LOGTAG, "reportKeyboardInsets(): " + left + ", " + top + ", " + right + ", " + bottom);
		onKeyboardInsetsChanged(left, top, right, bottom);
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
