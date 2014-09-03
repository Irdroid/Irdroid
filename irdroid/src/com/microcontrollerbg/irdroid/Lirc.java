package com.microcontrollerbg.irdroid;

import java.io.File;


public class Lirc {
	public static String POWER_TOGGLE = "Function18";
	
	static {
       System.loadLibrary("irdroid");
      }
	
	native int parse(String filename);
	native byte[] getIrBuffer(String irDevice, String irCode);
	native String[] getDeviceList();
	native String[] getCommandList(String irDevice);
	
	Lirc (){
	 
		File dir = new File("/mnt/sdcard/log");
		dir.mkdirs();
	}

}
