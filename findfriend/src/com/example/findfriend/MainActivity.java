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
	
	//�ҵ�λ�ø�����  
	private MyLocationOverlay myLocationOverlay; 
	//λ����ͼ���е�����  
	private int myOverlayIndex=0;
	//�Ƿ�λ���ҵ�λ�� 
	private boolean bmyLocal=true;  
	
	StringBuffer sb = new StringBuffer(256);
	
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
		mLocationClient=new LocationClient(getApplicationContext());
		
		myLocationOverlay=new MyLocationOverlay(mMapView);

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
		mLocationClient.registerLocationListener(myListener);
	}
	
	public void setLocationInfo(BDLocation location, boolean Poi){
		sb.setLength(0);
		sb.append("��ǰʱ�� : ");
	      sb.append(location.getTime());
	      sb.append("\n������ : ");
	      sb.append(location.getLocType());
	      sb.append("\nγ�� : ");
	      sb.append(location.getLatitude());
	      sb.append("\n���� : ");
	      sb.append(location.getLongitude());
	      sb.append("\n�뾶 : ");
	      sb.append(location.getRadius());
	      if(Poi){
	          if (location.getLocType() == BDLocation.TypeNetWorkLocation){
	              sb.append("\n��ַ : ");
	              sb.append(location.getAddrStr());
	         } 
	          if(location.hasPoi()){
	               sb.append("\n��Ȥ��:");
	               sb.append(location.getPoi());
	         }else{             
	               sb.append("û����Ȥ��");
	          }
	      }else{
		      if (location.getLocType() == BDLocation.TypeGpsLocation){
		           sb.append("\nspeed : ");
		           sb.append(location.getSpeed());
		           sb.append("\nsatellite : ");
		           sb.append(location.getSatelliteNumber());
		           } else if (location.getLocType() == BDLocation.TypeNetWorkLocation){
		           sb.append("\n��ַ  : ");
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
	    	//�����¸��汾��ȥ��poi����
	         if (poiLocation == null){
	                return ;
	          }
	         
	         setLocationInfo(poiLocation, true);
	         Log.v("��ַλ��", sb.toString());
	       }
	}
	//���ͻ����MENU��ť��ʱ�򣬵��ø÷���
	public boolean onCreateOptionsMenu(Menu menu) { 
		 menu.add(0, 1, 1, R.string.location);
		 menu.add(0, 2, 2, R.string.info);
		 return super.onCreateOptionsMenu(menu);
	}
	
	//����˵�
	public boolean onOptionsItemSelected(MenuItem item)  
	{
		switch(item.getItemId())
		{
		case 1: //�ҵ�λ��
			bmyLocal=true;  
			//����ͻ��˶�λ�����Ѿ��������ˣ���ֱ�ӷ���λ����
			if(mLocationClient != null && mLocationClient.isStarted()) 
			{
				 mLocationClient.requestLocation();  
			}
			else
			{
				//������λ����������ʱ����Զ�����λ����Ĭ��ΪrequestLocation
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
