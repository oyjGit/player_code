/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.example.liverecorddemo.ui.activity;

import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.graphics.Color;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.util.Log;
import android.view.KeyEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.view.SurfaceHolder.Callback;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

//import com.android.getH264FormMediaRecorder.VideoCameraActivity;
//import com.android.getH264FormMediaRecorder.VideoCameraActivity;
import com.example.liverecorddemo.R;
import com.example.liverecorddemo.config.Const;
import com.example.liverecorddemo.framework.BaseCallBack;
import com.example.liverecorddemo.framework.MgmtClassFactory;
import com.example.liverecorddemo.mgmt.VideoRecordInfoMgmt;
import com.example.liverecorddemo.ui.widget.GL2JNIView;
import com.example.liverecorddemo.ui.widget.SurFaceViewFactory;
import com.example.liverecorddemo.util.CLog;
import com.example.liverecorddemo.util.DateTimeUtil;
import com.yang.sdk.api.MediaPlayerAPI;
import com.yang.sdk.api.PlatformAPI;
import com.yang.sdk.api.PlatformAPI.AcceptMessageListener;

import android.view.SurfaceHolder.Callback;


@SuppressLint({ "InlinedApi", "SdCardPath" })
public class GL2JNIActivity extends Activity implements Callback, Runnable {
	private static final String TAG = "GL2JNIActivity";

	private static final int MSG_UPDATEPROGRRESS = 1;
	private static final int UPDATEPROGRRESS_INTERVAL = 1000;

	private static final int SEEKDATA_TYPE_MANUAL = 1;
	private static final int SEEKDATA_TYPE_AUTO = 2;
	GL2JNIView mSurfaceView;
	SurFaceViewFactory mSurFaceViewFactory;
	VideoRecordInfoMgmt mVideoRecordInfoMgmt;
	EditText mTitleTextView;
	SeekBar mSeekBar;
	
	PlatformAPI objPlatformAPI;
	
	VideoCaptureAndroid m_VideoCaptureAndroid;
	
	static int model = 1;
	
//	int mCid = 1;
	String mCid;

	String url;
	private String initTitle;
	
	int audiocapture = 0;
	int mDeviceType = 1;

	private String mIP;
	private long mFirstFrom;
	private long mFrom;
	private long mTo;
	private int mSeekType = SEEKDATA_TYPE_AUTO;
	private Long mTempTo;
	private Boolean mIsFirstLoadRecord = true;
	private Boolean mIsSeeking = false;

	private WakeLock mWakeLock;
	private PowerManager pManager;
	
	
	static {
		Boolean flag = SurFaceViewFactory.loadLibs(true);
		Log.v(TAG, flag + "");
	}
	int type;
	Handler mHandler = new Handler() {
		@Override
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case MSG_UPDATEPROGRRESS:
				int pos = MediaPlayerAPI.GetPlayPos() / 1000;
				CLog.v("MSG_UPDATEPROGRRESS-" + pos + "");
				setSeekbarPos(pos);
				break;

			default:
				break;
			}
		};
	};
	
	

	Message msg = new Message();

	private void setSeekbarPos(int pos) {
		CLog.v("setSeekbarPos" + pos);
		mSeekBar.setProgress(pos);
		msg.what = MSG_UPDATEPROGRRESS;
		mHandler.sendMessageDelayed(Message.obtain(msg),
				UPDATEPROGRRESS_INTERVAL);
	}

	private void setSeekbarPosDelay() {
		mHandler.removeMessages(MSG_UPDATEPROGRRESS);
		mHandler.sendMessageDelayed(Message.obtain(msg),
				UPDATEPROGRRESS_INTERVAL * 2);
	}

	@Override
	protected void onCreate(Bundle icicle) {
		super.onCreate(icicle);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
				WindowManager.LayoutParams.FLAG_FULLSCREEN);
		
		
		
		
		mSurFaceViewFactory = new SurFaceViewFactory();
		mSurfaceView = (GL2JNIView) mSurFaceViewFactory.GetSurfaceView(this);
		
		type = getIntent().getIntExtra("type", 0);
//		mCid = getIntent().getIntExtra("cid", 1);
		mCid = getIntent().getStringExtra("cid");

		mIP = getIntent().getStringExtra("ip");
