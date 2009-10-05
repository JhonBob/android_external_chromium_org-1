// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/logging.h"  // for NOTREACHED()
#include "base/mac_util.h"
#include "base/sys_string_conversions.h"
#include "chrome/browser/cocoa/event_utils.h"
#include "chrome/browser/cocoa/infobar.h"
#import "chrome/browser/cocoa/infobar_container_controller.h"
#import "chrome/browser/cocoa/infobar_controller.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "skia/ext/skia_utils_mac.h"
#include "third_party/GTM/AppKit/GTMUILocalizerAndLayoutTweaker.h"
#include "webkit/glue/window_open_disposition.h"


@interface InfoBarController (PrivateMethods)
// Closes the infobar by calling RemoveDelegate on the container.
// This will remove the infobar from its associated TabContents as
// well as trigger the deletion of this InfoBarController.  Once the
// delegate is removed from the container, it is no longer needed, so
// we ask it to delete itself.
- (void)closeInfoBar;
// Removes the ok and cancel buttons, and resizes the textfield to use the
// space.
- (void)removeButtons;
@end

@implementation InfoBarController

@synthesize containerController = containerController_;
@synthesize delegate = delegate_;

- (id)initWithDelegate:(InfoBarDelegate*)delegate {
  DCHECK(delegate);
  if ((self = [super initWithNibName:@"InfoBar"
                              bundle:mac_util::MainAppBundle()])) {
    delegate_ = delegate;
  }
  return self;
}

// All infobars have an icon, so we set up the icon in the base class
// awakeFromNib.
- (void)awakeFromNib {
  if (delegate_->GetIcon()) {
    [image_ setImage:gfx::SkBitmapToNSImage(*(delegate_->GetIcon()))];
  } else {
    // No icon, remove it from the view and grow the textfield to include the
    // space.
    NSRect imageFrame = [image_ frame];
    NSRect labelFrame = [label_ frame];
    labelFrame.size.width += NSMinX(imageFrame) - NSMinX(labelFrame);
    labelFrame.origin.x = imageFrame.origin.x;
    [image_ removeFromSuperview];
    [label_ setFrame:labelFrame];
  }

  [self addAdditionalControls];
}

// Called when someone clicks on the ok button.
- (void)ok:(id)sender {
  // Subclasses must override this method if they do not hide the ok button.
  NOTREACHED();
}

// Called when someone clicks on the cancel button.
- (void)cancel:(id)sender {
  // Subclasses must override this method if they do not hide the cancel button.
  NOTREACHED();
}

// Called when someone clicks on the close button.
- (void)dismiss:(id)sender {
  [self closeInfoBar];
}

- (void)addAdditionalControls {
  // Default implementation does nothing.
}

@end

@implementation InfoBarController (PrivateMethods)

- (void)closeInfoBar {
  // Calling RemoveDelegate() triggers notifications which will remove
  // the infobar view from the infobar container.  At that point it is
  // safe to ask the delegate to delete itself.
  DCHECK(delegate_);
  [containerController_ removeDelegate:delegate_];
  delegate_->InfoBarClosed();
  delegate_ = NULL;
}

- (void)removeButtons {
  // Extend the label all the way across.
  // Remove the ok and cancel buttons, since they are not needed.
  NSRect labelFrame = [label_ frame];
  labelFrame.size.width = NSMaxX([cancelButton_ frame]) - NSMinX(labelFrame);
  [okButton_ removeFromSuperview];
  [cancelButton_ removeFromSuperview];
  [label_ setFrame:labelFrame];
}

@end


/////////////////////////////////////////////////////////////////////////
// AlertInfoBarController implementation

@implementation AlertInfoBarController

// Alert infobars have a text message.
- (void)addAdditionalControls {
  // No buttons.
  [self removeButtons];

  // Insert the text.
  AlertInfoBarDelegate* delegate = delegate_->AsAlertInfoBarDelegate();
  [label_ setStringValue:base::SysWideToNSString(delegate->GetMessageText())];
}

@end


