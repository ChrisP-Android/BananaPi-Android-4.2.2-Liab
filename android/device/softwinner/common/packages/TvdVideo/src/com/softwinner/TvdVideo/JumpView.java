package com.softwinner.TvdVideo;

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.text.Editable;
import android.text.InputType;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnKeyListener;
import android.widget.Button;
import android.widget.EditText;

public class JumpView {
	OnTimeConfirmListener mOnTimeConfirmListener;
	DialogInterface.OnCancelListener mOnCancelListener;
	int mTimeMax;
	int mHourMax, mMinMax, mSecMax;
	View mJumpView;
	Context mContext;
	Dialog mJumpDialog;
	EditText mJumpHour, mJumpMin, mJumpSec;
	Editable mEditableHour, mEditableMin, mEditableSec;
	Button mConfirm;

	public interface OnTimeConfirmListener {
		void onTimeConfirm(int time);
	}

	public JumpView(Context context, int time,
			OnTimeConfirmListener confirmListener) {
		mOnTimeConfirmListener = confirmListener;
		mHourMax = time / 3600;
		mMinMax = (time / 60) % 60;
		mSecMax = time % 60;
		mTimeMax = time;
		mContext = context;
		mJumpView = View.inflate(mContext, R.layout.dialog_jump, null);

		mJumpHour = (EditText) mJumpView.findViewById(R.id.jump_hour);
		mJumpMin = (EditText) mJumpView.findViewById(R.id.jump_min);
		mJumpSec = (EditText) mJumpView.findViewById(R.id.jump_sec);
		mConfirm = (Button) mJumpView.findViewById(R.id.jump_confirm);
		String timeString = String.format("%02d", mHourMax);
		mJumpHour.setText(timeString);
		timeString = String.format("%02d", mMinMax);
		mJumpMin.setText(timeString);
		timeString = String.format("%02d", mSecMax);
		mJumpSec.setText(timeString);
		mEditableHour = mJumpHour.getEditableText();
		mEditableMin = mJumpMin.getEditableText();
		mEditableSec = mJumpSec.getEditableText();
		mJumpHour.setOnKeyListener(mJumpHourKeyListener);
		mJumpMin.setOnKeyListener(mJumpMinKeyListener);
		mJumpSec.setOnKeyListener(mJumpSecKeyListener);
		mConfirm.setOnKeyListener(mJumpConfirmKeyListener);
		mConfirm.setOnClickListener(mJumpConfirmClickListener);
		mJumpHour.setOnFocusChangeListener(OnHourFocusChangeListener);
		mJumpMin.setOnFocusChangeListener(OnMinFocusChangeListener);
		mJumpSec.setOnFocusChangeListener(OnSecFocusChangeListener);
		mJumpHour.requestFocus();
		mJumpHour.setSelection(0, 1);

		mJumpDialog = new Dialog(mContext, R.style.jump);
		mJumpDialog.setContentView(mJumpView);
		mJumpDialog.show();
	}

	public void setOnCancelListener(
			DialogInterface.OnCancelListener cancelListener) {
		mOnCancelListener = cancelListener;
		mJumpDialog.setOnCancelListener(mOnCancelListener);
	}

	public void jumpViewDismiss() {
		if (mJumpDialog != null) {
			mJumpDialog.dismiss();
			mJumpDialog = null;
		}
	}

	View.OnFocusChangeListener OnHourFocusChangeListener = new View.OnFocusChangeListener() {
		@Override
		public void onFocusChange(View v, boolean hasFocus) {
			if (hasFocus) {
				mJumpHour.setInputType(InputType.TYPE_NULL);
				mJumpHour.setSelection(0, 1);
				return;
			}
		}
	};

	View.OnFocusChangeListener OnMinFocusChangeListener = new View.OnFocusChangeListener() {
		@Override
		public void onFocusChange(View v, boolean hasFocus) {
			if (hasFocus) {
				mJumpMin.setInputType(InputType.TYPE_NULL);
				mJumpMin.setSelection(0, 1);
				return;
			}
		}
	};

