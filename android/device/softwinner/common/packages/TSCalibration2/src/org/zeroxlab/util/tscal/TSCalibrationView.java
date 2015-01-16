/* Copyright (C) 2010 0xlab.org
 * Authored by: Kan-Ru Chen <kanru@0xlab.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.zeroxlab.util.tscal;

import org.zeroxlab.util.tscal.R;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.view.MotionEvent;
import android.view.View;
import java.util.Properties;
import java.io.FileInputStream;
import android.util.Log;
import android.content.res.Resources;

public class TSCalibrationView extends View {

    final private static String TAG 	= "TSCalibration";
	final private static int LIMIT 			= 100;
	final private static int J_LIMIT 		= 100;
	final private static int P_DELTA 		= 50;
	final private static int X_COMPENSATE = 20;
	final private static int Y_COMPENSATE = 20;
	private static int P_LIMIT;
	private static int SCREEN_MAX_WIDTH;
	private static int SCREEN_MAX_HEIGHT;
	
	private int  mWidth;
	private int  mHeight;
    private int P_X_RIGHT_CORNER_MAX_LIMIT 		= 0;
    private int P_X_RIGHT_CORNER_MIN_LIMIT 		= 0;
                                              
    private int P_Y_RIGHT_CORNER_MAX_LIMIT 		= 0;
    private int P_Y_RIGHT_CORNER_MIN_LIMIT 		= 0;
                                              
    private int P_X_LEFT_CORNER_MAX_LIMIT 			= 0;
    private int P_X_LEFT_CORNER_MIN_LIMIT 			= 0;
                                              
    private int P_Y_LEFT_CORNER_MAX_LIMIT 			= 0;
    private int P_Y_LEFT_CORNER_MIN_LIMIT 			= 0;
                                              
    private int P_X_CENTER_MAX_LIMIT 					= 0;
    private int P_X_CENTER_MIN_LIMIT 					= 0;
                                              
    private int P_Y_CENTER_MAX_LIMIT 					= 0;
    private int P_Y_CENTER_MIN_LIMIT 					= 0;
	
	boolean					 mFileExist		= false;
	boolean					 mGetPara		= false;
	int[]					 mCurTpPara		= new int[7];
	Properties 				 mProp;

    private class TargetPoint 
    {
        public int 	x;
        public int 	y;
        public int 	calx;
        public int 	caly;
		public int  xfb;
		public int  yfb;
		
        public TargetPoint() 
		{
            this.x = 0;
            this.y = 0;
        }

		public void SetTargetPoint(int x,int y)
		{
			this.x = x;
			this.y = y;
		}
    }

	private class TargetCal
	{
		int[]			tppara		= new int[7];
		TargetPoint[] 	mTargetPoints;

		public TargetCal()
		{  
			
			mTargetPoints	 = new TargetPoint[5];
			mTargetPoints[0] = new TargetPoint();
			mTargetPoints[1] = new TargetPoint();
			mTargetPoints[2] = new TargetPoint();
			mTargetPoints[3] = new TargetPoint();
			mTargetPoints[4] = new TargetPoint();
		}
	}

	private class TsSample
	{
		public int x;
		public int y;

		public TsSample()
		{
			this.x = 0;
			this.y = 0;
		}
	}

    private int 			mStep = 0;
    private TargetCal 		mTargetCaluate;
    private TSCalibration 	mContext;
	
    public TSCalibrationView(TSCalibration context, int h, int w ,boolean fileexist) {
        super(context);
		Resources res = context.getResources();
        P_LIMIT=res.getInteger(R.integer.p_limit);
		SCREEN_MAX_WIDTH = res.getInteger(R.integer.screen_max_width);
	    SCREEN_MAX_HEIGHT = res.getInteger(R.integer.sreen_max_height);
        Log.v(TAG,P_LIMIT+":"+SCREEN_MAX_WIDTH+":"+SCREEN_MAX_HEIGHT);
		
		mHeight = h;
		mWidth  = w;
        P_X_RIGHT_CORNER_MAX_LIMIT 		= mWidth - X_COMPENSATE - 50 + P_DELTA;
        P_X_RIGHT_CORNER_MIN_LIMIT 		= mWidth - X_COMPENSATE - 50 - P_DELTA;
        
        P_Y_RIGHT_CORNER_MAX_LIMIT 		= mHeight - Y_COMPENSATE - 50 + P_DELTA;
        P_Y_RIGHT_CORNER_MIN_LIMIT 		= mHeight - Y_COMPENSATE - 50 - P_DELTA;
        
        P_X_LEFT_CORNER_MAX_LIMIT 		= 50 + X_COMPENSATE + P_DELTA;
        P_X_LEFT_CORNER_MIN_LIMIT 		= 50 + X_COMPENSATE - P_DELTA;
        
        P_Y_LEFT_CORNER_MAX_LIMIT 		= 50 + Y_COMPENSATE + P_DELTA;
        P_Y_LEFT_CORNER_MIN_LIMIT 		= 50 + Y_COMPENSATE - P_DELTA;
        
        P_X_CENTER_MAX_LIMIT 			= mWidth/2 + P_DELTA;
        P_X_CENTER_MIN_LIMIT 			= mWidth/2 - P_DELTA;
        
        P_Y_CENTER_MAX_LIMIT 			= mHeight/2 + P_DELTA;
        P_Y_CENTER_MIN_LIMIT 			= mHeight/2 - P_DELTA;
        mContext = context;
		mTargetCaluate	 = new TargetCal();
        mTargetCaluate.mTargetPoints[0].SetTargetPoint(50, 50);
        mTargetCaluate.mTargetPoints[1].SetTargetPoint(w - 50, 50);
        mTargetCaluate.mTargetPoints[2].SetTargetPoint(w - 50, h - 50);
        mTargetCaluate.mTargetPoints[3].SetTargetPoint(50, h - 50);
        mTargetCaluate.mTargetPoints[4].SetTargetPoint(w/2, h/2);

		mProp 				= new Properties();
		mFileExist 			= fileexist;
		if(mFileExist == true)
		{
			ReadParaFromProp();
		}
    }

    public void reset() 
	{
        mStep = 0;
    }


	private int variance_judge(TsSample[] tp_data, int nr,int limit)
	{
		int i, j;
		int diff_x;
		int diff_y;


		for (i = 0; i < nr - 1; i++) 
	    {
			for (j = i + 1; j < nr; j++) 
			{
				/*
				 * Calculate the variance between sample 'i'
				 * and sample 'j'.  X and Y values are treated
				 * separately.
				 */
				diff_x = tp_data[i].x - tp_data[j].x;
				if (diff_x < 0)
				{
					diff_x = -diff_x;
				}

				diff_y = tp_data[i].y - tp_data[j].y;
				if (diff_y < 0)
				{
					diff_y = -diff_y;
				}



				/*
				 * Is the variance between any two samples too large?
				 */
				if (diff_x > limit  || diff_y > limit)
				{
					
            		Log.w(TAG,"tp ="+diff_x+"diff_y="+diff_y+"\n");
                    return -1;
				}

			}
		}

		return 0;
	}