//		if (type == MainActivity.TYPE_MOBILE_LIVE) {
//			setContentView(R.layout.live);
//			String ip = getIntent().getStringExtra("ip");
//			url = "rtmp://" + ip + ":1935/demo/" + mCid;
//			InitSurfaceView();
//	        InitMediaSharePreference();	        
//			return;
//		}
		
		setContentView(R.layout.activity_main);
		FrameLayout mRelativeLayout = (FrameLayout) findViewById(R.id.rootview);
		mRelativeLayout.addView(mSurfaceView, 0, new ViewGroup.LayoutParams(
				ViewGroup.LayoutParams.MATCH_PARENT,
				ViewGroup.LayoutParams.MATCH_PARENT));

		mTitleTextView = (EditText) findViewById(R.id.tvtitle);
		mSeekBar = (SeekBar) findViewById(R.id.seekbarview);
		
		
		if (type == MainActivity.TYPE_MOBILE_LIVE) {
//			setContentView(R.layout.live);
			String ip = getIntent().getStringExtra("ip");
//			url = "rtmp://" + ip + ":1935/demo/" + mCid;
//			InitSurfaceView();
//	        InitMediaSharePreference();
			
			
			 MediaPlayerAPI objMediaPlayerAPI = new MediaPlayerAPI();	
				objMediaPlayerAPI.PlayStatusNativeCallback(1);
			
//			 PlatformAPI.ConnectPublicLiveEx("0DAE6C002F0F714E", "rtmp://54.183.239.230:1935/live/1003190", 100);

			
//			
			try {
				Thread.sleep(4000);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			
//			nRet = PlatformAPI.StartConnectToClient("53574C00F8D154DEF89DC3CB5000");
//			String connect = PlatformAPI.GetConnectionString();
//			
//			PlatformAPI.StartConnectToServer(2);
//			
//			mTitleTextView.setText(connect);
//			mTitleTextView.setTextColor(Color.GREEN);
//			
//			PlatformAPI.InitStreamEncoder(640, 480, 300, 0);
//			
//			m_VideoCaptureAndroid = new VideoCaptureAndroid(1,0);
//			m_VideoCaptureAndroid.startCapture(640, 480, 10000, 15000);

			
//			mDeviceType = 1;
//			audiocapture = 1;
//			new Thread (this).start();
			
//			return;
		}

		
		
		
//		
		
		if (type == MainActivity.TYPE_LIVE) {
			mSeekBar.setVisibility(View.GONE);
			String ip = getIntent().getStringExtra("ip");
			url = "rtmp://" + ip + ":1935/demo/" + mCid;
			url = ip;
//			setContentView(R.layout.activity_main);

		}
		else if(type == MainActivity.TYPE_SMOOTH_LIVE)
		{
			mSeekBar.setVisibility(View.GONE);
			String ip = getIntent().getStringExtra("ip");
			url = "rtmp://" + ip + ":1935/demo/" + mCid;
//			MediaPlayerAPI.SetSmoothPlay();
		}
		else if(type == MainActivity.TYPE_RECORD) {
			// 播放录像
			mSeekBar.setVisibility(View.GONE);
			try {
				initRecordData();
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		
//		MediaPlayerAPI.SetNativeCallback();

	}

	SharedPreferences mediaPreference;
	    private final String mediaShare = "media";
	    
	    private void InitMediaSharePreference()
	    {
	    	mediaPreference = this.getSharedPreferences(mediaShare, Context.MODE_PRIVATE);
	    }
	    LocalServerSocket lss;
	    LocalSocket sender;

		LocalSocket receiver;
	    
	    LocalServerSocket audiolss;
	    LocalSocket audioreceiver, audiosender;
	    
	    private void InitLocalSocket()
	    {
	    	try 
	    	{
				lss = new LocalServerSocket ("h264");
				receiver = new LocalSocket ();
				
				receiver.connect(new LocalSocketAddress("h264"));
				receiver.setReceiveBufferSize(500000);
				receiver.setSendBufferSize(500000);
				
				sender = lss.accept();
				sender.setReceiveBufferSize(500000);
				sender.setSendBufferSize(500000);
			} 
	    	catch (IOException e) 
			{
				e.printStackTrace();
				this.finish();
				return;
			}
	    }
	    
	    private void InitAudioLocalSocket()
	    {
	    	try 
	    	{
	    		audiolss = new LocalServerSocket ("aac");
				audioreceiver = new LocalSocket ();
				
				audioreceiver.connect(new LocalSocketAddress("aac"));
				audioreceiver.setReceiveBufferSize(500000);
				audioreceiver.setSendBufferSize(500000);
				
				audiosender = audiolss.accept();
				audiosender.setReceiveBufferSize(500000);
				audiosender.setSendBufferSize(500000);
			} 
	    	catch (IOException e) 
			{
				e.printStackTrace();
				this.finish();
				return;
			}
	    }
	    
	    private SurfaceView mliveSurfaceView;
	    private void InitSurfaceView()
	    {
	    	mliveSurfaceView = (SurfaceView)this.findViewById(R.id.surface_camera);
	    	SurfaceHolder holder = mliveSurfaceView.getHolder();
	    	holder.addCallback(this);
	    	holder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
	    }
	    
	    private SurfaceHolder mSurfaceHolder;
	    private boolean mMediaRecorderRecording = false;
	    private boolean mMediaAudioRecorderRecording = false;
	    
		public void surfaceChanged(SurfaceHolder holder, int format, int width,	int height) 
		{
			mSurfaceHolder  = holder;
			if(!mMediaRecorderRecording)
			{
				InitLocalSocket();
				getSPSAndPPS();
				initializeVideo();
				startVideoRecording();
			}
		}
		
		//get sps and pps if have record
		private void getSPSAndPPS()
		{
			StartMdatPlace = mediaPreference.getInt(String.format("mdata_%d%d.mdat", videoWidth,videoHeight),-1);
			if(StartMdatPlace!=-1)
			{
				byte[] temp = new byte[100];
				try 
				{
					FileInputStream file_in = GL2JNIActivity.this.openFileInput(String.format("%d%d.sps", videoWidth,videoHeight));
					int index = 0;
					int read=0;
					while(true)
					{
						read = file_in.read(temp,index,10);
						if(read==-1) break;
						else index += read;
					}
					Log.e("", "sps length:"+index);
					SPS = new byte[index];
					System.arraycopy(temp, 0, SPS, 0, index);
					file_in.close();
					
					index =0;
					//read PPS
					file_in = GL2JNIActivity.this.openFileInput(
							String.format("%d%d.pps", videoWidth,videoHeight));
					while(true)
					{
						read = file_in.read(temp,index,10);
						if(read==-1) break;
						else index+=read;
					}
					Log.e("", "pps length:"+index);
					PPS = new byte[index];
					System.arraycopy(temp, 0, PPS, 0, index);
					
				} 
				catch (FileNotFoundException e) 
				{
					e.printStackTrace();
				} 
				catch (IOException e) 
				{
					e.printStackTrace();
				}
			}
			else
			{
				SPS = null;
				PPS = null;
			}
		}
		
		
		public void surfaceCreated(SurfaceHolder holder) 
		{
			mSurfaceHolder = holder;
		}
		
		public void surfaceDestroyed(SurfaceHolder holder) 
		{
			
		}
		
		private MediaRecorder mMediaRecorder = null;
		private MediaRecorder mAudioMediaRecorder = null;
		private int videoWidth = 640;
		private int videoHeight = 480;
		private int videoRate = 15;
//		private boolean bStart = true;
		private String fd = "data/data/com.example.liverecorddemo/h264.3gp";
		
		private boolean initializeVideo()
		{
			if(mSurfaceHolder==null) return false;
			
//			TODO:get supported width and height
			
//			Camera camera = Camera.open();
//			Camera.Parameters parameters = camera.getParameters();
//			List<Size> sizes = parameters.getSupportedPreviewSizes();
//			Iterator<Size> iterator = sizes.iterator();
//			while(iterator.hasNext()){
//			Size optimalSize = iterator.next();
//			String s = String.format("w:%d. h:%d", optimalSize.width, optimalSize.height);
//			Log.e("preview", s);
//			}
//			camera.release();
			
			mMediaRecorderRecording = true;
			if(mMediaRecorder == null)
				mMediaRecorder = new MediaRecorder ();
			else
				mMediaRecorder.reset();			
			
//			mMediaRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);
			mMediaRecorder.setVideoSource(MediaRecorder.VideoSource.CAMERA);
			mMediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.THREE_GPP);
			mMediaRecorder.setVideoFrameRate(videoRate);
			mMediaRecorder.setVideoSize(videoWidth, videoHeight);
//			mMediaRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.AMR_NB);  
			mMediaRecorder.setVideoEncoder(MediaRecorder.VideoEncoder.H264);
			mMediaRecorder.setPreviewDisplay(mSurfaceHolder.getSurface());
			mMediaRecorder.setMaxDuration(0);
			mMediaRecorder.setMaxFileSize(0);
			if(SPS==null)
			{
				mMediaRecorder.setOutputFile(fd);
			}
			else
			{
				mMediaRecorder.setOutputFile(sender.getFileDescriptor());
			}
			try 
			{
				mMediaRecorder.prepare();
				mMediaRecorder.start();
			} 
			catch (IllegalStateException e) 
			{
				e.printStackTrace();
			} 
			catch (IOException e) 
			{
				e.printStackTrace();
				releaseMediaRecorder();
			}
			
			
			return true;
		}
		
		private boolean initializeAudio()
		{
			if(mSurfaceHolder==null) return false;
				
			mMediaAudioRecorderRecording = true;
			if(mAudioMediaRecorder == null)
				mAudioMediaRecorder = new MediaRecorder ();
			else
				mAudioMediaRecorder.reset();
		
			
			mAudioMediaRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);
			mAudioMediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.AAC_ADTS);
			mAudioMediaRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.AAC);
			mAudioMediaRecorder.setAudioSamplingRate(11025);
			mAudioMediaRecorder.setAudioChannels(1);
			mAudioMediaRecorder.setAudioEncodingBitRate(16000);
			mAudioMediaRecorder.setOutputFile(audiosender.getFileDescriptor());
