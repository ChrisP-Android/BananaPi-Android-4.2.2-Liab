package com.softwinner.nfs;

import java.util.ArrayList;

/**
 * NFS Server wrapper
 * @author A
 *
 */
public class NFSServer {
	private ArrayList<NFSFolder> folderList;		// Shared folder list
	private String serverIP = "";					// NFS server ip
	private String serverHostname = "";				// NFS server hostname, in nfs, this field current not used
	
	
	public NFSServer() {
		folderList = new ArrayList<NFSFolder>();
	}


	public ArrayList<NFSFolder> getFolderList() {
		return folderList;
	}


	public void setFolderList(ArrayList<NFSFolder> folderList) {
		this.folderList = folderList;
	}


	public String getServerIP() {
		return serverIP;
	}


	public void setServerIP(String serverIP) {
		this.serverIP = serverIP;
	}


	public String getServerHostname() {
		return serverHostname;
	}


	public void setServerHostname(String serverHostname) {
		this.serverHostname = serverHostname;
	}
	
	
}