	View.OnFocusChangeListener OnSecFocusChangeListener = new View.OnFocusChangeListener() {
		@Override
		public void onFocusChange(View v, boolean hasFocus) {
			if (hasFocus) {
				mJumpSec.setInputType(InputType.TYPE_NULL);
				mJumpSec.setSelection(0, 1);
				return;
			}
		}
	};

	View.OnKeyListener mJumpHourKeyListener = new OnKeyListener() {

		@Override
		public boolean onKey(View v, int keyCode, KeyEvent event) {
			// TODO Auto-generated method stub
			if (keyCode == KeyEvent.KEYCODE_BACK) {
				return false;
			} else if (keyCode == KeyEvent.KEYCODE_DPAD_CENTER) {
				int time = getCurrentTime();
				mOnTimeConfirmListener.onTimeConfirm(time);
			}

			if (event.getAction() == KeyEvent.ACTION_DOWN) {
				if (keyCode == KeyEvent.KEYCODE_DPAD_RIGHT) {
					int index = mJumpHour.getSelectionStart();
					if (index != 0) {
						mJumpMin.requestFocus();
					} else {
						mJumpHour.setSelection(1, 2);
					}
				} else if (keyCode == KeyEvent.KEYCODE_DPAD_LEFT) {
					int index = mJumpHour.getSelectionStart();
					if (index == 0) {
						mConfirm.requestFocus();
					} else {
						mJumpHour.setSelection(0, 1);
					}
				} else if (keyCode == KeyEvent.KEYCODE_DPAD_UP) {
					if (mHourMax == 0) {
						return true;
					}
					int index = mJumpHour.getSelectionStart();
					int time = getCurrentTime();
					int hour = Integer.parseInt(mJumpHour.getText().toString());
					if (index == 0) {
						if (hour / 10 == 9) {
							hour = hour % 10;
						} else {
							hour = hour + 10;
						}
					} else {
						if (hour % 10 == 9) {
							hour = hour / 10 * 10;
						} else {
							hour++;
						}
					}
					time = hour * 3600 + time - time / 3600 * 3600;
					String timeString = String.format("%02d", hour);
					mJumpHour.setText(String.valueOf(timeString));
					mJumpHour.setSelection(index, index + 1);
				} else if (keyCode == KeyEvent.KEYCODE_DPAD_DOWN) {
					if (mHourMax == 0) {
						return true;
					}
					int index = mJumpHour.getSelectionStart();
					int time = getCurrentTime();
					int hour = Integer.parseInt(mJumpHour.getText().toString());
					if (index == 0) {
						if (hour / 10 == 0) {
							hour = 90 + hour % 10;
						} else {
							hour = hour - 10;
						}
					} else {
						if (hour % 10 == 0) {
							hour = hour / 10 * 10 + 9;
						} else {
							hour--;
						}
					}
					time = hour * 3600 + time - time / 3600 * 3600;
					String timeString = String.format("%02d", hour);
					mJumpHour.setText(String.valueOf(timeString));
					mJumpHour.setSelection(index, index + 1);
				} else if (isNumKey(keyCode)) {
					int hour = Integer.parseInt(mJumpHour.getText().toString());
					int time = getCurrentTime();
					int index = mJumpHour.getSelectionStart();
					if (index == 0) {
						hour = (keyCode - KeyEvent.KEYCODE_0) * 10 + hour % 10;
					} else {
						hour = hour / 10 * 10 + (keyCode - KeyEvent.KEYCODE_0);
					}
					time = hour * 3600 + time - time / 3600 * 3600;
					String timeString = String.format("%02d", hour);
					mJumpHour.setText(String.valueOf(timeString));
					mJumpHour.setSelection(index, index + 1);
				}
			}

			return true;
		}
	};