/*----------------------------------------------------------------------------
 * FUNCTION NAME: variance_judge
 *
 * PURPOSE:       judge points available
 *
 *
 * INPUT:
 *   *data       Is a pointer to the array to be judged
 *   nr          the number of elements
 *   limit       criterion
 *
 * OUTPUT:
 *
 *
 * RETURN VALUE:
 *   ret        0  elements is a point
 *             -1  elements is not a point
 *
 *
 *---------------------------------------------------------------------------*/
/*
 * We have 7 complete samples.  Calculate the variance between each,
 * if the variance beteween any two samples too large, return fial;
 *
 */


private int point_to_point_judge(int[] data)
{
	int ret;
	int diff;
	int p_limit;

    p_limit = TSCalibrationView.P_LIMIT;
	
	if(data[0] < data[1])
		diff = data[1] - data[0];
	else
		diff = data[0] - data[1];
	
	if(diff < p_limit)
    {
	   
        Log.w(TAG,"tp ="+diff+"\n");
        ret = -1;
    }
        else
		ret =  0;
	
	return ret;		
}

/*----------------------------------------------------------------------------
 * FUNCTION NAME: judge_point_location
 *
 * PURPOSE: judge whether the pointer is located in where u want.
 * RETURN VALUE:
 *   ret         0 points all located where u want
 *              -1 at least one point not located on where u expected.
 *
 *
 *---------------------------------------------------------------------------*/
