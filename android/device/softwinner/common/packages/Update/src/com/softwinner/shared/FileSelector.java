package com.softwinner.shared;

import com.softwinner.update.R;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import java.io.File;
import java.io.FileFilter;
import java.io.FilenameFilter;

public class FileSelector extends Activity implements OnItemClickListener {

	public static final String FILE = "file";
	public static final String ROOT = "root";

	private File mCurrentDirectory;

	private LayoutInflater mInflater;

	private FileAdapter mAdapter = new FileAdapter();

	private ListView mListView;
	
	private String mRootPath;
	
	private StorageManager mStorageManager;
	private StorageVolume [] mVolumes;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		mInflater = LayoutInflater.from(this);
		setContentView(R.layout.file_list);
		mListView = (ListView) findViewById(R.id.file_list);
		mListView.setAdapter(mAdapter);		
		mListView.setOnItemClickListener(this);
		mStorageManager = (StorageManager) getSystemService(Context.STORAGE_SERVICE);
		mVolumes = mStorageManager.getVolumeList();
		Intent intent = getIntent();
		String rootPath = intent.getStringExtra(ROOT);
		if(rootPath != null){
			mAdapter.setCurrentList(new File(rootPath).listFiles(new VolumeFilter()));
			mRootPath = rootPath;
		}else{
			setResult(RESULT_CANCELED);
		}
	}

	@Override
	public void onItemClick(AdapterView<?> adapterView, View view,
			int position, long id) {
		File selectFile = (File) adapterView.getItemAtPosition(position);
		if (selectFile.isDirectory()) {
			mCurrentDirectory = selectFile;
			FileAdapter adapter = (FileAdapter) adapterView.getAdapter();
			adapter.setCurrentList(selectFile.listFiles(new ZipFilter()));
		} else if (selectFile.isFile()) {
			Intent intent = new Intent();			
			intent.putExtra(FILE, selectFile.getPath());
			setResult(RESULT_OK, intent);
			finish();
		}
	}

	@Override
	public void onBackPressed() {
		if (mCurrentDirectory == null
				|| mCurrentDirectory.getPath().equals(mRootPath)) {
			super.onBackPressed();
		} else {
			mCurrentDirectory = mCurrentDirectory.getParentFile();
			if(mCurrentDirectory.getPath().equals(mRootPath)){
				mAdapter.setCurrentList(mCurrentDirectory.listFiles(new VolumeFilter()));
			}else{
				mAdapter.setCurrentList(mCurrentDirectory.listFiles(new ZipFilter()));
			}
		}
	}
	
	public boolean isVolume(File file){
		return false;
	}

	private class FileAdapter extends BaseAdapter {

		private File mFiles[];
		
		private class ViewHolder{
			ImageView image;
			TextView fileName;
			TextView fileSize;
		}

		public void setCurrentList(File directory) {
			mFiles = directory.listFiles();
			notifyDataSetChanged();
		}
		
		public void setCurrentList(File[] files){
			mFiles = files;
			notifyDataSetChanged();
		}

		@Override
		public int getCount() {
			return mFiles == null ? 0 : mFiles.length;
		}

		@Override
		public File getItem(int position) {
			File file = mFiles == null ? null : mFiles[position];
			return file;
		}

		@Override
		public long getItemId(int position) {
			return position;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			if (convertView == null) {
				convertView = mInflater.inflate(R.layout.file_list_item, null);
				ViewHolder holder = new ViewHolder();
				holder.image = (ImageView) convertView.findViewById(R.id.image);
				holder.fileName = (TextView)convertView.findViewById(R.id.file_name);
				holder.fileSize = (TextView)convertView.findViewById(R.id.file_size);
				convertView.setTag(holder);
			}
			ViewHolder holder = (ViewHolder) convertView.getTag();
			File file = mFiles[position];
			if (isVolume(file)) {
				holder.image.setImageResource(R.drawable.litter_disk);
			} else if (file.isDirectory()) {
				holder.image.setImageResource(R.drawable.litter_file);
			} else if (file.isFile()) {
				holder.fileSize.setText(SwFile.byteToSize(file.length()));
				holder.image.setImageResource(R.drawable.litter_zip);
			}
			holder.fileName.setText(file.getName());
			return convertView;
		}

	}
	
	private class ZipFilter implements FileFilter{

		@Override
		public boolean accept(File pathname) {
			if(pathname != null){
				if(pathname.isDirectory()){
					return true;
				}
				String path = pathname.getPath().toLowerCase();
				return path.endsWith(".zip");
			}
			return false;
		}
		
	}
	
	private class VolumeFilter implements FileFilter{

		@Override
		public boolean accept(File pathname) {
			if(pathname == null) return false;
            for(StorageVolume volume:mVolumes){
            	String path = volume.getPath();
            	if(pathname.getPath().equals(path)){
            		return true;
            	}
            }
			return false;
		}
		
	}
}