/////////////////////////////////////////////////////////////////////////
// LinkInfoBarController implementation

@implementation LinkInfoBarController

// Link infobars have a text message, of which part is linkified.  We
// use an NSAttributedString to display styled text, and we set a
// NSLink attribute on the hyperlink portion of the message.  Infobars
// use a custom NSTextField subclass, which allows us to override
// textView:clickedOnLink:atIndex: and intercept clicks.
//
// TODO(rohitrao): Using an NSTextField here has some weird UI side
// effects, such as showing the wrong cursor at times.  Explore other
// solutions.  The About box legal block has the same issue, maybe share
// a solution.
- (void)addAdditionalControls {
  // No buttons.
  [self removeButtons];

  LinkInfoBarDelegate* delegate = delegate_->AsLinkInfoBarDelegate();
  size_t offset = std::wstring::npos;
  std::wstring message = delegate->GetMessageTextWithOffset(&offset);

  // Create an attributes dictionary for the entire message.  We have
  // to expicitly set the font the control's font.  We also override
  // the cursor to give us the normal cursor rather than the text
  // insertion cursor.
  NSMutableDictionary* linkAttributes =
      [NSMutableDictionary dictionaryWithObject:[NSCursor arrowCursor]
                           forKey:NSCursorAttributeName];
  [linkAttributes setObject:[label_ font]
                  forKey:NSFontAttributeName];

  // Create the attributed string for the main message text.
  NSMutableAttributedString* infoText =
      [[[NSMutableAttributedString alloc]
         initWithString:base::SysWideToNSString(message)] autorelease];
  [infoText addAttributes:linkAttributes
                    range:NSMakeRange(0, [infoText length])];

  // Add additional attributes to style the link text appropriately as
  // well as linkify it.  We use an empty string for the NSLink
  // attribute because the actual object we pass doesn't matter, but
  // it cannot be nil.
  [linkAttributes setObject:[NSColor blueColor]
                     forKey:NSForegroundColorAttributeName];
  [linkAttributes setObject:[NSNumber numberWithBool:YES]
                     forKey:NSUnderlineStyleAttributeName];
  [linkAttributes setObject:[NSCursor pointingHandCursor]
                     forKey:NSCursorAttributeName];
  [linkAttributes setObject:[NSString string]  // dummy value
                     forKey:NSLinkAttributeName];

  // Insert the link text into the string at the appropriate offset.
  [infoText insertAttributedString:
              [[[NSAttributedString alloc]
                 initWithString:base::SysWideToNSString(delegate->GetLinkText())
                     attributes:linkAttributes] autorelease]
            atIndex:offset];

  // Update the label view with the new text.  The view must be
  // selectable and allow editing text attributes for the
  // linkification to work correctly.
  [label_ setAllowsEditingTextAttributes:YES];
  [label_ setSelectable:YES];
  [label_ setAttributedStringValue:infoText];
}

// Called when someone clicks on the link in the infobar.  This method
// is called by the InfobarTextField on its delegate (the
// LinkInfoBarController).
- (void)linkClicked {
  WindowOpenDisposition disposition =
      event_utils::WindowOpenDispositionFromNSEvent([NSApp currentEvent]);
  if (delegate_->AsLinkInfoBarDelegate()->LinkClicked(disposition))
    [self closeInfoBar];
}

@end


/////////////////////////////////////////////////////////////////////////
// ConfirmInfoBarController implementation

@implementation ConfirmInfoBarController

// Called when someone clicks on the "OK" button.
- (IBAction)ok:(id)sender {
  if (delegate_->AsConfirmInfoBarDelegate()->Accept())
    [self closeInfoBar];
}

// Called when someone clicks on the "Cancel" button.
- (IBAction)cancel:(id)sender {
  if (delegate_->AsConfirmInfoBarDelegate()->Cancel())
    [self closeInfoBar];
}

