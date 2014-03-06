// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/trace.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <vector>

#include "base/file_util.h"
#include "base/json/string_escape.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/synchronization/lock.h"
#include "tools/gn/filesystem_utils.h"
#include "tools/gn/label.h"

namespace {

class TraceLog {
 public:
  TraceLog() {
    events_.reserve(16384);
  }
  ~TraceLog() {
    // Trace items leanked intentionally.
  }

  void Add(TraceItem* item) {
    base::AutoLock lock(lock_);
    events_.push_back(item);
  }

  // Returns a copy for threadsafety.
  std::vector<TraceItem*> events() const { return events_; }

 private:
  base::Lock lock_;

  std::vector<TraceItem*> events_;

  DISALLOW_COPY_AND_ASSIGN(TraceLog);
};

TraceLog* trace_log = NULL;

struct Coalesced {
  Coalesced() : name_ptr(NULL), total_duration(0.0), count(0) {}

  const std::string* name_ptr;  // Pointer to a string with the name in it.
  double total_duration;
  int count;
};

bool DurationGreater(const TraceItem* a, const TraceItem* b) {
  return a->delta() > b->delta();
}

bool CoalescedDurationGreater(const Coalesced& a, const Coalesced& b) {
  return a.total_duration > b.total_duration;
}

void SummarizeParses(std::vector<const TraceItem*>& loads,
                     std::ostream& out) {
  out << "File parse times: (time in ms, name)\n";

  std::sort(loads.begin(), loads.end(), &DurationGreater);

  for (size_t i = 0; i < loads.size(); i++) {
    out << base::StringPrintf(" %8.2f  ",
                              loads[i]->delta().InMillisecondsF());
    out << loads[i]->name() << std::endl;
  }
}

void SummarizeCoalesced(std::vector<const TraceItem*>& items,
                        std::ostream& out) {
  // Group by file name.
  std::map<std::string, Coalesced> coalesced;
  for (size_t i = 0; i < items.size(); i++) {
    Coalesced& c = coalesced[items[i]->name()];
    c.name_ptr = &items[i]->name();
    c.total_duration += items[i]->delta().InMillisecondsF();
    c.count++;
  }

  // Sort by duration.
  std::vector<Coalesced> sorted;
  for (std::map<std::string, Coalesced>::iterator iter = coalesced.begin();
       iter != coalesced.end(); ++iter)
    sorted.push_back(iter->second);
  std::sort(sorted.begin(), sorted.end(), &CoalescedDurationGreater);

  for (size_t i = 0; i < sorted.size(); i++) {
    out << base::StringPrintf(" %8.2f  %d  ",
                              sorted[i].total_duration, sorted[i].count);
    out << *sorted[i].name_ptr << std::endl;
  }
}

void SummarizeFileExecs(std::vector<const TraceItem*>& execs,
                        std::ostream& out) {
  out << "File execute times: (total time in ms, # executions, name)\n";
  SummarizeCoalesced(execs, out);
}

void SummarizeScriptExecs(std::vector<const TraceItem*>& execs,
                          std::ostream& out) {
  out << "Script execute times: (total time in ms, # executions, name)\n";
  SummarizeCoalesced(execs, out);
}

}  // namespace

TraceItem::TraceItem(Type type,
                     const std::string& name,
                     base::PlatformThreadId thread_id)
    : type_(type),
      name_(name),
      thread_id_(thread_id) {
}

TraceItem::~TraceItem() {
}

ScopedTrace::ScopedTrace(TraceItem::Type t, const std::string& name)
    : item_(NULL),
      done_(false) {
  if (trace_log) {
    item_ = new TraceItem(t, name, base::PlatformThread::CurrentId());
    item_->set_begin(base::TimeTicks::HighResNow());
  }
}

ScopedTrace::ScopedTrace(TraceItem::Type t, const Label& label)
    : item_(NULL),
      done_(false) {
  if (trace_log) {
    item_ = new TraceItem(t, label.GetUserVisibleName(false),
                          base::PlatformThread::CurrentId());
    item_->set_begin(base::TimeTicks::HighResNow());
  }
}

ScopedTrace::~ScopedTrace() {
  Done();
}

void ScopedTrace::SetToolchain(const Label& label) {
  if (item_)
    item_->set_toolchain(label.GetUserVisibleName(false));
}

void ScopedTrace::SetCommandLine(const CommandLine& cmdline) {
  if (item_)
    item_->set_cmdline(FilePathToUTF8(cmdline.GetArgumentsString()));
}

