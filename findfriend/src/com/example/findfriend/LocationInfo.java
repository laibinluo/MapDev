package com.example.findfriend;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;


//公司地址： 纬度 31.206558  经度 121.588155

public class LocationInfo extends Activity {
	protected void onCreate(Bundle savedInstanceState){
		super.onCreate(savedInstanceState);
		setContentView(R.layout.location_info);
		
		Bundle bundle = this.getIntent().getExtras();
		CharSequence  sb = bundle.getCharSequence("info");
		
		TextView tvInfo = (TextView) findViewById(R.id.info);
		tvInfo.setText(sb);
	}
}
