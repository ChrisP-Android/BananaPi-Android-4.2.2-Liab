
package com.softwinner.update;

import android.content.Context;
import android.os.Build;
import android.os.PowerManager;
import android.os.RecoverySystem;
import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.security.GeneralSecurityException;

public class OtaUpgradeUtils {

    public static final int ERROR_INVALID_UPGRADE_PACKAGE = 0;

    public static final int ERROR_FILE_DOES_NOT_EXIT = 1;

    public static final int ERROR_FILE_IO_ERROR = 2;

    public static final String CHCHE_PARTITION = Utils.CACHE_DIR;

    public static final String DEFAULT_PACKAGE_NAME = "update.zip";
    
    private static File RECOVERY_DIR = new File("/cache/recovery");
    private static File COMMAND_FILE = new File(RECOVERY_DIR, "command");
    private static File LOG_FILE = new File(RECOVERY_DIR, "log");
    private static String LAST_PREFIX = "last_";

    private Context mContext;
    
    private boolean mDeleteSource = false;

    public OtaUpgradeUtils(Context context) {
        mContext = context;
    }

    public interface ProgressListener extends RecoverySystem.ProgressListener {
        @Override
        public void onProgress(int progress);

        public void onVerifyFailed(int errorCode, Object object);

        public void onCopyProgress(int progress);

        public void onCopyFailed(int errorCode, Object object);
    }
    public boolean upgredeFromOta(File packageFile, ProgressListener progressListener) {
        try {
            RecoverySystem.verifyPackage(packageFile, progressListener, null);
        } catch (IOException e) {
            progressListener.onVerifyFailed(ERROR_FILE_DOES_NOT_EXIT, packageFile.getPath());
            e.printStackTrace();
            return false;
        } catch (GeneralSecurityException e) {
            progressListener.onVerifyFailed(ERROR_INVALID_UPGRADE_PACKAGE, packageFile.getPath());
            e.printStackTrace();
            return false;
        }

        File cache = new File(CHCHE_PARTITION);
        if(!cache.exists()){
        	cache.mkdirs();
        }
        boolean b = copyFile(packageFile, new File(CHCHE_PARTITION + "/" + DEFAULT_PACKAGE_NAME),
                progressListener);
        if (b && packageFile.getAbsolutePath().equals(UpdateService.DOWNLOAD_OTA_PATH) && mDeleteSource) {
            packageFile.delete();
        }
        if (b) {
        	try {
				Log.v(Utils.GROBLE_TAG, new File(CHCHE_PARTITION + "/" + DEFAULT_PACKAGE_NAME).getCanonicalPath());
			} catch (IOException e) {
				e.printStackTrace();
			}
            installPackage(mContext, new File(CHCHE_PARTITION + "/" + DEFAULT_PACKAGE_NAME));
            return true;
        }
        return false;
    }

    public boolean upgradeFromOta(String packagePath, ProgressListener progressListener) {
        return upgredeFromOta(new File(packagePath), progressListener);
    }
    
    public void deleteSource(boolean b){
        mDeleteSource = b;
    }

    public static boolean copyFile(File src, File dst, ProgressListener listener) {
        long inSize = src.length();
        long outSize = 0;
        int progress = 0;
        listener.onCopyProgress(progress);
        try {
            if (!dst.exists()) {
                dst.createNewFile();
            }
            FileInputStream in = new FileInputStream(src);
            FileOutputStream out = new FileOutputStream(dst);
            int length = -1;
            byte[] buf = new byte[1024];
            while ((length = in.read(buf)) != -1) {
                out.write(buf, 0, length);
                outSize += length;
                int temp = (int) (((float) outSize) / inSize * 100);
                if (temp != progress) {
                    progress = temp;
                    listener.onCopyProgress(progress);
                }
            }
            out.flush();
            in.close();
            out.close();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            return false;
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
        return true;
    }

    public static boolean copyFile(String src, String dst, ProgressListener listener) {
        return copyFile(new File(src), new File(dst), listener);
    }

    public static void installPackage(Context context, File packageFile) {
        try {
        	String filename = packageFile.getPath();
            Log.w(Utils.GROBLE_TAG, "!!! REBOOTING TO INSTALL " + filename + " !!!");
            String arg = "--update_package=" + filename;
        	bootCommand(context,arg);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public static boolean checkVersion(long newVersion, String product) {
        return (Build.TIME <= newVersion * 1000 && (Build.DEVICE.equals(product) || Build.PRODUCT
                .equals(product)));
    }

    public static boolean checkIncVersion(String fingerprinter, String product) {
        return (Build.FINGERPRINT.equals(fingerprinter) && (Build.DEVICE.equals(product) || Build.PRODUCT
                .equals(product)));
    }
    
    private static void bootCommand(Context context, String arg) throws IOException {
        RECOVERY_DIR.mkdirs();  // In case we need it
        COMMAND_FILE.delete();  // In case it's not writable
        LOG_FILE.delete();

        FileWriter command = new FileWriter(COMMAND_FILE);
        try {
            command.write(arg);
            command.write("\n");
        } finally {
            command.close();
        }

        // Having written the command file, go ahead and reboot
        PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        pm.reboot("recovery");

        throw new IOException("Reboot failed (no permissions?)");
    }
}
