package com.zd.UDP;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.DatagramSocket;

public class ServerCommunicationRunnable implements Runnable{
    private DatagramSocket socket;
    private BufferedReader reader;
    private String ServerIP;
    private int ServerPort;

    public ServerCommunicationRunnable(String IP, int Port){
        this.ServerPort = Port;
        this.ServerIP = IP;
        try{
            socket = new DatagramSocket(Port);
            reader = new BufferedReader(new InputStreamReader(System.in));
        } catch (Exception e) {
            e.printStackTrace();
        }

    }

    @Override
    public void run() {

    }
}