	View.OnKeyListener mJumpMinKeyListener = new OnKeyListener() {

		@Override
		public boolean onKey(View v, int keyCode, KeyEvent event) {
			// TODO Auto-generated method stub
			if (keyCode == KeyEvent.KEYCODE_BACK) {
				return false;
			} else if (keyCode == KeyEvent.KEYCODE_DPAD_CENTER) {
				int time = getCurrentTime();
				mOnTimeConfirmListener.onTimeConfirm(time);
			}

			if (event.getAction() == KeyEvent.ACTION_DOWN) {
				if (keyCode == KeyEvent.KEYCODE_DPAD_RIGHT) {
					int index = mJumpMin.getSelectionStart();
					if (index != 0) {
						mJumpSec.requestFocus();
					} else {
						mJumpMin.setSelection(1, 2);
					}
				} else if (keyCode == KeyEvent.KEYCODE_DPAD_LEFT) {
					int index = mJumpMin.getSelectionStart();
					if (index == 0) {
						mJumpHour.requestFocus();
					} else {
						mJumpMin.setSelection(0, 1);
					}
				} else if (keyCode == KeyEvent.KEYCODE_DPAD_UP) {
					int index = mJumpMin.getSelectionStart();
					int time = getCurrentTime();
					int min = Integer.parseInt(mJumpMin.getText().toString());
					if (index == 0) {
						if (min / 10 == 5) {
							min = min % 10;
						} else {
							min = min + 10;
						}
					} else {
						if (min % 10 == 9) {
							min = min / 10 * 10;
						} else {
							min++;
						}
					}
					time = min * 60 + time - time / 60 % 60 * 60;
					String timeString = String.format("%02d", min);
					mJumpMin.setText(String.valueOf(timeString));
					mJumpMin.setSelection(index, index + 1);
				} else if (keyCode == KeyEvent.KEYCODE_DPAD_DOWN) {
					int index = mJumpMin.getSelectionStart();
					int time = getCurrentTime();
					int min = Integer.parseInt(mJumpMin.getText().toString());
					if (index == 0) {
						if (min / 10 == 0) {
							min = 50 + min % 10;
						} else {
							min = min - 10;
						}
					} else {
						if (min % 10 == 0) {
							min = min / 10 * 10 + 9;
						} else {
							min--;
						}
					}
					time = min * 60 + time - time / 60 % 60 * 60;
					String timeString = String.format("%02d", min);
					mJumpMin.setText(String.valueOf(timeString));
					mJumpMin.setSelection(index, index + 1);
				} else if (isNumKey(keyCode)) {
					int min = Integer.parseInt(mJumpMin.getText().toString());
					int time = getCurrentTime();
					int index = mJumpMin.getSelectionStart();
					if (index == 0) {
						min = (keyCode - KeyEvent.KEYCODE_0) * 10 + min % 10;
					} else {
						min = min / 10 * 10 + (keyCode - KeyEvent.KEYCODE_0);
					}
					time = min * 60 + time - time / 60 % 60 * 60;
					String timeString = String.format("%02d", min);
					mJumpMin.setText(String.valueOf(timeString));
					mJumpMin.setSelection(index, index + 1);
				}
			}

			return true;
		}
	};