//			mAudioMediaRecorder.setOutputFile("/sdcard/aac.av");
			try 
			{
				mAudioMediaRecorder.prepare();
				mAudioMediaRecorder.start();
			} 
			catch (IllegalStateException e) 
			{
				e.printStackTrace();
			} 
			catch (IOException e) 
			{
				e.printStackTrace();
				releaseAudioMediaRecorder();
			}
			
			
			return true;
		}
		
		
		private void startVideoRecording()
		{
			new Thread (this).start();
			new Thread (registerThread).start();
			new Thread (registerAudioThread).start();
		}

		private OutputStream mOutput = null;
		private InputStream mInput = null;
		private boolean GetLoginResponse = false;
		
		private Runnable registerThread = new Runnable()
		{
			public void run() 
			{
			}
		};
		
		private Runnable registerAudioThread = new Runnable()
		{
			public void run() 
			{
				try 
				{
					
					while(!videocapture)
					{
						Thread.sleep(1000);
					}
					
					DataInputStream dataInput = new DataInputStream (audioreceiver.getInputStream());
					
					//skip ftyp box and mdat box(decisbe by phone)
//					ReadaacSize(-1,dataInput);
					
//					GL2JNILib.openLiveRecordCapture(url);
				
					final byte[] frame = new byte[]{(byte) 0x11,(byte) 0x90};
		
					while(mMediaAudioRecorderRecording&&mMediaRecorderRecording)
					{
						PlatformAPI.SendAudioBuffer(frame, 2, 0);
//						MediaPlayerAPI.SendAACInfoBuffer(frame, 2);
				
						ReadaacSize(7,dataInput);
						
						byte[] aactotal = new byte[300];
						System.arraycopy(aacframe, 0, aactotal, 0, 7);
						
						byte[] aacinfo = new byte[7];	
				
						int profile = (((int)aacframe[2] & 0xFF )>>6) + 0x01;
						int sample = (((int)aacframe[2]& 0xFF)>>2) & 0x0F;
						int channel = (((int)aacframe[2] & 0x01)<<2) + (((int)aacframe[3]& 0xFF)>>6);
						int datalen =((((((int)aacframe[3] & 0x03)<<3) + ((int)aacframe[4]& 0xFF)>>5))<<8) + (((int)aacframe[4]&0x1F)<<3) + (((int)aacframe[5]& 0xFF)>>5);
						
						Log.e("", "datalen:"+datalen);
						
						byte[] aac = new byte[datalen-7];
						ReadaacSize(datalen-7,dataInput);
						System.arraycopy(aacframe, 0, aactotal, 7, datalen-7);
//						System.arraycopy(aacframe, 0, aac, 0, datalen-7);
//						MediaPlayerAPI.SendAACBuffer(aac, datalen-7);
						
						PlatformAPI.SendAudioBuffer(aactotal, datalen, 0);
//						MediaPlayerAPI.SendAACBuffer(aactotal, datalen);
												
						
					}
				} 
				catch (IOException e) 
				{
					e.printStackTrace();
				} 
				catch (InterruptedException e) 
				{
					e.printStackTrace();
				}
			}
		};
		
		private void releaseMediaRecorder()
		{
			if(mMediaRecorder != null)
			{
				if(mMediaRecorderRecording)
				{
					mMediaRecorder.stop();
					mMediaRecorderRecording = false;
				}
				mMediaRecorder.reset();
				mMediaRecorder.release();
				mMediaRecorder = null;
			}
		}
		
		private void releaseAudioMediaRecorder()
		{
			if(mAudioMediaRecorder != null)
			{
				if(mMediaAudioRecorderRecording)
				{
					mAudioMediaRecorder.stop();
					mMediaAudioRecorderRecording = false;
				}
				mAudioMediaRecorder.reset();
				mAudioMediaRecorder.release();
				mAudioMediaRecorder = null;
			}
		}

		private final int MAXFRAMEBUFFER = 20480*4;//20K
		private byte[] h264frame = new byte[MAXFRAMEBUFFER];
		private byte[] aacframe = new byte[MAXFRAMEBUFFER];
		private final byte[] head = new byte[]{0x00,0x00,0x00,0x01};
		private boolean videocapture = false;
		
