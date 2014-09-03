package com.microcontrollerbg.irdroid;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.URL;
import java.net.URLConnection;

import android.net.ConnectivityManager;
import org.apache.http.util.ByteArrayBuffer;
import android.app.*;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.NetworkInfo;
import android.os.*;
import android.provider.Settings;
import android.widget.Toast;

public class Iconic extends Activity {
	ProgressDialog dialog;
    private Handler mHandler;
    //private final String PATH = "/mnt/sdcard/";
    Intent intent;
   private final String fileName = "/sdcard/tmp/t.conf";
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        intent= getIntent();
        if(haveInternet(this)){	
        	  dialog = ProgressDialog.show(Iconic.this, "",
        		        "Updating, please wait...", true);
        		   
        		      //  setContentView(R.layout.main);
        		       
        		      
        		        mHandler = new Handler();
        		     checkUpdate.start();
        }else if(!haveInternet(this)){
      
     showNoConnectionDialog(this);	
        }	  
        
    }
    /**
     * Checks if we have a valid Internet Connection on the device.
     * @param ctx
     * @return True if device has internet
     *
     * Code from: http://www.androidsnippets.org/snippets/131/
     */
    public static boolean haveInternet(Context ctx) {

        NetworkInfo info = (NetworkInfo) ((ConnectivityManager) ctx
                .getSystemService(Context.CONNECTIVITY_SERVICE)).getActiveNetworkInfo();

        if (info == null || !info.isConnected()) {
            return false;
        }
        if (info.isRoaming()) {
            // here is the roaming option you can change it if you want to
            // disable internet while roaming, just return false
            return true;
        }
        return true;
    }

    /**
         * Display a dialog that user has no internet connection
         * @param ctx1
         *
         * Code from: http://osdir.com/ml/Android-Developers/2009-11/msg05044.html
         */
        public static void showNoConnectionDialog(Context ctx1) {
            final Context ctx = ctx1;
            AlertDialog.Builder builder = new AlertDialog.Builder(ctx);
            builder.setCancelable(true);
            builder.setMessage("To update the DB you need internet connection");
            builder.setTitle("No data connection!");
            builder.setPositiveButton("settings", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    ctx.startActivity(new Intent(Settings.ACTION_WIRELESS_SETTINGS));
                }
            });
            builder.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();	
                
            
                    return;
                }
            });
            builder.setOnCancelListener(new DialogInterface.OnCancelListener() {
                public void onCancel(DialogInterface dialog) {
                    return;
                }
            });
            

            builder.show();
        }


    private Thread checkUpdate = new Thread() {
        public void run() {
            try {
            	  File file = new File(fileName);
            	  if (file.exists()) {
            		  file.delete();
            	  }
            	  URL updateURL = new URL("http://www.irdroid.com/db/t.conf");
           
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
         dialog.dismiss();
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