	View.OnKeyListener mJumpSecKeyListener = new OnKeyListener() {

		@Override
		public boolean onKey(View v, int keyCode, KeyEvent event) {
			// TODO Auto-generated method stub
			if (keyCode == KeyEvent.KEYCODE_BACK) {
				return false;
			} else if (keyCode == KeyEvent.KEYCODE_DPAD_CENTER) {
				int time = getCurrentTime();
				mOnTimeConfirmListener.onTimeConfirm(time);
			}

			if (event.getAction() == KeyEvent.ACTION_DOWN) {
				if (keyCode == KeyEvent.KEYCODE_DPAD_RIGHT) {
					int index = mJumpSec.getSelectionStart();
					if (index != 0) {
						mConfirm.requestFocus();
					} else {
						mJumpSec.setSelection(1, 2);
					}
				} else if (keyCode == KeyEvent.KEYCODE_DPAD_LEFT) {
					int index = mJumpSec.getSelectionStart();
					if (index == 0) {
						mJumpMin.requestFocus();
					} else {
						mJumpSec.setSelection(0, 1);
					}
				} else if (keyCode == KeyEvent.KEYCODE_DPAD_UP) {
					int index = mJumpSec.getSelectionStart();
					int time = getCurrentTime();
					int sec = Integer.parseInt(mJumpSec.getText().toString());
					if (index == 0) {
						if (sec / 10 == 5) {
							sec = sec % 10;
						} else {
							sec = sec + 10;
						}
					} else {
						if (sec % 10 == 9) {
							sec = sec / 10 * 10;
						} else {
							sec++;
						}
					}
					time = sec + time / 60 * 60;
					String timeString = String.format("%02d", sec);
					mJumpSec.setText(String.valueOf(timeString));
					mJumpSec.setSelection(index, index + 1);
				} else if (keyCode == KeyEvent.KEYCODE_DPAD_DOWN) {
					int index = mJumpSec.getSelectionStart();
					int time = getCurrentTime();
					int sec = Integer.parseInt(mJumpSec.getText().toString());
					if (index == 0) {
						if (sec / 10 == 0) {
							sec = 50 + sec % 10;
						} else {
							sec = sec - 10;
						}
					} else {
						if (sec % 10 == 0) {
							sec = sec / 10 * 10 + 9;
						} else {
							sec--;
						}
					}
					time = sec + time / 60 * 60;
					String timeString = String.format("%02d", sec);
					mJumpSec.setText(String.valueOf(timeString));
					mJumpSec.setSelection(index, index + 1);
				} else if (isNumKey(keyCode)) {
					int sec = Integer.parseInt(mJumpSec.getText().toString());
					int time = getCurrentTime();
					int index = mJumpSec.getSelectionStart();
					if (index == 0) {
						sec = (keyCode - KeyEvent.KEYCODE_0) * 10 + sec % 10;
					} else {
						sec = sec / 10 * 10 + (keyCode - KeyEvent.KEYCODE_0);
					}
					time = sec + time / 60 * 60;
					String timeString = String.format("%02d", sec);
					mJumpSec.setText(String.valueOf(timeString));
					mJumpSec.setSelection(index, index + 1);
				}
			}

			return true;
		}
	};

	View.OnKeyListener mJumpConfirmKeyListener = new OnKeyListener() {

		@Override
		public boolean onKey(View v, int keyCode, KeyEvent event) {
			// TODO Auto-generated method stub
			if (keyCode == KeyEvent.KEYCODE_BACK) {
				return false;
			} else if (keyCode == KeyEvent.KEYCODE_DPAD_CENTER) {
				int time = getCurrentTime();
				mOnTimeConfirmListener.onTimeConfirm(time);
			}

			if (event.getAction() == KeyEvent.ACTION_DOWN) {
				if (keyCode == KeyEvent.KEYCODE_DPAD_RIGHT
						|| keyCode == KeyEvent.KEYCODE_DPAD_UP) {
					mJumpHour.requestFocus();
				} else if (keyCode == KeyEvent.KEYCODE_DPAD_LEFT) {
					mJumpSec.requestFocus();
				}
			}

			return true;
		}

	};

	private View.OnClickListener mJumpConfirmClickListener = new OnClickListener() {

		@Override
		public void onClick(View arg0) {
			// TODO Auto-generated method stub
			int time = getCurrentTime();
			mOnTimeConfirmListener.onTimeConfirm(time);
		}
	};

	private boolean isNumKey(int nkey) {
		if (nkey >= KeyEvent.KEYCODE_0 && nkey <= KeyEvent.KEYCODE_9) {
			return true;
		} else {
			return false;
		}
	}

	private int getCurrentTime() {
		int hour = Integer.parseInt(mJumpHour.getText().toString());
		int min = Integer.parseInt(mJumpMin.getText().toString());
		int sec = Integer.parseInt(mJumpSec.getText().toString());

		return (hour * 3600 + min * 60 + sec);
	}
}