//		public void run() 
//		{
//			try 
//			{
//				if(SPS == null)
//				{
//					Log.e("", "Rlease MediaRecorder and get SPS and PPS");
//					Thread.sleep(1000);
//					releaseMediaRecorder();
//					//first we must get sps and pps
//					findSpsPps();
//					initializeVideo();
//					
//					InitAudioLocalSocket();
//					initializeAudio();
//					
//					
//				}
//				
//				DataInputStream dataInput = new DataInputStream (receiver.getInputStream());
//				
//				//skip ftyp box and mdat box(decisbe by phone)
//				ReadSize(StartMdatPlace,dataInput);
//				
//				FileOutputStream file_test = new FileOutputStream ("/sdcard/sanxing_i9000.264");
//				file_test.write(head);
//				file_test.write(SPS);//write sps
//				
//				file_test.write(head);
//				file_test.write(PPS);//write pps
//				
//				int h264length =0;
//				
//				MediaPlayerAPI.OpenLiveRecordCapture(url);
//				
//				videocapture = true;
//				/*
//				sendH264(SPS);//send SPS
//				sendH264(PPS);//send PPS
//				*/
//				
//				final byte[] frame = new byte[]{0x65,0x41};
//				
//				while(mMediaRecorderRecording)
//				{
//					h264length = dataInput.readInt();
//					if(h264length>65535 ||h264length<0) continue;
//					ReadSize(h264length,dataInput);
//					Log.e("", "h264length:"+h264length);
////					
////					//send selice to vlc 
//					byte[] h264 = new byte[h264length];
//					System.arraycopy(h264frame, 0, h264, 0, h264length);
//
////					//sendH264(h264);*/
//					
//					if(h264length <=0 )
//						continue;
//
//					if(h264[0] == frame[0])
//					{
//						MediaPlayerAPI.SendLiveRecordBuffer(SPS, SPS.length);
//						MediaPlayerAPI.SendLiveRecordBuffer(PPS, PPS.length);
//						MediaPlayerAPI.SendLiveRecordBuffer(h264, h264.length);
////						Thread.sleep(40);
//					}
//					else if(h264[0] == frame[1])
//					{
//						MediaPlayerAPI.SendLiveRecordBuffer(h264, h264.length);
//						Thread.sleep(60);
//					}
//				
//				}
//				file_test.close();
//			} 
//			catch (IOException e) 
//			{
//				e.printStackTrace();
//			} 
//			catch (InterruptedException e) 
//			{
//				e.printStackTrace();
//			}
//		}
		
		private void ReadaacSize(int h264length,DataInputStream dataInput) throws IOException, InterruptedException
		{
			int read = 0;
			int temp = 0;
			while(read<h264length)
			{
				temp= dataInput.read(aacframe, read, h264length-read);
				if(temp==-1)
				{
					Log.e("", "no data get wait for data coming.....");
					Thread.sleep(2000);
					continue;
				}
				read += temp;
			}
		}
		
		private void ReadSize(int h264length,DataInputStream dataInput) throws IOException, InterruptedException
		{
			int read = 0;
			int temp = 0;
			while(read<h264length)
			{
				temp= dataInput.read(h264frame, read, h264length-read);
				if(temp==-1)
				{
					Log.e("", "no data get wait for data coming.....");
					Thread.sleep(2000);
					continue;
				}
				read += temp;
			}
		}

		private byte[] SPS;
		private byte[] PPS;
		private int StartMdatPlace = 0;
		
		private void findSpsPps() throws IOException{
			File file = new File (fd);
			FileInputStream fileInput = new FileInputStream (file);
			
			int length = (int)file.length();
			byte[] data = new byte[length];
			fileInput.read(data);
			
			final byte[] mdat = new byte[]{0x6D,0x64,0x61,0x74};
			final byte[] avcc = new byte[]{0x61,0x76,0x63,0x43};
			
			
			//search from head
			for(int i =0 ; i<100;i++)//find StartMdatPlace
			{
				if(data[i] == mdat[0]&&data[i+1] == mdat[1] && data[i+2] == mdat[2] && data[i+3]==mdat[3])
				{
					StartMdatPlace = i+4;//find 
					break;
				}
			}
			Log.e("", "StartMdatPlace:"+StartMdatPlace);
			String mdatStr = String.format("mdata_%d%d.mdat",videoWidth,videoHeight);
			
			
			Editor editor = mediaPreference.edit();
			editor.putInt(mdatStr,StartMdatPlace);
			editor.commit();
			
			//search from end
			for(int i = length-1;i>0;i--)
			{
				if(data[i] == avcc[3] && data[i-1] == avcc[2] && data[i-2] == avcc[1] && data[i-3] == avcc[0])
				{
					//find avcc box
					int sps_start = i+7;
					
					byte[] sps_3gp = new byte[2];
					sps_3gp[1] = data[sps_start];
					sps_3gp[0] = data[sps_start+1];
//					Log.e("", "0:"+sps_3gp[0]+"1:"+sps_3gp[1]);
					
					sps_start+=2;//skip length
					
					int sps_length = bytes2short(sps_3gp);
					Log.e("", "SPS LENGTH:"+sps_length);
					
					SPS=new byte[sps_length];
					System.arraycopy(data, sps_start, SPS, 0, sps_length);
					
					//Save SPS
					FileOutputStream file_out = GL2JNIActivity.this.openFileOutput(String.format("%d%d.sps",videoWidth,videoHeight), Context.MODE_PRIVATE);
					file_out.write(SPS);
					file_out.close();
					
					int pps_start = sps_start+sps_length+1;
					byte[] pps_3gp =new byte[2];
					pps_3gp[1] = data[pps_start];
					pps_3gp[0] =data[pps_start+1];
					int pps_length = bytes2short(pps_3gp);
					Log.e("", "PPS LENGTH:"+pps_length);
					
					pps_start+=2;
					
					PPS = new byte[pps_length];
					System.arraycopy(data, pps_start, PPS,0,pps_length);

					//Save PPS
					file_out = GL2JNIActivity.this.openFileOutput(	String.format("%d%d.pps",videoWidth,videoHeight), Context.MODE_PRIVATE);
					file_out.write(PPS);
					file_out.close();
					break;
				}
			}
		}
		
		public short bytes2short(byte[] b)
	    {
		    short mask=0xff;
		    short temp=0;
		    short res=0;
		    for(int i=0;i<2;i++)
		    {
		        res<<=8;
		        temp=(short)(b[1-i]&mask);
		        res|=temp;
		     }
		     return res;
	    }

