// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/workspace_controller.h"

#include <map>

#include "ash/ash_switches.h"
#include "ash/root_window_controller.h"
#include "ash/screen_util.h"
#include "ash/shelf/shelf_layout_manager.h"
#include "ash/shelf/shelf_widget.h"
#include "ash/shell.h"
#include "ash/shell_window_ids.h"
#include "ash/system/status_area_widget.h"
#include "ash/test/ash_test_base.h"
#include "ash/test/shell_test_api.h"
#include "ash/test/test_shelf_delegate.h"
#include "ash/wm/panels/panel_layout_manager.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_util.h"
#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/root_window.h"
#include "ui/aura/test/event_generator.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/test/test_windows.h"
#include "ui/aura/window.h"
#include "ui/base/hit_test.h"
#include "ui/base/ui_base_types.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/scoped_animation_duration_scale_mode.h"
#include "ui/events/event_utils.h"
#include "ui/gfx/screen.h"
#include "ui/views/corewm/window_animations.h"
#include "ui/views/corewm/window_util.h"
#include "ui/views/widget/widget.h"

using aura::Window;

namespace ash {
namespace internal {

// Returns a string containing the names of all the children of |window| (in
// order). Each entry is separated by a space.
std::string GetWindowNames(const aura::Window* window) {
  std::string result;
  for (size_t i = 0; i < window->children().size(); ++i) {
    if (i != 0)
      result += " ";
    result += window->children()[i]->name();
  }
  return result;
}

// Returns a string containing the names of windows corresponding to each of the
// child layers of |window|'s layer. Any layers that don't correspond to a child
// Window of |window| are ignored. The result is ordered based on the layer
// ordering.
std::string GetLayerNames(const aura::Window* window) {
  typedef std::map<const ui::Layer*, std::string> LayerToWindowNameMap;
  LayerToWindowNameMap window_names;
  for (size_t i = 0; i < window->children().size(); ++i) {
    window_names[window->children()[i]->layer()] =
        window->children()[i]->name();
  }

  std::string result;
  const std::vector<ui::Layer*>& layers(window->layer()->children());
  for (size_t i = 0; i < layers.size(); ++i) {
    LayerToWindowNameMap::iterator layer_i =
        window_names.find(layers[i]);
    if (layer_i != window_names.end()) {
      if (!result.empty())
        result += " ";
      result += layer_i->second;
    }
  }
  return result;
}

class WorkspaceControllerTest : public test::AshTestBase {
 public:
  WorkspaceControllerTest() {}
  virtual ~WorkspaceControllerTest() {}

  aura::Window* CreateTestWindowUnparented() {
    aura::Window* window = new aura::Window(NULL);
    window->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_NORMAL);
    window->SetType(ui::wm::WINDOW_TYPE_NORMAL);
    window->Init(aura::WINDOW_LAYER_TEXTURED);
    return window;
  }

  aura::Window* CreateTestWindow() {
    aura::Window* window = new aura::Window(NULL);
    window->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_NORMAL);
    window->SetType(ui::wm::WINDOW_TYPE_NORMAL);
    window->Init(aura::WINDOW_LAYER_TEXTURED);
    ParentWindowInPrimaryRootWindow(window);
    return window;
  }

  aura::Window* CreateBrowserLikeWindow(const gfx::Rect& bounds) {
    aura::Window* window = CreateTestWindow();
    window->SetBounds(bounds);
    wm::WindowState* window_state = wm::GetWindowState(window);
    window_state->set_window_position_managed(true);
    window->Show();
    return window;
  }

  aura::Window* CreatePopupLikeWindow(const gfx::Rect& bounds) {
    aura::Window* window = CreateTestWindowInShellWithBounds(bounds);
    window->Show();
    return window;
  }

  aura::Window* CreateTestPanel(aura::WindowDelegate* delegate,
                                const gfx::Rect& bounds) {
    aura::Window* window = CreateTestWindowInShellWithDelegateAndType(
        delegate,
        ui::wm::WINDOW_TYPE_PANEL,
        0,
        bounds);
    test::TestShelfDelegate* shelf_delegate =
        test::TestShelfDelegate::instance();
    shelf_delegate->AddShelfItem(window);
    PanelLayoutManager* manager =
        static_cast<PanelLayoutManager*>(
            Shell::GetContainer(window->GetRootWindow(),
                                internal::kShellWindowId_PanelContainer)->
                                layout_manager());
    manager->Relayout();
    return window;
  }

  aura::Window* GetDesktop() {
    return Shell::GetContainer(Shell::GetPrimaryRootWindow(),
                               kShellWindowId_DefaultContainer);
  }

  gfx::Rect GetFullscreenBounds(aura::Window* window) {
    return Shell::GetScreen()->GetDisplayNearestWindow(window).bounds();
  }

  ShelfWidget* shelf_widget() {
    return Shell::GetPrimaryRootWindowController()->shelf();
  }

  ShelfLayoutManager* shelf_layout_manager() {
    return Shell::GetPrimaryRootWindowController()->GetShelfLayoutManager();
  }

  bool GetWindowOverlapsShelf() {
    return shelf_layout_manager()->window_overlaps_shelf();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(WorkspaceControllerTest);
};

// Assertions around adding a normal window.
TEST_F(WorkspaceControllerTest, AddNormalWindowWhenEmpty) {
  scoped_ptr<Window> w1(CreateTestWindow());
  w1->SetBounds(gfx::Rect(0, 0, 250, 251));

  wm::WindowState* window_state = wm::GetWindowState(w1.get());

  EXPECT_FALSE(window_state->HasRestoreBounds());

  w1->Show();

  EXPECT_FALSE(window_state->HasRestoreBounds());

  ASSERT_TRUE(w1->layer() != NULL);
  EXPECT_TRUE(w1->layer()->visible());

  EXPECT_EQ("0,0 250x251", w1->bounds().ToString());

  EXPECT_EQ(w1.get(), GetDesktop()->children()[0]);
}

// Assertions around maximizing/unmaximizing.
TEST_F(WorkspaceControllerTest, SingleMaximizeWindow) {
  scoped_ptr<Window> w1(CreateTestWindow());
  w1->SetBounds(gfx::Rect(0, 0, 250, 251));

  w1->Show();
  wm::ActivateWindow(w1.get());

  EXPECT_TRUE(wm::IsActiveWindow(w1.get()));

  ASSERT_TRUE(w1->layer() != NULL);
  EXPECT_TRUE(w1->layer()->visible());

  EXPECT_EQ("0,0 250x251", w1->bounds().ToString());

  // Maximize the window.
  w1->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_MAXIMIZED);

  EXPECT_TRUE(wm::IsActiveWindow(w1.get()));

  EXPECT_EQ(w1.get(), GetDesktop()->children()[0]);
  EXPECT_EQ(ScreenUtil::GetMaximizedWindowBoundsInParent(w1.get()).width(),
            w1->bounds().width());
  EXPECT_EQ(ScreenUtil::GetMaximizedWindowBoundsInParent(w1.get()).height(),
            w1->bounds().height());

  // Restore the window.
  w1->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_NORMAL);

  EXPECT_EQ(w1.get(), GetDesktop()->children()[0]);
  EXPECT_EQ("0,0 250x251", w1->bounds().ToString());
}

// Assertions around two windows and toggling one to be fullscreen.
TEST_F(WorkspaceControllerTest, FullscreenWithNormalWindow) {
  scoped_ptr<Window> w1(CreateTestWindow());
  scoped_ptr<Window> w2(CreateTestWindow());
  w1->SetBounds(gfx::Rect(0, 0, 250, 251));
  w1->Show();

  ASSERT_TRUE(w1->layer() != NULL);
  EXPECT_TRUE(w1->layer()->visible());

  w2->SetBounds(gfx::Rect(0, 0, 50, 51));
  w2->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_FULLSCREEN);
  w2->Show();
  wm::ActivateWindow(w2.get());

  // Both windows should be in the same workspace.
  EXPECT_EQ(w1.get(), GetDesktop()->children()[0]);
  EXPECT_EQ(w2.get(), GetDesktop()->children()[1]);

  gfx::Rect work_area(
      ScreenUtil::GetMaximizedWindowBoundsInParent(w1.get()));
  EXPECT_EQ(work_area.width(), w2->bounds().width());
  EXPECT_EQ(work_area.height(), w2->bounds().height());

  // Restore w2, which should then go back to one workspace.
  w2->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_NORMAL);
  EXPECT_EQ(50, w2->bounds().width());
  EXPECT_EQ(51, w2->bounds().height());
  EXPECT_TRUE(wm::IsActiveWindow(w2.get()));
}

