/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.settings.ethernet;

import com.android.settings.R;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.security.Credentials;
import android.security.KeyStore;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.EditText;
import android.net.ethernet.IEthernetManager;
import android.net.ethernet.EthernetManager;
import android.net.ethernet.EthernetDevInfo;
import android.net.InterfaceConfiguration;
import android.os.INetworkManagementService;
import android.util.Log;

import java.net.InetAddress;

public class EthernetDialog extends AlertDialog implements TextWatcher,
        View.OnClickListener, AdapterView.OnItemSelectedListener {

    public static final String TAG = "EthernetDialog";
    private final KeyStore mKeyStore = KeyStore.getInstance();
    private final DialogInterface.OnClickListener mListener;
	private EthernetDevInfo mInterfaceInfo;

    private boolean mEditing;

    private View mView;

    private CheckBox mDhcp_choice;
    private EditText mIpaddr;
    private EditText mMask;
    private EditText mDns;
    private EditText mGw;
    private EditText mMacaddr;

    public EthernetDialog(Context context, DialogInterface.OnClickListener listener, EthernetDevInfo interfaceinfo, boolean editing) {
        super(context);
        mListener = listener;
        mEditing = editing;
		mInterfaceInfo = interfaceinfo;
		
		if(mInterfaceInfo != null){
		}
    }

    @Override
    protected void onCreate(Bundle savedState) {
		mView = getLayoutInflater().inflate(R.layout.ethernet_dialog, null);
		setTitle(R.string.eth_advanced_configure);
		setView(mView);
		setInverseBackgroundForced(true);
		Context context = getContext();

		// First, find out all the fields.
		mDhcp_choice = (CheckBox) mView.findViewById(R.id.dhcp_choice);
		mIpaddr = (EditText) mView.findViewById(R.id.ipaddr_edit);
		mMask = (EditText) mView.findViewById(R.id.netmask_edit);
		mGw = (EditText) mView.findViewById(R.id.gw_edit);
		mDns = (EditText) mView.findViewById(R.id.dns_edit);
		mMacaddr= (EditText) mView.findViewById(R.id.macaddr_edit);

		// Second, copy values from the profile.
		mIpaddr.setText(mInterfaceInfo.getIpAddress());
		mMask.setText(mInterfaceInfo.getNetMask());
		mGw.setText(mInterfaceInfo.getGateWay());
		mDns.setText(mInterfaceInfo.getDnsAddr());
		mMacaddr.setText(mInterfaceInfo.getHwaddr().toUpperCase());
		if(mInterfaceInfo.getConnectMode() == EthernetDevInfo.ETHERNET_CONN_MODE_DHCP){
			mDhcp_choice.setChecked(true);
			mIpaddr.setEnabled(false);
			mMask.setEnabled(false);
			mGw.setEnabled(false);
			mDns.setEnabled(false);
		}else{
			mDhcp_choice.setChecked(false);
		}
		mMacaddr.setEnabled(false);

		// Third, add listeners to required fields.
		mDhcp_choice.setOnClickListener(this);
		mIpaddr.addTextChangedListener(this);
		mMask.addTextChangedListener(this);
		mGw.addTextChangedListener(this);
		mDns.addTextChangedListener(this);

		setButton(DialogInterface.BUTTON_NEGATIVE, context.getString(R.string.eth_cancel), mListener);
		setButton(DialogInterface.BUTTON_POSITIVE, context.getString(R.string.eth_ok), mListener);
		setButton(DialogInterface.BUTTON_NEUTRAL, context.getString(R.string.eth_advand), mListener);
		// Workaround to resize the dialog for the input method.
		super.onCreate(savedState);

		if(mEditing){
			mView.findViewById(R.id.eth_conf_editor).setVisibility(View.VISIBLE);
			getButton(DialogInterface.BUTTON_NEUTRAL).setVisibility(View.GONE);
			getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE |
					WindowManager.LayoutParams.SOFT_INPUT_STATE_VISIBLE);
		}else{
			mView.findViewById(R.id.eth_message_dialog).setVisibility(View.VISIBLE);
			getButton(DialogInterface.BUTTON_NEUTRAL).setEnabled(true);
		}
    }

    @Override
    public void afterTextChanged(Editable field) {
        //getButton(DialogInterface.BUTTON_POSITIVE).setEnabled(validate(mEditing));
    }

    @Override
    public void beforeTextChanged(CharSequence s, int start, int count, int after) {
    }

    @Override
    public void onTextChanged(CharSequence s, int start, int before, int count) {
    }

    @Override
    public void onClick(View v) {

		if(mDhcp_choice.isChecked()){
			mIpaddr.setEnabled(false);
			mMask.setEnabled(false);
			mDns.setEnabled(false);
			mGw.setEnabled(false);
		}else{
			mIpaddr.setEnabled(true);
			mMask.setEnabled(true);
			mDns.setEnabled(true);
			mGw.setEnabled(true);
		}
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {
    }

    private boolean validate(boolean editing) {
		/*
        if (!editing) {
            return mUsername.getText().length() != 0 && mPassword.getText().length() != 0;
        }
        if (mName.getText().length() == 0 || mServer.getText().length() == 0 ||
                !validateAddresses(mDnsServers.getText().toString(), false) ||
                !validateAddresses(mRoutes.getText().toString(), true)) {
            return false;
        }
        switch (mType.getSelectedItemPosition()) {
            case VpnProfile.TYPE_PPTP:
            case VpnProfile.TYPE_IPSEC_HYBRID_RSA:
                return true;

            case VpnProfile.TYPE_L2TP_IPSEC_PSK:
            case VpnProfile.TYPE_IPSEC_XAUTH_PSK:
                return mIpsecSecret.getText().length() != 0;

            case VpnProfile.TYPE_L2TP_IPSEC_RSA:
            case VpnProfile.TYPE_IPSEC_XAUTH_RSA:
                return mIpsecUserCert.getSelectedItemPosition() != 0;
        }
		*/
        return false;
    }

    private boolean validateAddresses(String addresses, boolean cidr) {
        try {
            for (String address : addresses.split(" ")) {
                if (address.isEmpty()) {
                    continue;
                }
                // Legacy VPN currently only supports IPv4.
                int prefixLength = 32;
                if (cidr) {
                    String[] parts = address.split("/", 2);
                    address = parts[0];
                    prefixLength = Integer.parseInt(parts[1]);
                }
                byte[] bytes = InetAddress.parseNumericAddress(address).getAddress();
                int integer = (bytes[3] & 0xFF) | (bytes[2] & 0xFF) << 8 |
                        (bytes[1] & 0xFF) << 16 | (bytes[0] & 0xFF) << 24;
                if (bytes.length != 4 || prefixLength < 0 || prefixLength > 32 ||
                        (prefixLength < 32 && (integer << prefixLength) != 0)) {
                    return false;
                }
            }
        } catch (Exception e) {
            return false;
        }
        return true;
    }

    boolean isEditing() {
        return mEditing;
    }

    public EthernetDevInfo getDevInfo() {
        // First, save common fields.
		if(mEditing){
			if(!mDhcp_choice.isChecked()){
				mInterfaceInfo.setConnectMode(EthernetDevInfo.ETHERNET_CONN_MODE_MANUAL);
				mInterfaceInfo.setIpAddress(mIpaddr.getText().toString());
				mInterfaceInfo.setNetMask(mMask.getText().toString());
				mInterfaceInfo.setDnsAddr(mDns.getText().toString());
				mInterfaceInfo.setGateWay(mGw.getText().toString());
			}else{
				mInterfaceInfo.setConnectMode(EthernetDevInfo.ETHERNET_CONN_MODE_DHCP);
			}
		}

        return mInterfaceInfo;
    }
}
