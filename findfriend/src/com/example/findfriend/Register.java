package com.example.findfriend;

import android.os.Bundle;
import android.widget.Button;
import android.widget.EditText;
import android.app.Activity;

public class Register extends Activity {
    private EditText mUser = null;
    private EditText mPasswd = null;
    private Button mLoginButton = null;
    private Button mRegisterButton = null;
    
	@Override
	protected void onCreate(Bundle savedInstanceState){
		super.onCreate(savedInstanceState);
		//注意：请在试用setContentView前初始化BMapManager对象，否则会报错
		setContentView(R.layout.register);
		
		mUser = (EditText) findViewById(R.id.user);
		mPasswd = (EditText) findViewById(R.id.passwd);
		
		mLoginButton = (Button)findViewById(R.id.login);
		mRegisterButton = (Button)findViewById(R.id.register);
		
	}
	
}

