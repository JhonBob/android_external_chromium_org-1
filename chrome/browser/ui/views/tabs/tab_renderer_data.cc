// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/tab_renderer_data.h"

TabRendererData::TabRendererData()
    : network_state(NETWORK_STATE_NONE),
      loading(false),
      crashed_status(base::TERMINATION_STATUS_STILL_RUNNING),
      incognito(false),
      show_icon(true),
      mini(false),
      blocked(false),
      app(false),
      mode(chrome::search::Mode::MODE_DEFAULT),
      background_state(
          chrome::search::ToolbarSearchAnimator::BACKGROUND_STATE_DEFAULT),
      search_background_opacity(-1.0f) {
}

TabRendererData::~TabRendererData() {}

bool TabRendererData::Equals(const TabRendererData& data) {
  return
      favicon.pixelRef() &&
      favicon.pixelRef() == data.favicon.pixelRef() &&
      favicon.pixelRefOffset() == data.favicon.pixelRefOffset() &&
      network_state == data.network_state &&
      title == data.title &&
      url == data.url &&
      loading == data.loading &&
      crashed_status == data.crashed_status &&
      incognito == data.incognito &&
      show_icon == data.show_icon &&
      mini == data.mini &&
      blocked == data.blocked &&
      app == data.app &&
      mode == data.mode &&
      background_state == data.background_state &&
      search_background_opacity == data.search_background_opacity;
}
