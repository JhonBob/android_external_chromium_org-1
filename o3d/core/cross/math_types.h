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


// This file contains definitions of various math types.

#ifndef O3D_CORE_CROSS_MATH_TYPES_H_
#define O3D_CORE_CROSS_MATH_TYPES_H_

#include "third_party/vectormath/files/vectormathlibrary/include/vectormath/scalar/cpp/vectormath_aos.h"  // NOLINT

namespace o3d {

// 3d-vector
//
typedef Vectormath::Aos::Vector3 Vector3;

// 4d-vector
//
typedef Vectormath::Aos::Vector4 Vector4;

// Point
//
typedef Vectormath::Aos::Point3 Point3;

// Quaternion
//
typedef Vectormath::Aos::Quat Quaternion;

// 3x3 rotation matrix
//
typedef Vectormath::Aos::Matrix3 Matrix3;

// 4x4 matrix
//
typedef Vectormath::Aos::Matrix4 Matrix4;

// Homogeneous transformation matrix
//
typedef Vectormath::Aos::Transform3 Transform3;

// Data types for passing data around in the scenegraph
typedef float Float;

}  // namespace o3d

#endif  // O3D_CORE_CROSS_MATH_TYPES_H_


