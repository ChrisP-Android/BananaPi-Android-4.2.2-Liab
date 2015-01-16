package com.softwinner.tvremoteserver;

public class VmouseController{
	
	public VmouseController(){
		
	}
	public void openVmouse()
	{  
		nativeOpenVmouse();
	}

	public void closeVmouse()
	{
		nativeCloseVmouse();
	}
	
	public void dispatchMouseEvent(int x, int y, int flag){
		nativeDispatchMouseEvent(x, y, flag);
	}
	  
	private native void nativeDispatchMouseEvent(int x, int y, int flag);
	private native void nativeOpenVmouse();
	private native void nativeCloseVmouse();
	
}