// Makes sure requests to change the bounds of a normal window go through.
TEST_F(WorkspaceControllerTest, ChangeBoundsOfNormalWindow) {
  scoped_ptr<Window> w1(CreateTestWindow());
  w1->Show();

  // Setting the bounds should go through since the window is in the normal
  // workspace.
  w1->SetBounds(gfx::Rect(0, 0, 200, 500));
  EXPECT_EQ(200, w1->bounds().width());
  EXPECT_EQ(500, w1->bounds().height());
}

// Verifies the bounds is not altered when showing and grid is enabled.
TEST_F(WorkspaceControllerTest, SnapToGrid) {
  scoped_ptr<Window> w1(CreateTestWindowUnparented());
  w1->SetBounds(gfx::Rect(1, 6, 25, 30));
  ParentWindowInPrimaryRootWindow(w1.get());
  // We are not aligning this anymore this way. When the window gets shown
  // the window is expected to be handled differently, but this cannot be
  // tested with this test. So the result of this test should be that the
  // bounds are exactly as passed in.
  EXPECT_EQ("1,6 25x30", w1->bounds().ToString());
}

// Assertions around a fullscreen window.
TEST_F(WorkspaceControllerTest, SingleFullscreenWindow) {
  scoped_ptr<Window> w1(CreateTestWindow());
  w1->SetBounds(gfx::Rect(0, 0, 250, 251));
  // Make the window fullscreen.
  w1->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_FULLSCREEN);
  w1->Show();
  wm::ActivateWindow(w1.get());

  EXPECT_EQ(w1.get(), GetDesktop()->children()[0]);
  EXPECT_EQ(GetFullscreenBounds(w1.get()).width(), w1->bounds().width());
  EXPECT_EQ(GetFullscreenBounds(w1.get()).height(), w1->bounds().height());

  // Restore the window. Use SHOW_STATE_DEFAULT as that is what we'll end up
  // with when using views::Widget.
  w1->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_DEFAULT);
  EXPECT_EQ("0,0 250x251", w1->bounds().ToString());

  EXPECT_EQ(w1.get(), GetDesktop()->children()[0]);
  EXPECT_EQ(250, w1->bounds().width());
  EXPECT_EQ(251, w1->bounds().height());

  // Back to fullscreen.
  w1->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_FULLSCREEN);
  EXPECT_EQ(w1.get(), GetDesktop()->children()[0]);
  EXPECT_EQ(GetFullscreenBounds(w1.get()).width(), w1->bounds().width());
  EXPECT_EQ(GetFullscreenBounds(w1.get()).height(), w1->bounds().height());
  wm::WindowState* window_state = wm::GetWindowState(w1.get());

  ASSERT_TRUE(window_state->HasRestoreBounds());
  EXPECT_EQ("0,0 250x251", window_state->GetRestoreBoundsInScreen().ToString());
}

// Assertions around minimizing a single window.
TEST_F(WorkspaceControllerTest, MinimizeSingleWindow) {
  scoped_ptr<Window> w1(CreateTestWindow());

  w1->Show();

  w1->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_MINIMIZED);
  EXPECT_FALSE(w1->layer()->IsDrawn());

  // Show the window.
  w1->Show();
  EXPECT_TRUE(wm::GetWindowState(w1.get())->IsNormalShowState());
  EXPECT_TRUE(w1->layer()->IsDrawn());
}

// Assertions around minimizing a fullscreen window.
TEST_F(WorkspaceControllerTest, MinimizeFullscreenWindow) {
  // Two windows, w1 normal, w2 fullscreen.
  scoped_ptr<Window> w1(CreateTestWindow());
  scoped_ptr<Window> w2(CreateTestWindow());
  w1->Show();
  w2->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_FULLSCREEN);
  w2->Show();

  wm::WindowState* w1_state = wm::GetWindowState(w1.get());
  wm::WindowState* w2_state = wm::GetWindowState(w2.get());

  w2_state->Activate();

  // Minimize w2.
  w2->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_MINIMIZED);
  EXPECT_TRUE(w1->layer()->IsDrawn());
  EXPECT_FALSE(w2->layer()->IsDrawn());

  // Show the window, which should trigger unminimizing.
  w2->Show();
  w2_state->Activate();

  EXPECT_TRUE(w2_state->IsFullscreen());
  EXPECT_TRUE(w1->layer()->IsDrawn());
  EXPECT_TRUE(w2->layer()->IsDrawn());

  // Minimize the window, which should hide the window.
  EXPECT_TRUE(w2_state->IsActive());
  w2_state->Minimize();
  EXPECT_FALSE(w2_state->IsActive());
  EXPECT_FALSE(w2->layer()->IsDrawn());
  EXPECT_TRUE(w1_state->IsActive());

  // Make the window normal.
  w2->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_NORMAL);
  EXPECT_EQ(w1.get(), GetDesktop()->children()[0]);
  EXPECT_EQ(w2.get(), GetDesktop()->children()[1]);
  EXPECT_TRUE(w2->layer()->IsDrawn());
}

