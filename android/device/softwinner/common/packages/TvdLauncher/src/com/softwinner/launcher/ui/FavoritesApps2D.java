package com.softwinner.launcher.ui;

import com.softwinner.launcher.ApplicationInfo;
import com.softwinner.launcher.Launcher;
import com.softwinner.launcher.LauncherApplication;
import com.softwinner.launcher.LauncherModel;
import com.softwinner.launcher.R;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.GridView;

import java.util.ArrayList;

public class FavoritesApps2D extends AllApps2D {
    LauncherModel mModel;
    
    private View mDelBtn;
    private View mUninstallBtn;
    private View.OnClickListener mMenuEvent = new View.OnClickListener(){
        @Override
        public void onClick(View v) {
            View selected = mGrid.getSelectedView();
            if(selected == null){
                return;
            }
            int position = mGrid.getPositionForView(selected);
            ApplicationInfo app = (ApplicationInfo) mGrid.getItemAtPosition(position);
            if(v.equals(mDelBtn)){
                LauncherModel.delFavoritesAppFromDataBase(getContext(), app.componentName);                
            }else if(v.equals(mUninstallBtn)){
                Uri packageURI = Uri.parse("package:" + app.componentName.getPackageName());
                Intent uninstallIntent = new Intent(Intent.ACTION_DELETE, packageURI);
                LauncherApplication.getLauncher().startActivity(uninstallIntent);
            }
            mMenuDialog.dismiss();
        }
    };

    public FavoritesApps2D(Context context, AttributeSet attrs) {
        super(context, attrs);
        mModel = LauncherApplication.getModel();    
    }

    @Override
    public void setApps(ArrayList<ApplicationInfo> list) {
        Log.d("filter","step");
        ArrayList<ApplicationInfo> filter = null;
        if(mModel != null){
            filter = mModel.appFiltrate(list, false);
        }
        super.setApps(filter);
    }

    @Override
    public void addApps(ArrayList<ApplicationInfo> list) {
        ArrayList<ApplicationInfo> filter = null;
        if(mModel != null){
            filter = mModel.appFiltrate(list, false);
        }
        super.addApps(filter);
    }

    @Override
    public void removeApps(ArrayList<ApplicationInfo> list) {
        ArrayList<ApplicationInfo> filter = null;
        if(mModel != null){
            filter = mModel.appFiltrate(list, false);
        }
        super.removeApps(filter);
    }

    @Override
    public void updateApps(ArrayList<ApplicationInfo> list) {
        ArrayList<ApplicationInfo> filter = null;
        if(mModel != null){
            filter = mModel.appFiltrate(list, false);
        }
        super.updateApps(filter);
    }
    
    @Override
    public Dialog createMenuDialog(Context context,ApplicationInfo app){
        Dialog dialog = new Dialog(context,R.style.menu_dialog);
        Window win = dialog.getWindow();
        WindowManager.LayoutParams params = new WindowManager.LayoutParams();
        params.y = 99999;
        params.x = 0;
        win.setAttributes(params);
        dialog.setCanceledOnTouchOutside(true);
        LayoutInflater inflater = dialog.getLayoutInflater();
        View content = inflater.inflate(R.layout.favorites_apps_menu, null);
        mDelBtn = content.findViewById(R.id.del_from_favorites);
        mUninstallBtn = content.findViewById(R.id.uninstall_app);
        if(mDelBtn != null){
            mDelBtn.setOnClickListener(mMenuEvent);
        }
        if(mUninstallBtn != null){
            mUninstallBtn.setOnClickListener(mMenuEvent);
        }
        dialog.setContentView(content);
        return dialog;
    }
}
