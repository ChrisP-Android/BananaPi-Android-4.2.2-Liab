package com.softwinner.tvremoteserver;

import android.os.SystemClock;
import android.os.IBinder;
import android.util.Log;
import android.view.IWindowManager;
import android.view.InputDevice;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import com.android.internal.view.IInputContext;
import com.android.internal.view.IInputMethodManager;
import android.content.Context;
import android.os.ServiceManager;
import android.hardware.input.InputManager;



import com.google.anymote.Key.Action;    
import com.google.anymote.Key.Code;
import com.google.anymote.common.ConnectInfo;
import com.google.anymote.server.RequestReceiver;

public class CommandDispatcher implements RequestReceiver {
	private final boolean DEBUG = true;
	private static final String TAG	= "CommandDispatcher";
	IWindowManager mWindowManager;
	IInputMethodManager service;
	VmouseController mVmouseController;
	
	
	public CommandDispatcher(IWindowManager manager)
	{
		mWindowManager = manager;
		IBinder b = ServiceManager.getService(Context.INPUT_METHOD_SERVICE);
        service = IInputMethodManager.Stub.asInterface(b);
        mVmouseController = new VmouseController();
	}

	/**
	   * Called when a key event is received on the server.
	   *
	   * @param keycode the received linux keycode
	   * @param action the action on the key (up/down)
	   */
	  public void onKeyEvent(Code keycode, Action action) {
		  if(DEBUG)Log.d(TAG, " keycode " + keycode + " action " + action);
		  int mAction;
		  mAction = action == Action.DOWN ? KeyEvent.ACTION_DOWN : KeyEvent.ACTION_UP;
		  switch (keycode)
		  {
		  case KEYCODE_BACK:
			  injectKeyEvent(KeyEvent.KEYCODE_BACK, mAction);
			  break;
		  case KEYCODE_HOME:
			  injectKeyEvent(KeyEvent.KEYCODE_HOME, mAction);
			  break;
		  case KEYCODE_VOLUME_DOWN:
			  injectKeyEvent(KeyEvent.KEYCODE_VOLUME_DOWN, mAction);
			  break;
		  case KEYCODE_VOLUME_UP:
			  injectKeyEvent(KeyEvent.KEYCODE_VOLUME_UP, mAction);
			  break;
		  case KEYCODE_MUTE:
			  injectKeyEvent(KeyEvent.KEYCODE_MUTE, mAction);
			  break;
		  case KEYCODE_DPAD_CENTER:
			  injectKeyEvent(KeyEvent.KEYCODE_DPAD_CENTER, mAction);
			  break;
		  case KEYCODE_DPAD_LEFT:
			  injectKeyEvent(KeyEvent.KEYCODE_DPAD_LEFT, mAction);
			  break;
		  case KEYCODE_DPAD_RIGHT:
			  injectKeyEvent(KeyEvent.KEYCODE_DPAD_RIGHT, mAction);
			  break;
		  case KEYCODE_DPAD_UP:
			  injectKeyEvent(KeyEvent.KEYCODE_DPAD_UP, mAction);
			  break;
		  case KEYCODE_DPAD_DOWN:
			  injectKeyEvent(KeyEvent.KEYCODE_DPAD_DOWN, mAction);
			  break;
		  case KEYCODE_MENU:
			  injectKeyEvent(KeyEvent.KEYCODE_MENU, mAction);
			  break;
		  case KEYCODE_PROG_BLUE:
			  injectKeyEvent(KeyEvent.KEYCODE_PROG_BLUE, mAction);
			  break;
		  case KEYCODE_PROG_GREEN:
			  injectKeyEvent(KeyEvent.KEYCODE_PROG_GREEN, mAction);
			  break;
		  case KEYCODE_PROG_RED:
			  injectKeyEvent(KeyEvent.KEYCODE_PROG_RED, mAction);
			  break;
		  case KEYCODE_PROG_YELLOW:
			  injectKeyEvent(KeyEvent.KEYCODE_PROG_YELLOW, mAction);
			  break;
		  case BTN_MOUSE:   
			  mVmouseController.dispatchMouseEvent(action.getNumber(), 0, 1);
			  break;
		 case KEYCODE_SETTINGS:
			  injectKeyEvent(KeyEvent.KEYCODE_SETTINGS, mAction);
			  break;
		  case KEYCODE_MEDIA_PLAY:
			  injectKeyEvent(KeyEvent.KEYCODE_MEDIA_PLAY, mAction);
			  break;
		  case KEYCODE_MEDIA_REWIND:
			  injectKeyEvent(KeyEvent.KEYCODE_MEDIA_REWIND, mAction);
			  break;
		  case KEYCODE_MEDIA_FAST_FORWARD:
			  injectKeyEvent(KeyEvent.KEYCODE_MEDIA_FAST_FORWARD, mAction);
			  break;
		  case KEYCODE_MEDIA_NEXT:
			  injectKeyEvent(KeyEvent.KEYCODE_MEDIA_NEXT, mAction);
			  break;
		  case KEYCODE_MEDIA_PREVIOUS:
			  injectKeyEvent(KeyEvent.KEYCODE_MEDIA_PREVIOUS, mAction);
			  break;
		  case KEYCODE_MEDIA_STOP:
			  injectKeyEvent(KeyEvent.KEYCODE_MEDIA_STOP, mAction);
			  break;
		  case KEYCODE_PAUSE:
			  injectKeyEvent(KeyEvent.KEYCODE_MEDIA_PAUSE, mAction);
			  break;
		  case KEYCODE_ZOOM_IN:
			  injectKeyEvent(KeyEvent.KEYCODE_ZOOM_IN, mAction);
			  break;
		  case KEYCODE_ZOOM_OUT:
			  injectKeyEvent(KeyEvent.KEYCODE_ZOOM_OUT, mAction);
			  break;
		  case KEYCODE_ESCAPE:
			  injectKeyEvent(KeyEvent.KEYCODE_ESCAPE, mAction);
			  break;
	      case KEYCODE_ENTER:
			  injectKeyEvent(KeyEvent.KEYCODE_ENTER, mAction);
			  break;
		  case KEYCODE_SEARCH:
			  injectKeyEvent(KeyEvent.KEYCODE_SEARCH, mAction);
			  break;
		  case KEYCODE_DEL:
			  injectKeyEvent(KeyEvent.KEYCODE_DEL, mAction);
			  break;
		  case KEYCODE_BOOKMARK:
			  injectKeyEvent(KeyEvent.KEYCODE_BOOKMARK, mAction);
			  break;
		  }
	}