private int judge_point_location(TargetCal cal)
{
	if(SCREEN_MAX_WIDTH != mWidth || SCREEN_MAX_HEIGHT != mHeight){
		Log.d(TAG, "screen mWidth" + mWidth + "!= " + SCREEN_MAX_WIDTH + "|| mHeight" + mHeight + "!=" + SCREEN_MAX_HEIGHT + ", the calibrate result may not as well as u expect.");
	}
	//judge y coordinate.
	if((cal.mTargetPoints[0].caly>P_Y_RIGHT_CORNER_MAX_LIMIT) || (cal.mTargetPoints[0].caly<P_Y_RIGHT_CORNER_MIN_LIMIT) ){
		if((cal.mTargetPoints[0].caly>P_Y_LEFT_CORNER_MAX_LIMIT) || (cal.mTargetPoints[0].caly<P_Y_LEFT_CORNER_MIN_LIMIT)){
			Log.d(TAG,"cal.mTargetPoints[0].caly = " + cal.mTargetPoints[0].caly + "not locate between" + P_Y_RIGHT_CORNER_MAX_LIMIT +" and " + P_Y_RIGHT_CORNER_MIN_LIMIT +
			          "also not locate between" + P_Y_LEFT_CORNER_MAX_LIMIT + "and" + P_Y_LEFT_CORNER_MIN_LIMIT + "\n");
			return -1;	
		}
	}

		if((cal.mTargetPoints[1].caly>P_Y_RIGHT_CORNER_MAX_LIMIT) || (cal.mTargetPoints[1].caly<P_Y_RIGHT_CORNER_MIN_LIMIT) ){
		if((cal.mTargetPoints[1].caly>P_Y_LEFT_CORNER_MAX_LIMIT) || (cal.mTargetPoints[1].caly<P_Y_LEFT_CORNER_MIN_LIMIT)){
			Log.d(TAG,"cal.mTargetPoints[1].caly = " + cal.mTargetPoints[1].caly + "not locate between" + P_Y_RIGHT_CORNER_MAX_LIMIT +" and " + P_Y_RIGHT_CORNER_MIN_LIMIT +
			          "also not locate between" + P_Y_LEFT_CORNER_MAX_LIMIT + "and" + P_Y_LEFT_CORNER_MIN_LIMIT + "\n");
			return -1;	
		}
	}
	
		if((cal.mTargetPoints[2].caly>P_Y_RIGHT_CORNER_MAX_LIMIT) || (cal.mTargetPoints[2].caly<P_Y_RIGHT_CORNER_MIN_LIMIT) ){
		if((cal.mTargetPoints[2].caly>P_Y_LEFT_CORNER_MAX_LIMIT) || (cal.mTargetPoints[2].caly<P_Y_LEFT_CORNER_MIN_LIMIT)){
			Log.d(TAG,"cal.mTargetPoints[2].caly = " + cal.mTargetPoints[2].caly + "not locate between" + P_Y_RIGHT_CORNER_MAX_LIMIT +" and " + P_Y_RIGHT_CORNER_MIN_LIMIT +
			          "also not locate between" + P_Y_LEFT_CORNER_MAX_LIMIT + "and" + P_Y_LEFT_CORNER_MIN_LIMIT + "\n");
			return -1;	
		}
	}
	
		if((cal.mTargetPoints[3].caly>P_Y_RIGHT_CORNER_MAX_LIMIT) || (cal.mTargetPoints[3].caly<P_Y_RIGHT_CORNER_MIN_LIMIT) ){
		if((cal.mTargetPoints[3].caly>P_Y_LEFT_CORNER_MAX_LIMIT) || (cal.mTargetPoints[3].caly<P_Y_LEFT_CORNER_MIN_LIMIT)){
			Log.d(TAG,"cal.mTargetPoints[3].caly = " + cal.mTargetPoints[3].caly + "not locate between" + P_Y_RIGHT_CORNER_MAX_LIMIT +" and " + P_Y_RIGHT_CORNER_MIN_LIMIT +
			          "also not locate between" + P_Y_LEFT_CORNER_MAX_LIMIT + "and" + P_Y_LEFT_CORNER_MIN_LIMIT + "\n");
			return -1;	
		}
	}
	
	//y center
		if((cal.mTargetPoints[4].caly>P_Y_CENTER_MAX_LIMIT) || (cal.mTargetPoints[4].caly<P_Y_CENTER_MIN_LIMIT) ){	
			Log.d(TAG,"cal.mTargetPoints[4].caly = " + cal.mTargetPoints[4].caly + "not locate between" + P_Y_CENTER_MAX_LIMIT +" and " + P_Y_CENTER_MIN_LIMIT + "\n");
			return -1;
	}
	//judge x coordinate.
		if((cal.mTargetPoints[0].calx>P_X_RIGHT_CORNER_MAX_LIMIT) || (cal.mTargetPoints[0].calx<P_X_RIGHT_CORNER_MIN_LIMIT) ){
		if((cal.mTargetPoints[0].calx>P_X_LEFT_CORNER_MAX_LIMIT) || (cal.mTargetPoints[0].calx<P_X_LEFT_CORNER_MIN_LIMIT)){
			Log.d(TAG,"cal.mTargetPoints[0].calx = " + cal.mTargetPoints[0].calx + "not locate between" + P_X_RIGHT_CORNER_MAX_LIMIT +" and " + P_X_RIGHT_CORNER_MIN_LIMIT +
			          "also not locate between" + P_X_LEFT_CORNER_MAX_LIMIT + "and" + P_X_LEFT_CORNER_MIN_LIMIT + "\n");
			return -1;	
		}
	}

		if((cal.mTargetPoints[1].calx>P_X_RIGHT_CORNER_MAX_LIMIT) || (cal.mTargetPoints[1].calx<P_X_RIGHT_CORNER_MIN_LIMIT) ){
		if((cal.mTargetPoints[1].calx>P_X_LEFT_CORNER_MAX_LIMIT) || (cal.mTargetPoints[1].calx<P_X_LEFT_CORNER_MIN_LIMIT)){
			Log.d(TAG,"cal.mTargetPoints[1].calx = " + cal.mTargetPoints[1].calx + "not locate between" + P_X_RIGHT_CORNER_MAX_LIMIT +" and " + P_X_RIGHT_CORNER_MIN_LIMIT +
			          "also not locate between" + P_X_LEFT_CORNER_MAX_LIMIT + "and" + P_X_LEFT_CORNER_MIN_LIMIT + "\n");
			return -1;	
		}
	}
	
		if((cal.mTargetPoints[2].calx>P_X_RIGHT_CORNER_MAX_LIMIT) || (cal.mTargetPoints[2].calx<P_X_RIGHT_CORNER_MIN_LIMIT) ){
		if((cal.mTargetPoints[2].calx>P_X_LEFT_CORNER_MAX_LIMIT) || (cal.mTargetPoints[2].calx<P_X_LEFT_CORNER_MIN_LIMIT)){
			Log.d(TAG,"cal.mTargetPoints[2].calx = " + cal.mTargetPoints[2].calx + "not locate between" + P_X_RIGHT_CORNER_MAX_LIMIT +" and " + P_X_RIGHT_CORNER_MIN_LIMIT +
			          "also not locate between" + P_X_LEFT_CORNER_MAX_LIMIT + "and" + P_X_LEFT_CORNER_MIN_LIMIT + "\n");
			return -1;	
		}
	}
	
		if((cal.mTargetPoints[3].calx>P_X_RIGHT_CORNER_MAX_LIMIT) || (cal.mTargetPoints[3].calx<P_X_RIGHT_CORNER_MIN_LIMIT) ){
		if((cal.mTargetPoints[3].calx>P_X_LEFT_CORNER_MAX_LIMIT) || (cal.mTargetPoints[3].calx<P_X_LEFT_CORNER_MIN_LIMIT)){
			Log.d(TAG,"cal.mTargetPoints[3].calx = " + cal.mTargetPoints[3].calx + "not locate between" + P_X_RIGHT_CORNER_MAX_LIMIT +" and " + P_X_RIGHT_CORNER_MIN_LIMIT +
			          "also not locate between" + P_X_LEFT_CORNER_MAX_LIMIT + "and" + P_X_LEFT_CORNER_MIN_LIMIT + "\n");
			return -1;	
		}
	}
	
		//x center
		if((cal.mTargetPoints[4].calx>P_X_CENTER_MAX_LIMIT) || (cal.mTargetPoints[4].calx<P_X_CENTER_MIN_LIMIT) ){	
			Log.d(TAG,"cal.mTargetPoints[4].calx = " + cal.mTargetPoints[4].calx + "not locate between" + P_X_CENTER_MAX_LIMIT +" and " + P_X_CENTER_MIN_LIMIT + "\n");
			return -1;
	}
	
	return 0;
}
/*----------------------------------------------------------------------------
 * FUNCTION NAME: judge_points
 *
 * PURPOSE:       judge that points is available point to calibration
 *
 *
 * INPUT:
 *   *cal      Is a pointer to the struct of calibration
 *
 * OUTPUT:
 *
 *
 * RETURN VALUE:
 *   ret         0 points is avaliable
 *              -1 points is unavaliable
 *
 *
 *---------------------------------------------------------------------------*/
