// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

import android.content.Context;
import android.content.pm.ActivityInfo;
import android.view.View;
import android.webkit.WebChromeClient.CustomViewCallback;

import org.chromium.content.browser.ContentVideoViewContextDelegate;

/**
 * This further delegates the responsibility displaying full-screen video to the
 * Webview client.
 */
public class AwContentVideoViewDelegate implements ContentVideoViewContextDelegate {
    private AwContentsClient mAwContentsClient;
    private Context mContext;

    public AwContentVideoViewDelegate(AwContentsClient client, Context context) {
        mAwContentsClient = client;
        mContext = context;
    }

    @Override
    public void onShowCustomView(View view) {
        CustomViewCallback cb = new CustomViewCallback() {
            @Override
            public void onCustomViewHidden() {
                // TODO: we need to invoke ContentVideoView.onDestroyContentVideoView() here.
            }
        };
        mAwContentsClient.onShowCustomView(view,
                ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED,
                cb);
    }

    @Override
    public void onDestroyContentVideoView() {
    }

    @Override
    public Context getContext() {
        return mContext;
    }

    @Override
    public String getPlayBackErrorText() {
        return mContext.getString(
                org.chromium.content.R.string.media_player_error_text_invalid_progressive_playback);
    }

    @Override
    public String getUnknownErrorText() {
        return mContext.getString(
              org.chromium.content.R.string.media_player_error_text_unknown);
    }

    @Override
    public String getErrorButton() {
        return mContext.getString(
              org.chromium.content.R.string.media_player_error_button);
    }

    @Override
    public String getErrorTitle() {
        return mContext.getString(
              org.chromium.content.R.string.media_player_error_title);
    }

    @Override
    public String getVideoLoadingText() {
        return mContext.getString(
              org.chromium.content.R.string.media_player_loading_video);
    }

    @Override
    public View getVideoLoadingProgressView() {
        return mAwContentsClient.getVideoLoadingProgressView();
    }
}