void ScopedTrace::Done() {
  if (!done_) {
    done_ = true;
    if (trace_log) {
      item_->set_end(base::TimeTicks::HighResNow());
      AddTrace(item_);
    }
  }
}

void EnableTracing() {
  CHECK(!trace_log);
  trace_log = new TraceLog;
}

void AddTrace(TraceItem* item) {
  trace_log->Add(item);
}

std::string SummarizeTraces() {
  if (!trace_log)
    return std::string();

  std::vector<TraceItem*> events = trace_log->events();

  // Classify all events.
  std::vector<const TraceItem*> parses;
  std::vector<const TraceItem*> file_execs;
  std::vector<const TraceItem*> script_execs;
  for (size_t i = 0; i < events.size(); i++) {
    switch (events[i]->type()) {
      case TraceItem::TRACE_FILE_PARSE:
        parses.push_back(events[i]);
        break;
      case TraceItem::TRACE_FILE_EXECUTE:
        file_execs.push_back(events[i]);
        break;
      case TraceItem::TRACE_SCRIPT_EXECUTE:
        script_execs.push_back(events[i]);
        break;
      case TraceItem::TRACE_FILE_LOAD:
      case TraceItem::TRACE_FILE_WRITE:
      case TraceItem::TRACE_DEFINE_TARGET:
        break;  // Ignore these for the summary.
    }
  }

  std::ostringstream out;
  SummarizeParses(parses, out);
  out << std::endl;
  SummarizeFileExecs(file_execs, out);
  out << std::endl;
  SummarizeScriptExecs(script_execs, out);
  out << std::endl;

  return out.str();
}

void SaveTraces(const base::FilePath& file_name) {
  std::ostringstream out;

  out << "{\"traceEvents\":[";

  std::string quote_buffer;  // Allocate outside loop to prevent reallocationg.

  // Write main thread metadata (assume this is being written on the main
  // thread).
  out << "{\"pid\":0,\"tid\":" << base::PlatformThread::CurrentId();
  out << ",\"ts\":0,\"ph\":\"M\",";
  out << "\"name\":\"thread_name\",\"args\":{\"name\":\"Main thread\"}},";

  std::vector<TraceItem*> events = trace_log->events();
  for (size_t i = 0; i < events.size(); i++) {
    const TraceItem& item = *events[i];

    if (i != 0)
      out << ",";
    out << "{\"pid\":0,\"tid\":" << item.thread_id();
    out << ",\"ts\":" << item.begin().ToInternalValue();
    out << ",\"ph\":\"X\"";  // "X" = complete event with begin & duration.
    out << ",\"dur\":" << item.delta().InMicroseconds();

    quote_buffer.resize(0);
    base::EscapeJSONString(item.name(), true, &quote_buffer);
    out << ",\"name\":" << quote_buffer;

    out << ",\"cat\":";
    switch (item.type()) {
      case TraceItem::TRACE_FILE_LOAD:
        out << "\"load\"";
        break;
      case TraceItem::TRACE_FILE_PARSE:
        out << "\"parse\"";
        break;
      case TraceItem::TRACE_FILE_EXECUTE:
        out << "\"file_exec\"";
        break;
      case TraceItem::TRACE_FILE_WRITE:
        out << "\"file_write\"";
        break;
      case TraceItem::TRACE_SCRIPT_EXECUTE:
        out << "\"script_exec\"";
        break;
      case TraceItem::TRACE_DEFINE_TARGET:
        out << "\"define\"";
    }

    if (!item.toolchain().empty() || !item.cmdline().empty()) {
      out << ",\"args\":{";
      bool needs_comma = false;
      if (!item.toolchain().empty()) {
        quote_buffer.resize(0);
        base::EscapeJSONString(item.toolchain(), true, &quote_buffer);
        out << "\"toolchain\":" << quote_buffer;
        needs_comma = true;
      }
      if (!item.cmdline().empty()) {
        quote_buffer.resize(0);
        base::EscapeJSONString(item.cmdline(), true, &quote_buffer);
        if (needs_comma)
          out << ",";
        out << "\"cmdline\":" << quote_buffer;
        needs_comma = true;
      }
      out << "}";
    }
    out << "}";
  }

  out << "]}";

  std::string out_str = out.str();
  base::WriteFile(file_name, out_str.data(),
                       static_cast<int>(out_str.size()));
}