// Verifies ShelfLayoutManager's visibility/auto-hide state is correctly
// updated.
TEST_F(WorkspaceControllerTest, ShelfStateUpdated) {
  // Since ShelfLayoutManager queries for mouse location, move the mouse so
  // it isn't over the shelf.
  aura::test::EventGenerator generator(
      Shell::GetPrimaryRootWindow(), gfx::Point());
  generator.MoveMouseTo(0, 0);

  scoped_ptr<Window> w1(CreateTestWindow());
  const gfx::Rect w1_bounds(0, 1, 101, 102);
  ShelfLayoutManager* shelf = shelf_layout_manager();
  shelf->SetAutoHideBehavior(ash::SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS);
  const gfx::Rect touches_shelf_bounds(
      0, shelf->GetIdealBounds().y() - 10, 101, 102);
  // Move |w1| to overlap the shelf.
  w1->SetBounds(touches_shelf_bounds);
  EXPECT_FALSE(GetWindowOverlapsShelf());

  // A visible ignored window should not trigger the overlap.
  scoped_ptr<Window> w_ignored(CreateTestWindow());
  w_ignored->SetBounds(touches_shelf_bounds);
  wm::GetWindowState(&(*w_ignored))->set_ignored_by_shelf(true);
  w_ignored->Show();
  EXPECT_FALSE(GetWindowOverlapsShelf());

  // Make it visible, since visible shelf overlaps should be true.
  w1->Show();
  EXPECT_TRUE(GetWindowOverlapsShelf());

  wm::ActivateWindow(w1.get());
  w1->SetBounds(w1_bounds);
  w1->Show();
  wm::ActivateWindow(w1.get());

  EXPECT_EQ(SHELF_AUTO_HIDE, shelf->visibility_state());

  // Maximize the window.
  w1->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_MAXIMIZED);
  EXPECT_EQ(SHELF_AUTO_HIDE, shelf->visibility_state());
  EXPECT_EQ(SHELF_AUTO_HIDE_HIDDEN, shelf->auto_hide_state());

  // Restore.
  w1->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_NORMAL);
  EXPECT_EQ(SHELF_AUTO_HIDE, shelf->visibility_state());
  EXPECT_EQ("0,1 101x102", w1->bounds().ToString());

  // Fullscreen.
  w1->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_FULLSCREEN);
  EXPECT_EQ(SHELF_HIDDEN, shelf->visibility_state());

  // Normal.
  w1->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_NORMAL);
  EXPECT_EQ(SHELF_AUTO_HIDE, shelf->visibility_state());
  EXPECT_EQ("0,1 101x102", w1->bounds().ToString());
  EXPECT_FALSE(GetWindowOverlapsShelf());

  // Move window so it obscures shelf.
  w1->SetBounds(touches_shelf_bounds);
  EXPECT_TRUE(GetWindowOverlapsShelf());

  // Move it back.
  w1->SetBounds(w1_bounds);
  EXPECT_FALSE(GetWindowOverlapsShelf());

  // Maximize again.
  w1->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_MAXIMIZED);
  EXPECT_EQ(SHELF_AUTO_HIDE, shelf->visibility_state());
  EXPECT_EQ(SHELF_AUTO_HIDE_HIDDEN, shelf->auto_hide_state());

  // Minimize.
  w1->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_MINIMIZED);
  EXPECT_EQ(SHELF_AUTO_HIDE, shelf->visibility_state());

  // Since the restore from minimize will restore to the pre-minimize
  // state (tested elsewhere), we abandon the current size and restore
  // rect and set them to the window.
  wm::WindowState* window_state = wm::GetWindowState(w1.get());

  gfx::Rect restore = window_state->GetRestoreBoundsInScreen();
  EXPECT_EQ("0,0 800x597", w1->bounds().ToString());
  EXPECT_EQ("0,1 101x102", restore.ToString());
  window_state->ClearRestoreBounds();
  w1->SetBounds(restore);

  // Restore.
  w1->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_NORMAL);
  EXPECT_EQ(SHELF_AUTO_HIDE, shelf->visibility_state());
  EXPECT_EQ("0,1 101x102", w1->bounds().ToString());

  // Create another window, maximized.
  scoped_ptr<Window> w2(CreateTestWindow());
  w2->SetBounds(gfx::Rect(10, 11, 250, 251));
  w2->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_MAXIMIZED);
  w2->Show();
  wm::ActivateWindow(w2.get());
  EXPECT_EQ(SHELF_AUTO_HIDE, shelf->visibility_state());
  EXPECT_EQ(SHELF_AUTO_HIDE_HIDDEN, shelf->auto_hide_state());
  EXPECT_EQ("0,1 101x102", w1->bounds().ToString());

  // Switch to w1.
  wm::ActivateWindow(w1.get());
  EXPECT_EQ(SHELF_AUTO_HIDE, shelf->visibility_state());
  EXPECT_EQ("0,1 101x102", w1->bounds().ToString());
  EXPECT_EQ(ScreenUtil::GetMaximizedWindowBoundsInParent(
                w2->parent()).ToString(),
            w2->bounds().ToString());

  // Switch to w2.
  wm::ActivateWindow(w2.get());
  EXPECT_EQ(SHELF_AUTO_HIDE, shelf->visibility_state());
  EXPECT_EQ(SHELF_AUTO_HIDE_HIDDEN, shelf->auto_hide_state());
  EXPECT_EQ("0,1 101x102", w1->bounds().ToString());
  EXPECT_EQ(ScreenUtil::GetMaximizedWindowBoundsInParent(w2.get()).ToString(),
            w2->bounds().ToString());

  // Turn off auto-hide, switch back to w2 (maximized) and verify overlap.
  shelf->SetAutoHideBehavior(SHELF_AUTO_HIDE_BEHAVIOR_NEVER);
  wm::ActivateWindow(w2.get());
  EXPECT_FALSE(GetWindowOverlapsShelf());

  // Move w1 to overlap shelf, it shouldn't change window overlaps shelf since
  // the window isn't in the visible workspace.
  w1->SetBounds(touches_shelf_bounds);
  EXPECT_FALSE(GetWindowOverlapsShelf());

  // Activate w1. Although w1 is visible, the overlap state is still false since
  // w2 is maximized.
  wm::ActivateWindow(w1.get());
  EXPECT_FALSE(GetWindowOverlapsShelf());

  // Restore w2.
  w2->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_NORMAL);
  EXPECT_TRUE(GetWindowOverlapsShelf());
}

// Verifies going from maximized to minimized sets the right state for painting
// the background of the launcher.
TEST_F(WorkspaceControllerTest, MinimizeResetsVisibility) {
  scoped_ptr<Window> w1(CreateTestWindow());
  w1->Show();
  wm::ActivateWindow(w1.get());
  w1->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_MAXIMIZED);
  EXPECT_EQ(SHELF_BACKGROUND_MAXIMIZED, shelf_widget()->GetBackgroundType());

  w1->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_MINIMIZED);
  EXPECT_EQ(SHELF_VISIBLE,
            shelf_layout_manager()->visibility_state());
  EXPECT_EQ(SHELF_BACKGROUND_DEFAULT, shelf_widget()->GetBackgroundType());
}

// Verifies window visibility during various workspace changes.
TEST_F(WorkspaceControllerTest, VisibilityTests) {
  scoped_ptr<Window> w1(CreateTestWindow());
  w1->Show();
  EXPECT_TRUE(w1->IsVisible());
  EXPECT_EQ(1.0f, w1->layer()->GetCombinedOpacity());

  // Create another window, activate it and make it fullscreen.
  scoped_ptr<Window> w2(CreateTestWindow());
  w2->Show();
  wm::ActivateWindow(w2.get());
  w2->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_FULLSCREEN);
  EXPECT_TRUE(w2->IsVisible());
  EXPECT_EQ(1.0f, w2->layer()->GetCombinedOpacity());
  EXPECT_TRUE(w1->IsVisible());

  // Switch to w1. |w1| should be visible on top of |w2|.
  wm::ActivateWindow(w1.get());
  EXPECT_TRUE(w1->IsVisible());
  EXPECT_EQ(1.0f, w1->layer()->GetCombinedOpacity());
  EXPECT_TRUE(w2->IsVisible());

  // Switch back to |w2|.
  wm::ActivateWindow(w2.get());
  EXPECT_TRUE(w2->IsVisible());
  EXPECT_EQ(1.0f, w2->layer()->GetCombinedOpacity());
  EXPECT_TRUE(w1->IsVisible());

  // Restore |w2|, both windows should be visible.
  w2->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_NORMAL);
  EXPECT_TRUE(w1->IsVisible());
  EXPECT_EQ(1.0f, w1->layer()->GetCombinedOpacity());
  EXPECT_TRUE(w2->IsVisible());
  EXPECT_EQ(1.0f, w2->layer()->GetCombinedOpacity());

  // Make |w2| fullscreen again, then close it.
  w2->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_FULLSCREEN);
  w2->Hide();
  EXPECT_FALSE(w2->IsVisible());
  EXPECT_EQ(1.0f, w1->layer()->GetCombinedOpacity());
  EXPECT_TRUE(w1->IsVisible());

  // Create |w2| and maximize it.
  w2.reset(CreateTestWindow());
  w2->Show();
  wm::ActivateWindow(w2.get());
  w2->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_MAXIMIZED);
  EXPECT_TRUE(w2->IsVisible());
  EXPECT_EQ(1.0f, w2->layer()->GetCombinedOpacity());
  EXPECT_TRUE(w1->IsVisible());

  // Close |w2|.
  w2.reset();
  EXPECT_EQ(1.0f, w1->layer()->GetCombinedOpacity());
  EXPECT_TRUE(w1->IsVisible());
}

// Verifies windows that are offscreen don't move when switching workspaces.
TEST_F(WorkspaceControllerTest, DontMoveOnSwitch) {
  aura::test::EventGenerator generator(
      Shell::GetPrimaryRootWindow(), gfx::Point());
  generator.MoveMouseTo(0, 0);

  scoped_ptr<Window> w1(CreateTestWindow());
  ShelfLayoutManager* shelf = shelf_layout_manager();
  const gfx::Rect touches_shelf_bounds(
      0, shelf->GetIdealBounds().y() - 10, 101, 102);
  // Move |w1| to overlap the shelf.
  w1->SetBounds(touches_shelf_bounds);
  w1->Show();
  wm::ActivateWindow(w1.get());

  // Create another window and maximize it.
  scoped_ptr<Window> w2(CreateTestWindow());
  w2->SetBounds(gfx::Rect(10, 11, 250, 251));
  w2->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_MAXIMIZED);
  w2->Show();
  wm::ActivateWindow(w2.get());

  // Switch to w1.
  wm::ActivateWindow(w1.get());
  EXPECT_EQ(touches_shelf_bounds.ToString(), w1->bounds().ToString());
}

