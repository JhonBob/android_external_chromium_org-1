// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_SEARCH_INSTANT_CONTROLLER_H_
#define CHROME_BROWSER_UI_SEARCH_INSTANT_CONTROLLER_H_

#include <list>
#include <string>
#include <utility>
#include <vector>

#include "base/basictypes.h"
#include "base/gtest_prod_util.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chrome/browser/search/instant_service_observer.h"
#include "chrome/browser/ui/omnibox/omnibox_edit_model.h"
#include "chrome/browser/ui/search/instant_commit_type.h"
#include "chrome/browser/ui/search/instant_overlay_model.h"
#include "chrome/browser/ui/search/instant_page.h"
#include "chrome/common/instant_types.h"
#include "chrome/common/omnibox_focus_state.h"
#include "chrome/common/search_types.h"
#include "content/public/common/page_transition_types.h"
#include "googleurl/src/gurl.h"
#include "ui/base/window_open_disposition.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/rect.h"

struct AutocompleteMatch;
struct InstantAutocompleteResult;

class AutocompleteProvider;
class AutocompleteResult;
class BrowserInstantController;
class InstantNTP;
class InstantOverlay;
class InstantService;
class InstantTab;
class TemplateURL;

namespace content {
class WebContents;
}

// Macro used for logging debug events. |message| should be a std::string.
#define LOG_INSTANT_DEBUG_EVENT(controller, message) \
    controller->LogDebugEvent(message)

