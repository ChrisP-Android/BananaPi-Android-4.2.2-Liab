package com.android.browser.preferences;

import java.util.ArrayList;
import java.util.HashMap;

import com.android.browser.BrowserSettings;
import com.android.browser.PreferenceKeys;
import com.android.browser.R;
import android.app.Fragment;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.RadioButton;
import android.widget.RadioGroup;

public class BrowserModeFragment extends Fragment implements
		RadioGroup.OnCheckedChangeListener {

	String[] mEntries;
	String[] mEntryValues;

	SharedPreferences mPref;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		mEntries = this.getResources().getStringArray(
				R.array.pref_development_ua_choices);
		mEntryValues = getResources().getStringArray(
				R.array.pref_development_ua_values);
		mPref = BrowserSettings.getInstance().getPreferences();
	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		RadioGroup radioGroup = (RadioGroup) inflater.inflate(
				R.layout.radio_group, null);
		radioGroup.setOnCheckedChangeListener(this);
		LayoutParams lp = new LayoutParams(LayoutParams.MATCH_PARENT,
				LayoutParams.WRAP_CONTENT);
		radioGroup.setLayoutParams(lp);
		String def = mPref.getString(PreferenceKeys.PREF_USER_AGENT,
				"3");
		if (mEntries != null && mEntryValues != null) {
			int length = mEntries.length;
			for (int i = 0; i < length; i++) {				
				RadioButton button = (RadioButton) inflater.inflate(
						R.layout.radio_button, null);
				button.setId(10086 + i);
				button.setText(mEntries[i]);
				button.setTag(mEntryValues[i]);
				button.setLayoutParams(lp);
				radioGroup.addView(button);
				if (def.equals(mEntryValues[i])) {
					button.setChecked(true);
				}
			}
		}
		return radioGroup;
	}

	@Override
	public void onCheckedChanged(RadioGroup group, int checkedId) {
		Log.v("chip", "" + checkedId);
		View child = group.findViewById(checkedId);
		String value = (String) child.getTag();
		Log.v("browser", "" + value);
		mPref.edit().putString(PreferenceKeys.PREF_USER_AGENT, value).apply();
	}
}