//		protected void onPause() 
//		{
//			super.onPause();
//			if(mMediaRecorderRecording)
//			{
//				releaseMediaRecorder();
//				try 
//				{
//					lss.close();
//					receiver.close();
//					sender.close();
//				} 
//				catch (IOException e) 
//				{
//					e.printStackTrace();
//				}
//				mMediaRecorderRecording = false;
//			}
//			finish();
//		}
	
	
	
	
	
	private void initRecordData() throws InterruptedException {
		mVideoRecordInfoMgmt = (VideoRecordInfoMgmt) MgmtClassFactory
				.getInstance().getMgmtClass(VideoRecordInfoMgmt.class);
		// Toast.makeText(getActivity(), "正在获取录像信息",
		// Toast.LENGTH_LONG).show();
		Intent intent = getIntent();
		mFrom = intent.getLongExtra("From", 0);
		mFirstFrom = intent.getLongExtra("From", 0);
		mTo = intent.getLongExtra("To", 0);
		initTitle = DateTimeUtil.timeStampToDateLong(mFrom) + "~"
				+ DateTimeUtil.timeStampToDateLong(mTo) + "---current:";
//		mTitleTextView.setText(initTitle + "00:00:00");
//		mTitleTextView.setTextColor(Color.GREEN);
		mSeekBar.setBackgroundColor(Color.WHITE);
		mSeekBar.setOnSeekBarChangeListener(mOnSeekBarChangeListener);
		mSeekBar.setMax((int) (mTo - mFrom));
		CLog.v("Max" + (int) (mTo - mFrom));
		CLog.v("mFrom:" + mFrom + "-mTo:" + mTo);
		CLog.v("mTo - mFrom:" + ((mTo - mFrom)));
		setSeekbarPos(0);
		mSeekType = SEEKDATA_TYPE_AUTO;
//		requestRecordInfo();
		
		
		
		initRecordplay();
	}

	Runnable mPlayRunnable = new Runnable() {
		@Override
		public void run() {
			CLog.v("mPlayRunnable:requestRecordInfo");
			requestRecordInfo();
		}
	};
	OnSeekBarChangeListener mOnSeekBarChangeListener = new OnSeekBarChangeListener() {

		@Override
		public void onStopTrackingTouch(SeekBar seekBar) {
			mIsSeeking = true;
			mFrom = seekBar.getProgress();
			mSeekType = SEEKDATA_TYPE_MANUAL;
			CLog.v("onStopTrackingTouch-mFrom:" + mFrom);
			setSeekbarPosDelay();
//			requestRecordInfo();
			
			PlatformAPI.Seek(10);

		}

		@Override
		public void onStartTrackingTouch(SeekBar seekBar) {

		}

		@Override
		public void onProgressChanged(SeekBar seekBar, int progress,
				boolean fromUser) {

//			mTitleTextView.setText(initTitle
//					+ DateTimeUtil.timeStampToDateList(mFirstFrom + progress));

		}
	};
	BaseCallBack<String> mVideoRecordInfoMgmtCallBack = new BaseCallBack<String>() {

		@Override
		public void error(Object object) {

		}

		@Override
		public void success(final String info) {
			url = info;
			mFrom = mTempTo;

			CLog.v("success:" + mIsFirstLoadRecord + "-mIsSeeking:"
					+ mIsSeeking);
			if (mIsFirstLoadRecord) {
				try {
					initRecordplay();
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				mIsFirstLoadRecord = false;
				mHandler.postDelayed(
						mPlayRunnable,
						(Const.REQUEST_RECORD_RANAGE - Const.REQUEST_PRE_RECORD_RANAGE) * 1000);
			} else {
				recordplay();
				if (mIsSeeking) {
					mIsSeeking = false;
					mHandler.postDelayed(
							mPlayRunnable,
							(Const.REQUEST_RECORD_RANAGE - Const.REQUEST_PRE_RECORD_RANAGE) * 1000);
				} else {
					mHandler.postDelayed(mPlayRunnable,
							Const.REQUEST_RECORD_RANAGE * 1000);
				}

			}
		}
	};

	private void requestRecordInfo() {
		mHandler.removeCallbacks(mPlayRunnable);
		if (mFrom == mTo) {

			return;
		}
		mTempTo = mFrom + Const.REQUEST_RECORD_RANAGE;
		mTempTo = mTempTo < mTo ? mTempTo : mTo;
//		CLog.v("mCid:" + mCid + "-mFrom:" + mFrom + "-tempTo:" + mTempTo);
//		mVideoRecordInfoMgmt.getVideoRecordInfo(this,mIP, mCid, mFrom, mTempTo,
//				mVideoRecordInfoMgmtCallBack);

	}

	 public void run()
	 {
		
		 int audioSource = MediaRecorder.AudioSource.MIC;
		 int sampleRateInHz = 16000;
		 int channelConfig = AudioFormat.CHANNEL_IN_MONO;
		 int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
		 int bufSize = 2 * AudioRecord.getMinBufferSize(sampleRateInHz, channelConfig,
		 audioFormat);
		 bufSize = 2048;
		 AudioRecord mRecord = new AudioRecord(audioSource, sampleRateInHz,
		 channelConfig, audioFormat, bufSize);
		
		 bufSize = PlatformAPI.StartTalking(sampleRateInHz, 1, 16, 160);
		 if (bufSize == -1) {
			 return;
		 }
		 
		 mRecord.startRecording();
		 byte audiodata[] = new byte[bufSize];
		 
		 boolean isRecord = true;
		 int readsize = 0;
		 while (audiocapture == 1) 
		 {
			 readsize = mRecord.read(audiodata, 0, bufSize);
			 
//			 for(int i=0;i<bufSize;i++)   
//			 {
//				 audiodata[i]= (byte) (audiodata[i]*2);
//			 }
	
//			 CLog.v("mCid:"+readsize);
			 PlatformAPI.SendAudioBuffer(audiodata, readsize, 0);
//			 MediaPlayerAPI.SendAudioBuffer(audiodata,readsize,mDeviceType);
//			 if(audiodata[0] == 0)
//				 continue;
//			 count++;
		 }
//		 }
		 mRecord.stop();
		 audiodata = null;
		 
		 PlatformAPI.StopTalking();
		 
//		 while(true)
//		 {
////			 String status = PlatformAPI.GetStatus();
////			 
//			int 	nRet = PlatformAPI.GetDeviceStatus("72B6C1005F0FE44E");
////				
//			CLog.v("D37F8400D30FEF4E status:"+nRet);
////			
////			int nRet1 = PlatformAPI.GetDeviceStatus("8DC095001F0FAA4E");
////			
////			CLog.v(" width:"+ MediaPlayerAPI.getStreamMediaParam(14)+"height "+MediaPlayerAPI.getStreamMediaParam(15));
////				nRet = PlatformAPI.GetDeviceStatus("72127000770F3050");
////				nRet = PlatformAPI.GetDeviceStatus("BA33FE002C0F2850");
////				if(nRet == 1)
////					break;
//				try {
//					Thread.sleep(1000);
//				} catch (InterruptedException e) {
//					// TODO Auto-generated catch block
//					e.printStackTrace();
//				}
//		 }
//	
	 }
	 
	 public void personalplay() throws InterruptedException
	 {
		 
		 MediaPlayerAPI objMediaPlayerAPI = new MediaPlayerAPI();	
		 objMediaPlayerAPI.PlayStatusNativeCallback(1);		 
		 
		objPlatformAPI = new PlatformAPI();
		objPlatformAPI.MessageNativeCallback();
	
			float version = PlatformAPI.GetPlatformVersion();
			CLog.v(" platform version:"+version);			
					
			
//		AcceptMessageListener mAcceptMessageListener = new AcceptMessageListener(){
//
//			@Override
//			public void accept(String message) {
//				// TODO Auto-generated method stub
////				mtextView.getText().toString() = message;
////				mtextView.setText(message);
//			}};
//			
//			
//		objMediaPlayerAPI.setAcceptListener(mAcceptMessageListener);


//			String status = PlatformAPI.GetStatus();
			
//			PlatformAPI.StartConnectToServer(4);
////			String status = PlatformAPI.GetStatus();
////			
//			PlatformAPI.StartConnectToServer(2);
//			
//			
			
//			float version = PlatformAPI.GetPlatformVersion();
//			CLog.v(" platform version:"+version);
//			while(true)
//			{
////				String status = PlatformAPI.GetStatus();
//				nRet = PlatformAPI.GetDeviceStatus("861D69006C0F7F4F");
//				
//				CLog.v("status:"+nRet);
////				nRet = PlatformAPI.GetDeviceStatus("72127000770F3050");
////				nRet = PlatformAPI.GetDeviceStatus("BA33FE002C0F2850");
//				if(nRet == 1)
//					break;
//				Thread.sleep(1000);
//			}
			
			
//			nRet = PlatformAPI.ConnectPrivateLive("A2ADD900C10F634E");				
//			nRet = PlatformAPI.ConnectPrivateLive("861D69006C0F7F4F");
//			if (nRet == -1) {
//				CLog.e("ConnectPrivateLive failed");
//				return;
//			}
				
			PlatformAPI.StartConnectToClient("35574C00000000C86263E5568f0ccd127214f8b92ae402be2becc344F89DC3CB5000");
				
//			String connect = PlatformAPI.GetConnectionString();
			
			PlatformAPI.InitStreamEncoder(480, 640, 500);	
			m_VideoCaptureAndroid = new VideoCaptureAndroid(1,0);
			m_VideoCaptureAndroid.startCapture(640, 480, 10000, 15000);
//			
			mDeviceType = 2;	
			audiocapture = 1;
			new Thread (this).start();

	 }
	 
	 public void publicplay() throws InterruptedException
	 {
		 MediaPlayerAPI objMediaPlayerAPI = new MediaPlayerAPI();	
		objMediaPlayerAPI.PlayStatusNativeCallback(1);
	

		 PlatformAPI.UpdateStatus();
		 
//		
			 Thread.sleep(2);
//		 
//		 PlatformAPI.StartConnectToClient(mIP);
		 
		 
//			}
//			else if(model == 1)
//			{
//				model = 2;
//				PlatformAPI.ConnectPublicLive("D37F8400D30FEF4E", "rtmp://222.186.32.4:1935/demo/1003135");
////				MediaPlayerAPI.ConnectPublic("D37F8400D30FEF4E");
//			}
//			else
//			{
//				model = 2;
//				MediaPlayerAPI.ConnectPublic("B0D04E00B70FFB4F");
//			}
//			MediaPlayerAPI.ConnectPublicLive("8A86EA00FE0F144E","rtmp://120.24.247.43:1935/demo/1003142");
			
//			MediaPlayerAPI.ConnectPublicLive("Testxyxxxx","rtmp://120.24.88.92/demo/555555");

//			mDeviceType = 2;
//			audiocapture = 1;
//			new Thread (this).start();
		 
	 }
	 
	 public void liverecordplay() throws InterruptedException
	 {
		
		MediaPlayerAPI objMediaPlayerAPI = new MediaPlayerAPI();	
		objMediaPlayerAPI.PlayStatusNativeCallback(1);

//			Thread.sleep(5000);
//			
//			PlatformAPI.StartConnectToServer(2);
			
//			nRet = PlatformAPI.QueryRecordSource("","D37F8400D30FEF4E", 1438256700, 1438260300,1438256700);
	
	 }

	public void videoplay() throws InterruptedException {
		CLog.v(" type:" + type + " -url:" + url);
		if (url == null || url.equals(""))
			return;
		if (type == MainActivity.TYPE_LIVE || type == MainActivity.TYPE_SMOOTH_LIVE) {
			
			
//			publicplay();
			
			if(type == MainActivity.TYPE_LIVE)
				publicplay();
			else
				personalplay();
			
//			liverecordplay();
			
			
			
			
			
			
			
//			MediaPlayerAPI objMediaPlayerAPI = new MediaPlayerAPI();
//			
//			objMediaPlayerAPI.PlayStatusNativeCallback();
//			
////			objMediaPlayerAPI.StreamInfoNativeCallback(1);
//			
//			MediaPlayerAPI.ConnectPublicLive("F44179009D0FF84A"); 
//			
//			MediaPlayerAPI.OpenVideoPlayer();
//			// GL2JNILib.closeAudioPlayer();
//			MediaPlayerAPI.OpenAudioPlayer();
			

//			
//			Thread.sleep(5000);
//			
////			String SN = PlatformAPI.ConnectAP("Test_TPLink", "82773103", 0);
//			
////			String cid = PlatformAPI.CameraBound("Test1111"); 
//
//			String status = PlatformAPI.GetStatus();
//			
//			PlatformAPI.ConnectMediaSource("DD96F300610F444E");
			
//			int nRet =PlatformAPI.CameraUnBound("13487800CB0F824E");

			
//			int n = PlatformAPI.SetCameraConfig("8C79EF000F0FCA4E","{\n \"ScreenOrientation\" : 0,\n \"SpeakerSwitch\" : 0,\n \"SpeakerVolume\" : 0,\n \"StatusLights\" : 1,\n \"SetRecord\" : 0,\n \"SetPublic\" : 1\n }\n");

			
//			MediaPlayerAPI.ConnectPublicLive("D5808100220F314E");//Test91b0082c		
			

			

		}

	}

	public void initRecordplay() throws InterruptedException {
		
		MediaPlayerAPI objMediaPlayerAPI = new MediaPlayerAPI();	
		objMediaPlayerAPI.PlayStatusNativeCallback(1);
		
		
//		 PlatformAPI.ConnectPublicLiveEx("3FCD3D00F50F594F", "rtmp://54.183.239.230:1935/live/1003469", 100);

		
//		objPlatformAPI = new PlatformAPI();
//		objPlatformAPI.MessageNativeCallback();
		
	
//		PlatformAPI.ConnectPrivateLive("AAE5E30046988096");
//		PlatformAPI.StartConnectToClient("33574C00F3190000F89DC3CB5000");
//		PlatformAPI.StartConnectToServer(2);
//		String connect = PlatformAPI.GetConnectionString();
//		Log.v(TAG, connect + "");
		
//		if(model == 1)
//		{
//			model++;
//			PlatformAPI.InitStreamEncoder(480, 640, 500);
//			m_VideoCaptureAndroid = new VideoCaptureAndroid(1,0);
//			m_VideoCaptureAndroid.startCapture(640, 480, 10000, 15000);
//		}
//		else
//		{
//			PlatformAPI.InitStreamEncoder(480, 640, 300, 90);
//			m_VideoCaptureAndroid = new VideoCaptureAndroid(1,0);
//			m_VideoCaptureAndroid.startCapture(640, 480, 10000, 15000);
//		}
		
//		mDeviceType = 1;
//		audiocapture = 1;
//		new Thread (this).start();
		
//	String status = PlatformAPI.GetStatus();
//		nRet = PlatformAPI.QueryRecordTest("","B60CCC00960F974F", 1448982000,1448986000,0);
//		nRet = PlatformAPI.QueryRecordTest("","051CAF00440FD050", 1448982000,1448986000,0);
		nRet = PlatformAPI.QueryRecordTest("","D37F8400D30FEF4E", 1456243200,1456310660,1456307060);
//		 nRet = PlatformAPI.StartDownloadRecord("","B60CCC00960F974F", 1448982000,1448983000, "/sdcard/record.mp4");


	}

	public void recordplay() {
		CLog.v("mSeekType:" + mSeekType + "mFrom:" + mFrom + " -url:" + url);
		if (url == null || url.equals(""))
			return;
//		MediaPlayerAPI.LocateStream( mFrom);
//		mSeekType = SEEKDATA_TYPE_AUTO;
//		GL2JNILib.CloseAudioPlayer();
	}

	public void videoclose(View view) {
		MediaPlayerAPI.CloseVideoPlayer();
	}

	public void audioopen(View view) {
		MediaPlayerAPI.OpenAudioPlayer();
	}

	public void audioclose(View view) {
		MediaPlayerAPI.CloseAudioPlayer();
	}

	@Override
	protected void onResume() {
		super.onResume();
		mSurFaceViewFactory.onResume();
		try {
			videoplay();
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		pManager = ((PowerManager) getSystemService(POWER_SERVICE));
		mWakeLock = pManager.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK
				| PowerManager.ON_AFTER_RELEASE, TAG);
		mWakeLock.acquire();
	}

	@Override
	protected void onPause() {
		super.onPause();
		mSurFaceViewFactory.onPause();
		if (null != mWakeLock) {
			mWakeLock.release();
		}
	}

	@Override
	protected void onRestart() {
		super.onRestart();
		mSurFaceViewFactory.onRestart();
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {

//		if (event.getKeyCode() == KeyEvent.KEYCODE_BACK) {
//			Log.v(TAG, "back");
//			android.os.Process.killProcess(android.os.Process.myPid());
//			finish();
//			return true;
//		}
		
		if (event.getKeyCode() == KeyEvent.KEYCODE_BACK) {
//			if(type == MainActivity.TYPE_LIVE || type == MainActivity.TYPE_SMOOTH_LIVE)
//			{
//				audiocapture = 0;
				
				
//				MediaPlayerAPI.CloseVideoPlayer();
//				PlatformAPI.Disconnect();
//				PlatformAPI.StopCloudService();
				
				
//				MediaPlayerAPI.Snapshot("/sdcard/1.bmp");
				
				
				
//				PlatformAPI.StopDownloadRecord();

//				MediaPlayerAPI.Snapshot();
//				PlatformAPI.DisconnectMediaSource();
//				try {
//					Thread.sleep(5);
//				} catch (InterruptedException e) {
//					// TODO Auto-generated catch block
//					e.printStackTrace();
//				}
//				String status = PlatformAPI.GetStatus();
//				PlatformAPI.StopCloudService();
				
//				m_VideoCaptureAndroid.stopCapture();
//				MediaPlayerAPI.StopCameraCapture();
//			}
//			else
//				MediaPlayerAPI.CloseVideoPlayer();
			
//			return true;
		}
		
		if (event.getKeyCode() == KeyEvent.KEYCODE_VOLUME_UP) 
		{
			MediaPlayerAPI.OpenRecord("/sdcard/play.mp4");
//			PlatformAPI.StartDownloadRecord("", "D5808100220F314E", 1456142400, 1456142430, "/sdcard/play.mp4");
//			MediaPlayerAPI.CloseAudioPlayer();
			
//			PlatformAPI.Seek(2000);
			
			
//			mDeviceType = 2;
//			audiocapture = 1;
//			new Thread (this).start();
		}
		if (event.getKeyCode() == KeyEvent.KEYCODE_VOLUME_DOWN) 
		{
			MediaPlayerAPI.CloseRecord();
//			PlatformAPI.StopDownloadRecord();
//			MediaPlayerAPI.OpenAudioPlayer();
			
//			audiocapture = 0;
		}
		
//		PlatformAPI.StopDownloadRecord();
//		m_VideoCaptureAndroid.stopCapture();
		PlatformAPI.Disconnect();
//		PlatformAPI.UninitStreamEncoder();
		PlatformAPI.StopCloudService();
		
//		PlatformAPI.Disconnect();
		
//		m_VideoCaptureAndroid.stopCapture();
//		MediaPlayerAPI.StopCameraCapture();
		
//		MediaPlayerAPI.CloseVideoPlayer();
		
//		audiocapture = 0;
//		PlatformAPI.DisconnectMediaSource();
//		String status = PlatformAPI.GetStatus();
//		PlatformAPI.StopCloudService();
		
//		try {
//			Thread.sleep(2);
//		} catch (InterruptedException e) {
//			// TODO Auto-generated catch block
//			e.printStackTrace();
//		}
				
		return super.onKeyDown(keyCode, event);
	}
}
