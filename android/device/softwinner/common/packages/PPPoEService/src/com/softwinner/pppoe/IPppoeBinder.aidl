package com.softwinner.pppoe;

interface IPppoeBinder{
    boolean isConnect();
    void connect(String iface, String username);
}