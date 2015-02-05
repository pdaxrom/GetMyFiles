package com.getmyfiles.gui;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.text.ClipboardManager;
import android.text.method.LinkMovementMethod;
import android.util.Log;
import android.view.ContextMenu;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.ToggleButton;

public class GetMyFilesActivity extends Activity {
	private final Context context = this;
	private static final String website_url = "http://getmyfil.es";
	private static final String update_url = "http://getmyfil.es/?p=download";
	private static final String TAG = "getMyFiles";
	private static final int ACTIVITY_CREATE = 1;
	private static final int NOTIFICATION_ID = 1;
	private ImageButton pathButton;
	private TextView pathView;
	private ToggleButton connectButton;
	private TextView urlView;
	private String execDir;
	private ProgressDialog pd;

	private MyShareInfo myShareInfo;
	
	private class MyShareInfo {
		public Thread thread;
		public String path;
		public String url;
	}
    static {
        System.loadLibrary("getmyfiles");
    }

    public native int connectClient(String[] folders);
    public native void disconnectClient();
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        execDir = this.getCacheDir().getParentFile().getAbsolutePath() + "/lib";
        pathView = (TextView) findViewById(R.id.pathText);
        urlView = (TextView) findViewById(R.id.urlText);
        pathButton = (ImageButton) findViewById(R.id.pathButton);
        connectButton = (ToggleButton) findViewById(R.id.connectButton);

        myShareInfo  = (MyShareInfo) getLastNonConfigurationInstance();
        
        if (myShareInfo == null) {
        	myShareInfo = new MyShareInfo();
        	share_mode(false);        	
        }

        if (myShareInfo != null) {
            if (myShareInfo.thread != null && myShareInfo.thread.isAlive()) {
            	Log.i(TAG, "thread alive!!! " + myShareInfo.path + " " + myShareInfo.url);
            	pathView.setText(myShareInfo.path);
            	share_mode(true);
            	output(myShareInfo.url);
            }
        }
        	
        Log.i(TAG, "Android version " + Build.VERSION.SDK_INT);
        if (Build.VERSION.SDK_INT > 10) {
        	registerForContextMenu(urlView);
        }
        pathButton.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
            	Intent myIntent = new Intent(GetMyFilesActivity.this, FileBrowserActivity.class);
            	//GetMyFilesActivity.this.startActivity(myIntent);
            	startActivityForResult(myIntent, ACTIVITY_CREATE);

            }
        });
        pd = new ProgressDialog(this);
        pd.setTitle(getString(R.string.connecting) + "...");
        pd.setMessage(getString(R.string.wait) + "...");
        pd.setIndeterminate(true);
        pd.setCancelable(false);
        
        connectButton.setOnClickListener(new OnClickListener() {
        	public void onClick(View v) {
        		if (myShareInfo.thread != null && myShareInfo.thread.isAlive()) {
        			//myShareInfo.thread.interrupt();
        			disconnectClient();
        			share_mode(false);
        		} else {
    				pd.show();
    				myShareInfo.thread = new MyThread();
    				myShareInfo.thread.start();
        		}
        	}
        });
    }

	@Override
	public Object onRetainNonConfigurationInstance() {
		myShareInfo.path = new String(pathView.getText().toString());
		myShareInfo.url = new String(urlView.getText().toString());
		return myShareInfo;
	}

/*
	@Override
	protected void onDestroy() {
		Log.i(TAG, "Finish native client before application exit");
		if (myShareInfo.thread != null && myShareInfo.thread.isAlive()) {
//			myShareInfo.thread.interrupt();
		}
		super.onDestroy();
	}
 */

	@Override
	public void onBackPressed() {
		Log.i(TAG, "Finish native client before application exit");
		if (myShareInfo.thread != null && myShareInfo.thread.isAlive()) {
			myShareInfo.thread.interrupt();
		}
		super.onBackPressed();	
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

    @Override
    public void onCreateContextMenu(ContextMenu menu, View view, ContextMenu.ContextMenuInfo menuInfo) {
            TextView textView = (TextView) view;
            menu.setHeaderTitle(textView.getText()).add(0, 0, 0, R.string.menu_copy_to_clipboard);
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        ((ClipboardManager) getSystemService(CLIPBOARD_SERVICE)).setText(urlView.getText());
        return true;
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.menu, menu);
        return true;
    }
    
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.about:
 			   Intent i = new Intent(Intent.ACTION_VIEW);
			   i.setData(Uri.parse(website_url));
			   startActivity(i);
			   break;
        }
        return true;
    }
    
    private Handler handler = new Handler();
    
    public void showServerDirectory(String url) {
    	Log.i(TAG, "-------------------------- XYU!!!!! " + url);
//		urlView.setEnabled(true);
//		urlView.setText(url);
//		urlView.setMovementMethod(LinkMovementMethod.getInstance());
    	hide_progress();
    	output(url);
    }
    
    public void updateClient(String version) {
    	Log.i(TAG, "update client to version " + version);
    }
    
    private void output(final String str) {
    	Runnable proc = new Runnable() {
    		public void run() {
    			urlView.setEnabled(true);
    			urlView.setText(str);
    			urlView.setMovementMethod(LinkMovementMethod.getInstance());
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
        		urlView.setText("");
        		if (!m) {
        			connectButton.setChecked(false);
        		}
    		}
    	};
    	handler.post(proc);
    }

    private void hide_progress() {
    	Runnable proc = new Runnable() {
    		public void run() {
    			pd.hide();
    		}
    	};
    	handler.post(proc);
    }
    
    private void show_update_alert() {
    	Runnable proc = new Runnable() {
    		public void run() {
    	    	AlertDialog alertDialog = new AlertDialog.Builder(context).create();
    	    	alertDialog.setTitle(R.string.app_name);
    	    	alertDialog.setMessage(getString(R.string.update_message));
    	    	alertDialog.setButton(getString(R.string.update_button), new DialogInterface.OnClickListener() {
    	    		   public void onClick(DialogInterface dialog, int which) {
    	    			   Intent i = new Intent(Intent.ACTION_VIEW);
    	    			   i.setData(Uri.parse(update_url));
    	    			   startActivity(i);
    	    		   }
    	    		});
    	    	alertDialog.setIcon(android.R.drawable.ic_dialog_alert);
    	    	alertDialog.show();    			
    		}
    	};
    	handler.post(proc);
    }
    
    private void show_notification_disconnect() {
    	Runnable proc = new Runnable() {
    		public void run() {
    			CharSequence title = getString(R.string.app_name);
    	        CharSequence message = getString(R.string.disconnect_message);
    	 
    	        NotificationManager notificationManager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
    	        Notification notification = new Notification(android.R.drawable.stat_notify_error, getString(R.string.app_name), System.currentTimeMillis());
    	        notification.flags |= Notification.FLAG_AUTO_CANCEL;
    	 
    	        Intent notificationIntent = new Intent(context, GetMyFilesActivity.class); //(Intent.ACTION_MAIN);
    	        //notificationIntent.setClass(context.getApplicationContext(), GetMyFilesActivity.class);
    	        notificationIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);
    	        PendingIntent pendingIntent = PendingIntent.getActivity(context, 0, notificationIntent, 0);
    	 
    	        notification.setLatestEventInfo(context, title, message, pendingIntent);
    	        notificationManager.notify(NOTIFICATION_ID, notification);
    		}
    	};
    	handler.post(proc);
    }
    
    public class MyThread extends Thread {
    	public void run() {
    		share_mode(true);
    		String[] dirs = { pathView.getText().toString() };
    		connectClient(dirs);    		
			share_mode(false);
    	}
    }
}