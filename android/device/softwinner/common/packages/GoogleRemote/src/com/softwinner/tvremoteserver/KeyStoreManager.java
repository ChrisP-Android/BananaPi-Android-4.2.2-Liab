package com.softwinner.tvremoteserver;

import com.google.polo.ssl.SslUtil;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Build;
import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.security.GeneralSecurityException;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.cert.Certificate;
import java.security.cert.X509Certificate;
import java.util.Enumeration;

import javax.net.ssl.KeyManager;
import javax.net.ssl.KeyManagerFactory;
import javax.net.ssl.TrustManager;
import javax.net.ssl.TrustManagerFactory;

/**
 * Key store manager.
 *
 */
@SuppressLint("WorldWriteableFiles")
public final class KeyStoreManager {

  private static final String LOG_TAG = "KeyStoreManager";

  private static final String KEYSTORE_FILENAME = "remoteserver.keystore";

  private static final char[] KEYSTORE_PASSWORD = "1234567890".toCharArray();

  /**
   * Alias for the remote controller (local) identity in the {@link KeyStore}.
   */
  private static final String LOCAL_IDENTITY_ALIAS = "anymote-remote-server";

  /**
   * Alias pattern for anymote server identities in the {@link KeyStore}
   */
  private static final String REMOTE_IDENTITY_ALIAS_PATTERN =  
      "anymote-client-%X";

  private final Context mContext;

  private final KeyStore mKeyStore;

  public KeyStoreManager(Context context) {
    this.mContext = context;
    this.mKeyStore = load();
  }

  /**
   * Loads key store from storage, or creates new one if storage is missing key
   * store or corrupted.
   */
  private KeyStore load() {
    KeyStore keyStore;
    try {
      keyStore = KeyStore.getInstance(KeyStore.getDefaultType());
    } catch (KeyStoreException e) {
      throw new IllegalStateException(
          "Unable to get default instance of KeyStore", e);
    }
    try {
    	Log.d("LIn", "openFileInput....");
      FileInputStream fis = mContext.openFileInput(KEYSTORE_FILENAME);
      keyStore.load(fis, KEYSTORE_PASSWORD);
    } catch (IOException e) {
      Log.v(LOG_TAG, "Unable open keystore file", e);
      keyStore = null;
    } catch (GeneralSecurityException e) {
      Log.v(LOG_TAG, "Unable open keystore file", e);
      keyStore = null;
    }

    if (keyStore != null) {
      // KeyStore loaded
      return keyStore;
    }

    try {
      keyStore = createKeyStore();
    } catch (GeneralSecurityException e) {
      throw new IllegalStateException("Unable to create identity KeyStore", e);
    }
    store(keyStore);
    return keyStore;
  }

  public boolean hasServerIdentityAlias() {
    try {
      if (!mKeyStore.containsAlias(LOCAL_IDENTITY_ALIAS)) {
        Log.e(
            LOG_TAG, "Key store missing identity for " + LOCAL_IDENTITY_ALIAS);
        return false;
      }
    } catch (KeyStoreException e) {
      Log.e(LOG_TAG, "Key store exception occurred", e);
      return false;
    }
    return true;
  }

  public void initializeKeyStore(String id) {
    clearKeyStore();
    try {
      Log.v(LOG_TAG, "Generating key pair ...");
      KeyPairGenerator kg = KeyPairGenerator.getInstance("RSA");
      KeyPair keyPair = kg.generateKeyPair();

      Log.v(LOG_TAG, "Generating certificate ...");
      String name = getCertificateName(id);
      X509Certificate cert = SslUtil.generateX509V3Certificate(keyPair, name);
      Certificate[] chain = {cert};

      Log.v(LOG_TAG, "Adding key to keystore  ...");
      mKeyStore.setKeyEntry(
          LOCAL_IDENTITY_ALIAS, keyPair.getPrivate(), null, chain);

      Log.d(LOG_TAG, "Key added!");
    } catch (GeneralSecurityException e) {
      throw new IllegalStateException("Unable to create identity KeyStore", e);
    }
    store(mKeyStore);
  }

  private static KeyStore createKeyStore() throws GeneralSecurityException {
    KeyStore keyStore = KeyStore.getInstance(KeyStore.getDefaultType());
    try {
      keyStore.load(null, KEYSTORE_PASSWORD);
    } catch (IOException e) {
      throw new GeneralSecurityException("Unable to create empty keyStore", e);
    }
    return keyStore;
  }

  @SuppressLint("WorldReadableFiles")
private void store(KeyStore keyStore) {
    try {
      	FileOutputStream fos = mContext.openFileOutput(KEYSTORE_FILENAME,Context.MODE_PRIVATE);
      	keyStore.store(fos, KEYSTORE_PASSWORD);
      	fos.close();
    } catch (IOException e) {
      throw new IllegalStateException("Unable to store keyStore", e);
    } catch (GeneralSecurityException e) {
      throw new IllegalStateException("Unable to store keyStore", e);
    }
  }

  /**
   * Stores current state of key store.
   */
  public synchronized void store() {
    store(mKeyStore);
  }

  /**
   * Returns the name that should be used in a new certificate.
   * <p>
   * The format is: "CN=anymote/PRODUCT/DEVICE/MODEL/unique identifier"
   */
  private static final String getCertificateName(String id) {
    return "CN=anymote/" + Build.PRODUCT + "/" + Build.DEVICE + "/"
        + Build.MODEL + "/" + id;
  }

  /**
   * @return key managers loaded for this service.
   */
  public synchronized KeyManager[] getKeyManagers()
      throws GeneralSecurityException {
    if (mKeyStore == null) {
      throw new NullPointerException("null mKeyStore");
    }
    KeyManagerFactory factory =
        KeyManagerFactory.getInstance(KeyManagerFactory.getDefaultAlgorithm());
    factory.init(mKeyStore, "".toCharArray());
    return factory.getKeyManagers();
  }

  /**
   * @return trust managers loaded for this service.
   */
  public synchronized TrustManager[] getTrustManagers()
      throws GeneralSecurityException {
    // Build a new set of TrustManagers based on the KeyStore.
    TrustManagerFactory tmf = TrustManagerFactory.getInstance(
        TrustManagerFactory.getDefaultAlgorithm());
    tmf.init(mKeyStore);
    return tmf.getTrustManagers();
  }

  public synchronized void storeCertificate(Certificate peerCert) {
    try {
      String alias = String.format(
          KeyStoreManager.REMOTE_IDENTITY_ALIAS_PATTERN, peerCert.hashCode());
      if (mKeyStore.containsAlias(alias)) {
        Log.w(LOG_TAG, "Deleting existing entry for " + alias);
        mKeyStore.deleteEntry(alias);
      }
      Log.i(LOG_TAG, "Adding cert to keystore: " + alias);
      mKeyStore.setCertificateEntry(alias, peerCert);
      store();
    } catch (KeyStoreException e) {
      Log.e(LOG_TAG, "Storing cert failed", e);
    }
  }

  private void clearKeyStore() {
    try {
        for (Enumeration<String> e = mKeyStore.aliases();
                e.hasMoreElements();) {
            final String alias = e.nextElement();
            Log.v(LOG_TAG, "Deleting alias: " + alias);
            mKeyStore.deleteEntry(alias);
        }
    } catch (KeyStoreException e) {
        Log.e(LOG_TAG, "Clearing certificates failed", e);
    }
    store();
  }
}