// Verifies that windows that are completely offscreen move when switching
// workspaces.
TEST_F(WorkspaceControllerTest, MoveOnSwitch) {
  aura::test::EventGenerator generator(
      Shell::GetPrimaryRootWindow(), gfx::Point());
  generator.MoveMouseTo(0, 0);

  scoped_ptr<Window> w1(CreateTestWindow());
  ShelfLayoutManager* shelf = shelf_layout_manager();
  const gfx::Rect w1_bounds(0, shelf->GetIdealBounds().y(), 100, 200);
  // Move |w1| so that the top edge is the same as the top edge of the shelf.
  w1->SetBounds(w1_bounds);
  w1->Show();
  wm::ActivateWindow(w1.get());
  EXPECT_EQ(w1_bounds.ToString(), w1->bounds().ToString());

  // Create another window and maximize it.
  scoped_ptr<Window> w2(CreateTestWindow());
  w2->SetBounds(gfx::Rect(10, 11, 250, 251));
  w2->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_MAXIMIZED);
  w2->Show();
  wm::ActivateWindow(w2.get());

  // Increase the size of the WorkAreaInsets. This would make |w1| fall
  // completely out of the display work area.
  gfx::Insets insets =
      Shell::GetScreen()->GetPrimaryDisplay().GetWorkAreaInsets();
  insets.Set(0, 0, insets.bottom() + 30, 0);
  Shell::GetInstance()->SetDisplayWorkAreaInsets(w1.get(), insets);

  // Switch to w1. The window should have moved.
  wm::ActivateWindow(w1.get());
  EXPECT_NE(w1_bounds.ToString(), w1->bounds().ToString());
}

namespace {

// WindowDelegate used by DontCrashOnChangeAndActivate.
class DontCrashOnChangeAndActivateDelegate
    : public aura::test::TestWindowDelegate {
 public:
  DontCrashOnChangeAndActivateDelegate() : window_(NULL) {}

  void set_window(aura::Window* window) { window_ = window; }

  // WindowDelegate overrides:
  virtual void OnBoundsChanged(const gfx::Rect& old_bounds,
                               const gfx::Rect& new_bounds) OVERRIDE {
    if (window_) {
      wm::ActivateWindow(window_);
      window_ = NULL;
    }
  }

 private:
  aura::Window* window_;

  DISALLOW_COPY_AND_ASSIGN(DontCrashOnChangeAndActivateDelegate);
};

}  // namespace

// Exercises possible crash in W2. Here's the sequence:
// . minimize a maximized window.
// . remove the window (which happens when switching displays).
// . add the window back.
// . show the window and during the bounds change activate it.
TEST_F(WorkspaceControllerTest, DontCrashOnChangeAndActivate) {
  // Force the shelf
  ShelfLayoutManager* shelf = shelf_layout_manager();
  shelf->SetAutoHideBehavior(SHELF_AUTO_HIDE_BEHAVIOR_NEVER);

  DontCrashOnChangeAndActivateDelegate delegate;
  scoped_ptr<Window> w1(CreateTestWindowInShellWithDelegate(
      &delegate, 1000, gfx::Rect(10, 11, 250, 251)));

  w1->Show();
  wm::WindowState* w1_state = wm::GetWindowState(w1.get());
  w1_state->Activate();
  w1_state->Maximize();
  w1_state->Minimize();

  w1->parent()->RemoveChild(w1.get());

  // Do this so that when we Show() the window a resize occurs and we make the
  // window active.
  shelf->SetAutoHideBehavior(SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS);

  ParentWindowInPrimaryRootWindow(w1.get());
  delegate.set_window(w1.get());
  w1->Show();
}

// Verifies a window with a transient parent not managed by workspace works.
TEST_F(WorkspaceControllerTest, TransientParent) {
  // Normal window with no transient parent.
  scoped_ptr<Window> w2(CreateTestWindow());
  w2->SetBounds(gfx::Rect(10, 11, 250, 251));
  w2->Show();
  wm::ActivateWindow(w2.get());

  // Window with a transient parent. We set the transient parent to the root,
  // which would never happen but is enough to exercise the bug.
  scoped_ptr<Window> w1(CreateTestWindowUnparented());
  views::corewm::AddTransientChild(
      Shell::GetInstance()->GetPrimaryRootWindow(), w1.get());
  w1->SetBounds(gfx::Rect(10, 11, 250, 251));
  ParentWindowInPrimaryRootWindow(w1.get());
  w1->Show();
  wm::ActivateWindow(w1.get());

  // The window with the transient parent should get added to the same parent as
  // the normal window.
  EXPECT_EQ(w2->parent(), w1->parent());
}

// Test the placement of newly created windows.
TEST_F(WorkspaceControllerTest, BasicAutoPlacingOnCreate) {
  if (!SupportsHostWindowResize())
    return;
  UpdateDisplay("1600x1200");
  // Creating a popup handler here to make sure it does not interfere with the
  // existing windows.
  gfx::Rect source_browser_bounds(16, 32, 640, 320);
  scoped_ptr<aura::Window> browser_window(CreateBrowserLikeWindow(
      source_browser_bounds));

  // Creating a popup to make sure it does not interfere with the positioning.
  scoped_ptr<aura::Window> browser_popup(CreatePopupLikeWindow(
      gfx::Rect(16, 32, 128, 256)));

  browser_window->Show();
  browser_popup->Show();

  { // With a shown window it's size should get returned.
    scoped_ptr<aura::Window> new_browser_window(CreateBrowserLikeWindow(
        source_browser_bounds));
    // The position should be right flush.
    EXPECT_EQ("960,32 640x320", new_browser_window->bounds().ToString());
  }

  { // With the window shown - but more on the right side then on the left
    // side (and partially out of the screen), it should default to the other
    // side and inside the screen.
    gfx::Rect source_browser_bounds(gfx::Rect(1000, 600, 640, 320));
    browser_window->SetBounds(source_browser_bounds);

    scoped_ptr<aura::Window> new_browser_window(CreateBrowserLikeWindow(
        source_browser_bounds));
    // The position should be left & bottom flush.
    EXPECT_EQ("0,600 640x320", new_browser_window->bounds().ToString());

    // If the other window was already beyond the point to get right flush
    // it will remain where it is.
    EXPECT_EQ("1000,600 640x320", browser_window->bounds().ToString());
  }

  { // Make sure that popups do not get changed.
    scoped_ptr<aura::Window> new_popup_window(CreatePopupLikeWindow(
        gfx::Rect(50, 100, 300, 150)));
    EXPECT_EQ("50,100 300x150", new_popup_window->bounds().ToString());
  }

  browser_window->Hide();
  { // If a window is there but not shown the default should be centered.
    scoped_ptr<aura::Window> new_browser_window(CreateBrowserLikeWindow(
        gfx::Rect(50, 100, 300, 150)));
    EXPECT_EQ("650,100 300x150", new_browser_window->bounds().ToString());
  }
}