private int judge_points(TargetCal cal)
{
	int 		ret = 0;
	TsSample[]	tp_data;
	int[]  		data = {0,0};
	int  		limit;
	int  		j_limit;

	tp_data 		= new TsSample[2];
	tp_data[0]	    = new TsSample();
	tp_data[1]	    = new TsSample();
	
    limit 			= TSCalibrationView.LIMIT;
    j_limit 		= 2*TSCalibrationView.J_LIMIT;

	tp_data[0].y 	= cal.mTargetPoints[0].caly;
	tp_data[1].y 	= cal.mTargetPoints[1].caly;
	tp_data[0].x 	= 0;
	tp_data[1].x 	= 0;
	data[0] 		= cal.mTargetPoints[0].calx;
	data[1] 		= cal.mTargetPoints[1].calx;
	
	if(0!=judge_point_location(cal)){
		Log.d(TAG, "at least, one point not locate on expected location. \n");
		return -1;
	}
	
	Log.d(TAG,"cal.mTargetPoints[0].caly = " + cal.mTargetPoints[0].caly + "cal.mTargetPoints[1].caly = " + cal.mTargetPoints[1].caly + 
	           "cal.mTargetPoints[0].calx =" + cal.mTargetPoints[0].calx + "cal.mTargetPoints[1].calx =" + cal.mTargetPoints[1].calx);
	if((variance_judge(tp_data ,2,limit) != 0)||(point_to_point_judge(data) != 0))
	{
		ret = -1;
		Log.d(TAG,"Unmath TOP left and Top Right \n");
        Log.d(TAG,"tp ="+cal.mTargetPoints[0].caly+", %d\n"+cal.mTargetPoints[0].caly+cal.mTargetPoints[1].caly);
		return ret;
	}

	tp_data[0].x 	= cal.mTargetPoints[1].calx;
	tp_data[1].x 	= cal.mTargetPoints[2].calx;
	tp_data[0].y 	= 0;
	tp_data[1].y 	= 0;
	data[0] 		= cal.mTargetPoints[1].caly;
	data[1] 		= cal.mTargetPoints[2].caly;
	Log.d(TAG,"cal.mTargetPoints[1].calx = " + cal.mTargetPoints[1].calx + "cal.mTargetPoints[2].calx = " + cal.mTargetPoints[2].calx + 
           "cal.mTargetPoints[1].caly =" + cal.mTargetPoints[1].caly + "cal.mTargetPoints[2].caly =" + cal.mTargetPoints[2].caly);
	if((variance_judge(tp_data ,2,limit) != 0)|| (point_to_point_judge(data) != 0))
	{
		ret = -1;
		Log.d(TAG,"Unmath TOP Right and Bot Right \n");
		Log.w(TAG,"tp = %d, %d\n"+cal.mTargetPoints[1].calx+cal.mTargetPoints[2].calx);
		return ret;
	}

	tp_data[0].y 	= cal.mTargetPoints[2].caly;
	tp_data[1].y 	= cal.mTargetPoints[3].caly;
	tp_data[0].x 	= 0;
	tp_data[1].x 	= 0;
	data[0] 		= cal.mTargetPoints[2].calx;
	data[1] 		= cal.mTargetPoints[3].calx;
	Log.d(TAG,"cal.mTargetPoints[2].caly = " + cal.mTargetPoints[2].caly + "cal.mTargetPoints[3].caly = " + cal.mTargetPoints[3].caly + 
           "cal.mTargetPoints[2].calx =" + cal.mTargetPoints[2].calx + "cal.mTargetPoints[3].calx =" + cal.mTargetPoints[3].calx);
	if((variance_judge(tp_data ,2,limit) != 0)|| (point_to_point_judge(data)!= 0))
	{
		ret = -1;
		Log.d(TAG,"Unmath Bot Right and Bot Left \n");
		Log.w(TAG,"tp = %d, %d\n"+cal.mTargetPoints[2].caly+cal.mTargetPoints[3].caly);
		return ret;
	}

	tp_data[0].x 	= cal.mTargetPoints[0].calx;
	tp_data[1].x 	= cal.mTargetPoints[3].calx;
	tp_data[0].y 	= 0;
	tp_data[1].y 	= 0;
	data[0] 		= cal.mTargetPoints[0].caly;
	data[1] 		= cal.mTargetPoints[3].caly;
	Log.d(TAG,"cal.mTargetPoints[0].calx = " + cal.mTargetPoints[0].calx + "cal.mTargetPoints[3].calx = " + cal.mTargetPoints[3].calx + 
           "cal.mTargetPoints[0].caly =" + cal.mTargetPoints[0].caly + "cal.mTargetPoints[3].caly =" + cal.mTargetPoints[3].caly);
	if((variance_judge(tp_data ,2,limit) != 0)|| (point_to_point_judge(data) != 0))
	{
		ret = -1;
		Log.d(TAG,"Unmath TOP Left and Bot Left \n");
		Log.d(TAG,"tp = %d, %d\n"+cal.mTargetPoints[0].calx+cal.mTargetPoints[3].calx);
		return ret;
	}

	tp_data[0].x 	= (cal.mTargetPoints[0].calx + cal.mTargetPoints[1].calx + cal.mTargetPoints[2].calx + cal.mTargetPoints[3].calx)/4;
	tp_data[1].x 	= cal.mTargetPoints[4].calx;
	
	tp_data[0].y 	= (cal.mTargetPoints[0].caly + cal.mTargetPoints[1].caly + cal.mTargetPoints[2].caly + cal.mTargetPoints[3].caly)/4;
	tp_data[1].y 	= cal.mTargetPoints[4].caly;

		Log.d(TAG,"Center tx ="+cal.mTargetPoints[0].calx +cal.mTargetPoints[1].calx+cal.mTargetPoints[2].calx+cal.mTargetPoints[3].calx +cal.mTargetPoints[4].calx+tp_data[0].x + "\n");
		Log.d(TAG,"Center ty ="+cal.mTargetPoints[0].caly +cal.mTargetPoints[1].caly+cal.mTargetPoints[2].caly+cal.mTargetPoints[3].caly +cal.mTargetPoints[4].caly+tp_data[1].y + "\n");
	if((variance_judge(tp_data ,2, j_limit) != 0))
	{
		ret = -1;
		Log.d(TAG,"Unmath Center X ,y  and The other point \n");
		Log.d(TAG,"tx = \n"+cal.mTargetPoints[0].calx +cal.mTargetPoints[1].calx+cal.mTargetPoints[2].calx+cal.mTargetPoints[3].calx +cal.mTargetPoints[4].calx+tp_data[0].x + "\n");
		Log.d(TAG,"ty = \n"+cal.mTargetPoints[0].caly +cal.mTargetPoints[1].caly+cal.mTargetPoints[2].caly+cal.mTargetPoints[3].caly +cal.mTargetPoints[4].caly+tp_data[1].y + "\n");
		return ret;
	}

	ret  = 0;
	return ret;
}


