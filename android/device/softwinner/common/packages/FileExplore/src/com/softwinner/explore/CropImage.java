package com.softwinner.explore;

import java.io.IOException;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.ExifInterface;
import android.util.Log;

public class CropImage {
	private Context context;
	private Intent intent;
	private int outputX = 0;
	private int outputY = 0;
	private String srcImg = null;
	private static final String TAG = CropImage.class.getSimpleName();
	public CropImage(Context context,Intent intent, String srcImage){
		this.context = context;
		outputX = intent.getIntExtra("outputX", 48);
		outputY = intent.getIntExtra("outputY", 48);
		if(outputX*outputY > 128 * 128){
			outputX = 48;
			outputY = 48;
		}
		srcImg = srcImage;
		Log.d(TAG,"srcImg:" + srcImg + "  outputX:" + outputX + "  outputY:" + outputY);
	}
	
	public Intent saveResourceToIntent(){
		Intent ret = new Intent();
		ThumbnailCreator creator = new ThumbnailCreator(context, outputX, outputY);
		Bitmap bmp = creator.createThumbnail(srcImg);
		if(bmp == null) Log.e(TAG, "saveResourceToIntent()  get thumbnail res fail");
		ret.putExtra("data", bmp);
		return ret;
	}
}