package android.os;

import android.util.Log;
import android.os.IBinder;
import android.os.Binder;
import android.os.IDynamicPManager;

/**
 * Dynamic Power Manager
 */
public class DynamicPManager
{
	private static final String TAG 			 = "DynamicPManager";
	public static final int CPU_MODE_PERFORMENCE = 0x00000001;	/* cpu will run in the highnest freq*/
	public static final int CPU_MODE_FANTACY     = 0x00000002;  /* cpu will auto decrease freq, so the power can decrease */	
	public  static final String DPM_SERVICE		 = "DynamicPManager";

	private IDynamicPManager service;	
	private IBinder 		 binder;
		
	public DynamicPManager() {
		IBinder b = ServiceManager.getService(DPM_SERVICE);
		service	  = IDynamicPManager.Stub.asInterface(b);
	}

	public void acquireCpuFreqLock(int flag) {
		if( binder == null)
		{
		    Log.d(TAG,"acquireCpuFreqLock");
			binder = new Binder();
			try{
				service.acquireCpuFreqLock(binder, flag);
			}catch(RemoteException e){			 
			}	
		}
	}
	
	public void releaseCpuFreqLock() {
		if( binder != null ) {
			try{
				service.releaseCpuFreqLock(binder);
				binder = null;
			}catch(RemoteException e){
			}
		}		
	}
	
	public void notifyUsrEvent(){
		try{
			service.notifyUsrEvent();
		}catch(RemoteException e){
		}
	}
}
