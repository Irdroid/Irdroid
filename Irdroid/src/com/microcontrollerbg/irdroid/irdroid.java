package com.microcontrollerbg.irdroid;


import java.io.FileInputStream;

import java.io.FileOutputStream;



//import com.microcontrollerbg.irdroid.R;





import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.AlertDialog.Builder;


import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Bundle;
import android.os.Handler;
import android.os.SystemClock;

import android.os.Vibrator;
import android.text.method.LinkMovementMethod;
import android.util.Log;

import android.view.KeyEvent;

import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;


import android.view.View.OnTouchListener;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;

import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

public class irdroid extends Activity {
public int number=0;
byte     	    buffer[];
	protected String com;
	protected String dev;
	SharedPreferences mPrefs;
private AudioManager audio;
 public AudioTrack ir;
	public  int bufSize = AudioTrack.getMinBufferSize(48000,
    		AudioFormat.CHANNEL_CONFIGURATION_STEREO,
    		AudioFormat.ENCODING_PCM_8BIT);

	//private final static String fileName = "/sdcard/t.conf";
	private final static String LIRCD_CONF_FILE = "/sdcard/tmp/t.conf";
	///private final static String LIRCD_CONF_FILE = "/sdcard/lircd.conf";
	private Handler mHandler = new Handler();
	public String mycmd;
     
	// global variables
	TextView tv;
	Lirc lirc;
	ArrayAdapter<String> deviceList;
	ArrayAdapter<String> commandList;
	private Vibrator myVib;
	public String gdevice;	
 
    private Runnable voldown = new Runnable()
    {
        public void run()
        {
        	
        		ir.release();
        
        String coolcmd="VOL-";
        //	Log.i("repeatBtn", "repeat click");
            myVib.vibrate(50);		
		       
	        
	       
	        
          
          	try {
				
				 sendSignal(gdevice, coolcmd);
           //	Log.i("repeatBtn", "MotionEvent.ACTION_DOWN");
             //  mHandler.removeCallbacks(mUpdateTask);
              
             	} catch (IllegalStateException e) {
 					// TODO Auto-generated catch block
 						e.printStackTrace();
 					}		
            mHandler.postAtTime(this, SystemClock.uptimeMillis() +
250);
            
        }
    };
    private Runnable volup = new Runnable()
    {
        public void run()
        {
        	
        		ir.release();
        
        String coolcmd="VOL+";
        //	Log.i("repeatBtn", "repeat click");
            myVib.vibrate(50);		
		       
	        
	       
	        
          
          	try {
				
				 sendSignal(gdevice, coolcmd);
           //	Log.i("repeatBtn", "MotionEvent.ACTION_DOWN");
             //  mHandler.removeCallbacks(mUpdateTask);
              
             	} catch (IllegalStateException e) {
 					// TODO Auto-generated catch block
 						e.printStackTrace();
 					}		
            mHandler.postAtTime(this, SystemClock.uptimeMillis() +
250);
            
        }
    };
    
