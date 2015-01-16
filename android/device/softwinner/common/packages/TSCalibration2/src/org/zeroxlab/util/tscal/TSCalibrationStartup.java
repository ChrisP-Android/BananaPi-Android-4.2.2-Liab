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

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.BroadcastReceiver;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.util.Log;
import java.io.File;

public class TSCalibrationStartup extends Activity {

    private static String cal_path = "/data/pointercal";


    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        File calFile = new File(cal_path);
        if (!calFile.exists()) 
		{
            Intent starterIntent = new Intent(this, TSCalibration.class);
            startActivityForResult(starterIntent, 0);
        } 
		else 
		{
            deleteFromPackageManger();
            //Intent starterIntent = new Intent(this, TSCalibration.class);
            //startActivityForResult(starterIntent, 0);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
    	//Log.d("Result","requestCode1 = " + requestCode);
        super.onActivityResult(requestCode, resultCode, data);
		//Log.d("Result","requestCode2 = " + requestCode);
        deleteFromPackageManger();
        finish();
		//Log.d("Result","requestCode1 = " + requestCode);
    }

	/*public class BootBroadcastReceiver extends BroadcastReceiver 
    {
	    @Override
	    public void onReceive(Context context, Intent intent) 
	    {
	        String action = "android.intent.action.MAIN";
	        String category = "android.intent.category.LAUNCHER";
	        
	        Intent mIntent = new Intent(context, TSCalibrationStartup.class);
	        mIntent.setAction(action);
	        mIntent.addCategory(category);
	        mIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
	        context.startActivity(mIntent);
	    }
	}*/

    private void deleteFromPackageManger() {
        // remove this activity from the package manager.
        PackageManager pm = getPackageManager();
        ComponentName name = new ComponentName(this, TSCalibrationStartup.class);
        pm.setComponentEnabledSetting(name, PackageManager.COMPONENT_ENABLED_STATE_DISABLED, 0);
    }
}