	  /**
	   * Called when a mouse movement is received.
	   *
	   * @param xDelta relative movement of the mouse along the x-axis
	   * @param yDelta relative movement of the mouse along the y-axis
	   */
	  public void onMouseEvent(int xDelta, int yDelta) {
		  //if(DEBUG)Log.d(TAG, " xdelta " + xDelta + " ydelta " + yDelta); 
		  mVmouseController.dispatchMouseEvent(xDelta, yDelta, 2);
	}

	  /**
	   * Called when a mouse wheel event is received.
	   *
	   * @param xScroll the scrolling along the x-axis
	   * @param yScroll the scrolling along the x-axis
	   */
	  public void onMouseWheel(int xScroll, int yScroll) {
		  if(DEBUG)Log.d(TAG, " xscroll " + xScroll + " yscroll " + yScroll); 
		  mVmouseController.dispatchMouseEvent(xScroll, yScroll, 3);
	}

	  /**
	   * A general way to send data to the server.
	   * <p>
	   * The interpretation of this is up to the server.
	   *
	   * @param data the data represented as a string
	   */
	  public void onData(String type, String data) {
		  if(DEBUG)Log.d(TAG, " type " + type + " data " + data + " key  unicode " + data.codePointAt(0));
		  IInputContext inputContext;  
		  if (data.length() > 1){
		  	  try {
		  	  inputContext = service.getInputContext();
			  if (inputContext != null){
			  	  Log.d(TAG, " commit text in ondata  ");
				  inputContext.commitText(data, 1);
			  }
		  	  }catch (Exception e) {
				  e.printStackTrace();
			  }
			  return;
		  }
		  char key = data.charAt(0);
		  int code = 0;
		  if (key >= 'a' && key <= 'z')
		  {
			  code = key - 'a' + KeyEvent.KEYCODE_A;
			  injectKeyEvent(code, KeyEvent.ACTION_DOWN);
			  injectKeyEvent(code, KeyEvent.ACTION_UP);
			  return;
		  } 
		  try{
		  inputContext = service.getInputContext();
		  if (inputContext != null){
			  if(DEBUG)Log.d(TAG, " commit text in ondata  ");
			  inputContext.commitText(data, 1);
		  }
		  }catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

	}

	  /**
	   * A message sent upon the connection of a device.
	   *
	   * @param connectInfo the connection information sent by a remote device
	   */
	  public void onConnect(ConnectInfo connectInfo) {
	  	  openVmouse();
		  if(DEBUG)Log.d(TAG, " onconnet info " + connectInfo.toString());
	}

	  /**
	   * Called when viewing the URI is requested.
	   *
	   * @param uri URI to be opened
	   * @return {@code true} if succeeded
	   */
	  public boolean onFling(String uri) {
		  if(DEBUG)Log.d(TAG, " intent uri " + uri);
		return false;
	}
	  
	  private void injectKeyEvent(int keycode, int action){
		  if(DEBUG)Log.d(TAG, " inject keyevent code " + keycode + " ation " + action);
		  long now = SystemClock.uptimeMillis();
		  final KeyEvent event = new KeyEvent(now, now, action, keycode, 0, 0, KeyCharacterMap.VIRTUAL_KEYBOARD, 0, 
				  KeyEvent.FLAG_FROM_SYSTEM | KeyEvent.FLAG_VIRTUAL_HARD_KEY, InputDevice.SOURCE_KEYBOARD);
		  InputManager.getInstance().injectInputEvent(event, InputManager.INJECT_INPUT_EVENT_MODE_ASYNC);
	  }

	  public void openVmouse()   
	  {
	  	mVmouseController.openVmouse();  
	  }

	  public void closeVmouse()
	  {
	  	mVmouseController.closeVmouse();
	  }

}