// Confirm infobars can have OK and/or cancel buttons, depending on
// the return value of GetButtons().  We create each button if
// required and position them to the left of the close button.
- (void)addAdditionalControls {
  ConfirmInfoBarDelegate* delegate = delegate_->AsConfirmInfoBarDelegate();
  int visibleButtons = delegate->GetButtons();

  NSRect okButtonFrame = [okButton_ frame];
  NSRect cancelButtonFrame = [cancelButton_ frame];

  DCHECK(NSMaxX(okButtonFrame) < NSMinX(cancelButtonFrame))
      << "Cancel button expected to be on the right of the Ok button in nib";

  CGFloat rightEdge = NSMaxX(cancelButtonFrame);
  CGFloat spaceBetweenButtons =
      NSMinX(cancelButtonFrame) - NSMaxX(okButtonFrame);
  CGFloat spaceBeforeButtons =
      NSMinX(okButtonFrame) - NSMaxX([label_ frame]);

  // Update and position the Cancel button if needed.  Otherwise, hide it.
  if (visibleButtons & ConfirmInfoBarDelegate::BUTTON_CANCEL) {
    [cancelButton_ setTitle:base::SysWideToNSString(
          delegate->GetButtonLabel(ConfirmInfoBarDelegate::BUTTON_CANCEL))];
    [GTMUILocalizerAndLayoutTweaker sizeToFitView:cancelButton_];
    cancelButtonFrame = [cancelButton_ frame];

    // Position the cancel button to the left of the Close button.
    cancelButtonFrame.origin.x = rightEdge - cancelButtonFrame.size.width;
    [cancelButton_ setFrame:cancelButtonFrame];

    // Update the rightEdge
    rightEdge = NSMinX(cancelButtonFrame);
  } else {
    [cancelButton_ removeFromSuperview];
  }

  // Update and position the OK button if needed.  Otherwise, hide it.
  if (visibleButtons & ConfirmInfoBarDelegate::BUTTON_OK) {
    [okButton_ setTitle:base::SysWideToNSString(
          delegate->GetButtonLabel(ConfirmInfoBarDelegate::BUTTON_OK))];
    [GTMUILocalizerAndLayoutTweaker sizeToFitView:okButton_];
    okButtonFrame = [okButton_ frame];

    // If we had a Cancel button, leave space between the buttons.
    if (visibleButtons & ConfirmInfoBarDelegate::BUTTON_CANCEL) {
      rightEdge -= spaceBetweenButtons;
    }

    // Position the OK button on our current right edge.
    okButtonFrame.origin.x = rightEdge - okButtonFrame.size.width;
    [okButton_ setFrame:okButtonFrame];


    // Update the rightEdge
    rightEdge = NSMinX(okButtonFrame);
  } else {
    [okButton_ removeFromSuperview];
  }

  // If we had either button, leave space before the edge of the textfield.
  if ((visibleButtons & ConfirmInfoBarDelegate::BUTTON_CANCEL) ||
      (visibleButtons & ConfirmInfoBarDelegate::BUTTON_OK)) {
    rightEdge -= spaceBeforeButtons;
  }

  NSRect frame = [label_ frame];
  DCHECK(rightEdge > NSMinX(frame))
      << "Need to make the xib larger to handle buttons with text this long";
  frame.size.width = rightEdge - NSMinX(frame);
  [label_ setFrame:frame];

  // Set the text.
  [label_ setStringValue:base::SysWideToNSString(delegate->GetMessageText())];
}

@end


//////////////////////////////////////////////////////////////////////////
// CreateInfoBar() implementations

InfoBar* AlertInfoBarDelegate::CreateInfoBar() {
  AlertInfoBarController* controller =
      [[AlertInfoBarController alloc] initWithDelegate:this];
  return new InfoBar(controller);
}

InfoBar* LinkInfoBarDelegate::CreateInfoBar() {
  LinkInfoBarController* controller =
      [[LinkInfoBarController alloc] initWithDelegate:this];
  return new InfoBar(controller);
}

InfoBar* ConfirmInfoBarDelegate::CreateInfoBar() {
  ConfirmInfoBarController* controller =
      [[ConfirmInfoBarController alloc] initWithDelegate:this];
  return new InfoBar(controller);
}
