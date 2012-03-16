// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/android/visit_sql_handler.h"

#include "base/logging.h"
#include "chrome/browser/history/history_database.h"

using base::Time;

namespace history {

namespace {

// The interesting columns of this handler.
const BookmarkRow::BookmarkColumnID kInterestingColumns[] = {
    BookmarkRow::CREATED, BookmarkRow::VISIT_COUNT,
    BookmarkRow::LAST_VISIT_TIME };

} // namespace

VisitSQLHandler::VisitSQLHandler(HistoryDatabase* history_db)
    : SQLHandler(kInterestingColumns, arraysize(kInterestingColumns)),
      history_db_(history_db) {
}

VisitSQLHandler::~VisitSQLHandler() {
}

// The created time is updated according the given |row|.
// We simulate updating created time by
// a. Remove all visits.
// b. Insert a new visit which has visit time same as created time.
// c. Insert the number of visits according the visit count in urls table.
//
// Visit row is insertted/removed to keep consistent with urls table.
// a. If the visit count in urls table is less than the visit rows in visit
//    table, all existent visits will be removed. The new visits will be
//    insertted according the value in urls table.
// b. Otherwise, only add the increased number of visit count.
bool VisitSQLHandler::Update(const BookmarkRow& row,
                             const TableIDRows& ids_set) {
  for (TableIDRows::const_iterator id = ids_set.begin();
       id != ids_set.end(); ++id) {
    VisitVector visits;
    if (!history_db_->GetVisitsForURL(id->url_id, &visits))
      return false;
    int visit_count_in_table = visits.size();
    URLRow url_row;
    if (!history_db_->GetURLRow(id->url_id, &url_row))
      return false;
    int visit_count_needed = url_row.visit_count();

    // If created time is updated or new visit count is less than the current
    // one, delete all visit rows.
    if (row.is_value_set_explicitly(BookmarkRow::CREATED) ||
        visit_count_in_table > visit_count_needed) {
      if (!DeleteVisitsForURL(id->url_id))
        return false;
      visit_count_in_table = 0;
    }

    if (row.is_value_set_explicitly(BookmarkRow::CREATED) &&
        visit_count_needed > 0) {
      if (!AddVisit(id->url_id, row.created()))
        return false;
      visit_count_in_table++;
    }

    if (!AddVisitRows(id->url_id, visit_count_needed - visit_count_in_table,
                       url_row.last_visit()))
      return false;
  }
  return true;
}

bool VisitSQLHandler::Insert(BookmarkRow* row) {
  DCHECK(row->is_value_set_explicitly(BookmarkRow::URL_ID));

  URLRow url_row;
  if (!history_db_->GetURLRow(row->url_id(), &url_row))
    return false;

  int visit_count = url_row.visit_count();

  // Add a row if the last visit time is different from created time.
  if (row->is_value_set_explicitly(BookmarkRow::CREATED) &&
      row->created() != url_row.last_visit() && visit_count > 0) {
    if (!AddVisit(row->url_id(), row->created()))
      return false;
    visit_count--;
  }

  if (!AddVisitRows(row->url_id(), visit_count, url_row.last_visit()))
    return false;

  return true;
}

bool VisitSQLHandler::Delete(const TableIDRows& ids_set) {
  for (TableIDRows::const_iterator ids = ids_set.begin();
       ids != ids_set.end(); ++ids) {
    DeleteVisitsForURL(ids->url_id);
  }
  return true;
}

bool VisitSQLHandler::AddVisit(URLID url_id, const Time& visit_time) {
  // TODO : Is 'content::PageTransition::AUTO_BOOKMARK' proper?
  // if not, a new content::PageTransition type will need.
  VisitRow visit_row(url_id, visit_time, 0,
                     content::PAGE_TRANSITION_AUTO_BOOKMARK, 0);
  return history_db_->AddVisit(&visit_row, SOURCE_BROWSED);
}

bool VisitSQLHandler::AddVisitRows(URLID url_id,
                                   int visit_count,
                                   const Time& last_visit_time) {
  int64 last_update_value = last_visit_time.ToInternalValue();
  for (int i = 0; i < visit_count; i++) {
    if (!AddVisit(url_id, Time::FromInternalValue(last_update_value - i)))
      return false;
  }
  return true;
}

bool VisitSQLHandler::DeleteVisitsForURL(URLID url_id) {
  VisitVector visits;
  if (!history_db_->GetVisitsForURL(url_id, &visits))
    return false;

  for (VisitVector::const_iterator v = visits.begin(); v != visits.end(); ++v) {
    history_db_->DeleteVisit(*v);
  }
  return true;
}

}  // namespace history.
