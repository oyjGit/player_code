package com.example.liverecorddemo.ui.widget;

import android.content.Context;
import android.util.Log;
import android.view.SurfaceView;

public class SurFaceViewFactory {
	SurfaceView mSurfaceView;

	static Boolean mUseSoftDecode;

	public SurfaceView GetSurfaceView(Context context) {
		if (mUseSoftDecode == true) {
			Log.v("SurFaceViewFactory", "use GL2JNIView");

			mSurfaceView = new GL2JNIView(context);
		} else {
			Log.v("SurFaceViewFactory", "use SurfaceJNIView");

			mSurfaceView = new SurfaceJNIView(context);
		}

		return mSurfaceView;
	}

	public void onResume() {
		try {
			if (mUseSoftDecode == true) {
				GL2JNIView mGlView = (GL2JNIView) mSurfaceView;
				if (mGlView != null) {

					mGlView.onResume();
				}
			} else {
				SurfaceJNIView mGlView = (SurfaceJNIView) mSurfaceView;
				if (mGlView != null) {
					mGlView.onResume();
				}
			}
		} catch (Exception e) {
			Log.e("SurFaceViewFactory", "onResume Exception");
		}

	}

	public void onPause() {
		try {
			if (mUseSoftDecode == true) {
				GL2JNIView mGlView = (GL2JNIView) mSurfaceView;
				if (mGlView != null) {
					mGlView.onPause();
				}
			} else {
				SurfaceJNIView mGlView = (SurfaceJNIView) mSurfaceView;
				if (mGlView != null) {
					mGlView.onPause();
				}
			}
		} catch (Exception e) {
			Log.e("SurFaceViewFactory", "onPause Exception");
		}
	}

	public void onRestart() {
		try {
			if (mUseSoftDecode != true) {
				SurfaceJNIView mGlView = (SurfaceJNIView) mSurfaceView;
				if (mGlView != null) {
					mGlView.onRestart();
				}
			}
		} catch (Exception e) {
			Log.e("SurFaceViewFactory", "onResume Exception");
		}

	}

	public static Boolean loadLibs(Boolean useSoftDecode) {
		Log.v("SurFaceViewFactory", "loadLibs ffmpeg ");
		if (!useSoftDecode) {
			String str = android.os.Build.VERSION.RELEASE;
			String a40 = "4.0";
			String a41 = "4.1";

			try {
				if (str.compareTo(a41) >= 0) {
					System.loadLibrary("hxbmcjplayer");
				} else if (str.compareTo(a40) >= 0) {
					System.loadLibrary("h40jplayer");
				} else {
					useSoftDecode = true;
				}
			} catch (Exception e) {
				Log.e("loadLibs", " Exception");
				useSoftDecode = true;
			} catch (Error e) {
				Log.e("loadLibs", " Error");
				useSoftDecode = true;
			}

		}

		if (useSoftDecode == true) {
			
			Log.v("SurFaceViewFactory", "loadLibs jplayer ");
		}

		mUseSoftDecode = useSoftDecode;

		return useSoftDecode;
	}
}
