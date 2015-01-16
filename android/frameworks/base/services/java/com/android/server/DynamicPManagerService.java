package com.android.server;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Binder;
import android.os.DynamicPManager;
import android.os.IBinder;
import android.os.IDynamicPManager;
import android.os.RemoteException;
import android.util.Log;

import java.io.FileWriter;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;

public class DynamicPManagerService extends IDynamicPManager.Stub
{	
	private static final String TAG= "DynamicPManagerService";
	private static final String CPUFREQUNCYY_POLICY_PATH = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor";
	private static final String CPUFREQUNCYY_USREVT      = "/sys/devices/system/cpu/cpu0/cpufreq/user_event_notify";
	
	private static final String POLICY_PERFORMANCE 		 = "performance";
	private static final String POLICY_FANTASY     		 = "fantasys";
		
	private ClientList clients = new ClientList(); 	
	private Context mContext;
	private int defFlag= DynamicPManager.CPU_MODE_FANTACY;
	//private int mPowerState; // current power state 
		
	public DynamicPManagerService(Context context){
		mContext = context;	
		mContext.registerReceiver(mBroadcastReceiver,
                new IntentFilter(Intent.ACTION_BOOT_COMPLETED), null, null);
	}
	
	private BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
	    public void onReceive(Context context, Intent intent) {
	        String action = intent.getAction();
	        if (action.equals(Intent.ACTION_BOOT_COMPLETED)) {	      
	              //setCpuFrequnecyPolicy(clients.gatherState());  
	              new DynamicPowerPolicyChangeThread(mContext).start();        
	        }
	    }
	};
	
	private void acquireCpuFreqLockLocked(IBinder b, int flag)
	{
		Client ci = new Client(flag, b);
		clients.addClient(ci);
			
		setCpuFrequnecyPolicy(clients.gatherState());		
	}

	public void acquireCpuFreqLock(IBinder b, int flag)
	{		
		long ident = Binder.clearCallingIdentity();
		try {
			synchronized(clients){
				acquireCpuFreqLockLocked(b, flag);
			}
		}finally{
			Binder.restoreCallingIdentity(ident);
		}
	}
		
	private void releaseCpuFreqLockLocked(IBinder b)
	{
		Client ci = clients.rmClient(b);
		if( ci == null)
			return;
		
		ci.binder.unlinkToDeath(ci, 0);		
		setCpuFrequnecyPolicy(clients.gatherState());
	}

	public void releaseCpuFreqLock(IBinder b)
	{
		long ident = Binder.clearCallingIdentity();
		try {
			synchronized(clients){
				releaseCpuFreqLockLocked(b);
			}
		}finally{
			Binder.restoreCallingIdentity(ident);
		}
	}
	
	private void setCpuFrequnecyPolicy(int flag)
	{
	    Log.i(TAG,"setCpuFrequnecyPolicy flag :" + flag);
	    FileWriter wr=null;	  
		try { 
		    wr = new FileWriter(CPUFREQUNCYY_POLICY_PATH); 
			if( (flag & DynamicPManager.CPU_MODE_PERFORMENCE) != 0) {			   
			    wr.write(POLICY_PERFORMANCE);						
			    wr.close();		
			}else if( (flag & DynamicPManager.CPU_MODE_FANTACY) != 0) {			  
				wr.write(POLICY_FANTASY);				
				wr.close();			
			}else{
				Log.i(TAG, "unsupported cpufreq policy");
			}					    
		}catch(IOException e){
		    Log.i(TAG," setCpuFrequnecyPolicy error: " + e.getMessage()); 
		}
	}
	
	
	public void notifyUsrEvent(){
		try {
			char[] buffer = new char[1];			
			FileReader file = new FileReader(CPUFREQUNCYY_USREVT);
			file.read(buffer, 0, 1);
			file.close();
		}catch(IOException e){
		    Log.i(TAG," notifyUsrEvent: " + e.getMessage()); 
		}		
	}
	
	private class Client implements IBinder.DeathRecipient
	{
		Client(int flag, IBinder b) {
			this.binder = b;
			this.flag = flag;				
			
			try {
                b.linkToDeath(this, 0);
            } catch (RemoteException e) {
                binderDied();
            }			
		}
		
		public void binderDied() {	// client be kill, so we will delete record in ArrayList
			synchronized(clients){
				releaseCpuFreqLockLocked(binder);
			}
		}

		final IBinder binder;	
		final int flag;	  		// client request flag		
 	}
	
	private class ClientList extends ArrayList<Client>
	{
		void addClient(Client ci)
		{
			int index = getIndex(ci.binder);
			if( index < 0 )
			{
				this.add(ci);
			}
		}
		Client rmClient(IBinder b)
		{
			int index = getIndex(b);
			if( index >= 0 )
			{
				return this.remove(index);
			}else{
				return null;
			}
		}
		int getIndex(IBinder b)
		{
			int N = this.size();
			for(int i=0; i<N; i++)
			{
				if( this.get(i).binder == b)
					return i;			
			}
			return -1;
		}
		int gatherState()
		{	    
		    if( defFlag ==  DynamicPManager.CPU_MODE_PERFORMENCE ){
		        return DynamicPManager.CPU_MODE_PERFORMENCE;
		    }
		    
		    int ret = DynamicPManager.CPU_MODE_FANTACY;
			int N = this.size();			
			for( int i=0; i<N; i++){
				Client ci = this.get(i);
				if( (ci.flag & DynamicPManager.CPU_MODE_PERFORMENCE) != 0){
				    ret = DynamicPManager.CPU_MODE_PERFORMENCE;
				    break;
				}					 			
			}
			return ret;
		}
	}
}
