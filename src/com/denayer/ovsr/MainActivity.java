package com.denayer.ovsr;

import java.io.File;
import java.io.FileOutputStream;
import java.text.SimpleDateFormat;
import java.util.Date;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Point;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.provider.MediaStore;
import android.view.Display;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.ImageButton;


public class MainActivity extends Activity {
    private Uri mImageCaptureUri;
    private ImageButton Input_button;
    private ImageButton Output_button;
    private Bitmap bitmap   = null;
    private File file;
    
    private static final int PICK_FROM_CAMERA = 1;
    private static final int PICK_FROM_FILE = 2;
 
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
 
        setContentView(R.layout.activity_main);
 
        final String [] items           = new String [] {"From Camera", "From SD Card"};
        ArrayAdapter<String> adapter  = new ArrayAdapter<String> (this, android.R.layout.select_dialog_item,items);
        AlertDialog.Builder builder     = new AlertDialog.Builder(this);
 
        builder.setTitle("Select Image");
        builder.setAdapter( adapter, new DialogInterface.OnClickListener() {
            @SuppressLint("SimpleDateFormat")
			public void onClick( DialogInterface dialog, int item ) {
                if (item == 0) {
                    Intent intent    = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);

                    //save file
                    String SavePath = Environment.getExternalStorageDirectory().toString();
                    SimpleDateFormat formatter = new SimpleDateFormat("yyMMddHHmmss");
        	        Date now = new Date();
        	        String fileName = formatter.format(now) + ".jpg";
                    file = new File(SavePath, "OVSR"+fileName);
                    
                    mImageCaptureUri = Uri.fromFile(file);
 
                    try {
                        intent.putExtra(android.provider.MediaStore.EXTRA_OUTPUT, mImageCaptureUri);
                        intent.putExtra("return-data", true);
 
                        startActivityForResult(intent, PICK_FROM_CAMERA);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
 
                    dialog.cancel();
                } else {
                    Intent intent = new Intent();
 
                    intent.setType("image/*");
                    intent.setAction(Intent.ACTION_GET_CONTENT);
 
                    startActivityForResult(Intent.createChooser(intent, "Complete action using"), PICK_FROM_FILE);
                }
            }
        } );
 
        final AlertDialog dialog = builder.create();
 
        ((ImageButton) findViewById(R.id.imageButton1)).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dialog.show();
            }
        });
        
        createBoxes();
    }

	@Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode != RESULT_OK) return;
        String path     = "";
 
        if (requestCode == PICK_FROM_FILE) {
            mImageCaptureUri = data.getData();
            path = getRealPathFromURI(mImageCaptureUri); //from Gallery
 
            if (path == null)
                path = mImageCaptureUri.getPath(); //from File Manager
 
            if (path != null)
                bitmap  = BitmapFactory.decodeFile(path);
        } else {
            path    = mImageCaptureUri.getPath();
            bitmap  = BitmapFactory.decodeFile(path);
            try {                
                FileOutputStream out = new FileOutputStream(file);
                int BHeight = bitmap.getHeight()/2;
                int BWidth = bitmap.getWidth()/2;
                bitmap = Bitmap.createScaledBitmap(bitmap, BWidth, BHeight, false);
                bitmap.compress(Bitmap.CompressFormat.JPEG, 100, out);
                out.flush();
                out.close();
                
                MediaStore.Images.Media.insertImage(getContentResolver(),file.getAbsolutePath(),file.getName(),file.getName());

    	     } catch (Exception e) {
    	            e.printStackTrace();
    	     }
        }
        Display display = getWindowManager().getDefaultDisplay();
        Point size = new Point();
        display.getSize(size);
        int width = size.x/2 - 15;
        int height = size.y/2 - 15;
        
         
        bitmap = Bitmap.createScaledBitmap(bitmap, width, height, false);
        Input_button = (ImageButton)findViewById(R.id.imageButton1);
        Input_button.setImageBitmap(bitmap);
        Output_button = (ImageButton)findViewById(R.id.imageButton2);
        Output_button.setImageBitmap(bitmap);
        
        System.gc();
    }
 
	public String getRealPathFromURI(Uri contentUri) {
        String [] proj      = {MediaStore.Images.Media.DATA};
		Cursor cursor       = getContentResolver().query( contentUri, proj, null, null,null);
 
        if (cursor == null) return null;
 
        int column_index    = cursor.getColumnIndexOrThrow(MediaStore.Images.Media.DATA);
 
        cursor.moveToFirst();
 
        return cursor.getString(column_index);
    }
	public void createBoxes()
	{
        //choose box voor opencl of renderscript te selecteren
        final String [] itemsEdgeBox           = new String [] {"Opencl", "Renderscript"};
        ArrayAdapter<String> adapterEdgeBox  = new ArrayAdapter<String> (this, android.R.layout.select_dialog_item,itemsEdgeBox);
        AlertDialog.Builder builderEdgeBox     = new AlertDialog.Builder(this);
 
        builderEdgeBox.setTitle("Select Language");
        builderEdgeBox.setAdapter( adapterEdgeBox, new DialogInterface.OnClickListener() {
			public void onClick( DialogInterface dialogEdgeBox, int item ) {
                if (item == 0) {
                	//opencl
                } else {
                	//renderscipt
                }
            }
        } );
        final AlertDialog dialogEdgeBox = builderEdgeBox.create();
        
        (findViewById(R.id.EdgeButton)).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dialogEdgeBox.show();
            }
        });
        
        ArrayAdapter<String> adapterSharpenBox  = new ArrayAdapter<String> (this, android.R.layout.select_dialog_item,itemsEdgeBox);        
        AlertDialog.Builder builderSharpenBox     = new AlertDialog.Builder(this);
        builderSharpenBox.setTitle("Select Language");
        builderSharpenBox.setAdapter( adapterSharpenBox, new DialogInterface.OnClickListener() {
			public void onClick( DialogInterface dialogEdgeBox, int item ) {
                if (item == 0) {
                	//opencl
                } else {
                	//renderscipt
                }
            }
        } );
        final AlertDialog dialogSharpenBox = builderSharpenBox.create();
        
        (findViewById(R.id.SharpenButton)).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dialogSharpenBox.show();
            }
        });
        ArrayAdapter<String> adapterMediaanBox  = new ArrayAdapter<String> (this, android.R.layout.select_dialog_item,itemsEdgeBox);        
        AlertDialog.Builder builderMediaanBox     = new AlertDialog.Builder(this);
        builderMediaanBox.setTitle("Select Language");
        builderMediaanBox.setAdapter( adapterMediaanBox, new DialogInterface.OnClickListener() {
			public void onClick( DialogInterface dialogEdgeBox, int item ) {
                if (item == 0) {
                	//opencl
                } else {
                	//renderscipt
                }
            }
        } );
        final AlertDialog dialogMediaanBox = builderSharpenBox.create();
        
        (findViewById(R.id.MediaanButton)).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
            	dialogMediaanBox.show();
            }
        });
        //einde choose box
	}
}