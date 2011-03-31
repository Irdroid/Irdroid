package com.microcontrollerbg.irdroid;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.URL;
import java.net.URLConnection;


import org.apache.http.util.ByteArrayBuffer;
import android.app.*;
import android.content.Intent;
import android.os.*;
import android.widget.Toast;

public class Iconic extends Activity {

    private Handler mHandler;
    //private final String PATH = "/mnt/sdcard/";
    Intent intent;
   private final String fileName = "/sdcard/tmp/t.conf";
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
      //  setContentView(R.layout.main);
        intent= getIntent();
      
        mHandler = new Handler();
     checkUpdate.start();
    }

    private Thread checkUpdate = new Thread() {
        public void run() {
            try {
            	  File file = new File(fileName);
            	  if (file.exists()) {
            		  file.delete();
            	  }
            	  URL updateURL = new URL("http://www.georgibakalski.info/car/t.conf");
           
                URLConnection conn = updateURL.openConnection();
                InputStream is = conn.getInputStream();
                BufferedInputStream bis = new BufferedInputStream(is);
                ByteArrayBuffer baf = new ByteArrayBuffer(50);

                int current = 0;
                while((current = bis.read()) != -1){
                    baf.append((byte)current);
                }
                File tmpdir = new File("/sdcard/tmp/");
              if(!tmpdir.exists()){
                // have the object build the directory structure, if needed.
                tmpdir.mkdirs();
              }
                /* Convert the Bytes read to a String. */
              //  html = new String(baf.toByteArray());
                FileOutputStream fos = new FileOutputStream(file);
           

                
                fos.write(baf.toByteArray());

              fos.close();
        //  if (file.exists()){
        	 setResult(RESULT_OK, intent);
          	  finish();
          
          
         // }
          
       //     intent.putExtra("returnedData", msg);
        
        	 
          
              

                mHandler.post(showUpdate);
             
               
              
            } catch (Exception e) {
            }
        }
    };

    private Runnable showUpdate = new Runnable(){
        public void run(){
      Toast.makeText(Iconic.this, "Success!", 
		Toast.LENGTH_SHORT).show();
     
        }
    };
}

