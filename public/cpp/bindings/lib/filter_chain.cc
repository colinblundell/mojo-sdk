// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/mojo/public/cpp/bindings/lib/filter_chain.h"

#include <algorithm>

#include "third_party/mojo/public/cpp/environment/logging.h"

namespace mojo {
namespace internal {

FilterChain::FilterChain(MessageReceiver* sink) : sink_(sink) {
}

FilterChain::FilterChain(RValue other) : sink_(other.object->sink_) {
  other.object->sink_ = nullptr;
  filters_.swap(other.object->filters_);
}

FilterChain& FilterChain::operator=(RValue other) {
  std::swap(sink_, other.object->sink_);
  filters_.swap(other.object->filters_);
  return *this;
}

FilterChain::~FilterChain() {
  for (std::vector<MessageFilter*>::iterator iter = filters_.begin();
       iter != filters_.end();
       ++iter) {
    delete *iter;
  }
}

void FilterChain::SetSink(MessageReceiver* sink) {
  MOJO_DCHECK(!sink_);
  sink_ = sink;
  if (!filters_.empty())
    filters_.back()->set_sink(sink);
}

MessageReceiver* FilterChain::GetHead() {
  MOJO_DCHECK(sink_);
  return filters_.empty() ? sink_ : filters_.front();
}

}  // namespace internal
}  // namespace mojo
