package com.getmyfiles.gui;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;
import android.widget.ToggleButton;

public class GetMyFilesActivity extends Activity {
	private static final String TAG = "getMyFiles";
	private static final int ACTIVITY_CREATE = 1;
	private Button pathButton;
	private TextView pathView;
	private ToggleButton connectButton;
	private TextView urlView;
	private Thread clientThread;
	private String execDir;
	private Process process = null;
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        execDir = this.getCacheDir().getParentFile().getAbsolutePath() + "/lib";
        pathView = (TextView) findViewById(R.id.pathText);
        urlView = (TextView) findViewById(R.id.urlText);
        pathButton = (Button) findViewById(R.id.pathButton);
        connectButton = (ToggleButton) findViewById(R.id.connectButton);
        share_mode(false);
        pathButton.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
            	Intent myIntent = new Intent(GetMyFilesActivity.this, FileBrowserActivity.class);
            	//GetMyFilesActivity.this.startActivity(myIntent);
            	startActivityForResult(myIntent, ACTIVITY_CREATE);

            }
        });
        connectButton.setOnClickListener(new OnClickListener() {
        	public void onClick(View v) {
        		if (clientThread != null && clientThread.isAlive()) {
        			clientThread.interrupt();
        		} else {
        			clientThread = new MyThread();
        			clientThread.start();
        		}
        	}
        });
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
      super.onActivityResult(requestCode, resultCode, data);
      if (requestCode == ACTIVITY_CREATE) {
    	  if (resultCode == RESULT_OK) {
    		  String path = data.getExtras().getString("path");
    		  pathView.setText(path);
    	  }
      }
    }
    
    private Handler handler = new Handler();
    
    private void output(final String str) {
    	Runnable proc = new Runnable() {
    		public void run() {
    			urlView.setEnabled(true);
    			urlView.setText(str);
    		}
    	};
    	handler.post(proc);
    }
    
    private void share_mode(final boolean m) {
    	Runnable proc = new Runnable() {
    		public void run() {
        		pathButton.setClickable(!m);
        		pathView.setEnabled(!m);
        		urlView.setEnabled(false);
    		}
    	};
    	handler.post(proc);
    }
    
    public class MyThread extends Thread {
    	public void run() {
    		share_mode(true);
			String exe = execDir + "/libgetmyfiles.so " + pathView.getText().toString();
			Log.i("run app", exe);
			try {
				Process process = Runtime.getRuntime().exec(exe);
				try {    		    
					BufferedReader procerr = new BufferedReader(new InputStreamReader(process.getErrorStream()));
					while (true) {
						if (procerr.ready()) {
							try {
								String errstr = procerr.readLine();
								Log.i(TAG, errstr);
								if (errstr.startsWith("Server directory: ")) {
									String url = errstr.substring(18);
									Log.i(TAG, url);
									output(url);
								}
							} catch (IOException ie) {
								Log.e(TAG, "procerr exception: " + ie);
								break;
							}
						} else {
							Thread.sleep(1000);
						}
					}
					procerr.close();    			
					process.waitFor();
				} catch (IOException e) {
					throw new RuntimeException(e);
				} catch (InterruptedException e) {
					//throw new RuntimeException(e);
					Log.i(TAG, "interrupt " + e);
					process.destroy();
				}
			} catch (IOException ie) {
				Log.e(TAG, "exec() " + ie);
			}
			share_mode(false);
    	}
    }
}