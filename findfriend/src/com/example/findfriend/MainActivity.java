package com.example.findfriend;

import android.os.Bundle;
import android.app.Activity;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.app.Activity;
import android.content.res.Configuration;
import android.os.Bundle;
import android.view.Menu;
import android.widget.FrameLayout;
import android.widget.Toast;
import com.baidu.mapapi.BMapManager;
import com.baidu.mapapi.map.LocationData;
import com.baidu.mapapi.map.MKMapViewListener;
import com.baidu.mapapi.map.MapController;
import com.baidu.mapapi.map.MapPoi;
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
	//我的位置覆盖物  
	private MyLocationOverlay myOverlay; 
	//位置在图层中的索引  
	private int myOverlayIndex=0;
	//是否定位到我的位置 
	private boolean bmyLocal=true;  
	
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
		mLocationClient=new LocationClient(this);
		
		myOverlay=new MyLocationOverlay(mMapView);
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
		mLocationClient.registerLocationListener(locationListener);
		if(mLocationClient!=null&&!mLocationClient.isStarted()) 
		{
			mLocationClient.requestLocation(); 
			mLocationClient.start(); 
		}
		else
			Log.e("LocSDK3", "locClient is null or not started"); 
	}
	
	private BDLocationListener locationListener=new BDLocationListener() 
	{
		public void onReceiveLocation(BDLocation arg0) {
			 Dispose(arg0);  
		}
		
		public void onReceivePoi(BDLocation arg0) {
			Dispose(arg0); 
		}
		
		private void Dispose(BDLocation location)
		{
			if(location==null)
				return;
			 StringBuffer sb = new StringBuffer(256);
			 sb.append("time : ");
			 sb.append(location.getTime());
			 sb.append("\nerror code : ");
			 sb.append(location.getLocType());
			 sb.append("\nlatitude : ");
			 sb.append(location.getLatitude());
			 sb.append("\nlontitude : ");
			 sb.append(location.getLongitude());
			 sb.append("\nradius : ");
			 sb.append(location.getRadius());
			 if (location.getLocType() == BDLocation.TypeGpsLocation){
				 sb.append("\nspeed : ");
				 sb.append(location.getSpeed());
				 sb.append("\nsatellite : ");
				 sb.append(location.getSatelliteNumber());
			 } 
			 else if (location.getLocType() == BDLocation.TypeNetWorkLocation){
				 sb.append("\naddr : ");
				 sb.append(location.getAddrStr());
			 }
			 //poiLocation
			 if(location.hasPoi()){
				 sb.append("\nPoi:");
				 sb.append(location.getPoi());
				 }else{
				 sb.append("\nnoPoi information");
			} 
			 //需要定位到我的位置？
			 if(bmyLocal)
			 {
				 double lat=location.getLatitude();
				 double lon=location.getLongitude();
				 
				
				 LocationData data=new LocationData();
				 data.latitude=lat;
				 data.longitude=lon;
				 data.direction=2.0f;
				 myOverlay.setData(data);
				 //检查覆盖物是否存在，存在则修改，否则则添加
				 if(mMapView.getOverlays().contains(myOverlay))
				 {
					 mMapView.getOverlays().set(myOverlayIndex,myOverlay);
				 }
				 else{
					 myOverlayIndex=mMapView.getOverlays().size();
					 mMapView.getOverlays().add(myOverlay);	
				 }
				 
				 
				 GeoPoint geoPoint=new GeoPoint((int)(lat* 1E6),(int)(lon* 1E6));				 
				 mMapView.getController().setCenter(geoPoint);
				 			 
				 mMapView.refresh();
				 bmyLocal=false;
			 }
			 Log.e("定位结果：",sb.toString());
		}
	};
	
	//处理菜单
	public boolean onOptionsItemSelected(MenuItem item)  
	{
		switch(item.getItemId())
		{
		case 1: //我的位置
			bmyLocal=true;  
			//如果客户端定位服务已经启动过了，则直接发起定位请求
			if(mLocationClient!=null&&mLocationClient.isStarted()) 
			{
				 mLocationClient.requestLocation();  
			}
			else
			{
				//启动定位服务，启动的时候会自动发起定位请求，默认为requestLocation
				mLocationClient.start(); 
			}
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
