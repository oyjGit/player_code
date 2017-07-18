package com.example.liverecorddemo.ui.widget;
import android.content.Context;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.yang.sdk.api.MediaPlayerAPI;



public class SurfaceJNIView extends SurfaceView implements SurfaceHolder.Callback {
	SurfaceHolder mHolder;

	public SurfaceJNIView(Context context) {
		super(context);

		mHolder = getHolder();
		mHolder.addCallback(this);
	}

	@Override
	public void surfaceChanged(SurfaceHolder arg0, int arg1, int arg2, int arg3) {
		// TODO Auto-generated method stub
		Log.e("SurfaceJNIView", "surfaceChanged");
	}

	@Override
	public void surfaceCreated(SurfaceHolder arg0) {
		// TODO Auto-generated method stub

		Log.e("SurfaceJNIView", "surfaceCreated");

		MediaPlayerAPI.InitSurface(getHolder().getSurface(), getWidth(), getHeight());

	}

	@Override
	public void surfaceDestroyed(SurfaceHolder arg0) {
		// TODO Auto-generated method stub
		Log.e("SurfaceJNIView", "surfaceDestroyed");

		MediaPlayerAPI.UninitSurface();

	}

	public void onResume() {
		// Log.e("SurfaceJNIView", "onResume");
	}

	public void onPause() {
		// Log.e("SurfaceJNIView", "onPause");
	}

	public void onRestart() {
		Log.e("SurfaceJNIView", "onRestart");

	}
}
