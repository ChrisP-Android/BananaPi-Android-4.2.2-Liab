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

import java.io.File;
import java.io.FileOutputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import android.view.WindowManager;
import android.content.Context;	
import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Display;
import android.view.MotionEvent;
import android.view.KeyEvent;
import android.util.Log;
import android.os.ServiceManager;
import android.view.WindowManager;
import android.view.IWindowManager;

public class TSCalibration extends Activity {

    final private static String TAG = "TSCalibration";
    final private static String POINTERCAL = "/data/pointercal";
    final private static String defaultPointercalValues = "1 0 0 0 0 1 1 1 1 0 0 0 0 1 1 1\n";
    final private static File FILE = new File(POINTERCAL);
    boolean	  mSetContentView = false;

    private TSCalibrationView mTSCalibrationView;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Display display 	= getWindowManager().getDefaultDisplay();
        mTSCalibrationView 	= new TSCalibrationView(this,
                                                   display.getHeight(),
                                                   display.getWidth(),FILE.exists());
        setContentView(R.layout.intro);
    }

    @Override
    public void onResume() {
        super.onResume();
        mTSCalibrationView.reset();
        //reset();
    }

    @Override
	public boolean dispatchTouchEvent(MotionEvent event) 
	{
		boolean  ret;
		
		if(mSetContentView == false)
		{
			return onTouchEvent(event);
		}
		
		ret = mTSCalibrationView.onTouchEvent(event);
		if(ret == false || mTSCalibrationView.isFinished())
		{
        	return onTouchEvent(event);
    	}
    	
    	return true;
    }
    
    public boolean onTouchEvent(MotionEvent event)
    {
		//Log.d(TAG,"onTouchEvent" + event);
		if (mTSCalibrationView.isFinished()) {
			if(!FILE.exists())
			{
				//Log.d("Result","requestCode15" + "\n");
           	 	mTSCalibrationView.dumpCalData(FILE);
        	}
        	else
        	{
        		//Log.d("Result","requestCode15" + "\n");
           	 	mTSCalibrationView.dumpCalData(FILE);
           	 	
           	 	try 
				{
					IWindowManager mWindowManager	= IWindowManager.Stub.asInterface(ServiceManager.getService(Context.WINDOW_SERVICE));
                    
                	mWindowManager.resetInputCalibration();
		        } 
		        catch (Exception e) 
		        {
		            Log.d(TAG,"mWindowManager resetInputCalibration failed!");
		        }
        		
        	}
            setResult(0);
			//Log.d("Result","requestCode13" + "\n");
            finish();
			//Log.d("Result","requestCode14" + "\n");
        } else {
        	if (event.getAction() != MotionEvent.ACTION_UP)
        	{
            	return true;
        	}
        	mSetContentView = true;
			Log.d(TAG, "setContentView");
            setContentView(mTSCalibrationView);
        }
        return true;
    }

	public boolean dispatchKeyEvent(KeyEvent e) 
	{  
        return true;  
    } 

    private void reset() {
        try {
            FileOutputStream fos = new FileOutputStream(FILE);
            fos.write(defaultPointercalValues.getBytes());
            fos.flush();
            fos.getFD().sync();
            fos.close();
        } catch (FileNotFoundException e) {
        } catch (IOException e) {
        }
    }

    public void onCalTouchEvent(MotionEvent ev) {
        mTSCalibrationView.invalidate();
        if (mTSCalibrationView.isFinished()) {
            setContentView(R.layout.done);
        }
    }
}
