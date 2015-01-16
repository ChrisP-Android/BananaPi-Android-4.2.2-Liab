/*
    Open Manager, an open source file manager for the Android system
    Copyright (C) 2009, 2010, 2011  Joe Berria <nexesdevelopment@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

package com.softwinner.explore;

import java.io.File;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.ExifInterface;
import android.os.Handler;
import android.util.Log;
import android.widget.ImageView;
import android.graphics.Matrix;

public class ThumbnailCreator {
	private int width;
	private int height;
	private Context context;
	private static final String TAG = ThumbnailCreator.class.getSimpleName();

	public ThumbnailCreator(Context context,int width, int height) {
		this.context = context;
		this.width = width;
		this.height = height;
	}

	public Bitmap hasBitmapCached(String imageSrc) {
		File f = new File(imageSrc);
		Bitmap bmp = MessageCache.getInstance().loadThumbnailMessage(imageSrc);
		if(bmp != null){
			//如果保存的修改日期不一致,说明文件被修改过,需要重新生成缩略图
			if(MessageCache.getInstance().loadModifiedTime(imageSrc) != f.lastModified()){
				Log.d(TAG," the file had been change since last time,I should request thumbnail again ");
				return null;
			}
		}
		return bmp;
	}


	public void clearBitmapCache() {
		MessageCache.getInstance().clearThumbnailCache();
	}

	public void setBitmapToImageView(final String imageSrc,
									 final ImageView icon) {

		Thread thread = new Thread() {
			public void run() {
				synchronized (this) {
					final Bitmap bmp = createThumbnail(imageSrc);
					if(bmp != null){
						MessageCache.getInstance().saveThumbnailMessage(imageSrc, bmp);
						File f = new File(imageSrc);
						MessageCache.getInstance().saveModifiedTime(imageSrc, f.lastModified());
						((Activity)context).runOnUiThread(new Runnable() {
							@Override
							public void run() {
								icon.setImageBitmap(bmp);
							}
						});
					}
				}
			}
		};

		thread.start();
	}

	/**
	 * 创建缩略图
	 * @param imageSrc
	 * @return 返回null,如果这时无法获得缩略图
	 */
	public Bitmap createThumbnail(String imageSrc)
	{
		boolean isJPG = false;
		Bitmap thumbnail = null;
		try
		{
			String ext = imageSrc.substring(imageSrc.lastIndexOf(".") + 1);
			if(ext.equalsIgnoreCase("jpg") || ext.equalsIgnoreCase("jpeg"))
			{
				isJPG = true;
			}
		}catch(IndexOutOfBoundsException e)
		{
			e.printStackTrace();
			return null;
		}

		if(isJPG)
		{
			try {
				ExifInterface mExif = null;
				mExif = new ExifInterface(imageSrc);
				if(mExif != null)
				{
					byte[] thumbData = mExif.getThumbnail();
					if(thumbData == null)
					{
						thumbnail = createThumbnailByOptions(imageSrc);
					}
					else
					{
					    int orient = mExif.getAttributeInt(ExifInterface.TAG_ORIENTATION, ExifInterface.ORIENTATION_NORMAL);
                        thumbnail = BitmapFactory.decodeByteArray(thumbData, 0, thumbData.length);
                        Matrix m = new Matrix();
                        float centerX = (float)thumbnail.getWidth()/2;
                        float centerY = (float)thumbnail.getHeight()/2;
                        switch(orient){
                        case ExifInterface.ORIENTATION_ROTATE_90:
                            m.setRotate(90.0f, centerX, centerY);
                            thumbnail = Bitmap.createBitmap(thumbnail, 0, 0,
                                thumbnail.getWidth(), thumbnail.getHeight(), m, false);
                            break;
                        case ExifInterface.ORIENTATION_ROTATE_180:
                            m.setRotate(180.0f, centerX, centerY);
                            thumbnail = Bitmap.createBitmap(thumbnail, 0, 0,
                                thumbnail.getWidth(), thumbnail.getHeight(), m, false);
                            break;
                        case ExifInterface.ORIENTATION_ROTATE_270:
                            m.setRotate(270.0f, centerX, centerY);
                            thumbnail = Bitmap.createBitmap(thumbnail, 0, 0,
                                thumbnail.getWidth(), thumbnail.getHeight(), m, false);
                            break;
                        default:

                        }

						thumbnail = Bitmap.createScaledBitmap(thumbnail, width, height, false);
					}
				}
				else
				{
					thumbnail = createThumbnailByOptions(imageSrc);
				}
			} catch (Exception e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
				return null;
			}
		}
		else
		{
			thumbnail = createThumbnailByOptions(imageSrc);
		}
		return thumbnail;
	}

	private Bitmap createThumbnailByOptions(String imageSrc)
	{
		try{
			Bitmap thumb = null;
			BitmapFactory.Options options = new BitmapFactory.Options();
			options.inJustDecodeBounds = true;
			thumb = BitmapFactory.decodeFile(imageSrc, options);
			int be = (int) (Math.min(options.outWidth / width, options.outHeight / height));
			if(be <= 0)
				be = 1;
			options.inSampleSize = be;
			options.inJustDecodeBounds = false;
			thumb = BitmapFactory.decodeFile(imageSrc, options);
			if(thumb == null)
			{
				/* 当不可decode时返回null */
				return null;
			}
			thumb = Bitmap.createScaledBitmap(thumb, width, height, false);
			Log.d(TAG,"image:" + imageSrc + "  orignal size:" + options.outWidth + "*" + options.outHeight);
			Log.d(TAG,"image:" + imageSrc + "  thumb size:" + String.valueOf(width) + "*" + String.valueOf(height));
			return thumb;
		}catch(Exception e){
			return null;
		}
	}
}