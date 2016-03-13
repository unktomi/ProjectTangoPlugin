package com.projecttango.plugin;

import java.io.File;
import android.net.Uri;
import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.database.Cursor;
import android.content.Intent;
import android.widget.Toast;
import android.util.Log;

public class ProjectTangoInterface {

//Use a different activity code in order to distinguish between calls
	public static final int TANGO_INTENT_ACTIVITYCODE = 1129;
	public static final int TANGO_IMPORT_INTENT_ACTIVITYCODE = 1130;
	public static final int TANGO_EXPORT_INTENT_ACTIVITYCODE = 1131;

	public static final String EXTRA_KEY_PERMISSIONTYPE = "PERMISSIONTYPE";
	public static final String EXTRA_VALUE_ADF = "ADF_LOAD_SAVE_PERMISSION";
	private static final String EXTRA_KEY_SOURCEUUID = "SOURCE_UUID";
	private static final String EXTRA_KEY_SOURCEFILE = "SOURCE_FILE";
	private static final String EXTRA_KEY_DESTINATIONFILE = "DESTINATION_FILE";
	
	private static final String INTENT_CLASSPACKAGE = "com.projecttango.tango";
	private static final String INTENT_IMPORTEXPORT_CLASSNAME = "com.google.atap.tango.RequestImportExportActivity";


	private static final String TAG = "TANGO";
	private boolean mIsTangoConnectionReady = false;

	private boolean IsTangoAreaLearningEnabled = false;
	private boolean HasAttemptedPreviousIntentCall = false;

	public ProjectTangoInterface(boolean IsTangoAreaLearningEnabled) {

		this.IsTangoAreaLearningEnabled = IsTangoAreaLearningEnabled;
	}

	public void pause() {
		Log.w("TangoLifecycleDebugging", "Activity stopped! Connection is no longer ready...");
		mIsTangoConnectionReady = false;
	}

	public void resume(Activity currentActivity) {
		if (!mIsTangoConnectionReady && !HasAttemptedPreviousIntentCall) 
		{
	
			//Avoid attempting the event call twice!
			HasAttemptedPreviousIntentCall = true;

			//@TODO: Introduce the permissions for TangoAreaLearningSave/Load, TangoADFImport and TangoADFExport.
			if (IsTangoAreaLearningEnabled) 
			{
				//Check to see if we've already got this permission
				if(!hasPermission(currentActivity, EXTRA_VALUE_ADF))
				{
			        Log.w(TAG, "About to create a new Intent.");
					Intent ADF_LoadSave_Intent = new Intent();
					ADF_LoadSave_Intent.setAction("android.intent.action.REQUEST_TANGO_PERMISSION");
					ADF_LoadSave_Intent.putExtra(EXTRA_KEY_PERMISSIONTYPE, EXTRA_VALUE_ADF);
				
					Log.w("TangoLifecycleDebugging", "Created Intent for ADF!");
			
					currentActivity.startActivityForResult(ADF_LoadSave_Intent, TANGO_INTENT_ACTIVITYCODE);
					Log.w("TangoLifecycleDebugging", "Started ADF event activity for result!");
				}
				else
				{
			        Log.w(TAG, "Permissions were found for this permission type.");
				}
			}
			else
			{
				mIsTangoConnectionReady = true;
			}
		}
	}
	
	public void requestImportPermissions(Activity currentActivity, String Filepath)
	{
		Log.w("TangoLifecycleDebugging", "ProjectTangoInterface.Java::requestImportPermissions: Starting function!");

		Log.w("TangoLifecycleDebugging", "Filepath String = " + Filepath);
				
		Intent exportIntent = new Intent();
		exportIntent.setClassName(INTENT_CLASSPACKAGE, INTENT_IMPORTEXPORT_CLASSNAME);
		exportIntent.putExtra(EXTRA_KEY_SOURCEFILE, Filepath);
		currentActivity.startActivityForResult(exportIntent, TANGO_IMPORT_INTENT_ACTIVITYCODE);

	}

	public void requestExportPermissions(Activity currentActivity, String UUID, String Filepath)
	{
		Log.w("TangoLifecycleDebugging", "ProjectTangoInterface.Java::requestExportPermissions: Starting function!");

		Log.w("TangoLifecycleDebugging", "UUID String = " + UUID);
		Log.w("TangoLifecycleDebugging", "Filepath String = " + Filepath);
				
		//Prepare the folder for export
		getAppSpaceADFFolder(Filepath);

		Intent exportIntent = new Intent();
		exportIntent.setClassName(INTENT_CLASSPACKAGE, INTENT_IMPORTEXPORT_CLASSNAME);
		exportIntent.putExtra(EXTRA_KEY_SOURCEUUID, UUID);
		exportIntent.putExtra(EXTRA_KEY_DESTINATIONFILE, Filepath);
		currentActivity.startActivityForResult(exportIntent, TANGO_EXPORT_INTENT_ACTIVITYCODE);

	}

