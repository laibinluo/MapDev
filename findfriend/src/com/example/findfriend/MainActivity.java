package com.example.findfriend;

import android.os.Bundle;
import android.app.Activity;
import android.content.Intent;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;

import com.baidu.mapapi.BMapManager;
import com.baidu.mapapi.map.LocationData;
import com.baidu.mapapi.map.MapController;
import com.baidu.mapapi.map.MapView;
import com.baidu.mapapi.map.MyLocationOverlay;
import com.baidu.mapapi.map.RouteOverlay;
import com.baidu.mapapi.map.TransitOverlay;
import com.baidu.mapapi.search.MKAddrInfo;
import com.baidu.mapapi.search.MKBusLineResult;
import com.baidu.mapapi.search.MKDrivingRouteResult;
import com.baidu.mapapi.search.MKPlanNode;
import com.baidu.mapapi.search.MKPoiResult;
import com.baidu.mapapi.search.MKSearch;
import com.baidu.mapapi.search.MKSearchListener;
import com.baidu.mapapi.search.MKShareUrlResult;
import com.baidu.mapapi.search.MKSuggestionResult;
import com.baidu.mapapi.search.MKTransitRouteResult;
import com.baidu.mapapi.search.MKWalkingRouteResult;
import com.baidu.platform.comapi.basestruct.GeoPoint;
import com.baidu.location.BDLocation;
import com.baidu.location.BDLocationListener;
import com.baidu.location.LocationClient;
import com.baidu.location.LocationClientOption;


public class MainActivity extends Activity {
	private static final OnClickListener BusListener = null;
	private LocationData locData = new LocationData();
	private BMapManager mBMapMan = null;  
	private MapView mMapView = null;  
	private double mFriendLatitude = 0;
	private double mFriendLongitude = 0;
	private LocationClient mLocationClient=null;
	public BDLocationListener myListener = new MyLocationListener();
	
	//�ҵ�λ�ø�����  
	private MyLocationOverlay myLocationOverlay; 
	
	private RouteOverlay mWalkingRouteOverlay;
	private TransitOverlay  mBusRouteOverlay;
	private RouteOverlay mCarRouteOverlay;
	//λ����ͼ���е�����  
	private int myOverlayIndex=0;
	private boolean mMyself = true;
	
	StringBuffer sb = new StringBuffer(256);
	private Button btnCar;
	private Button btnBus;
	private Button btnWalking;
	private EditText tvLatitude;
	private EditText tvLongitude;
	
	private String strCity;
	
	private MKSearch mMKSearch = new MKSearch();
	
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
		
		mWalkingRouteOverlay = new RouteOverlay(MainActivity.this, mMapView);
		mBusRouteOverlay = new TransitOverlay(MainActivity.this, mMapView);
		mCarRouteOverlay = new RouteOverlay(MainActivity.this, mMapView);
		

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
		if(mLocationClient != null && !mLocationClient.isStarted()) 
		{
			mLocationClient.start();
		}
		
		btnCar = (Button) findViewById(R.id.car);
		btnBus = (Button) findViewById(R.id.bus);
		btnWalking = (Button) findViewById(R.id.walking);
		
		tvLatitude = (EditText) findViewById(R.id.latitude);
		tvLongitude = (EditText) findViewById(R.id.longitude);
		
