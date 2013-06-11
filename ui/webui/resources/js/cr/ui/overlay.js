// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Provides dialog-like behaviors for the tracing UI.
 */
cr.define('cr.ui.overlay', function() {

  /**
   * Gets the top, visible overlay. It makes the assumption that if multiple
   * overlays are visible, the last in the byte order is topmost.
   * TODO(estade): rely on aria-visibility instead?
   * @return {HTMLElement} The overlay.
   */
  function getTopOverlay() {
    var overlays = document.querySelectorAll('.overlay:not([hidden])');
    return overlays[overlays.length - 1];
  }

  /**
   * Returns a visible default button of the overlay, if it has one. If the
   * overlay has more than one, the first one will be returned.
   *
   * @param {HTMLElement} overlay The .overlay.
   * @return {HTMLElement} The default button.
   */
  function getDefaultButton(overlay) {
    function isHidden(node) { return node.hidden; }
    var defaultButtons =
        overlay.querySelectorAll('.page .button-strip > .default-button');
    for (var i = 0; i < defaultButtons.length; i++) {
      if (!findAncestor(defaultButtons[i], isHidden))
        return defaultButtons[i];
    }
    return null;
  }

  /**
   * Makes initializations which must hook at the document level.
   */
  function globalInitialization() {
    document.addEventListener('keydown', function(e) {
      var overlay = getTopOverlay();
      if (!overlay)
        return;

      // Close the overlay on escape.
      if (e.keyCode == 27)  // Escape
        cr.dispatchSimpleEvent(overlay, 'cancelOverlay');

      // Execute the overlay's default button on enter, unless focus is on an
      // element that has standard behavior for the enter key.
      var forbiddenTagNames = /^(A|BUTTON|SELECT|TEXTAREA)$/;
      if (e.keyIdentifier == 'Enter' &&
          !forbiddenTagNames.test(document.activeElement.tagName)) {
        var button = getDefaultButton(overlay);
        if (button)
          button.click();
      }
    });

    window.addEventListener('resize', setMaxHeightAllPages);

    setMaxHeightAllPages();
  }

  /**
   * Sets the max-height of all pages in all overlays, based on the window
   * height.
   */
  function setMaxHeightAllPages() {
    var pages = document.querySelectorAll('.overlay .page');

    var maxHeight = Math.min(0.9 * window.innerHeight, 640) + 'px';
    for (var i = 0; i < pages.length; i++)
      pages[i].style.maxHeight = maxHeight;
  }

  /**
   * Adds behavioral hooks for the given overlay.
   * @param {HTMLElement} overlay The .overlay.
   */
  function setupOverlay(overlay) {
    // Close the overlay on clicking any of the pages' close buttons.
    var closeButtons = overlay.querySelectorAll('.page > .close-button');
    for (var i = 0; i < closeButtons.length; i++) {
      closeButtons[i].addEventListener('click', function(e) {
        cr.dispatchSimpleEvent(overlay, 'cancelOverlay');
      });
    }

    // Remove the 'pulse' animation any time the overlay is hidden or shown.
    overlay.__defineSetter__('hidden', function(value) {
      this.classList.remove('pulse');
      if (value)
        this.setAttribute('hidden', true);
      else
        this.removeAttribute('hidden');
    });
    overlay.__defineGetter__('hidden', function() {
      return this.hasAttribute('hidden');
    });

    // Shake when the user clicks away.
    overlay.addEventListener('click', function(e) {
      // Only pulse if the overlay was the target of the click.
      if (this != e.target)
        return;

      // This may be null while the overlay is closing.
      var overlayPage = this.querySelector('.page:not([hidden])');
      if (overlayPage)
        overlayPage.classList.add('pulse');
    });
    overlay.addEventListener('webkitAnimationEnd', function(e) {
      e.target.classList.remove('pulse');
    });
  }

  return {
    globalInitialization: globalInitialization,
    setupOverlay: setupOverlay,
  };
});

document.addEventListener('DOMContentLoaded',
                          cr.ui.overlay.globalInitialization);