// Test the basic auto placement of one and or two windows in a "simulated
// session" of sequential window operations.
TEST_F(WorkspaceControllerTest, BasicAutoPlacingOnShowHide) {
  // Test 1: In case there is no manageable window, no window should shift.

  scoped_ptr<aura::Window> window1(CreateTestWindowInShellWithId(0));
  window1->SetBounds(gfx::Rect(16, 32, 640, 320));
  gfx::Rect desktop_area = window1->parent()->bounds();

  scoped_ptr<aura::Window> window2(CreateTestWindowInShellWithId(1));
  // Trigger the auto window placement function by making it visible.
  // Note that the bounds are getting changed while it is invisible.
  window2->Hide();
  window2->SetBounds(gfx::Rect(32, 48, 256, 512));
  window2->Show();

  // Check the initial position of the windows is unchanged.
  EXPECT_EQ("16,32 640x320", window1->bounds().ToString());
  EXPECT_EQ("32,48 256x512", window2->bounds().ToString());

  // Remove the second window and make sure that the first window
  // does NOT get centered.
  window2.reset();
  EXPECT_EQ("16,32 640x320", window1->bounds().ToString());

  wm::WindowState* window1_state = wm::GetWindowState(window1.get());
  // Test 2: Set up two managed windows and check their auto positioning.
  window1_state->set_window_position_managed(true);

  scoped_ptr<aura::Window> window3(CreateTestWindowInShellWithId(2));
  wm::GetWindowState(window3.get())->set_window_position_managed(true);
  // To avoid any auto window manager changes due to SetBounds, the window
  // gets first hidden and then shown again.
  window3->Hide();
  window3->SetBounds(gfx::Rect(32, 48, 256, 512));
  window3->Show();
  // |window1| should be flush left and |window3| flush right.
  EXPECT_EQ("0,32 640x320", window1->bounds().ToString());
  EXPECT_EQ(base::IntToString(
                desktop_area.width() - window3->bounds().width()) +
            ",48 256x512", window3->bounds().ToString());

  // After removing |window3|, |window1| should be centered again.
  window3.reset();
  EXPECT_EQ(
      base::IntToString(
          (desktop_area.width() - window1->bounds().width()) / 2) +
      ",32 640x320", window1->bounds().ToString());

  // Test 3: Set up a manageable and a non manageable window and check
  // positioning.
  scoped_ptr<aura::Window> window4(CreateTestWindowInShellWithId(3));
  // To avoid any auto window manager changes due to SetBounds, the window
  // gets first hidden and then shown again.
  window1->Hide();
  window1->SetBounds(gfx::Rect(16, 32, 640, 320));
  window4->SetBounds(gfx::Rect(32, 48, 256, 512));
  window1->Show();
  // |window1| should be centered and |window4| untouched.
  EXPECT_EQ(
      base::IntToString(
          (desktop_area.width() - window1->bounds().width()) / 2) +
      ",32 640x320", window1->bounds().ToString());
  EXPECT_EQ("32,48 256x512", window4->bounds().ToString());

  // Test4: A single manageable window should get centered.
  window4.reset();
  window1_state->set_bounds_changed_by_user(false);
  // Trigger the auto window placement function by showing (and hiding) it.
  window1->Hide();
  window1->Show();
  // |window1| should be centered.
  EXPECT_EQ(
      base::IntToString(
          (desktop_area.width() - window1->bounds().width()) / 2) +
      ",32 640x320", window1->bounds().ToString());
}

// Test the proper usage of user window movement interaction.
TEST_F(WorkspaceControllerTest, TestUserMovedWindowRepositioning) {
  scoped_ptr<aura::Window> window1(CreateTestWindowInShellWithId(0));
  window1->SetBounds(gfx::Rect(16, 32, 640, 320));
  gfx::Rect desktop_area = window1->parent()->bounds();
  scoped_ptr<aura::Window> window2(CreateTestWindowInShellWithId(1));
  window2->SetBounds(gfx::Rect(32, 48, 256, 512));
  window1->Hide();
  window2->Hide();
  wm::WindowState* window1_state = wm::GetWindowState(window1.get());
  wm::WindowState* window2_state = wm::GetWindowState(window2.get());

  window1_state->set_window_position_managed(true);
  window2_state->set_window_position_managed(true);
  EXPECT_FALSE(window1_state->bounds_changed_by_user());
  EXPECT_FALSE(window2_state->bounds_changed_by_user());

  // Check that the current location gets preserved if the user has
  // positioned it previously.
  window1_state->set_bounds_changed_by_user(true);
  window1->Show();
  EXPECT_EQ("16,32 640x320", window1->bounds().ToString());
  // Flag should be still set.
  EXPECT_TRUE(window1_state->bounds_changed_by_user());
  EXPECT_FALSE(window2_state->bounds_changed_by_user());

  // Turn on the second window and make sure that both windows are now
  // positionable again (user movement cleared).
  window2->Show();

  // |window1| should be flush left and |window2| flush right.
  EXPECT_EQ("0,32 640x320", window1->bounds().ToString());
  EXPECT_EQ(
      base::IntToString(desktop_area.width() - window2->bounds().width()) +
      ",48 256x512", window2->bounds().ToString());
  // FLag should now be reset.
  EXPECT_FALSE(window1_state->bounds_changed_by_user());
  EXPECT_FALSE(window2_state->bounds_changed_by_user());

  // Going back to one shown window should keep the state.
  window1_state->set_bounds_changed_by_user(true);
  window2->Hide();
  EXPECT_EQ("0,32 640x320", window1->bounds().ToString());
  EXPECT_TRUE(window1_state->bounds_changed_by_user());
}

// Test if the single window will be restored at original position.
TEST_F(WorkspaceControllerTest, TestSingleWindowsRestoredBounds) {
  scoped_ptr<aura::Window> window1(
      CreateTestWindowInShellWithBounds(gfx::Rect(100, 100, 100, 100)));
  scoped_ptr<aura::Window> window2(
      CreateTestWindowInShellWithBounds(gfx::Rect(110, 110, 100, 100)));
  scoped_ptr<aura::Window> window3(
      CreateTestWindowInShellWithBounds(gfx::Rect(120, 120, 100, 100)));
  window1->Hide();
  window2->Hide();
  window3->Hide();
  wm::GetWindowState(window1.get())->set_window_position_managed(true);
  wm::GetWindowState(window2.get())->set_window_position_managed(true);
  wm::GetWindowState(window3.get())->set_window_position_managed(true);

  window1->Show();
  wm::ActivateWindow(window1.get());
  window2->Show();
  wm::ActivateWindow(window2.get());
  window3->Show();
  wm::ActivateWindow(window3.get());
  EXPECT_EQ(0, window1->bounds().x());
  EXPECT_EQ(window2->GetRootWindow()->bounds().right(),
            window2->bounds().right());
  EXPECT_EQ(0, window3->bounds().x());

  window1->Hide();
  EXPECT_EQ(window2->GetRootWindow()->bounds().right(),
            window2->bounds().right());
  EXPECT_EQ(0, window3->bounds().x());

  // Being a single window will retore the original location.
  window3->Hide();
  wm::ActivateWindow(window2.get());
  EXPECT_EQ("110,110 100x100", window2->bounds().ToString());

  // Showing the 3rd will push the 2nd window left.
  window3->Show();
  wm::ActivateWindow(window3.get());
  EXPECT_EQ(0, window2->bounds().x());
  EXPECT_EQ(window3->GetRootWindow()->bounds().right(),
            window3->bounds().right());

  // Being a single window will retore the original location.
  window2->Hide();
  EXPECT_EQ("120,120 100x100", window3->bounds().ToString());
}

// Test that user placed windows go back to their user placement after the user
// closes all other windows.
TEST_F(WorkspaceControllerTest, TestUserHandledWindowRestore) {
  scoped_ptr<aura::Window> window1(CreateTestWindowInShellWithId(0));
  gfx::Rect user_pos = gfx::Rect(16, 42, 640, 320);
  window1->SetBounds(user_pos);
  wm::WindowState* window1_state = wm::GetWindowState(window1.get());

  window1_state->SetPreAutoManageWindowBounds(user_pos);
  gfx::Rect desktop_area = window1->parent()->bounds();

  // Create a second window to let the auto manager kick in.
  scoped_ptr<aura::Window> window2(CreateTestWindowInShellWithId(1));
  window2->SetBounds(gfx::Rect(32, 48, 256, 512));
  window1->Hide();
  window2->Hide();
  wm::GetWindowState(window1.get())->set_window_position_managed(true);
  wm::GetWindowState(window2.get())->set_window_position_managed(true);
  window1->Show();
  EXPECT_EQ(user_pos.ToString(), window1->bounds().ToString());
  window2->Show();

  // |window1| should be flush left and |window2| flush right.
  EXPECT_EQ("0," + base::IntToString(user_pos.y()) +
            " 640x320", window1->bounds().ToString());
  EXPECT_EQ(
      base::IntToString(desktop_area.width() - window2->bounds().width()) +
      ",48 256x512", window2->bounds().ToString());
  window2->Hide();

  // After the other window get hidden the window has to move back to the
  // previous position and the bounds should still be set and unchanged.
  EXPECT_EQ(user_pos.ToString(), window1->bounds().ToString());
  ASSERT_TRUE(window1_state->pre_auto_manage_window_bounds());
  EXPECT_EQ(user_pos.ToString(),
            window1_state->pre_auto_manage_window_bounds()->ToString());
}

