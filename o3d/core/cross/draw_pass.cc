/*
 * Copyright 2009, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// This file contains the definition of DrawPass.

#include "core/cross/precompile.h"
#include "core/cross/draw_pass.h"
#include "core/cross/draw_list.h"
#include "core/cross/transformation_context.h"

namespace o3d {

O3D_DEFN_CLASS(DrawPass, RenderNode);

const char* DrawPass::kDrawListParamName =
    O3D_STRING_CONSTANT("drawList");
const char* DrawPass::kSortMethodParamName =
    O3D_STRING_CONSTANT("sortMethod");

DrawPass::DrawPass(ServiceLocator* service_locator)
    : RenderNode(service_locator) {
  RegisterParamRef(kDrawListParamName, &draw_list_param_);
  RegisterParamRef(kSortMethodParamName, &sort_method_param_);
}

void DrawPass::Render(RenderContext* render_context) {
  DrawList* drawlist = draw_list();
  if (!drawlist) {
    return;
  }

  // Draw the elements of this list.
  drawlist->Render(render_context, sort_method());
}

ObjectBase::Ref DrawPass::Create(ServiceLocator* service_locator) {
  return ObjectBase::Ref(new DrawPass(service_locator));
}
}  // namespace o3d
