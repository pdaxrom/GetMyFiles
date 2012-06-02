package com.getmyfiles.gui;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;

public class GetMyFilesActivity extends Activity {
	private static final int ACTIVITY_CREATE = 1;
	private Button pathButton;
	private TextView pathView;
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        pathView = (TextView) findViewById(R.id.pathText);
        pathButton = (Button) findViewById(R.id.pathButton);
        pathButton.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
            	Intent myIntent = new Intent(GetMyFilesActivity.this, FileBrowserActivity.class);
            	//GetMyFilesActivity.this.startActivity(myIntent);
            	startActivityForResult(myIntent, ACTIVITY_CREATE);

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
}