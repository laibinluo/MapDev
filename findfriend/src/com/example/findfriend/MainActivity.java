package com.example.findfriend;

import android.os.Bundle;
import android.app.Activity;
import android.content.Intent;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import com.baidu.mapapi.BMapManager;
import com.baidu.mapapi.map.LocationData;
import com.baidu.mapapi.map.MapController;
import com.baidu.mapapi.map.MapView;
import com.baidu.mapapi.map.MyLocationOverlay;
import com.baidu.platform.comapi.basestruct.GeoPoint;
import com.baidu.location.BDLocation;
import com.baidu.location.BDLocationListener;
import com.baidu.location.LocationClient;
import com.baidu.location.LocationClientOption;


public class MainActivity extends Activity {
	BMapManager mBMapMan = null;  
	MapView mMapView = null;  

	private LocationClient mLocationClient=null;
	public BDLocationListener myListener = new MyLocationListener();
	
	//我的位置覆盖物  
	private MyLocationOverlay myLocationOverlay; 
	//位置在图层中的索引  
	private int myOverlayIndex=0;
	//是否定位到我的位置 
	private boolean bmyLocal=true;  
	
	StringBuffer sb = new StringBuffer(256);
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		mBMapMan=new BMapManager(getApplication());
		mBMapMan.init(null); 
		//注意：请在试用setContentView前初始化BMapManager对象，否则会报错
		setContentView(R.layout.activity_main);
		mMapView=(MapView)findViewById(R.id.bmapsView);
		mMapView.setBuiltInZoomControls(true);
		//设置启用内置的缩放控件
		MapController mMapController=mMapView.getController();
		// 得到mMapView的控制权,可以用它控制和驱动平移和缩放
		GeoPoint point =new GeoPoint((int)(39.915* 1E6),(int)(116.404* 1E6));
		//用给定的经纬度构造一个GeoPoint，单位是微度 (度 * 1E6)
		mMapController.setCenter(point);//设置地图中心点
		mMapController.setZoom(12);//设置地图zoom级别
		
		////////////////////////定位功能代码开始
		mLocationClient=new LocationClient(getApplicationContext());
		
		myLocationOverlay=new MyLocationOverlay(mMapView);

		LocationClientOption option=new LocationClientOption();
		option.setOpenGps(true);
		option.setAddrType("all");//返回的定位结果包含地址信息 
		option.setCoorType("bd09ll");//返回的定位结果是百度经纬度,默认值gcj02
		
		//当不设此项，或者所设的整数值小于1000（ms）时，采用一次定位模式。 
		//option.setScanSpan(5000);//设置发起定位请求的间隔时间为5000ms
		option.disableCache(true);//禁止启用缓存定位 
		option.setPoiNumber(5); //最多返回POI个数 
		option.setPoiDistance(1000); //poi查询距离 
		option.setPoiExtraInfo(true); //是否需要POI的电话和地址等详细信息
		mLocationClient.setLocOption(option);
		//注册位置监听
		mLocationClient.registerLocationListener(myListener);
	}
	
	public void setLocationInfo(BDLocation location, boolean Poi){
		sb.setLength(0);
		sb.append("当前时间 : ");
	      sb.append(location.getTime());
	      sb.append("\n错误码 : ");
	      sb.append(location.getLocType());
	      sb.append("\n纬度 : ");
	      sb.append(location.getLatitude());
	      sb.append("\n经度 : ");
	      sb.append(location.getLongitude());
	      sb.append("\n半径 : ");
	      sb.append(location.getRadius());
	      if(Poi){
	          if (location.getLocType() == BDLocation.TypeNetWorkLocation){
	              sb.append("\n地址 : ");
	              sb.append(location.getAddrStr());
	         } 
	          if(location.hasPoi()){
	               sb.append("\n兴趣点:");
	               sb.append(location.getPoi());
	         }else{             
	               sb.append("没有兴趣点");
	          }
	      }else{
		      if (location.getLocType() == BDLocation.TypeGpsLocation){
		           sb.append("\nspeed : ");
		           sb.append(location.getSpeed());
		           sb.append("\nsatellite : ");
		           sb.append(location.getSatelliteNumber());
		           } else if (location.getLocType() == BDLocation.TypeNetWorkLocation){
		           sb.append("\n地址  : ");
		           sb.append(location.getAddrStr());
		        } 
	      }
	}
	
	public class MyLocationListener implements BDLocationListener {
	    @Override
	   public void onReceiveLocation(BDLocation location) {
	      if (location == null)
	          return ;
	      setLocationInfo(location, false);
	      
	       
	      LocationData locData = new LocationData();
	      
	      locData.latitude = location.getLatitude();
	      locData.longitude = location.getLongitude();
	      locData.direction = 2.0f;
	      myLocationOverlay.setData(locData); 
	      if(mMapView.getOverlays().contains(myLocationOverlay)){
	    	   mMapView.getOverlays().set(myOverlayIndex,myLocationOverlay);
	      }else{
	    	   myOverlayIndex = mMapView.getOverlays().size();
	    	   mMapView.getOverlays().add(myLocationOverlay);
	      }
	      mMapView.refresh();
	      mMapView.getController().animateTo(new GeoPoint((int)(locData.latitude*1e6), (int)(locData.longitude* 1e6)));  
	    }
	    public void onReceivePoi(BDLocation poiLocation) {
	    	//将在下个版本中去除poi功能
	         if (poiLocation == null){
	                return ;
	          }
	         
	         setLocationInfo(poiLocation, true);
	         Log.v("地址位置", sb.toString());
	       }
	}
	//当客户点击MENU按钮的时候，调用该方法
	public boolean onCreateOptionsMenu(Menu menu) { 
		 menu.add(0, 1, 1, R.string.location);
		 menu.add(0, 2, 2, R.string.info);
		 return super.onCreateOptionsMenu(menu);
	}
	
	//处理菜单
	public boolean onOptionsItemSelected(MenuItem item)  
	{
		switch(item.getItemId())
		{
		case 1: //我的位置
			bmyLocal=true;  
			//如果客户端定位服务已经启动过了，则直接发起定位请求
			if(mLocationClient != null && mLocationClient.isStarted()) 
			{
				 mLocationClient.requestLocation();  
			}
			else
			{
				//启动定位服务，启动的时候会自动发起定位请求，默认为requestLocation
				mLocationClient.start(); 
			}
			break;
		case 2:
			Intent intent = new Intent();
			intent.setClass(MainActivity.this, LocationInfo.class);
			
			Bundle bundle = new Bundle();
			bundle.putCharSequence("info", sb);
			intent.putExtras(bundle);
			startActivity(intent);
			break;
		}
		return true;
	}
	
	@Override
	protected void onDestroy(){
		if(mLocationClient!=null&&mLocationClient.isStarted()) 
			mLocationClient.stop(); 
        mMapView.destroy();
        if(mBMapMan!=null){
                mBMapMan.destroy();
                mBMapMan=null;
        }
        super.onDestroy();
	}
	@Override
	protected void onPause(){
        mMapView.onPause();
        if(mBMapMan!=null){
            mBMapMan.stop();
        }
        super.onPause();
	}
	@Override
	protected void onResume(){
        mMapView.onResume();
        if(mBMapMan!=null){
        	mBMapMan.start();
        }
        super.onResume();
	}
}
