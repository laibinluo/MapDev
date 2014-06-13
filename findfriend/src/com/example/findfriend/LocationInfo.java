package com.example.findfriend;

import android.app.Activity;
import android.os.Bundle;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;

import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

//公司地址： 纬度 31.206558  经度 121.588155

public class LocationInfo extends Activity {
	private String TAG = "findfirend";
    private EditText mEditText = null;
    private Button connectButton = null;
    private Button sendButton = null;
    private int BUFF_SIZE = 512;
    
    private Socket socket = null;
    private OutputStream outStream = null;
    
    private static final String IP = "192.168.33.110";
    private static final int port = 6666;
    
	protected void onCreate(Bundle savedInstanceState){
		super.onCreate(savedInstanceState);
		
        setContentView(R.layout.location_info);
        
        mEditText = (EditText)this.findViewById(R.id.edittext);
        connectButton = (Button)this.findViewById(R.id.connectbutton);
        sendButton = (Button)this.findViewById(R.id.sendbutton);
        sendButton.setEnabled(false);
        
        final InetSocketAddress addr = new InetSocketAddress(IP, port); //创建socket
        socket = new Socket();
        
        //连接按钮监听
        connectButton.setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		try {
					socket.connect(addr);
					ByteArrayOutputStream outSteam = new ByteArrayOutputStream(); 
					byte [] receive = new byte[BUFF_SIZE];
					outSteam.write(receive, 0, BUFF_SIZE);
							
	        		socket.getInputStream().read(receive);
	        	
	        		String strTmp = new String(receive);
	        		mEditText.setText(strTmp);
	        		Log.v(TAG, strTmp);
	        		
				} catch (IOException e) {
					// TODO Auto-generated catch block
					Log.v(TAG, "connect or read error");
					e.printStackTrace();
				} 
        		
        		try {
					socket.close();
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
        	}
        });
        
        //发送数据按钮监听
        sendButton.setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		try {
					socket.connect(addr);
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
        		
        	}
        });
	}
	
    public void onDestroy()
    {
        super.onDestroy();
        try {
			socket.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
    }
}
