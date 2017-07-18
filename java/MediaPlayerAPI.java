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

package com.yang.sdk.api;

import android.app.Activity;
import android.util.Log;
import android.view.Surface;

// Wrapper for native library

/**
 * @author ��
 *
 */
public class MediaPlayerAPI {	
	
		
	// 状态回调消息体 					//增加缓冲回调参数
	public void PlayStatusNativeCallback(int aMessageType,int aMessagePara1,int aMessagePara2)    
	 {  
		
		 Log.e("PlayStatusNativeCallback","message="+aMessageType+":messagePara="+aMessagePara1+":messagePara="+aMessagePara2);  
	 } 
	
	//设置播放器状态回调函数
	public  native void PlayStatusNativeCallback(int aOpenFlag);
	//获取流媒体相关参数                      		1.分辨率宽 2.分辨率高  3.下载速度  4.上传速度  5.帧率 
	public static native String getStreamMediaParam(int aParamType);    //增加分辨率宽高
	//获取调试信息					 1.当前视频下载速度 2.当前音频下载速度 3.当前视频帧率 4.当前音频帧率 5.平均视频下载速度 6.平均音频下载速度 7.私有 RTMP URL
	public static native String getDebugInfo();
	//初始化jni surface						  surface句柄     surfaceview宽度   高度
	public static native int  InitSurface(Surface aSurface,int aWidth, int aHeight);
	//反初始化jni surface
	public static native void UninitSurface();
	//打开播放器
	public static native int  OpenVideoPlayer();
	//关闭播放器
	public static native void CloseVideoPlayer();
	//打开音频播放
	public static native int  OpenAudioPlayer();
	//关闭音频播放
	public static native void CloseAudioPlayer();
	//glsurface重绘
	public static native void DrawVideo(); 
	//获取点播播放位置
	public static native int  GetPlayPos();
	//打开播放录制							   录制文件路径
	public static native int  OpenRecord(String aFileName);
	//关闭录制
	public static native void CloseRecord();
	//截屏
	public static native int  Snapshot(String aFileName);




/*-------待实现--------*/	
	//设置音量大小									音量大小      
	public static native int  SetVolume(int aVolume);
	//获取音量大小
	public static native int  GetVolume();
	//暂停
	public static native void Pause();
	//播放
	public static native void Play();
	//设置点播播放速度								速率
	public static native void SetScale(float aScale);
	//获取点播播放速度
	public static native float GetScale();
}
