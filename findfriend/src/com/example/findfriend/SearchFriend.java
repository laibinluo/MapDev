package com.example.findfriend;

public class SearchFriend {
	private double mFriendLatitude;
	private double mFriendLongitude;
	private double mLocalLatitude;
	private double mLocalLongitude;
	
	SearchFriend(){
		mFriendLatitude = 0;
		mFriendLongitude = 0;
	}
	
	public void setFriendInfo(double latitude, double longitude) {
		mFriendLatitude = latitude;
		mFriendLongitude = longitude;		
	}
	
	public void setLocalInfo(double latitude, double longitude){
		mLocalLatitude = latitude;
		mLocalLongitude = longitude;	
	}
}