// Test that a window from normal to minimize will repos the remaining.
TEST_F(WorkspaceControllerTest, ToMinimizeRepositionsRemaining) {
  scoped_ptr<aura::Window> window1(CreateTestWindowInShellWithId(0));
  wm::WindowState* window1_state = wm::GetWindowState(window1.get());
  window1_state->set_window_position_managed(true);
  window1->SetBounds(gfx::Rect(16, 32, 640, 320));
  gfx::Rect desktop_area = window1->parent()->bounds();

  scoped_ptr<aura::Window> window2(CreateTestWindowInShellWithId(1));
  wm::WindowState* window2_state = wm::GetWindowState(window2.get());
  window2_state->set_window_position_managed(true);
  window2->SetBounds(gfx::Rect(32, 48, 256, 512));

  window1_state->Minimize();

  // |window2| should be centered now.
  EXPECT_TRUE(window2->IsVisible());
  EXPECT_TRUE(window2_state->IsNormalShowState());
  EXPECT_EQ(base::IntToString(
                (desktop_area.width() - window2->bounds().width()) / 2) +
            ",48 256x512", window2->bounds().ToString());

  window1_state->Restore();
  // |window1| should be flush right and |window3| flush left.
  EXPECT_EQ(base::IntToString(
                desktop_area.width() - window1->bounds().width()) +
            ",32 640x320", window1->bounds().ToString());
  EXPECT_EQ("0,48 256x512", window2->bounds().ToString());
}

// Test that minimizing an initially maximized window will repos the remaining.
TEST_F(WorkspaceControllerTest, MaxToMinRepositionsRemaining) {
  scoped_ptr<aura::Window> window1(CreateTestWindowInShellWithId(0));
  wm::WindowState* window1_state = wm::GetWindowState(window1.get());
  window1_state->set_window_position_managed(true);
  gfx::Rect desktop_area = window1->parent()->bounds();

  scoped_ptr<aura::Window> window2(CreateTestWindowInShellWithId(1));
  wm::WindowState* window2_state = wm::GetWindowState(window2.get());
  window2_state->set_window_position_managed(true);
  window2->SetBounds(gfx::Rect(32, 48, 256, 512));

  window1_state->Maximize();
  window1_state->Minimize();

  // |window2| should be centered now.
  EXPECT_TRUE(window2->IsVisible());
  EXPECT_TRUE(window2_state->IsNormalShowState());
  EXPECT_EQ(base::IntToString(
                (desktop_area.width() - window2->bounds().width()) / 2) +
            ",48 256x512", window2->bounds().ToString());
}

// Test that nomral, maximize, minimizing will repos the remaining.
TEST_F(WorkspaceControllerTest, NormToMaxToMinRepositionsRemaining) {
  scoped_ptr<aura::Window> window1(CreateTestWindowInShellWithId(0));
  window1->SetBounds(gfx::Rect(16, 32, 640, 320));
  wm::WindowState* window1_state = wm::GetWindowState(window1.get());
  window1_state->set_window_position_managed(true);
  gfx::Rect desktop_area = window1->parent()->bounds();

  scoped_ptr<aura::Window> window2(CreateTestWindowInShellWithId(1));
  wm::WindowState* window2_state = wm::GetWindowState(window2.get());
  window2_state->set_window_position_managed(true);
  window2->SetBounds(gfx::Rect(32, 40, 256, 512));

  // Trigger the auto window placement function by showing (and hiding) it.
  window1->Hide();
  window1->Show();

  // |window1| should be flush right and |window3| flush left.
  EXPECT_EQ(base::IntToString(
                desktop_area.width() - window1->bounds().width()) +
            ",32 640x320", window1->bounds().ToString());
  EXPECT_EQ("0,40 256x512", window2->bounds().ToString());

  window1_state->Maximize();
  window1_state->Minimize();

  // |window2| should be centered now.
  EXPECT_TRUE(window2->IsVisible());
  EXPECT_TRUE(window2_state->IsNormalShowState());
  EXPECT_EQ(base::IntToString(
                (desktop_area.width() - window2->bounds().width()) / 2) +
            ",40 256x512", window2->bounds().ToString());
}

// Test that nomral, maximize, normal will repos the remaining.
TEST_F(WorkspaceControllerTest, NormToMaxToNormRepositionsRemaining) {
  scoped_ptr<aura::Window> window1(CreateTestWindowInShellWithId(0));
  window1->SetBounds(gfx::Rect(16, 32, 640, 320));
  wm::WindowState* window1_state = wm::GetWindowState(window1.get());
  window1_state->set_window_position_managed(true);
  gfx::Rect desktop_area = window1->parent()->bounds();

  scoped_ptr<aura::Window> window2(CreateTestWindowInShellWithId(1));
  wm::GetWindowState(window2.get())->set_window_position_managed(true);
  window2->SetBounds(gfx::Rect(32, 40, 256, 512));

  // Trigger the auto window placement function by showing (and hiding) it.
  window1->Hide();
  window1->Show();

  // |window1| should be flush right and |window3| flush left.
  EXPECT_EQ(base::IntToString(
                desktop_area.width() - window1->bounds().width()) +
            ",32 640x320", window1->bounds().ToString());
  EXPECT_EQ("0,40 256x512", window2->bounds().ToString());

  window1_state->Maximize();
  window1_state->Restore();

  // |window1| should be flush right and |window2| flush left.
  EXPECT_EQ(base::IntToString(
                desktop_area.width() - window1->bounds().width()) +
            ",32 640x320", window1->bounds().ToString());
  EXPECT_EQ("0,40 256x512", window2->bounds().ToString());
}

// Test that animations are triggered.
TEST_F(WorkspaceControllerTest, AnimatedNormToMaxToNormRepositionsRemaining) {
  ui::ScopedAnimationDurationScaleMode normal_duration_mode(
      ui::ScopedAnimationDurationScaleMode::NORMAL_DURATION);
  scoped_ptr<aura::Window> window1(CreateTestWindowInShellWithId(0));
  window1->Hide();
  window1->SetBounds(gfx::Rect(16, 32, 640, 320));
  gfx::Rect desktop_area = window1->parent()->bounds();
  scoped_ptr<aura::Window> window2(CreateTestWindowInShellWithId(1));
  window2->Hide();
  window2->SetBounds(gfx::Rect(32, 48, 256, 512));

  wm::GetWindowState(window1.get())->set_window_position_managed(true);
  wm::GetWindowState(window2.get())->set_window_position_managed(true);
  // Make sure nothing is animating.
  window1->layer()->GetAnimator()->StopAnimating();
  window2->layer()->GetAnimator()->StopAnimating();
  window2->Show();

  // The second window should now animate.
  EXPECT_FALSE(window1->layer()->GetAnimator()->is_animating());
  EXPECT_TRUE(window2->layer()->GetAnimator()->is_animating());
  window2->layer()->GetAnimator()->StopAnimating();

  window1->Show();
  EXPECT_TRUE(window1->layer()->GetAnimator()->is_animating());
  EXPECT_TRUE(window2->layer()->GetAnimator()->is_animating());

  window1->layer()->GetAnimator()->StopAnimating();
  window2->layer()->GetAnimator()->StopAnimating();
  // |window1| should be flush right and |window2| flush left.
  EXPECT_EQ(base::IntToString(
                desktop_area.width() - window1->bounds().width()) +
            ",32 640x320", window1->bounds().ToString());
  EXPECT_EQ("0,48 256x512", window2->bounds().ToString());
}

