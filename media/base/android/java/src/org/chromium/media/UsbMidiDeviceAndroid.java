// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.media;

import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;
import android.hardware.usb.UsbRequest;
import android.os.Handler;
import android.util.SparseArray;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;

/**
 * Owned by its native counterpart declared in usb_midi_device_android.h.
 * Refer to that class for general comments.
 */
@JNINamespace("media")
class UsbMidiDeviceAndroid {
    /**
     * A connection handle for this device.
     */
    private final UsbDeviceConnection mConnection;

    /**
     * A map from endpoint number to UsbEndpoint.
     */
    private final SparseArray<UsbEndpoint> mEndpointMap;

    /**
     * A map from UsbEndpoint to UsbRequest associated to it.
     */
    private final Map<UsbEndpoint, UsbRequest> mRequestMap;

    /**
     * The handler used for posting events on the main thread.
     */
    private final Handler mHandler;

    /**
     * True if this device is closed.
     */
    private boolean mIsClosed;

    /**
     * The identifier of this device.
     */
    private long mNativePointer;

    /**
     * Audio interface subclass code for MIDI.
     */
    static final int MIDI_SUBCLASS = 3;

    /**
     * Constructs a UsbMidiDeviceAndroid.
     * @param manager
     * @param device The USB device which this object is assocated with.
     */
    UsbMidiDeviceAndroid(UsbManager manager, UsbDevice device) {
        mConnection = manager.openDevice(device);
        mEndpointMap = new SparseArray<UsbEndpoint>();
        mRequestMap = new HashMap<UsbEndpoint, UsbRequest>();
        mHandler = new Handler();
        mIsClosed = false;
        mNativePointer = 0;

        for (int i = 0; i < device.getInterfaceCount(); ++i) {
            UsbInterface iface = device.getInterface(i);
            if (iface.getInterfaceClass() != UsbConstants.USB_CLASS_AUDIO ||
                iface.getInterfaceSubclass() != MIDI_SUBCLASS) {
                continue;
            }
            mConnection.claimInterface(iface, true);
            for (int j = 0; j < iface.getEndpointCount(); ++j) {
                UsbEndpoint endpoint = iface.getEndpoint(j);
                if (endpoint.getDirection() == UsbConstants.USB_DIR_OUT) {
                    mEndpointMap.put(endpoint.getEndpointNumber(), endpoint);
                }
            }
        }
        // Start listening for input endpoints.
        // This function will create and run a thread if there is USB-MIDI endpoints in the
        // device. Note that because UsbMidiDevice is shared among all tabs and the thread
        // will be terminated when the device is disconnected, at most one thread can be created
        // for each connected USB-MIDI device.
        startListen(device);
    }

    /**
     * Starts listening for input endpoints.
     */
    private void startListen(final UsbDevice device) {
        final Map<UsbEndpoint, ByteBuffer> bufferForEndpoints =
            new HashMap<UsbEndpoint, ByteBuffer>();

        for (int i = 0; i < device.getInterfaceCount(); ++i) {
            UsbInterface iface = device.getInterface(i);
            if (iface.getInterfaceClass() != UsbConstants.USB_CLASS_AUDIO ||
                iface.getInterfaceSubclass() != MIDI_SUBCLASS) {
                continue;
            }
            for (int j = 0; j < iface.getEndpointCount(); ++j) {
                UsbEndpoint endpoint = iface.getEndpoint(j);
                if (endpoint.getDirection() == UsbConstants.USB_DIR_IN) {
                    ByteBuffer buffer = ByteBuffer.allocate(endpoint.getMaxPacketSize());
                    UsbRequest request = new UsbRequest();
                    request.initialize(mConnection, endpoint);
                    request.queue(buffer, buffer.remaining());
                    bufferForEndpoints.put(endpoint, buffer);
                }
            }
        }
        if (bufferForEndpoints.isEmpty()) {
            return;
        }
        // bufferForEndpoints must not be accessed hereafter on this thread.
        new Thread() {
            public void run() {
                while (true) {
                    UsbRequest request = mConnection.requestWait();
                    if (request == null) {
                        // When the device is closed requestWait will fail.
                        break;
                    }
                    UsbEndpoint endpoint = request.getEndpoint();
                    if (endpoint.getDirection() != UsbConstants.USB_DIR_IN) {
                        continue;
                    }
                    ByteBuffer buffer = bufferForEndpoints.get(endpoint);
                    int length = getInputDataLength(buffer);
                    if (length > 0) {
                        buffer.rewind();
                        final byte[] bs = new byte[length];
                        buffer.get(bs, 0, length);
                        postOnDataEvent(endpoint.getEndpointNumber(), bs);
                    }
                    buffer.rewind();
                    request.queue(buffer, buffer.capacity());
                }
            }
        }.start();
    }

    /**
     * Posts a data input event to the main thread.
     */
    private void postOnDataEvent(final int endpointNumber, final byte[] bs) {
        mHandler.post(new Runnable() {
                public void run() {
                    if (mIsClosed) {
                        return;
                    }
                    nativeOnData(mNativePointer, endpointNumber, bs);
                }
            });
    }

    /**
     * Register the own native pointer.
     */
    @CalledByNative
    void registerSelf(long nativePointer) {
        mNativePointer = nativePointer;
    }

    /**
     * Sends a USB-MIDI data to the device.
     * @param endpointNumber The endpoint number of the destination endpoint.
     * @param bs The data to be sent.
     */
    @CalledByNative
    void send(int endpointNumber, byte[] bs) {
        if (mIsClosed) {
            return;
        }
        UsbEndpoint endpoint = mEndpointMap.get(endpointNumber);
        if (endpoint == null) {
            return;
        }
        UsbRequest request = mRequestMap.get(endpoint);
        if (request == null) {
            request = new UsbRequest();
            request.initialize(mConnection, endpoint);
            mRequestMap.put(endpoint, request);
        }
        request.queue(ByteBuffer.wrap(bs), bs.length);
    }

    /**
     * Returns the descriptors bytes of this device.
     * @return The descriptors bytes of this device.
     */
    @CalledByNative
    byte[] getDescriptors() {
        if (mConnection == null) {
            return new byte[0];
        }
        return mConnection.getRawDescriptors();
    }

    /**
     * Closes the device connection.
     */
    @CalledByNative
    void close() {
        mEndpointMap.clear();
        for (UsbRequest request : mRequestMap.values()) {
            request.close();
        }
        mRequestMap.clear();
        mConnection.close();
        mNativePointer = 0;
        mIsClosed = true;
    }

    /**
     * Returns the length of a USB-MIDI input.
     * Since the Android API doesn't provide us the length,
     * we calculate it manually.
     */
    private static int getInputDataLength(ByteBuffer buffer) {
        int position = buffer.position();
        // We assume that the data length is always divisable by 4.
        for (int i = 0; i < position; i += 4) {
            // Since Code Index Number 0 is reserved, it is not a valid USB-MIDI data.
            if (buffer.get(i) == 0) {
                return i;
            }
        }
        return position;
    }

    private static native void nativeOnData(long nativeUsbMidiDeviceAndroid,
                                            int endpointNumber,
                                            byte[] data);
}
