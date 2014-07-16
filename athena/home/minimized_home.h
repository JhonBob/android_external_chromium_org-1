// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATHENA_HOME_MINIMIZED_HOME_H_
#define ATHENA_HOME_MINIMIZED_HOME_H_

namespace aura {
class Window;
}

namespace views {
class Widget;
}

namespace athena {

class MinimizedHomeDragDelegate {
 public:
  virtual ~MinimizedHomeDragDelegate() {}

  virtual void OnDragUpCompleted() = 0;
};

// Note that |delegate| is guaranteed to be alive as long as the returned widget
// is alive.
views::Widget* CreateMinimizedHome(aura::Window* container,
                                   MinimizedHomeDragDelegate* delegate);

}  // namespace athena

#endif  // ATHENA_HOME_MINIMIZED_HOME_H_