    private Runnable next = new Runnable()
    {
        public void run()
        {
        	
        		ir.release();
        
        String coolcmd="P+";
        //	Log.i("repeatBtn", "repeat click");
            myVib.vibrate(50);		
		       
	        
	       
	        
          
          	try {
				
				 sendSignal(gdevice, coolcmd);
           //	Log.i("repeatBtn", "MotionEvent.ACTION_DOWN");
             //  mHandler.removeCallbacks(mUpdateTask);
              
             	} catch (IllegalStateException e) {
 					// TODO Auto-generated catch block
 						e.printStackTrace();
 					}		
            mHandler.postAtTime(this, SystemClock.uptimeMillis() +
250);
            
        }
    };
    private Runnable prev = new Runnable()
    {
        public void run()
        {
        	
        		ir.release();
        
        String coolcmd="P+";
        //	Log.i("repeatBtn", "repeat click");
            myVib.vibrate(50);		
		       
	        
	       
	        
          
          	try {
				
				 sendSignal(gdevice, coolcmd);
           //	Log.i("repeatBtn", "MotionEvent.ACTION_DOWN");
             //  mHandler.removeCallbacks(mUpdateTask);
              
             	} catch (IllegalStateException e) {
 					// TODO Auto-generated catch block
 						e.printStackTrace();
 					}		
            mHandler.postAtTime(this, SystemClock.uptimeMillis() +
250);
            
        }
    };
    public void onCreate(Bundle savedInstanceState) {
    	
    	audio = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
    	int currentVolume = audio.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
    	audio.setStreamVolume(AudioManager.STREAM_MUSIC,currentVolume/2, 0);
    	audio = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
    	

   


        super.onCreate(savedInstanceState);
 

        
      //  LinearLayout ll = new LinearLayout(this);
       // ll.setOrientation(LinearLayout.VERTICAL);
        //ll.setKeepScreenOn(true);
       
        firstRunPreferences();
        if(getFirstRun()){
        	About();
        	setRunned();
        	
        }
        setContentView(R.layout.apple);
    //    Button info_play = (Button) findViewById(R.id.Button02);
        Button apple_volup = (Button) findViewById(R.id.apple_volup);
        Button apple_voldn = (Button) findViewById(R.id.apple_voldn);
        Button apple_menu = (Button) findViewById(R.id.apple_menu);
        Button apple_next = (Button) findViewById(R.id.apple_next);
        Button apple_prev = (Button) findViewById(R.id.apple_prev);
        Button apple_play = (Button) findViewById(R.id.apple_play);
       //   Button apple_menu = (Button) findViewById(R.id.apple_menu);
       final Spinner spinDevice = (Spinner) findViewById(R.id.Spinner01);
       final Spinner spinCommand = (Spinner) findViewById(R.id.Spinner02);
  //    Button apple_voldn = (Button) findViewById(R.id.apple_voldn);
//mAudioManager.setVolumeControlStream(AudioManager.STREAM_MUSIC);


       // setContentView(ll);
   //    final Handler mHandler = new Handler(); 
   //     ScrollView sv = new ScrollView(this);
   //     tv = new TextView(this);
        //tv.setBackgroundColor();
    //    sv.addView(tv);
        lirc = new Lirc();
       
        myVib = (Vibrator) this.getSystemService(VIBRATOR_SERVICE);
   

     
    	// Initialize adapter for device spinner
     deviceList = new ArrayAdapter<String>(this,android.R.layout.simple_spinner_item);
	deviceList.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);

        // Initialize device spinner
  //  final Spinner spinDevice = new Spinner(this);
    	spinDevice.setPrompt("Select a device");
    	spinDevice.setAdapter(deviceList);