// InstantController drives Chrome Instant, i.e., the browser implementation of
// the Embedded Search API (see http://dev.chromium.org/embeddedsearch).
//
// In extended mode, InstantController maintains and coordinates three
// instances of InstantPage:
//  (1) An InstantOverlay instance that is used to show search suggestions and
//      results in an overlay over a non-search page.
//  (2) An InstantNTP instance which is a preloaded search page that will be
//      swapped-in the next time the user navigates to the New Tab Page. It is
//      never shown to the user in an uncommitted state.
//  (3) An InstantTab instance which points to the currently active tab, if it
//      supports the Embedded Search API.
//
// All three are backed by a WebContents. InstantOverlay and InstantNTP own
// their corresponding WebContents; InstantTab does not. In non-extended mode,
// only an InstantOverlay instance is kept.
//
// InstantController is owned by Browser via BrowserInstantController.
class InstantController : public InstantPage::Delegate,
                          public InstantServiceObserver {
 public:
  // For reporting fallbacks to local overlay.
  enum InstantFallbackReason {
    INSTANT_FALLBACK_NONE = 0,
    INSTANT_FALLBACK_UNKNOWN = 1,
    INSTANT_FALLBACK_INSTANT_URL_EMPTY = 2,
    INSTANT_FALLBACK_ORIGIN_PATH_MISMATCH = 3,
    INSTANT_FALLBACK_INSTANT_NOT_SUPPORTED = 4,
    INSTANT_FALLBACK_NO_OVERLAY = 5,
    INSTANT_FALLBACK_JAVASCRIPT_DISABLED = 6,
    INSTANT_FALLBACK_MAX = 7,
  };

  InstantController(BrowserInstantController* browser,
                    bool extended_enabled);
  virtual ~InstantController();

  // Called when the Autocomplete flow is about to start. Sets up the
  // appropriate page to send user updates to.  May reset |instant_tab_| or
  // switch to a local fallback |overlay_| as necessary.
  void OnAutocompleteStart();

  // Invoked as the user types into the omnibox. |user_text| is what the user
  // has typed. |full_text| is what the omnibox is showing. These may differ if
  // the user typed only some text, and the rest was inline autocompleted. If
  // |verbatim| is true, search results are shown for the exact omnibox text,
  // rather than the best guess as to what the user means. Returns true if the
  // update is accepted (i.e., if |match| is a search rather than a URL).
  // |is_keyword_search| is true if keyword searching is in effect.
  bool Update(const AutocompleteMatch& match,
              const string16& user_text,
              const string16& full_text,
              size_t selection_start,
              size_t selection_end,
              bool verbatim,
              bool user_input_in_progress,
              bool omnibox_popup_is_open,
              bool escape_pressed,
              bool is_keyword_search);

  // Releases and returns the NTP WebContents. May be NULL. Loads a new
  // WebContents for the NTP.
  scoped_ptr<content::WebContents> ReleaseNTPContents() WARN_UNUSED_RESULT;

  // Sets the bounds of the omnibox popup, in screen coordinates.
  void SetPopupBounds(const gfx::Rect& bounds);

  // Sets the stored start-edge margin and width of the omnibox.
  void SetOmniboxBounds(const gfx::Rect& bounds);

  // Send autocomplete results from |providers| to the overlay page.
  void HandleAutocompleteResults(
      const std::vector<AutocompleteProvider*>& providers,
      const AutocompleteResult& result);

  // Called when the default search provider changes. Resets InstantNTP and
  // InstantOverlay.
  void OnDefaultSearchProviderChanged();

  // Called when the user presses up or down. |count| is a repeat count,
  // negative for moving up, positive for moving down. Returns true if Instant
  // handled the key press.
  bool OnUpOrDownKeyPressed(int count);

  // Called when the user has arrowed into the suggestions but wants to cancel,
  // typically by pressing ESC. The omnibox text is expected to have been
  // reverted to |full_text| by the OmniboxEditModel prior to calling this.
  // |match| is the match reverted to.
  void OnCancel(const AutocompleteMatch& match,
                const string16& user_text,
                const string16& full_text);

  // Called when the user navigates to a URL from the omnibox. This will send
  // an onsubmit notification to the instant page.
  void OmniboxNavigateToURL();

  // Notifies |instant_Tab_| to toggle voice search.
  void ToggleVoiceSearch();

  // The overlay WebContents. May be NULL. InstantController retains ownership.
  content::WebContents* GetOverlayContents() const;

  // The ntp WebContents. May be NULL. InstantController retains ownership.
  content::WebContents* GetNTPContents() const;

  // Returns true if Instant is showing a search results overlay. Returns false
  // if the overlay is not showing, or if it's showing only suggestions.
  bool IsOverlayingSearchResults() const;

  // Called if the browser is navigating to a search URL for |search_terms| with
  // search-term-replacement enabled. If |instant_tab_| can be used to process
  // the search, this does so and returns true. Else, returns false.
  bool SubmitQuery(const string16& search_terms);

  // If the overlay is showing search results, commits the overlay, calling
  // CommitInstant() on the browser, and returns true. Else, returns false.
  bool CommitIfPossible(InstantCommitType type);

  // Called to indicate that the omnibox focus state changed with the given
  // |reason|. If |focus_state| is FOCUS_NONE, |view_gaining_focus| is set to
  // the view gaining focus.
  void OmniboxFocusChanged(OmniboxFocusState focus_state,
                           OmniboxFocusChangeReason reason,
                           gfx::NativeView view_gaining_focus);

  // The search mode in the active tab has changed. Pass the message down to
  // the overlay which will notify the renderer. Create |instant_tab_| if the
  // |new_mode| reflects an Instant search results page.
  void SearchModeChanged(const SearchMode& old_mode,
                         const SearchMode& new_mode);

  // The user switched tabs. Hide the overlay. Create |instant_tab_| if the
  // newly active tab is an Instant search results page.
  void ActiveTabChanged();

  // The user is about to switch tabs. Commit the overlay if needed.
  void TabDeactivated(content::WebContents* contents);

  // Sets whether Instant should show result overlays. |use_local_page_only|
  // will force the use of baked-in page as the Instant URL and is only
  // applicable if |extended_enabled_| is true.
  void SetInstantEnabled(bool instant_enabled, bool use_local_page_only);

  // Called when someone else swapped in a different contents in the |overlay_|.
  void SwappedOverlayContents();

  // Called when contents for |overlay_| received focus.
  void FocusedOverlayContents();

  // Called when the |overlay_| might be stale. If it's actually stale, and the
  // omnibox doesn't have focus, and the overlay isn't showing, the |overlay_|
  // is deleted and recreated. Else the refresh is skipped.
  void ReloadOverlayIfStale();

  // Called when the |overlay_|'s main frame has finished loading.
  void OverlayLoadCompletedMainFrame();

  // Adds a new event to |debug_events_| and also DVLOG's it. Ensures that
  // |debug_events_| doesn't get too large.
  void LogDebugEvent(const std::string& info) const;

  // Resets list of debug events.
  void ClearDebugEvents();

  // Returns the correct Instant URL to use from the following possibilities:
  //   o The default search engine's Instant URL
  //   o The local page (see GetLocalInstantURL())
  // Returns empty string if no valid Instant URL is available (this is only
  // possible in non-extended mode where we don't have a local page fall-back).
  virtual std::string GetInstantURL() const;

  // See comments for |debug_events_| below.
  const std::list<std::pair<int64, std::string> >& debug_events() {
    return debug_events_;
  }

  // Returns the transition type of the last AutocompleteMatch passed to Update.
  content::PageTransition last_transition_type() const {
    return last_transition_type_;
  }

  // Non-const for Add/RemoveObserver only.  Other model changes should only
  // happen through the InstantController interface.
  InstantOverlayModel* model() { return &model_; }

  // Used by BrowserInstantController to notify InstantController about the
  // instant support change event for the active web contents.
  void InstantSupportChanged(InstantSupportState instant_support);

 protected:
  // Accessors are made protected for testing purposes.
  virtual bool extended_enabled() const;

  virtual InstantOverlay* overlay() const;
  virtual InstantTab* instant_tab() const;
  virtual InstantNTP* ntp() const;

  virtual Profile* profile() const;

  // Returns true if Javascript is enabled and false otherwise.
  virtual bool IsJavascriptEnabled() const;

  // Returns true if the browser is in startup.
  virtual bool InStartup() const;

 private:
  friend class InstantExtendedManualTest;
  friend class InstantTestBase;
#define UNIT_F(test) FRIEND_TEST_ALL_PREFIXES(InstantControllerTest, test)
  UNIT_F(DoesNotSwitchToLocalNTPIfOnCurrentNTP);
  UNIT_F(DoesNotSwitchToLocalNTPIfOnLocalNTP);
  UNIT_F(IsJavascriptEnabled);
  UNIT_F(IsJavascriptEnabledChecksContentSettings);
  UNIT_F(IsJavascriptEnabledChecksPrefs);
  UNIT_F(PrefersRemoteNTPOnStartup);
  UNIT_F(ShouldSwitchToLocalOverlay);
  UNIT_F(SwitchesToLocalNTPIfJSDisabled);
  UNIT_F(SwitchesToLocalNTPIfNoInstantSupport);
  UNIT_F(SwitchesToLocalNTPIfNoNTPReady);
  UNIT_F(SwitchesToLocalNTPIfPathBad);
#undef UNIT_F
  FRIEND_TEST_ALL_PREFIXES(InstantExtendedTest, ExtendedModeIsOn);
  FRIEND_TEST_ALL_PREFIXES(InstantExtendedTest, MostVisited);
  FRIEND_TEST_ALL_PREFIXES(InstantExtendedTest, NTPIsPreloaded);
  FRIEND_TEST_ALL_PREFIXES(InstantExtendedTest, PreloadedNTPIsUsedInNewTab);
  FRIEND_TEST_ALL_PREFIXES(InstantExtendedTest, PreloadedNTPIsUsedInSameTab);
  FRIEND_TEST_ALL_PREFIXES(InstantExtendedTest, PreloadedNTPForWrongProvider);
  FRIEND_TEST_ALL_PREFIXES(InstantExtendedTest, PreloadedNTPRenderViewGone);
  FRIEND_TEST_ALL_PREFIXES(InstantExtendedTest,
                           PreloadedNTPDoesntSupportInstant);
  FRIEND_TEST_ALL_PREFIXES(InstantExtendedTest, ProcessIsolation);
  FRIEND_TEST_ALL_PREFIXES(InstantExtendedTest, UnrelatedSiteInstance);
  FRIEND_TEST_ALL_PREFIXES(InstantExtendedTest, OnDefaultSearchProviderChanged);
  FRIEND_TEST_ALL_PREFIXES(InstantExtendedTest,
                           AcceptingURLSearchDoesNotNavigate);
  FRIEND_TEST_ALL_PREFIXES(InstantExtendedTest, AcceptingJSSearchDoesNotRunJS);
  FRIEND_TEST_ALL_PREFIXES(InstantExtendedTest,
                           ReloadSearchAfterBackReloadsCorrectQuery);
  FRIEND_TEST_ALL_PREFIXES(InstantExtendedFirstTabTest,
                           RedirectToLocalOnLoadFailure);
  FRIEND_TEST_ALL_PREFIXES(InstantExtendedTest, KeyboardTogglesVoiceSearch);
  FRIEND_TEST_ALL_PREFIXES(InstantExtendedTest, HomeButtonAffectsMargin);
  FRIEND_TEST_ALL_PREFIXES(InstantExtendedTest, SearchReusesInstantTab);
  FRIEND_TEST_ALL_PREFIXES(InstantExtendedTest,
                           SearchDoesntReuseInstantTabWithoutSupport);
  FRIEND_TEST_ALL_PREFIXES(InstantExtendedTest,
                           TypedSearchURLDoesntReuseInstantTab);
  FRIEND_TEST_ALL_PREFIXES(InstantExtendedTest,
                           DispatchMVChangeEventWhileNavigatingBackToNTP);

  // Overridden from InstantPage::Delegate:
  // TODO(shishir): We assume that the WebContent's current RenderViewHost is
  // the RenderViewHost being created which is not always true. Fix this.
  virtual void InstantPageRenderViewCreated(
      const content::WebContents* contents) OVERRIDE;
  virtual void InstantSupportDetermined(
      const content::WebContents* contents,
      bool supports_instant) OVERRIDE;
  virtual void InstantPageRenderViewGone(
      const content::WebContents* contents) OVERRIDE;
  virtual void InstantPageAboutToNavigateMainFrame(
      const content::WebContents* contents,
      const GURL& url) OVERRIDE;
  virtual void SetSuggestions(
      const content::WebContents* contents,
      const std::vector<InstantSuggestion>& suggestions) OVERRIDE;
  virtual void ShowInstantOverlay(
      const content::WebContents* contents,
      int height,
      InstantSizeUnits units) OVERRIDE;
  virtual void LogDropdownShown() OVERRIDE;
  virtual void FocusOmnibox(const content::WebContents* contents,
                            OmniboxFocusState state) OVERRIDE;
  virtual void NavigateToURL(
      const content::WebContents* contents,
      const GURL& url,
      content::PageTransition transition,
      WindowOpenDisposition disposition,
      bool is_search_type) OVERRIDE;
  virtual void InstantPageLoadFailed(content::WebContents* contents) OVERRIDE;

  // Overridden from InstantServiceObserver:
  virtual void ThemeInfoChanged(const ThemeBackgroundInfo& theme_info) OVERRIDE;
  virtual void MostVisitedItemsChanged(
      const std::vector<InstantMostVisitedItem>& items) OVERRIDE;

  // Invoked by the InstantLoader when the Instant page wants to delete a
  // Most Visited item.
  virtual void DeleteMostVisitedItem(const GURL& url) OVERRIDE;

  // Invoked by the InstantLoader when the Instant page wants to undo a
  // Most Visited deletion.
  virtual void UndoMostVisitedDeletion(const GURL& url) OVERRIDE;

  // Invoked by the InstantLoader when the Instant page wants to undo all
  // Most Visited deletions.
  virtual void UndoAllMostVisitedDeletions() OVERRIDE;

  // Helper function to navigate the given contents to the local fallback
  // Instant URL and trim the history correctly.
  void RedirectToLocalNTP(content::WebContents* contents);

  // Helper for OmniboxFocusChanged. Commit or discard the overlay.
  void OmniboxLostFocus(gfx::NativeView view_gaining_focus);

  // Returns the local Instant URL. (Just a convenience wrapper around
  // chrome::GetLocalInstantURL.)
  virtual std::string GetLocalInstantURL() const;

  // Returns true if |page| has an up-to-date Instant URL and supports Instant.
  // Note that local URLs will not pass this check.
  bool PageIsCurrent(const InstantPage* page) const;

  // Recreates |ntp_| using |instant_url|.
  void ResetNTP(const std::string& instant_url);

  // Reloads a new InstantNTP.  Called when |ntp_| is stale.
  void ReloadStaleNTP();

  // Returns true if we should switch to using the local NTP.
  bool ShouldSwitchToLocalNTP() const;

  // Recreates |overlay_| using |instant_url|. |overlay_| will be NULL if
  // |instant_url| is empty or if there is no active tab.
  void ResetOverlay(const std::string& instant_url);

  // Returns an enum value indicating the reason to fallback.
  InstantFallbackReason ShouldSwitchToLocalOverlay() const;

  // If the active tab is an Instant search results page, sets |instant_tab_| to
  // point to it. Else, deletes any existing |instant_tab_|.
  void ResetInstantTab();

  // Sends theme info, omnibox bounds, font info, etc. down to the Instant tab.
  void UpdateInfoForInstantTab();

  // Returns whether input is in progress, i.e. if the omnibox has focus and the
  // active tab is in mode SEARCH_SUGGESTIONS.
  bool IsInputInProgress() const;

  // Hide the overlay. Also sends an onchange event (with blank query) to the
  // overlay, telling it to clear out results for any old queries.
  void HideOverlay();

  // Like HideOverlay(), but doesn't call OnStaleOverlay(). Use HideOverlay()
  // unless you are going to call overlay_.reset() yourself subsequently.
  void HideInternal();

  // Counterpart to HideOverlay(). Asks the |browser_| to display the overlay
  // with the given |height| in |units|.
  void ShowOverlay(int height, InstantSizeUnits units);

  // Send the omnibox popup bounds to the page.
  void SendPopupBoundsToPage();

  // If possible, tries to mutate |suggestion| to a valid suggestion. Returns
  // true if successful. (Note that |suggestion| may be modified even if this
  // returns false.)
  bool FixSuggestion(InstantSuggestion* suggestion) const;

  // Returns true if the local page is being used.
  bool UsingLocalPage() const;

  // Returns true iff |use_tab_for_suggestions_| is true and |instant_tab_|
  // exists.
  bool UseTabForSuggestions() const;

  // Populates InstantAutocompleteResult with AutocompleteMatch details.
  // |autocomplete_match_index| specifies the index of |match| in the
  // AutocompleteResult. If the |match| is obtained from auto complete
  // providers, then the |autocomplete_match_index| is set to kNoMatchIndex.
  void PopulateInstantAutocompleteResultFromMatch(
      const AutocompleteMatch& match,
      size_t autocomplete_match_index,
      InstantAutocompleteResult* result);

  // Returns the InstantService for the browser profile.
  InstantService* GetInstantService() const;

  BrowserInstantController* const browser_;

  // Whether the extended API and regular API are enabled. If both are false,
  // Instant is effectively disabled.
  const bool extended_enabled_;
  bool instant_enabled_;

  // If true, the Instant URL is set to kChromeSearchLocalNtpUrl.
  bool use_local_page_only_;

  // If true, preload an NTP into |ntp_|.
  bool preload_ntp_;

  // The state of the overlay page, i.e., the page owned by |overlay_|. Ignored
  // if |instant_tab_| is in use.
  InstantOverlayModel model_;

  // The three instances of InstantPage maintained by InstantController as
  // described above. All three may be non-NULL in extended mode.  If
  // |instant_tab_| is not NULL, then |overlay_| is guaranteed to be hidden and
  // messages will be sent to |instant_tab_| instead.
  //
  // In non-extended mode, only |overlay_| is ever non-NULL.
  scoped_ptr<InstantOverlay> overlay_;
  scoped_ptr<InstantNTP> ntp_;
  scoped_ptr<InstantTab> instant_tab_;

  // If true, send suggestion-related events (such as user key strokes, auto
  // complete results, etc.) to |instant_tab_| instead of |overlay_|. Once set
  // to false, will stay false until the overlay is hidden or committed.
  bool use_tab_for_suggestions_;

  // The most recent full_text passed to Update(). If empty, we'll not accept
  // search suggestions from |overlay_| or |instant_tab_|.
  string16 last_omnibox_text_;

  // The most recent user_text passed to Update(). Used to filter out-of-date
  // URL suggestions from the Instant page. Set in Update() and cleared when
  // the page sets temporary text (SetSuggestions() with REPLACE behavior).
  string16 last_user_text_;

  // True if the last Update() had an inline autocompletion. Used only to make
  // sure that we don't accidentally suggest gray text suggestion in that case.
  bool last_omnibox_text_has_inline_autocompletion_;

  // The most recent verbatim passed to Update(). Used only to ensure that we
  // don't accidentally suggest an inline autocompletion.
  bool last_verbatim_;

  // The most recent suggestion received from the page, minus any prefix that
  // the user has typed.
  InstantSuggestion last_suggestion_;

  // See comments on the getter above.
  content::PageTransition last_transition_type_;

  // True if the last match passed to Update() was a search (versus a URL).
  // Used to ensure that the overlay page is committable.
  bool last_match_was_search_;

  // Omnibox focus state.
  OmniboxFocusState omnibox_focus_state_;

  // The reason for the most recent omnibox focus change.
  OmniboxFocusChangeReason omnibox_focus_change_reason_;

  // The search model mode for the active tab.
  SearchMode search_mode_;

  // Current omnibox popup bounds.
  gfx::Rect popup_bounds_;

  // Last popup bounds passed to the page.
  gfx::Rect last_popup_bounds_;

  // The start-edge margin and width of the omnibox, used by the page to align
  // its suggestions with the omnibox.
  gfx::Rect omnibox_bounds_;

  // Timer used to update the bounds of the omnibox popup.
  base::OneShotTimer<InstantController> update_bounds_timer_;

  // Search terms extraction (for autocomplete history matches) doesn't work
  // on Instant URLs. So, whenever the user commits an Instant search, we add
  // an equivalent non-Instant search URL to history, so that the search shows
  // up in autocomplete history matches.
  // TODO(sreeram): Remove when http://crbug.com/155373 is fixed.
  GURL url_for_history_;

  // The timestamp at which query editing began. This value is used when the
  // overlay is showed and cleared when the overlay is hidden.
  base::Time first_interaction_time_;

  // Indicates that the first interaction time has already been logged.
  bool first_interaction_time_recorded_;

  // Whether to allow the overlay to show search suggestions. In general, the
  // overlay is allowed to show search suggestions whenever |search_mode_| is
  // MODE_SEARCH_SUGGESTIONS, except in those cases where this is false.
  bool allow_overlay_to_show_search_suggestions_;

  // List of events and their timestamps, useful in debugging Instant behaviour.
  mutable std::list<std::pair<int64, std::string> > debug_events_;

  DISALLOW_COPY_AND_ASSIGN(InstantController);
};

#endif  // CHROME_BROWSER_UI_SEARCH_INSTANT_CONTROLLER_H_
