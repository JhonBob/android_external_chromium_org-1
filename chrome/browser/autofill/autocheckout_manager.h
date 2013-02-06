// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_AUTOFILL_AUTOCHECKOUT_MANAGER_H_
#define CHROME_BROWSER_AUTOFILL_AUTOCHECKOUT_MANAGER_H_

#include <string>

#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/string16.h"
#include "chrome/browser/autofill/autocheckout_page_meta_data.h"

class AutofillField;
class AutofillManager;
class AutofillProfile;
class CreditCard;
class FormStructure;
class GURL;

struct FormData;
struct FormFieldData;

namespace autofill {
struct WebElementDescriptor;
}

namespace content {
struct SSLStatus;
}

class AutocheckoutManager {
 public:
  explicit AutocheckoutManager(AutofillManager* autofill_manager);
  virtual ~AutocheckoutManager();

  // Fill all the forms seen by the Autofill manager with the information
  // gathered from the requestAutocomplete dialog.
  void FillForms();

  // Sets |page_meta_data_| with the meta data for the current page.
  void OnLoadedPageMetaData(
      scoped_ptr<autofill::AutocheckoutPageMetaData> page_meta_data);

  // Show the requestAutocomplete dialog.
  virtual void ShowAutocheckoutDialog(const GURL& frame_url,
                                      const content::SSLStatus& ssl_status);

  // Whether or not the current page is the start of a multipage Autofill flow.
  bool IsStartOfAutofillableFlow() const;

  // Whether or not the current page is part of a multipage Autofill flow.
  bool IsInAutofillableFlow() const;

 private:
  // Callback called from AutofillDialogController on filling up the UI form.
  void ReturnAutocheckoutData(const FormStructure* result);

  // Sets value of form field data |field_to_fill| based on the Autofill
  // field type specified by |field|.
  void SetValue(const AutofillField& field, FormFieldData* field_to_fill);

  AutofillManager* autofill_manager_;  // WEAK; owns us

  // Credit card verification code.
  string16 cvv_;

  // Profile built using the data supplied by requestAutocomplete dialog.
  scoped_ptr<AutofillProfile> profile_;

  // Credit card built using the data supplied by requestAutocomplete dialog.
  scoped_ptr<CreditCard> credit_card_;

  // Autocheckout specific page meta data.
  scoped_ptr<autofill::AutocheckoutPageMetaData> page_meta_data_;

  base::WeakPtrFactory<AutocheckoutManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AutocheckoutManager);
};

#endif  // CHROME_BROWSER_AUTOFILL_AUTOCHECKOUT_MANAGER_H_