    	// Command adapter
        commandList = new ArrayAdapter<String>(this,android.R.layout.simple_spinner_item);
        commandList.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);

        // Parse configuration file and update device adapter
        parse(LIRCD_CONF_FILE);
       
    	spinDevice.setOnItemSelectedListener(new Spinner.OnItemSelectedListener() {
       	    public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
       	    	String [] str = lirc.getCommandList(spinDevice.getSelectedItem().toString());
     	    	commandList.clear();
     	    	gdevice=spinDevice.getSelectedItem().toString();
            	for (int i=0; i<str.length; i++){
            		commandList.add(str[i]);
            	}
            	
            //	gdevice = spinDevice.getSelectedItem().toString();
            	
            	
            	
     	    }

    	    @Override
			public void onNothingSelected(AdapterView<?> arg0) {
			}
    	});
    	
  //  	ll.addView(spinDevice);
   	
 //       final Spinner spinCommand = new Spinner(this);   
     	spinCommand.setPrompt("Select a command");
    	spinCommand.setAdapter(commandList);
    
    	spinCommand.setOnItemSelectedListener(new Spinner.OnItemSelectedListener() {

			@Override
			public void onItemSelected(AdapterView<?> parent, View view,
					int pos, long id) {
				
				String device = spinDevice.getSelectedItem().toString();	
				 mycmd = spinCommand.getSelectedItem().toString();
		     if (ir!=null){   
			try {
            	 sendSignal(device, mycmd);
               //	Log.i("repeatBtn", "MotionEvent.ACTION_DOWN");
                 //  mHandler.removeCallbacks(mUpdateTask);
                  
                 	} catch (IllegalStateException e) {
     					// TODO Auto-generated catch block
     						e.printStackTrace();
     					}	  
		     }
		     }

			@Override
			

			public void onNothingSelected(AdapterView<?> arg0) {
				// TODO Auto-generated method stub
				
			}
			
    	});
   // 	ll.addView(spinCommand);
    	
           
    //	  apple_menu.setOnClickListener(new OnClickListener(){

          //      //  @Override
 //                public void onClick(View arg0) {
               	
 //          	  try {
                		
        //      	  String device = spinDevice.getSelectedItem().toString();
       		//	       String cmd = spinCommand.getSelectedItem().toString();
    // 			       sendSignal(gdevice, gcmd);
                		
          			     
  	//			} catch (IllegalStateException e) {
  				// TODO Auto-generated catch block
 	//		e.printStackTrace();
 //		}	  
                	  
   			  // Toast.makeText(getApplicationContext(), "Command Sent", Toast.LENGTH_SHORT).show();
                	 //   Intent myIntent = new Intent(.getContext(), Activity2.class);
                	//  String device=  lir.dev.toString();
                // 	 String cmd= lir.com.toString();
                //  lir.deviceList.getItem(0);
                 //	lir.lirc = new Lirc();
    //lir.sendSignal(device, cmd);
       //         }
     //  });
   
    
    	
        apple_voldn.setOnTouchListener(new OnTouchListener()
        {
            public boolean onTouch(View view, MotionEvent motionevent)
            {
                int action = motionevent.getAction();
               
                if (action == MotionEvent.ACTION_DOWN)
                {
                	
                	if (spinDevice.getSelectedItem()==null || spinCommand.getSelectedItem() == null) {
    		        	Toast.makeText(getApplicationContext(), "Please select a device and a command", Toast.LENGTH_SHORT).show();
    		        	
    		        	return true;
    		        }
                	myVib.vibrate(50);		
    		      
    		        String mycmd="VOL-";
    		       
    		        
                  
                  	try {
        				
        				 sendSignal(gdevice, mycmd);
                   //	Log.i("repeatBtn", "MotionEvent.ACTION_DOWN");
                     //  mHandler.removeCallbacks(mUpdateTask);
                      
                     	} catch (IllegalStateException e) {
         					// TODO Auto-generated catch block
         						e.printStackTrace();
         					}		 
                  	  	
                  	
                   mHandler.postAtTime(voldown,
SystemClock.uptimeMillis() + 250);
                }
                
                else if (action == MotionEvent.ACTION_UP)
                {
             //      Log.i("repeatBtn", "MotionEvent.ACTION_UP");\
                	try {
						Thread.sleep(180);
						if(ir!=null){
							ir.flush();
							ir.release();
							
						}
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
                    mHandler.removeCallbacks(voldown);

}
                return false;
            }
            
        }); 
       
        apple_next.setOnTouchListener(new OnTouchListener()
        {
            public boolean onTouch(View view, MotionEvent motionevent)
            {
                int action = motionevent.getAction();
                if (action == MotionEvent.ACTION_DOWN)
                {
                	if (spinDevice.getSelectedItem()==null || spinCommand.getSelectedItem() == null) {
    		        	Toast.makeText(getApplicationContext(), "Please select a device and a command", Toast.LENGTH_SHORT).show();
    		        	return true;
    		        }
                	 myVib.vibrate(50);
                	
              
                String	gcmd="P+";
                
                	  
                	  try {
                     	 sendSignal(gdevice, gcmd);
                   	Log.i("repeatBtn", "MotionEvent.ACTION_DOWN");
                     //  mHandler.removeCallbacks(mUpdateTask);
                      
                     	} catch (IllegalStateException e) {
         					// TODO Auto-generated catch block
         						e.printStackTrace();
         					}	 
                	
                 //   mHandler.removeCallbacks(mUpdateTask);
                   
                    mHandler.postAtTime(next,
SystemClock.uptimeMillis() + 250);
                
                }
                else if (action == MotionEvent.ACTION_UP)
                {
             //      Log.i("repeatBtn", "MotionEvent.ACTION_UP");\
                	try {
						Thread.sleep(150);
						if(ir!=null){
							ir.flush();
							ir.release();
							
						}
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
                    mHandler.removeCallbacks(next);
                	
                }
                return false;
            }
        });  
        
        apple_prev.setOnTouchListener(new OnTouchListener()
        {
            public boolean onTouch(View view, MotionEvent motionevent)
            {
                int action = motionevent.getAction();
                if (action == MotionEvent.ACTION_DOWN)
                {
                	if (spinDevice.getSelectedItem()==null || spinCommand.getSelectedItem() == null) {
    		        	Toast.makeText(getApplicationContext(), "Please select a device and a command", Toast.LENGTH_SHORT).show();
    		        	return true;
    		        }
                	 myVib.vibrate(50);
                	
                	String gcmd="P-";
               	

                  	try {
                    	 sendSignal(gdevice, gcmd);
                  //	Log.i("repeatBtn", "MotionEvent.ACTION_DOWN");
                    //  mHandler.removeCallbacks(mUpdateTask);
                     
                    	} catch (IllegalStateException e) {
        					// TODO Auto-generated catch block
        						e.printStackTrace();
        					}	 
                  	 
             //       mHandler.removeCallbacks(mUpdateTask);
                   
                   mHandler.postAtTime(prev,
SystemClock.uptimeMillis() + 250);
                
                }
                else if (action == MotionEvent.ACTION_UP)
                {
             //      Log.i("repeatBtn", "MotionEvent.ACTION_UP");\
      //    /          
                	
                	
                	try {
						Thread.sleep(150);
						if(ir!=null){
							ir.flush();
							ir.release();
							
						}
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
                   mHandler.removeCallbacks(prev);
                	
                }
                return false;
            }
        });  
        
        apple_menu.setOnTouchListener(new OnTouchListener()
        {
            public boolean onTouch(View view, MotionEvent motionevent)
            {
                int action = motionevent.getAction();
                if (action == MotionEvent.ACTION_DOWN)
                {
                	if (spinDevice.getSelectedItem()==null || spinCommand.getSelectedItem() == null) {
    		        	Toast.makeText(getApplicationContext(), "Please select a device and a command", Toast.LENGTH_SHORT).show();
    		        	return true;
    		        }
                	 myVib.vibrate(50);
              
                	String  gcmd="MUTE";
                	
                  	
                  	try {
                     	 sendSignal(gdevice, gcmd);
                   //	Log.i("repeatBtn", "MotionEvent.ACTION_DOWN");
                     //  mHandler.removeCallbacks(mUpdateTask);
                      
                     	} catch (IllegalStateException e) {
         					// TODO Auto-generated catch block
         						e.printStackTrace();
         					}	 
               // 	Log.i("repeatBtn", "MotionEvent.ACTION_DOWN");
               //     mHandler.removeCallbacks(mUpdateTask);
                   
            //        mHandler.postAtTime(mUpdateTask,
//S/ystemClock.uptimeMillis() + 200);
                
                }
                else if (action == MotionEvent.ACTION_UP)
                {
             //      Log.i("repeatBtn", "MotionEvent.ACTION_UP");\
                	try {
						Thread.sleep(150);
						if(ir!=null){
							ir.flush();
							ir.release();
							
						}
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
            //        mHandler.removeCallbacks(mUpdateTask);
                
                }
                return false;
            }
        });  
        apple_play.setOnTouchListener(new OnTouchListener()
        {
            public boolean onTouch(View view, MotionEvent motionevent)
            {
                int action = motionevent.getAction();
                if (action == MotionEvent.ACTION_DOWN)
                {
                	if (spinDevice.getSelectedItem()==null || spinCommand.getSelectedItem() == null) {
    		        	Toast.makeText(getApplicationContext(), "Please select a device and a command", Toast.LENGTH_SHORT).show();
    		        	return true;
    		        }
                	 myVib.vibrate(50);
                	// String cmd=null;
                	String gcmd="POWER";
                	
                  	
                  	 try {
                      	 sendSignal(gdevice, gcmd);
                    //	Log.i("repeatBtn", "MotionEvent.ACTION_DOWN");
                      //  mHandler.removeCallbacks(mUpdateTask);
                       
                      	} catch (IllegalStateException e) {
          					// TODO Auto-generated catch block
          						e.printStackTrace();
          					}
                      	
                	Log.i("repeatBtn", "MotionEvent.ACTION_DOWN");
          //          mHandler.removeCallbacks(mUpdateTask);
                   
                //    mHandler.postAtTime(mUpdateTask,
//SystemClock.uptimeMillis() + 200);
                
                }
                else if (action == MotionEvent.ACTION_UP)
                {
             //      Log.i("repeatBtn", "MotionEvent.ACTION_UP");\
                	try {
						Thread.sleep(150);
						if(ir!=null){
							ir.flush();
							ir.release();
							
						}
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
          //          mHandler.removeCallbacks(mUpdateTask);
                	
                }
                return false;
                
            }
        });  
        apple_volup.setOnTouchListener(new OnTouchListener()
        {
           
        	public boolean onTouch(View view, MotionEvent motionevent)
            {
                int action = motionevent.getAction();
           
                if (action == MotionEvent.ACTION_DOWN)
                {
                	
                	
                	if (spinDevice.getSelectedItem()==null || spinCommand.getSelectedItem() == null) {
    		        	Toast.makeText(getApplicationContext(), "Please select a device and a command", Toast.LENGTH_SHORT).show();
    		        	return true;
    		        }
                	 myVib.vibrate(50);
              
                String	 mycmd="VOL+";
                	
                  
                 // 	 sendSignal(gdevice, gcmd);
                  	 
                  	try {
                    	 sendSignal(gdevice, mycmd);
                  //	Log.i("repeatBtn", "MotionEvent.ACTION_DOWN");
                    //  mHandler.removeCallbacks(mUpdateTask);
                     
                    	} catch (IllegalStateException e) {
        					// TODO Auto-generated catch block
        						e.printStackTrace();
        					}


                //	Log.i("repeatBtn", "MotionEvent.ACTION_DOWN");
     //               mHandler.removeCallbacks(mUpdateTask);
                   
                   mHandler.postAtTime(volup,
SystemClock.uptimeMillis() + 250);
                
                }
                else if (action == MotionEvent.ACTION_UP)
                {
             //      Log.i("repeatBtn", "MotionEvent.ACTION_UP");\
                    try {
						Thread.sleep(150);
						if(ir!=null){
							ir.flush();
							ir.release();
							
						}
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
                    mHandler.removeCallbacks(volup);
                	
                }
                return false;
           
            }
        
        }); 
       
        
        
        
        
        
        
        
        
        
        
        
        
        
    

	//		@Override
			
		//	public void onClick(View v) {
	//	        if (spinDevice.getSelectedItem()==null || spinCommand.getSelectedItem() == null) {
		 //       	Toast.makeText(getApplicationContext(), "Please select a device and a command", Toast.LENGTH_SHORT).show();
		   //     	return;
		   //     }
		           // AndrolircMain.this.dev = spinDevice.getSelectedItem().toString();
			       //    AndrolircMain.this.com = spinCommand.getSelectedItem().toString();
			        //    Intent myIntent = new Intent(v.getContext(), Activity2.class);
			       //     startActivityForResult(myIntent, 0);
	//		String device = spinDevice.getSelectedItem().toString();
	//	       String cmd = spinCommand.getSelectedItem().toString();
		//       sendSignal(device, cmd);
	//		}});

  
        
   
    }

    
    
    
    public String selectFile(){
		
    	final EditText ed = new EditText(this);
    		
        	Builder builder = new Builder(this);
        	builder.setTitle("Select a file to parse");
        	builder.setView(ed);
        	builder.setPositiveButton("Save", new DialogInterface.OnClickListener() {  	
    			public void onClick(DialogInterface dialog, int which) {
    				if (which == Dialog.BUTTON_NEGATIVE) {
    					dialog.dismiss();
    					return;
    				}
    				parse(ed.getText().toString());
    			}});    			
    		builder.setNegativeButton("OK", null);
    	   	final AlertDialog asDialog = builder.create();
    	  	asDialog.show();  
        	return null;
	
    }

    public String About(){
    	AlertDialog.Builder about = new AlertDialog.Builder(this);
        about.setTitle(R.string.app_name)
              //  .setIcon(R.drawable.dialog_icon)
               .setMessage(R.string.info)
               .setCancelable(true)
               .setNegativeButton("Dismiss", new DialogInterface.OnClickListener() {
                  public void onClick(DialogInterface dialog, int id) {
                        dialog.dismiss();
                   }
              });

        AlertDialog welcomeAlert = about.create();
        welcomeAlert.show();
   //      Make the textview clickable. Must be called after show()
        ((TextView)welcomeAlert.findViewById(android.R.id.message)).setMovementMethod(LinkMovementMethod.getInstance());
return null;
	
    }
    
    

    
    
    public boolean parse (String config_file) {
    	
    	java.io.File file = new java.io.File(config_file);
    	

    	if (!file.exists()) {
    		if (config_file != LIRCD_CONF_FILE)
    			Toast.makeText(getApplicationContext(), "The Selected file doesn't exist", Toast.LENGTH_SHORT).show();
    		else
    			Toast.makeText(getApplicationContext(), "Configuartion file missing, please update the db", Toast.LENGTH_SHORT).show();
    		//selectFile();
    		return false;
    	}
    	
    	if (lirc.parse(config_file) == 0) {
 			Toast.makeText(getApplicationContext(), "Couldn't parse the selected file", Toast.LENGTH_SHORT).show();
			//selectFile();
    		return false;
    	}
    	
    	// Save the file since it has been parsed successfully
    	if (config_file != LIRCD_CONF_FILE) {
	        try {
	            FileInputStream in = new FileInputStream(config_file);
	            FileOutputStream out = new FileOutputStream(LIRCD_CONF_FILE);
	            byte[] buf = new byte[1024];
	            int i = 0;
	            while ((i = in.read(buf)) != -1) {
	                out.write(buf, 0, i);
	            }
	            in.close();
	            out.close();
	        } catch(Exception e) {
    			tv.append("Probleme saving configuration file: "+ e.getMessage());
	        }
    	}
    	
    	updateDeviceList();
    	return true;
    }
    
    
    
    
    public void updateDeviceList(){
        String [] str = lirc.getDeviceList();
        
        if (str == null){
 			Toast.makeText(getApplicationContext(), "Invalid, empty or missing config file", Toast.LENGTH_SHORT).show();
			//selectFile();
    		return;
        }
        
        deviceList.clear();
        for (int i=0; i<str.length; i++){
        	Log.e("ANDRPOLIRC", String.valueOf(i)+"/"+String.valueOf(str.length)+ ": "+str[i]);
        	deviceList.add(str[i]);
        }
        
       // Log.e("ANDRPOLIRC", "Device list successfuly updated. Number of devices: "+String.valueOf(str.length)+"---"+deviceList.getItem(1));
    }
    
    
    
    void sendSignal(String device, String cmd) {
    	
//buffer=null;
 buffer   =lirc.getIrBuffer(device, cmd);

    if (buffer == null) {
    	Toast.makeText(getApplicationContext(), "Empty Buffer!", Toast.LENGTH_SHORT).show();
    	return;
    }
    ir = new AudioTrack(AudioManager.STREAM_MUSIC, 48000, 
  			AudioFormat.CHANNEL_CONFIGURATION_STEREO, AudioFormat.ENCODING_PCM_8BIT, 
  			bufSize, AudioTrack.MODE_STATIC);

    if (bufSize < buffer.length)
     	bufSize = buffer.length;
	

   	
	 	 
    
    
 

	

   
  
        ir.write(buffer, 0, buffer.length);
	
	// Debug
//	tv.append("Minimum buffer size: "+ 
//			String.valueOf(AudioTrack.getMinBufferSize(48000,
//    		AudioFormat.CHANNEL_CONFIGURATION_STEREO,
//    		AudioFormat.ENCODING_PCM_8BIT))+"\n");
//	tv.append("buffer size: "+ String.valueOf(buffer.length)+"\n");
//	tv.append("bits written: "+ String.valueOf(res)+"\n");
	
    
     	ir.setStereoVolume(1,1);

  ir.play();    	

	// ir.release();
 //ir=null;

//ir.release();


 //int a;
	//a=ir.getPlayState();
	
	//ir.release();
//Save sample into file for tests
//	try {
//		FileOutputStream myOutput = new FileOutputStream("/data/data/com.zokama.androlirc/power_toggle.raw");
//		myOutput.write(buffer, 0, buffer.length);
//		} catch (Exception e) {
//			e.printStackTrace();
//		}
	
//	tv.append(device + ": " +cmd + " command sent\n");
    }
   
    
    
    void deleteConfigFile() {
    	java.io.File file = new java.io.File(LIRCD_CONF_FILE);
    	if (!file.exists()) 
   			Toast.makeText(getApplicationContext(), "Configuartion file missing\n" +
   					"No file to delete", Toast.LENGTH_SHORT).show();
    	else
    		if (file.delete()){
    			Toast.makeText(getApplicationContext(), "File deleted successfully", Toast.LENGTH_SHORT).show();
deviceList.clear();
commandList.clear();
    		//	selectFile();
    		}
    		else
    			Toast.makeText(getApplicationContext(), "Couldn't delete the file", Toast.LENGTH_SHORT).show();
    }
    
    
    
    public boolean onCreateOptionsMenu(Menu menu) {
        menu.add(0, 0, 0, "Parse file").setIcon(android.R.drawable.ic_menu_upload);
        menu.add(0, 1, 0, "Clear conf").setIcon(android.R.drawable.ic_menu_delete);
        menu.add(0, 2, 0, "Update db").setIcon(android.R.drawable.ic_input_add);
        menu.add(0, 3, 0, "Send").setIcon(android.R.drawable.arrow_up_float);
        menu.add(0, 4, 0, "About").setIcon(android.R.drawable.ic_menu_help);
        return true;
    }
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
    	  super.onActivityResult(requestCode, resultCode, data);
    	  if(resultCode==RESULT_OK && requestCode==1){
    		
    		  parse(LIRCD_CONF_FILE);
    		  updateDeviceList();
    		  
    	//	  updateDeviceList();
    	 //  Toast.makeText(getApplicationContext(), "Doobre", Toast.LENGTH_SHORT).show();
    	  }
    	 }

    
    /**
 
     * get if this is the first run
 
     *
 
     * @return returns true, if this is the first run

     */

        public boolean getFirstRun() {
 
        return mPrefs.getBoolean("firstRun", true);
 
     }
 
     

     /**

     * store the first run

     */

     public void setRunned() {

        SharedPreferences.Editor edit = mPrefs.edit();

        edit.putBoolean("firstRun", false);

        edit.commit();

     }

     
     public boolean onKeyDown(int keyCode, KeyEvent event) {
        switch (keyCode) {
    	    case KeyEvent.KEYCODE_VOLUME_UP:
    	        audio.adjustStreamVolume(AudioManager.STREAM_MUSIC,
    	                AudioManager.ADJUST_RAISE, AudioManager.FLAG_SHOW_UI);
    	        return true;
    	    case KeyEvent.KEYCODE_VOLUME_DOWN:
    	        audio.adjustStreamVolume(AudioManager.STREAM_MUSIC,
    	                AudioManager.ADJUST_LOWER, AudioManager.FLAG_SHOW_UI);
    	        return true;
    	    
    	        
    	    default:
    	    	super.onKeyDown(keyCode, event);
return false;
    	    }
    	}

     

     

     /**

     * setting up preferences storage

     */

     public void firstRunPreferences() {

        Context mContext = this.getApplicationContext();

        mPrefs = mContext.getSharedPreferences("myAppPrefs", 0); //0 = mode private. only this app can read these preferences

     }

    public boolean onOptionsItemSelected(MenuItem item) {
    	switch (item.getItemId()) {
    	case 0:
    		selectFile();
    		break;
    	case 1:
    		deleteConfigFile();
    		break;
    	case 2:
    	//selectFile();
    		Intent myIntent = new Intent(irdroid.this, Iconic.class);
              startActivityForResult(myIntent, 1);
              break;
    	case 3:
    		try {
				
				 sendSignal(gdevice, mycmd);
          //	Log.i("repeatBtn", "MotionEvent.ACTION_DOWN");
            //  mHandler.removeCallbacks(mUpdateTask);
             
            	} catch (IllegalStateException e) {
					// TODO Auto-generated catch block
						e.printStackTrace();
					}	
    		break;
    	case 4:
        	
    		About();
                  break;
    	}
    	return false;
    }
}