private	int perform_calibration(int count, int[] para) {
		int mStep,ret,j;
		float n, x, y, x2, y2, xy, z, zx, zy;
		float det, a, b, c, e, f, i;
		float scaling = 65536.0f;
		int[] cx  = new int[5];
		int[] cy  = new int[5];
		int[] xfb = new int[5];
		int[] yfb = new int[5];
		int[] ca  = new int[7] ;
		
	
		for(mStep  = 0; mStep < count; mStep++)
		{
			cx[mStep]	= (int)mTargetCaluate.mTargetPoints[mStep].calx ;
			cy[mStep]	= (int)mTargetCaluate.mTargetPoints[mStep].caly;
			xfb[mStep]	= (int)mTargetCaluate.mTargetPoints[mStep].x;
			yfb[mStep]	= (int)mTargetCaluate.mTargetPoints[mStep].y;
	   }

	
	// Get sums for matrix
		n = x = y = x2 = y2 = xy = 0;
		for(j=0;j<5;j++) {
			n += 1.0;
			x += (float)cx[j];
			y += (float)cy[j];
			x2 += (float)(cx[j]*cx[j]);
			y2 += (float)(cy[j]*cy[j]);
			xy += (float)(cx[j]*cy[j]);
		}
	
	// Get determinant of matrix -- check if determinant is too small
		det = n*(x2*y2 - xy*xy) + x*(xy*y - x*y2) + y*(x*xy - y*x2);
		if(det < 0.1 && det > -0.1) {
			Log.v(TAG,"ts_calibrate: determinant is too small -- " + det + "\n");
			return 0;
		}
	
	// Get elements of inverse matrix
		a = (x2*y2 - xy*xy)/det;
		b = (xy*y - x*y2)/det;
		c = (x*xy - y*x2)/det;
		e = (n*y2 - y*y)/det;
		f = (x*y - n*xy)/det;
		i = (n*x2 - x*x)/det;
	
	// Get sums for x calibration
		z = zx = zy = 0;
		for(j=0;j<5;j++) {
			z += (float)xfb[j];
			zx += (float)(xfb[j]*cx[j]);
			zy += (float)(xfb[j]*cy[j]);
		}
	
	// Now multiply out to get the calibration for framebuffer x coord
		ca[0] = (int)((a*z + b*zx + c*zy)*(scaling));
		ca[1] = (int)((b*z + e*zx + f*zy)*(scaling));
		ca[2] = (int)((c*z + f*zx + i*zy)*(scaling));
	
		Log.v(TAG,ca[0]+ " " + ca[1] + " " + ca[2]);
	
	// Get sums for y calibration
		z = zx = zy = 0;
		for(j=0;j<5;j++) 
		{
			z += (float)yfb[j];
			zx += (float)(yfb[j]*cx[j]);
			zy += (float)(yfb[j]*cy[j]);
		}
	
	// Now multiply out to get the calibration for framebuffer y coord
		ca[3] = (int)((a*z + b*zx + c*zy)*(scaling));
		ca[4] = (int)((b*z + e*zx + f*zy)*(scaling));
		ca[5] = (int)((c*z + f*zx + i*zy)*(scaling));
	
	
	// If we got here, we're OK, so assign scaling to a[6] and return
		ca[6] = (int)scaling;
	

		para[0] = ca[1];
		para[1] = ca[2];
		para[2] = ca[0];
		para[3] = ca[4];
		para[4] = ca[5];
		para[5] = ca[3];
		para[6] = ca[6];
		
		return 1;
	
	}


    public boolean isFinished() 
	{
		int  ret;

		if(mStep >=5)
		{
			ret = judge_points(mTargetCaluate);
			if(ret == 0)
			{
			    perform_calibration(5,mTargetCaluate.tppara);
				return true;
			}

			mStep = 0;

			return false;
		}
		else
		{
			return false;
		}
    }