	public boolean handleActivityResult(int requestCode, int resultCode, Intent data, Activity currentActivity) 
	{
		Log.w("TangoLifecycleDebugging", "Starting to handle Activity Result...");

		if (requestCode == TANGO_INTENT_ACTIVITYCODE) 
		{
		
			Log.w("TangoLifecycleDebugging", "Recieved Tango Intent Activity Code!");

			// Make sure the request was successful
			if (resultCode == Activity.RESULT_CANCELED) 
			{
				Log.w("TangoLifecycleDebugging", "Tango Intent Activity: Result is User Cancelled!");
				Toast.makeText(currentActivity, "This app requires Tango permissions!", Toast.LENGTH_LONG).show();
				return false;
			}

			Log.w("TangoLifecycleDebugging", "Tango Intent Activity; result was not cancelled, setting connection flag to 'Ready'!");
			mIsTangoConnectionReady = true;

			return true;
		}
		else if (requestCode == TANGO_IMPORT_INTENT_ACTIVITYCODE)
		{
			Log.w("TangoLifecycleDebugging", "Tango Import Intent Activity; result is being handled!");
			if (resultCode == -1) 
			{
				Log.w("TangoLifecycleDebugging", "Tango Import Intent Activity: Result is Success!");
				Toast.makeText(currentActivity, "Tango Import Intent Activity: Result is Success, -1!", Toast.LENGTH_LONG).show();
				return true;
			}			
			if (resultCode == 0) 
			{
				Log.w("TangoLifecycleDebugging", "Tango Import Intent Activity: Result is Cancelled!");
				Toast.makeText(currentActivity, "Tango Import Intent Activity: Result is Cancelled, 0!", Toast.LENGTH_LONG).show();
				return false;
			}
			if (resultCode == 1) 
			{
				Log.w("TangoLifecycleDebugging", "Tango Import Intent Activity: Result is User Denied!");
				Toast.makeText(currentActivity, "Tango Import Intent Activity: Result is Denied, 1!", Toast.LENGTH_LONG).show();
				return false;
			}

			Toast.makeText(currentActivity, "Tango Import Intent Activity: Result unknown", Toast.LENGTH_LONG).show();
			Log.w("TangoLifecycleDebugging", "Tango Import Intent Activity: Result unknown!");		
			return true;
		}
		else if (requestCode == TANGO_EXPORT_INTENT_ACTIVITYCODE)
		{
			Log.w("TangoLifecycleDebugging", "Tango Export Intent Activity; result is being handled!");
			if (resultCode == -1) 
			{
				Log.w("TangoLifecycleDebugging", "Tango Export Intent Activity: Result is Success!");
				Toast.makeText(currentActivity, "Tango Export Intent Activity: Result is Success, -1!", Toast.LENGTH_LONG).show();
				return true;
			}			
			if (resultCode == 0) 
			{
				Log.w("TangoLifecycleDebugging", "Tango Export Intent Activity: Result is Cancelled!");
				Toast.makeText(currentActivity, "Tango Export Intent Activity: Result is Cancelled, 0!", Toast.LENGTH_LONG).show();
				return false;
			}
			if (resultCode == 1) 
			{
				Log.w("TangoLifecycleDebugging", "Tango Export Intent Activity: Result is User Denied!");
				Toast.makeText(currentActivity, "Tango Export Intent Activity: Result is Denied, 1!", Toast.LENGTH_LONG).show();
				return false;
			}

			Toast.makeText(currentActivity, "Tango Export Intent Activity: Result unknown", Toast.LENGTH_LONG).show();
			Log.w("TangoLifecycleDebugging", "Tango Export Intent Activity: Result unknown!");		
			return true;
		}
		
		Log.w("TangoLifecycleDebugging", "Found a non-Tango activity result!");
		return false;
	}

	private void getAppSpaceADFFolder(String Filepath) 
	{
		Log.w("TangoLifecycleDebugging", "Tango Export: Preparing new folder.");
		File file = new File(Filepath);
		if (!file.exists()) {
		Log.w("TangoLifecycleDebugging", "Tango Export: File does not yet exist.");
			file.mkdirs();
		}
		Log.w("TangoLifecycleDebugging", "Tango Export: Completed file setup!");

		return;
	}

	public static boolean hasPermission(Context context, String permissionType){
	    
	    Uri uri = Uri.parse("content://com.google.atap.tango.PermissionStatusProvider/" +
	            permissionType);
	    Cursor cursor = context.getContentResolver().query(uri, null, null, null, null);
	    if (cursor == null) {
	        return false;
	    } else {
	        return true;
	    }
	}
	
}
