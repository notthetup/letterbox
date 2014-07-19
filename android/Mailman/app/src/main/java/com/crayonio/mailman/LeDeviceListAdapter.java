package com.crayonio.mailman;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.view.View;
import android.widget.TextView;

/**
 * Created by chinmay on 19/7/14.
 */
public class LeDeviceListAdapter {

    private final Context mContext;
    private final Activity mParentActivity;
    private ArrayList<BluetoothDevice> mDevicesList = new ArrayList<BluetoothDevice>(1);
    public static String TAG = "Mailman";

    private BluetoothManager mBluetoothManager;
    private BluetoothAdapter mBluetoothAdapter;
    private String mBluetoothDeviceAddress;
    private BluetoothGatt mBluetoothGatt;
    private int mConnectionState = STATE_DISCONNECTED;
    private String CLIENT_CHARACTERISTIC_CONFIG = "00002902-0000-1000-8000-00805f9b34fb";

    private String mNRF8001 = "D5:2F:8B:5C:15:7E";
    private String mUARTTXService = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
    private String mUARTTXChar = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";
    //private String mUARTTXDesc = "00002902-0000-1000-8000-00805f9b34fb";

    private String mSensorTag = "BC:6A:29:AB:DB:85";
    private String mButtonService = "0000ffe0-0000-1000-8000-00805f9b34fb";
    private String mButtonDataChar = "0000ffe1-0000-1000-8000-00805f9b34fb";


    private TextView numView;

    private static final int STATE_DISCONNECTED = 0;
    private static final int STATE_CONNECTING = 1;
    private static final int STATE_CONNECTED = 2;

    public final static String ACTION_GATT_CONNECTED =
            "com.example.bluetooth.le.ACTION_GATT_CONNECTED";
    public final static String ACTION_GATT_DISCONNECTED =
            "com.example.bluetooth.le.ACTION_GATT_DISCONNECTED";
    public final static String ACTION_GATT_SERVICES_DISCOVERED =
            "com.example.bluetooth.le.ACTION_GATT_SERVICES_DISCOVERED";
    public final static String ACTION_DATA_AVAILABLE =
            "com.example.bluetooth.le.ACTION_DATA_AVAILABLE";
    public final static String EXTRA_DATA =
            "com.example.bluetooth.le.EXTRA_DATA";

    static enum SimpleKeysStatus {
        // Warning: The order in which these are defined matters.
        OFF_OFF, OFF_ON, ON_OFF, ON_ON;
    }

    public LeDeviceListAdapter(Context context, Activity parentActivity) {
        this.mContext = context;
        this.mParentActivity = parentActivity;
        numView = (TextView) mParentActivity.findViewById(R.id.num);
    }

    public void addDevice(BluetoothDevice device) {


        if (!mDevicesList.contains(device)) {
            Log.i(TAG, "Added device " + device);
            mDevicesList.add(device);
            if (device.getAddress().equalsIgnoreCase(mNRF8001)) {
                Log.i(TAG, "Found NRF");
                mBluetoothGatt = device.connectGatt(mContext, false, mGattCallback);
            } else if (device.getAddress().equalsIgnoreCase(mSensorTag)) {
                Log.i(TAG, "Found SensorTag");
                //mBluetoothGatt = device.connectGatt(mContext, false, mGattCallback);
            }
        }
    }


    private final BluetoothGattCallback mGattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status,
                                            int newState) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                mConnectionState = STATE_CONNECTED;
                Log.i(TAG, "Connected to GATT server.");
                Log.i(TAG, "Attempting to start service discovery:" +
                        mBluetoothGatt.discoverServices());

            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                mConnectionState = STATE_DISCONNECTED;
                Log.i(TAG, "Disconnected from GATT server.");
            }
        }

        @Override
        // New services discovered
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                Log.i(TAG, "Services Discovered :" );
                for (int i = 0; i < gatt.getServices().size(); i++) {
                    Log.i(TAG, i + " : " + gatt.getServices().get(i).getUuid());
                }
                BluetoothGattService myService = gatt.getService(UUID.fromString(mUARTTXService));
                if (myService != null) {
                    Log.i(TAG, "Found TX Service!!");
                    BluetoothGattCharacteristic myChar = myService.getCharacteristic(UUID.fromString(mUARTTXChar));
                    if (myChar != null){
                        Log.i(TAG,"Found TX Characteristic " + myChar.getProperties());
                        boolean stat = gatt.setCharacteristicNotification(myChar, true);
                        for (int i = 0; i < myChar.getDescriptors().size(); i++) {
                            Log.i(TAG, i + " " + myChar.getDescriptors().get(i).getUuid());
                        }
                        if(stat){
                            BluetoothGattDescriptor config = myChar.getDescriptor(UUID.fromString(CLIENT_CHARACTERISTIC_CONFIG));
                            config.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
                            gatt.writeDescriptor(config); //Enabled remotely
                        }

                    }
                }
            } else {
                Log.w(TAG, "onServicesDiscovered received: " + status);
            }
        }

        @Override
        // Result of a characteristic read operation
        public void onCharacteristicRead(BluetoothGatt gatt,
                                         BluetoothGattCharacteristic characteristic,
                                         int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                Log.i(TAG, "Read " + characteristic.getUuid());
            }
        }

        @Override
        // Characteristic notification
        public void onCharacteristicChanged(BluetoothGatt gatt,
                                            BluetoothGattCharacteristic characteristic) {
            /*
             * The key state is encoded into 1 unsigned byte.
             * bit 0: right key.
             * bit 1: left key.
             * bit 2: side key (test mode only).
             */
            final Integer encodedInteger = characteristic.getIntValue(BluetoothGattCharacteristic.FORMAT_UINT8, 0);

            Log.i(TAG, "RX Received " + encodedInteger);
            mParentActivity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    numView.setText(encodedInteger.toString());
                }
            });
        }
    };
}
