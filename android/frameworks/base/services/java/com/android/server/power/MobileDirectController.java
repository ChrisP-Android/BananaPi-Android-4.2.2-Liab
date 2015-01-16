package com.android.server.power;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import android.util.Slog;
import android.os.SystemProperties;

public class MobileDirectController {
	
	private final String TAG = "MobileDirectController";
	private final boolean DEBUG = true;
	private final String ENTWORK_ENABLE_PATH = "/sys/class/sw_3g_module/modem/modem_power";
	private static MobileDirectController instance;
	private MobileDirectController() {
		
	}
	public static MobileDirectController getInstance() {
		if(instance == null) {
			instance = new MobileDirectController();
		}
		return instance;
	}
	
	public boolean isMobileModeAvailable() {
		File file = new File(ENTWORK_ENABLE_PATH);
		return file.exists();
	}
	
	public boolean isNetworkEnable() {
		File file = new File(ENTWORK_ENABLE_PATH);
		BufferedReader reader = null;
		try {
			reader = new BufferedReader(new FileReader(file));
			String networkEnable = reader.readLine();
			if(networkEnable != null && networkEnable.equals("1")) {
				return true;
			}
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} finally {
			if(reader != null)
						try {
							reader.close();
						} catch (IOException e) {
							e.printStackTrace();
						}
		}
		return false;
	}
	
	public boolean setNetworkEnable(boolean enable) {
		if(DEBUG)
			Slog.d(TAG,"setNetworkEnable" + enable);
		File file = new File(ENTWORK_ENABLE_PATH);
		FileWriter writer = null;
		try {
/*			
			if(enable) {
         		 SystemProperties.set("ctl.start", "ril-daemon");
			} else {
         		 SystemProperties.set("ctl.stop", "ril-daemon");
          		 SystemProperties.set("ctl.stop", "pppd-gprs");		
			}
*/
			writer = new FileWriter(file);
			writer.write(enable? "1" : "0");
			writer.close();

			return true;
		} catch (IOException e) {
			e.printStackTrace();
		} finally {
			if(writer != null) {
				try {
					writer.close();
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
		}
		return false;
	}
}
