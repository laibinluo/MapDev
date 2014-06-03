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
	//�ҵ�λ�ø�����  
	private MyLocationOverlay myOverlay; 
	//λ����ͼ���е�����  
	private int myOverlayIndex=0;
	//�Ƿ�λ���ҵ�λ�� 
	private boolean bmyLocal=true;  
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		mBMapMan=new BMapManager(getApplication());
		mBMapMan.init(null); 
		//ע�⣺��������setContentViewǰ��ʼ��BMapManager���󣬷���ᱨ��
		setContentView(R.layout.activity_main);
		mMapView=(MapView)findViewById(R.id.bmapsView);
		mMapView.setBuiltInZoomControls(true);
		//�����������õ����ſؼ�
		MapController mMapController=mMapView.getController();
		// �õ�mMapView�Ŀ���Ȩ,�����������ƺ�����ƽ�ƺ�����
		GeoPoint point =new GeoPoint((int)(39.915* 1E6),(int)(116.404* 1E6));
		//�ø����ľ�γ�ȹ���һ��GeoPoint����λ��΢�� (�� * 1E6)
		mMapController.setCenter(point);//���õ�ͼ���ĵ�
		mMapController.setZoom(12);//���õ�ͼzoom����
		
		////////////////////////��λ���ܴ��뿪ʼ
		mLocationClient=new LocationClient(this);
		
		myOverlay=new MyLocationOverlay(mMapView);
		LocationClientOption option=new LocationClientOption();
		option.setOpenGps(true);
		option.setAddrType("all");//���صĶ�λ���������ַ��Ϣ 
		option.setCoorType("bd09ll");//���صĶ�λ����ǰٶȾ�γ��,Ĭ��ֵgcj02
		
		//���������������������ֵС��1000��ms��ʱ������һ�ζ�λģʽ�� 
		//option.setScanSpan(5000);//���÷���λ����ļ��ʱ��Ϊ5000ms
		option.disableCache(true);//��ֹ���û��涨λ 
		option.setPoiNumber(5); //��෵��POI���� 
		option.setPoiDistance(1000); //poi��ѯ���� 
		option.setPoiExtraInfo(true); //�Ƿ���ҪPOI�ĵ绰�͵�ַ����ϸ��Ϣ
		mLocationClient.setLocOption(option);
		//ע��λ�ü���
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
			 //��Ҫ��λ���ҵ�λ�ã�
			 if(bmyLocal)
			 {
				 double lat=location.getLatitude();
				 double lon=location.getLongitude();
				 
				
				 LocationData data=new LocationData();
				 data.latitude=lat;
				 data.longitude=lon;
				 data.direction=2.0f;
				 myOverlay.setData(data);
				 //��鸲�����Ƿ���ڣ��������޸ģ����������
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
			 Log.e("��λ�����",sb.toString());
		}
	};
	
	//����˵�
	public boolean onOptionsItemSelected(MenuItem item)  
	{
		switch(item.getItemId())
		{
		case 1: //�ҵ�λ��
			bmyLocal=true;  
			//����ͻ��˶�λ�����Ѿ��������ˣ���ֱ�ӷ���λ����
			if(mLocationClient!=null&&mLocationClient.isStarted()) 
			{
				 mLocationClient.requestLocation();  
			}
			else
			{
				//������λ����������ʱ����Զ�����λ����Ĭ��ΪrequestLocation
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
