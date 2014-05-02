// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.printing;

import android.print.PrintDocumentAdapter;
import android.util.SparseArray;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;
import org.chromium.base.ThreadUtils;

/**
 * This class is responsible for communicating with its native counterpart through JNI to handle
 * the generation of PDF.  On the Java side, it works with a {@link PrintingController}
 * to talk to the framework.
 */
@JNINamespace("printing")
public class PrintingContext implements PrintingContextInterface {

    /**
     * Mapping from a file descriptor (as originally provided from
     * {@link PrintDocumentAdapter#onWrite}) to a PrintingContext.
     *
     * This is static because a static method of the native code (inside PrintingContextAndroid)
     * needs to find Java PrintingContext class corresponding to a file descriptor.
     **/
    private static final SparseArray<PrintingContext> PRINTING_CONTEXT_MAP =
            new SparseArray<PrintingContext>();

    /** The controller this object interacts with, which in turn communicates with the framework. */
    private final PrintingController mController;

    /** The pointer to the native PrintingContextAndroid object. */
    private final long mNativeObject;

    private PrintingContext(long ptr) {
        mController = PrintingControllerImpl.getInstance();
        mNativeObject = ptr;
    }

    /**
     * Updates PRINTING_CONTEXT_MAP to map from the file descriptor to this object.
     * @param fileDescriptor The file descriptor passed down from
     *                       {@link PrintDocumentAdapter#onWrite}.
     * @param delete If true, delete the entry (if it exists). If false, add it to the map.
     */
    @Override
    public void updatePrintingContextMap(int fileDescriptor, boolean delete) {
        ThreadUtils.assertOnUiThread();
        if (delete) {
            PRINTING_CONTEXT_MAP.remove(fileDescriptor);
        } else {
            PRINTING_CONTEXT_MAP.put(fileDescriptor, this);
        }
    }

    /**
     * Notifies the native side that the user just chose a new set of printing settings.
     * @param success True if the user has chosen printing settings necessary for the
     *                generation of PDF, false if there has been a problem.
     */
    @Override
    public void askUserForSettingsReply(boolean success) {
        nativeAskUserForSettingsReply(mNativeObject, success);
    }

    @CalledByNative
    public static PrintingContext create(long nativeObjectPointer) {
        ThreadUtils.assertOnUiThread();
        return new PrintingContext(nativeObjectPointer);
    }

    @CalledByNative
    public int getFileDescriptor() {
        ThreadUtils.assertOnUiThread();
        return mController.getFileDescriptor();
    }

    @CalledByNative
    public int getDpi() {
        ThreadUtils.assertOnUiThread();
        return mController.getDpi();
    }

    @CalledByNative
    public int getWidth() {
        ThreadUtils.assertOnUiThread();
        return mController.getPageWidth();
    }

    @CalledByNative
    public int getHeight() {
        ThreadUtils.assertOnUiThread();
        return mController.getPageHeight();
    }

    @CalledByNative
    public static void pdfWritingDone(int fd, boolean success) {
        ThreadUtils.assertOnUiThread();
        // TODO(cimamoglu): Do something when fd == -1.
        if (PRINTING_CONTEXT_MAP.get(fd) != null) {
            ThreadUtils.assertOnUiThread();
            PrintingContext printingContext = PRINTING_CONTEXT_MAP.get(fd);
            printingContext.mController.pdfWritingDone(success);
            PRINTING_CONTEXT_MAP.remove(fd);
        }
    }

    @CalledByNative
    public int[] getPages() {
        ThreadUtils.assertOnUiThread();
        return mController.getPageNumbers();
    }

    @CalledByNative
    public void pageCountEstimationDone(final int maxPages) {
        ThreadUtils.assertOnUiThread();
        // If the printing dialog has already finished, tell Chromium that operation is cancelled.
        if (mController.hasPrintingFinished()) {
            // NOTE: We don't call nativeAskUserForSettingsReply (hence Chromium callback in
            // AskUserForSettings callback) twice.  See {@link PrintingControllerImpl#onFinish}
            // for more explanation.
            nativeAskUserForSettingsReply(mNativeObject, false);
        } else {
            mController.setPrintingContext(this);
            mController.pageCountEstimationDone(maxPages);
        }
    }

    private native void nativeAskUserForSettingsReply(
            long nativePrintingContextAndroid,
            boolean success);
}