/*
    public void dumpCalData(File file) {
        StringBuilder sb = new StringBuilder();
        for (TargetPoint point : mTargetCaluate.mTargetPoints) {
            sb.append(point.calx);
            sb.append(" ");
            sb.append(point.caly);
            sb.append(" ");
        }
        for (TargetPoint point : mTargetCaluate.mTargetPoints) {
            sb.append(point.x);
            sb.append(" ");
            sb.append(point.y);
            sb.append(" ");
        }
        try {
            FileOutputStream fos = new FileOutputStream(file);
            fos.write(sb.toString().getBytes());
            fos.flush();
            fos.getFD().sync();
            fos.close();
        } catch (FileNotFoundException e) {
            Log.e(TAG, "Cannot open file " + file);
        } catch (IOException e) {
            Log.e(TAG, "Cannot write file " + file);
        }
    }
*/
	public void WriteParaToProp()
	{
		if(mProp == null)
		{
			Log.d(TAG,"mProp is null");
		}
		else
		{
			mProp.setProperty("TP_PARA1",Integer.toString(mTargetCaluate.tppara[0]));
			mProp.setProperty("TP_PARA2",Integer.toString(mTargetCaluate.tppara[1]));
			mProp.setProperty("TP_PARA3",Integer.toString(mTargetCaluate.tppara[2]));
			mProp.setProperty("TP_PARA4",Integer.toString(mTargetCaluate.tppara[3]));
			mProp.setProperty("TP_PARA5",Integer.toString(mTargetCaluate.tppara[4]));
			mProp.setProperty("TP_PARA6",Integer.toString(mTargetCaluate.tppara[5]));
			mProp.setProperty("TP_PARA7",Integer.toString(mTargetCaluate.tppara[6]));
			
			try
			{
				 FileOutputStream fos = new FileOutputStream("/data/pointercal");         
          		 mProp.store(fos, "Copyright");          
          		 fos.close();
			}
			catch(Exception e)
			{
				Log.d(TAG,"file input stream error!");
			}
		}
	}
	
	public void ReadParaFromProp()
	{
		if(mProp == null)
		{
			Log.d(TAG,"mProp is null");
		}
		else
		{
			try
			{
				if(mGetPara == false)
				{
					Log.d(TAG,"mProp is not null");
					
					FileInputStream mFis = new FileInputStream("/data/pointercal");  
					mProp.load(mFis);
					mCurTpPara[0] = Integer.parseInt(mProp.getProperty("TP_PARA1"));
					mCurTpPara[1] = Integer.parseInt(mProp.getProperty("TP_PARA2"));
					mCurTpPara[2] = Integer.parseInt(mProp.getProperty("TP_PARA3"));
					mCurTpPara[3] = Integer.parseInt(mProp.getProperty("TP_PARA4"));
					mCurTpPara[4] = Integer.parseInt(mProp.getProperty("TP_PARA5"));
					mCurTpPara[5] = Integer.parseInt(mProp.getProperty("TP_PARA6"));
					mCurTpPara[6] = Integer.parseInt(mProp.getProperty("TP_PARA7"));
				    mFis.close();
					
					mGetPara = true;
				}
			}
			catch(Exception e)
			{
				Log.d(TAG,"file input stream error!");
			}
			
		}
	}
	
    /*public void dumpCalData(File file) {
        int i;
		int k;

        StringBuilder sb = new StringBuilder();
        for (i = 0; i < 7; i++) {
			k = i + 1;
            sb.append("TP_PARA" + k + "=" + mTargetCaluate.tppara[i] + "\n");
            //sb.append(" ");
        }
        WriteParaToProp();

        try {
            FileOutputStream fos = new FileOutputStream(file);
            fos.write(sb.toString().getBytes());
            fos.flush();
            fos.getFD().sync();
            fos.close();
        } catch (FileNotFoundException e) {
            Log.e(TAG, "Cannot open file " + file);
        } catch (IOException e) {
            Log.e(TAG, "Cannot write file " + file);
        }
    }*/

	public void dumpCalData(File file) 
	{
        WriteParaToProp();
    }


    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        if (isFinished())
            return true;
        if (ev.getAction() != MotionEvent.ACTION_UP)
            return true;

		if(mFileExist == false)
		{
			mTargetCaluate.mTargetPoints[mStep].calx = (int)ev.getRawX();
        	mTargetCaluate.mTargetPoints[mStep].caly = (int)ev.getRawY();
		}
		else
		{
			float  x;
			float  y;
			float  rawX;
			float  rawY;
			
			ReadParaFromProp();
				
			rawX = ev.getRawX();
			rawY = ev.getRawY();
			
			x    = (mCurTpPara[6] * rawX - mCurTpPara[2])/(mCurTpPara[0] + mCurTpPara[1]);
			y    = (mCurTpPara[6] * rawY - mCurTpPara[5])/(mCurTpPara[3] + mCurTpPara[4]);
        	mTargetCaluate.mTargetPoints[mStep].calx = (int)x;
        	mTargetCaluate.mTargetPoints[mStep].caly = (int)y;
    	}
        mStep++;
        mContext.onCalTouchEvent(ev);
        return true;
    }

    @Override
    protected void onDraw(Canvas canvas) 
    {
        if (isFinished())
            return;
        canvas.drawColor(Color.BLACK);
        drawTarget(canvas, mTargetCaluate.mTargetPoints[mStep].x, mTargetCaluate.mTargetPoints[mStep].y);
    }

    private void drawTarget(Canvas c, int x, int y) {
        Paint white = new Paint(Paint.ANTI_ALIAS_FLAG);
        Paint red = new Paint(Paint.ANTI_ALIAS_FLAG);
        white.setColor(Color.WHITE);
        red.setColor(Color.RED);
        c.drawLine(x - 20,y,x+20,y,white);
        c.drawLine(x,y - 20,x,y + 20,white);
/*
        c.drawCircle(x, y, 13, red);
        c.drawCircle(x, y, 11, white);
        c.drawCircle(x, y, 9, red);
        c.drawCircle(x, y, 7, white);
        c.drawCircle(x, y, 5, red);
        c.drawCircle(x, y, 3, white);
        c.drawCircle(x, y, 1, red);
*/
    }
}