		btnCar.setOnClickListener(new CarListener());
		btnBus.setOnClickListener(new BusListener());
		btnWalking.setOnClickListener(new WalkingListener());
	}
	
	public void getFriendInfo(){
		mFriendLatitude = Double.parseDouble(tvLatitude.getText().toString());
		mFriendLongitude = Double.parseDouble(tvLongitude.getText().toString());
	}
	
	class BusListener  implements OnClickListener {
		public void onClick(View v){
			mMyself = false;
			mLocationClient.requestLocation();
			mMKSearch.init(mBMapMan, new ComMKSearchListener());
			ByBus();
		}
	}
	
	class CarListener  implements OnClickListener {
		public void onClick(View v){
			mMyself = false;
			mLocationClient.requestLocation();
			mMKSearch.init(mBMapMan, new ComMKSearchListener());
			ByCar();
		}
	}
	
	class WalkingListener implements OnClickListener {
		public void onClick(View v){
			mMyself = false;
			mLocationClient.requestLocation();
			mMKSearch.init(mBMapMan, new ComMKSearchListener());
			ByWalking();
		}
	}
	
	public void setLocationInfo(BDLocation location, boolean Poi){
		sb.setLength(0);
		strCity=location.getCity();
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
	      
	      locData.latitude = location.getLatitude();
	      locData.longitude = location.getLongitude();
	      locData.direction = 2.0f;
	      
	      if(mMyself){
	    	  showLocalInfo();
	      }
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
		 menu.add(0, 3, 3, R.string.exit);
		 return super.onCreateOptionsMenu(menu);
	}
	
	//����˵�
	public boolean onOptionsItemSelected(MenuItem item)  
	{
		switch(item.getItemId())
		{
		case 1: 
			//�ҵ�λ��
			mMyself = true;
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
		case 3:
			finish();
			System.exit(0);  
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
        		if(mMKSearch != null){
        			mMKSearch.destory();
        		}
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
	
	public void ByCar(){
		getFriendInfo();
		MKPlanNode start = new MKPlanNode();
		start.pt = new GeoPoint((int)(locData.latitude*1e6), (int)(locData.longitude* 1e6));
		MKPlanNode end = new MKPlanNode();
		end.pt = new GeoPoint((int)(mFriendLatitude*1e6), (int)(mFriendLongitude*1e6));// ���üݳ�·���������ԣ�ʱ�����ȡ��������ٻ�������
		mMKSearch.setDrivingPolicy(MKSearch.ECAR_TIME_FIRST);
		mMKSearch.drivingSearch(null, start, null, end);
	}
	
	public void ByBus(){
		getFriendInfo();
		MKPlanNode start = new MKPlanNode();
		start.pt = new GeoPoint((int)(locData.latitude*1e6), (int)(locData.longitude* 1e6));
		MKPlanNode end = new MKPlanNode();
		end.pt = new GeoPoint((int)(mFriendLatitude*1e6), (int)(mFriendLongitude*1e6));// ���üݳ�·���������ԣ�ʱ�����ȡ��������ٻ�������
		mMKSearch.setTransitPolicy(MKSearch.EBUS_TIME_FIRST);
		mMKSearch.transitSearch(strCity, start, end);
	}
	
	public void ByWalking(){
		getFriendInfo();
		MKPlanNode start = new MKPlanNode();
		start.pt = new GeoPoint((int)(locData.latitude*1e6), (int)(locData.longitude* 1e6));
		MKPlanNode end = new MKPlanNode();
		end.pt = new GeoPoint((int)(mFriendLatitude*1e6), (int)(mFriendLongitude*1e6));// ���üݳ�·���������ԣ�ʱ�����ȡ��������ٻ�������
		mMKSearch.walkingSearch(null, start, null, end);
	}
	
	class ComMKSearchListener implements MKSearchListener{
		
		public void onGetDrivingRouteResult(MKDrivingRouteResult result, int iError) {
			if (result == null) {
				return;
			}
			mCarRouteOverlay.setData(result.getPlan(0).getRoute(0));
			mMapView.getOverlays().clear();
		    mMapView.getOverlays().add(mCarRouteOverlay);
			mMapView.refresh();
		}
		public void onGetTransitRouteResult(MKTransitRouteResult res,
				int error) {
			mBusRouteOverlay.setData(res.getPlan(0));
			mMapView.getOverlays().clear();
		    mMapView.getOverlays().add(mBusRouteOverlay);
			mMapView.refresh();
		}

		public void onGetWalkingRouteResult(MKWalkingRouteResult res,
				int error) { 
			mWalkingRouteOverlay.setData(res.getPlan(0).getRoute(0));
			mMapView.getOverlays().clear();
		    mMapView.getOverlays().add(mWalkingRouteOverlay);
			mMapView.refresh();
		}
		public void onGetAddrResult(MKAddrInfo res, int error) {
		}
		public void onGetPoiResult(MKPoiResult res, int arg1, int arg2) {
		}
		public void onGetBusDetailResult(MKBusLineResult result, int iError) {
		}

		@Override
		public void onGetSuggestionResult(MKSuggestionResult res, int arg1) {
		}

		@Override
		public void onGetPoiDetailSearchResult(int type, int iError) {
			// TODO Auto-generated method stub
		}

		@Override
		public void onGetShareUrlResult(MKShareUrlResult result, int type,
				int error) {
			// TODO Auto-generated method stub
			
		}
    }
		
	public void  showLocalInfo(){
		myLocationOverlay.setData(locData); 
		mMapView.getOverlays().clear();
	      if(mMapView.getOverlays().contains(myLocationOverlay)){
	    	   mMapView.getOverlays().set(myOverlayIndex,myLocationOverlay);
	      }else{
	    	   myOverlayIndex = mMapView.getOverlays().size();
	    	   mMapView.getOverlays().add(myLocationOverlay);
	      }
	      mMapView.refresh();
	      mMapView.getController().animateTo(new GeoPoint((int)(locData.latitude*1e6), (int)(locData.longitude* 1e6))); 
	}
}