// This tests simulates a browser and an app and verifies the ordering of the
// windows and layers doesn't get out of sync as various operations occur. Its
// really testing code in FocusController, but easier to simulate here. Just as
// with a real browser the browser here has a transient child window
// (corresponds to the status bubble).
TEST_F(WorkspaceControllerTest, VerifyLayerOrdering) {
  scoped_ptr<Window> browser(aura::test::CreateTestWindowWithDelegate(
      NULL, ui::wm::WINDOW_TYPE_NORMAL, gfx::Rect(5, 6, 7, 8), NULL));
  browser->SetName("browser");
  ParentWindowInPrimaryRootWindow(browser.get());
  browser->Show();
  wm::ActivateWindow(browser.get());

  // |status_bubble| is made a transient child of |browser| and as a result
  // owned by |browser|.
  aura::test::TestWindowDelegate* status_bubble_delegate =
      aura::test::TestWindowDelegate::CreateSelfDestroyingDelegate();
  status_bubble_delegate->set_can_focus(false);
  Window* status_bubble =
      aura::test::CreateTestWindowWithDelegate(status_bubble_delegate,
                                               ui::wm::WINDOW_TYPE_POPUP,
                                               gfx::Rect(5, 6, 7, 8),
                                               NULL);
  views::corewm::AddTransientChild(browser.get(), status_bubble);
  ParentWindowInPrimaryRootWindow(status_bubble);
  status_bubble->SetName("status_bubble");

  scoped_ptr<Window> app(aura::test::CreateTestWindowWithDelegate(
      NULL, ui::wm::WINDOW_TYPE_NORMAL, gfx::Rect(5, 6, 7, 8), NULL));
  app->SetName("app");
  ParentWindowInPrimaryRootWindow(app.get());

  aura::Window* parent = browser->parent();

  app->Show();
  wm::ActivateWindow(app.get());
  EXPECT_EQ(GetWindowNames(parent), GetLayerNames(parent));

  // Minimize the app, focus should go the browser.
  app->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_MINIMIZED);
  EXPECT_TRUE(wm::IsActiveWindow(browser.get()));
  EXPECT_EQ(GetWindowNames(parent), GetLayerNames(parent));

  // Minimize the browser (neither windows are focused).
  browser->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_MINIMIZED);
  EXPECT_FALSE(wm::IsActiveWindow(browser.get()));
  EXPECT_FALSE(wm::IsActiveWindow(app.get()));
  EXPECT_EQ(GetWindowNames(parent), GetLayerNames(parent));

  // Show the browser (which should restore it).
  browser->Show();
  EXPECT_EQ(GetWindowNames(parent), GetLayerNames(parent));

  // Activate the browser.
  ash::wm::ActivateWindow(browser.get());
  EXPECT_TRUE(wm::IsActiveWindow(browser.get()));
  EXPECT_EQ(GetWindowNames(parent), GetLayerNames(parent));

  // Restore the app. This differs from above code for |browser| as internally
  // the app code does this. Restoring this way or using Show() should not make
  // a difference.
  app->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_NORMAL);
  EXPECT_EQ(GetWindowNames(parent), GetLayerNames(parent));

  // Activate the app.
  ash::wm::ActivateWindow(app.get());
  EXPECT_TRUE(wm::IsActiveWindow(app.get()));
  EXPECT_EQ(GetWindowNames(parent), GetLayerNames(parent));
}

namespace {

// Used by DragMaximizedNonTrackedWindow to track how many times the window
// hierarchy changes affecting the specified window.
class DragMaximizedNonTrackedWindowObserver
    : public aura::WindowObserver {
 public:
  DragMaximizedNonTrackedWindowObserver(aura::Window* window)
  : change_count_(0),
    window_(window) {
  }

  // Number of times OnWindowHierarchyChanged() has been received.
  void clear_change_count() { change_count_ = 0; }
  int change_count() const {
    return change_count_;
  }

  // aura::WindowObserver overrides:
  // Counts number of times a window is reparented. Ignores reparenting into and
  // from a docked container which is expected when a tab is dragged.
  virtual void OnWindowHierarchyChanged(
      const HierarchyChangeParams& params) OVERRIDE {
    if (params.target != window_ ||
        (params.old_parent->id() == kShellWindowId_DefaultContainer &&
         params.new_parent->id() == kShellWindowId_DockedContainer) ||
        (params.old_parent->id() == kShellWindowId_DockedContainer &&
         params.new_parent->id() == kShellWindowId_DefaultContainer)) {
      return;
    }
    change_count_++;
  }

 private:
  int change_count_;
  aura::Window* window_;

  DISALLOW_COPY_AND_ASSIGN(DragMaximizedNonTrackedWindowObserver);
};

}  // namespace

// Verifies that a new maximized window becomes visible after its activation
// is requested, even though it does not become activated because a system
// modal window is active.
TEST_F(WorkspaceControllerTest, SwitchFromModal) {
  scoped_ptr<Window> modal_window(CreateTestWindowUnparented());
  modal_window->SetBounds(gfx::Rect(10, 11, 21, 22));
  modal_window->SetProperty(aura::client::kModalKey, ui::MODAL_TYPE_SYSTEM);
  ParentWindowInPrimaryRootWindow(modal_window.get());
  modal_window->Show();
  wm::ActivateWindow(modal_window.get());

  scoped_ptr<Window> maximized_window(CreateTestWindow());
  maximized_window->SetProperty(
      aura::client::kShowStateKey, ui::SHOW_STATE_MAXIMIZED);
  maximized_window->Show();
  wm::ActivateWindow(maximized_window.get());
  EXPECT_TRUE(maximized_window->IsVisible());
}

namespace {

// Subclass of WorkspaceControllerTest that runs tests with docked windows
// enabled and disabled.
class WorkspaceControllerTestDragging
    : public WorkspaceControllerTest,
      public testing::WithParamInterface<bool> {
 public:
  WorkspaceControllerTestDragging() {}
  virtual ~WorkspaceControllerTestDragging() {}

  // testing::Test:
  virtual void SetUp() OVERRIDE {
    WorkspaceControllerTest::SetUp();
    if (!docked_windows_enabled()) {
      CommandLine::ForCurrentProcess()->AppendSwitch(
          ash::switches::kAshDisableDockedWindows);
    }
  }

  bool docked_windows_enabled() const { return GetParam(); }

 private:
  DISALLOW_COPY_AND_ASSIGN(WorkspaceControllerTestDragging);
};

}  // namespace

// Verifies that when dragging a window over the shelf overlap is detected
// during and after the drag.
TEST_P(WorkspaceControllerTestDragging, DragWindowOverlapShelf) {
  aura::test::TestWindowDelegate delegate;
  delegate.set_window_component(HTCAPTION);
  scoped_ptr<Window> w1(aura::test::CreateTestWindowWithDelegate(
      &delegate, ui::wm::WINDOW_TYPE_NORMAL, gfx::Rect(5, 5, 100, 50), NULL));
  ParentWindowInPrimaryRootWindow(w1.get());

  ShelfLayoutManager* shelf = shelf_layout_manager();
  shelf->SetAutoHideBehavior(SHELF_AUTO_HIDE_BEHAVIOR_NEVER);

  // Drag near the shelf.
  aura::test::EventGenerator generator(
      Shell::GetPrimaryRootWindow(), gfx::Point());
  generator.MoveMouseTo(10, 10);
  generator.PressLeftButton();
  generator.MoveMouseTo(100, shelf->GetIdealBounds().y() - 70);

  // Shelf should not be in overlapped state.
  EXPECT_FALSE(GetWindowOverlapsShelf());

  generator.MoveMouseTo(100, shelf->GetIdealBounds().y() - 20);

  // Shelf should detect overlap. Overlap state stays after mouse is released.
  EXPECT_TRUE(GetWindowOverlapsShelf());
  generator.ReleaseLeftButton();
  EXPECT_TRUE(GetWindowOverlapsShelf());
}

