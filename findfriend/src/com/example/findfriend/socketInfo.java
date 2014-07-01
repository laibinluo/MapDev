package com.example.findfriend;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;

public class socketInfo {
    private static final String IP = "192.168.33.110";
    private static final int port = 6666;
    private int BUFF_SIZE = 512;
    private Socket socket = null;
    
    final InetSocketAddress addr;
    
    socketInfo(){
    	 addr = new InetSocketAddress(IP, port); //´´½¨socket
    	 socket = new Socket();
    }
    
    void connection() throws IOException{
    	socket.connect(addr);
    }
    
    boolean login(String user, String passwd){
    	if((null == user) || (null == passwd)){
    		return false;
    	}
    	
    	int len = user.length() + passwd.length();
      	//1£ºTCP£¬ R£ºregister, 
    	String strHead = "1R";
    	
    	
    	return true;
    }

    
}