// Verifies that when dragging a window autohidden shelf stays hidden during
// and after the drag.
TEST_P(WorkspaceControllerTestDragging, DragWindowKeepsShelfAutohidden) {
  aura::test::TestWindowDelegate delegate;
  delegate.set_window_component(HTCAPTION);
  scoped_ptr<Window> w1(aura::test::CreateTestWindowWithDelegate(
      &delegate, ui::wm::WINDOW_TYPE_NORMAL, gfx::Rect(5, 5, 100, 50), NULL));
  ParentWindowInPrimaryRootWindow(w1.get());

  ShelfLayoutManager* shelf = shelf_layout_manager();
  shelf->SetAutoHideBehavior(SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS);
  EXPECT_EQ(SHELF_AUTO_HIDE_HIDDEN, shelf->auto_hide_state());

  // Drag very little.
  aura::test::EventGenerator generator(
      Shell::GetPrimaryRootWindow(), gfx::Point());
  generator.MoveMouseTo(10, 10);
  generator.PressLeftButton();
  generator.MoveMouseTo(12, 12);

  // Shelf should be hidden during and after the drag.
  EXPECT_EQ(SHELF_AUTO_HIDE_HIDDEN, shelf->auto_hide_state());
  generator.ReleaseLeftButton();
  EXPECT_EQ(SHELF_AUTO_HIDE_HIDDEN, shelf->auto_hide_state());
}

INSTANTIATE_TEST_CASE_P(DockedOrNot, WorkspaceControllerTestDragging,
                        ::testing::Bool());

// Verifies that events are targeted properly just outside the window edges.
TEST_F(WorkspaceControllerTest, WindowEdgeHitTest) {
  aura::test::TestWindowDelegate d_first, d_second;
  scoped_ptr<Window> first(aura::test::CreateTestWindowWithDelegate(&d_first,
      123, gfx::Rect(20, 10, 100, 50), NULL));
  ParentWindowInPrimaryRootWindow(first.get());
  first->Show();

  scoped_ptr<Window> second(aura::test::CreateTestWindowWithDelegate(&d_second,
      234, gfx::Rect(30, 40, 40, 10), NULL));
  ParentWindowInPrimaryRootWindow(second.get());
  second->Show();

  ui::EventTarget* root = first->GetRootWindow();
  ui::EventTargeter* targeter = root->GetEventTargeter();

  // The windows overlap, and |second| is on top of |first|. Events targeted
  // slightly outside the edges of the |second| window should still be targeted
  // to |second| to allow resizing the windows easily.

  const int kNumPoints = 4;
  struct {
    const char* direction;
    gfx::Point location;
  } points[kNumPoints] = {
    { "left", gfx::Point(28, 45) },  // outside the left edge.
    { "top", gfx::Point(50, 38) },  // outside the top edge.
    { "right", gfx::Point(72, 45) },  // outside the right edge.
    { "bottom", gfx::Point(50, 52) },  // outside the bottom edge.
  };
  // Do two iterations, first without any transform on |second|, and the second
  // time after applying some transform on |second| so that it doesn't get
  // targeted.
  for (int times = 0; times < 2; ++times) {
    SCOPED_TRACE(times == 0 ? "Without transform" : "With transform");
    aura::Window* expected_target = times == 0 ? second.get() : first.get();
    for (int i = 0; i < kNumPoints; ++i) {
      SCOPED_TRACE(points[i].direction);
      const gfx::Point& location = points[i].location;
      ui::MouseEvent mouse(ui::ET_MOUSE_MOVED, location, location, ui::EF_NONE,
                           ui::EF_NONE);
      ui::EventTarget* target = targeter->FindTargetForEvent(root, &mouse);
      EXPECT_EQ(expected_target, target);

      ui::TouchEvent touch(ui::ET_TOUCH_PRESSED, location, 0,
                           ui::EventTimeForNow());
      target = targeter->FindTargetForEvent(root, &touch);
      EXPECT_EQ(expected_target, target);
    }
    // Apply a transform on |second|. After the transform is applied, the window
    // should no longer be targeted.
    gfx::Transform transform;
    transform.Translate(70, 40);
    second->SetTransform(transform);
  }
}

// Verifies events targeting just outside the window edges for panels.
TEST_F(WorkspaceControllerTest, WindowEdgeHitTestPanel) {
  aura::test::TestWindowDelegate delegate;
  scoped_ptr<Window> window(CreateTestPanel(&delegate,
                                           gfx::Rect(20, 10, 100, 50)));
  ui::EventTarget* root = window->GetRootWindow();
  ui::EventTargeter* targeter = root->GetEventTargeter();
  const gfx::Rect bounds = window->bounds();
  const int kNumPoints = 5;
  struct {
    const char* direction;
    gfx::Point location;
    bool is_target_hit;
  } points[kNumPoints] = {
    { "left", gfx::Point(bounds.x() - 2, bounds.y() + 10), true },
    { "top", gfx::Point(bounds.x() + 10, bounds.y() - 2), true },
    { "right", gfx::Point(bounds.right() + 2, bounds.y() + 10), true },
    { "bottom", gfx::Point(bounds.x() + 10, bounds.bottom() + 2), true },
    { "outside", gfx::Point(bounds.x() + 10, bounds.y() - 31), false },
  };
  for (int i = 0; i < kNumPoints; ++i) {
    SCOPED_TRACE(points[i].direction);
    const gfx::Point& location = points[i].location;
    ui::MouseEvent mouse(ui::ET_MOUSE_MOVED, location, location, ui::EF_NONE,
                         ui::EF_NONE);
    ui::EventTarget* target = targeter->FindTargetForEvent(root, &mouse);
    if (points[i].is_target_hit)
      EXPECT_EQ(window.get(), target);
    else
      EXPECT_NE(window.get(), target);

    ui::TouchEvent touch(ui::ET_TOUCH_PRESSED, location, 0,
                         ui::EventTimeForNow());
    target = targeter->FindTargetForEvent(root, &touch);
    if (points[i].is_target_hit)
      EXPECT_EQ(window.get(), target);
    else
      EXPECT_NE(window.get(), target);
  }
}

// Verifies events targeting just outside the window edges for docked windows.
TEST_F(WorkspaceControllerTest, WindowEdgeHitTestDocked) {
  if (!switches::UseDockedWindows())
    return;
  aura::test::TestWindowDelegate delegate;
  // Make window smaller than the minimum docked area so that the window edges
  // are exposed.
  delegate.set_maximum_size(gfx::Size(180, 200));
  scoped_ptr<Window> window(aura::test::CreateTestWindowWithDelegate(&delegate,
      123, gfx::Rect(20, 10, 100, 50), NULL));
  ParentWindowInPrimaryRootWindow(window.get());
  aura::Window* docked_container = Shell::GetContainer(
      window->GetRootWindow(), internal::kShellWindowId_DockedContainer);
  docked_container->AddChild(window.get());
  window->Show();
  ui::EventTarget* root = window->GetRootWindow();
  ui::EventTargeter* targeter = root->GetEventTargeter();
  const gfx::Rect bounds = window->bounds();
  const int kNumPoints = 5;
  struct {
    const char* direction;
    gfx::Point location;
    bool is_target_hit;
  } points[kNumPoints] = {
    { "left", gfx::Point(bounds.x() - 2, bounds.y() + 10), true },
    { "top", gfx::Point(bounds.x() + 10, bounds.y() - 2), true },
    { "right", gfx::Point(bounds.right() + 2, bounds.y() + 10), true },
    { "bottom", gfx::Point(bounds.x() + 10, bounds.bottom() + 2), true },
    { "outside", gfx::Point(bounds.x() + 10, bounds.y() - 31), false },
  };
  for (int i = 0; i < kNumPoints; ++i) {
    SCOPED_TRACE(points[i].direction);
    const gfx::Point& location = points[i].location;
    ui::MouseEvent mouse(ui::ET_MOUSE_MOVED, location, location, ui::EF_NONE,
                         ui::EF_NONE);
    ui::EventTarget* target = targeter->FindTargetForEvent(root, &mouse);
    if (points[i].is_target_hit)
      EXPECT_EQ(window.get(), target);
    else
      EXPECT_NE(window.get(), target);

    ui::TouchEvent touch(ui::ET_TOUCH_PRESSED, location, 0,
                         ui::EventTimeForNow());
    target = targeter->FindTargetForEvent(root, &touch);
    if (points[i].is_target_hit)
      EXPECT_EQ(window.get(), target);
    else
      EXPECT_NE(window.get(), target);
  }
}

}  // namespace internal
}  // namespace ash
