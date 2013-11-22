// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RESOURCES_PICTURE_PILE_BASE_H_
#define CC_RESOURCES_PICTURE_PILE_BASE_H_

#include <list>
#include <utility>

#include "base/containers/hash_tables.h"
#include "base/memory/ref_counted.h"
#include "cc/base/cc_export.h"
#include "cc/base/region.h"
#include "cc/base/tiling_data.h"
#include "cc/resources/picture.h"
#include "ui/gfx/size.h"

namespace base {
class Value;
}

namespace cc {

class CC_EXPORT PicturePileBase : public base::RefCounted<PicturePileBase> {
 public:
  PicturePileBase();
  explicit PicturePileBase(const PicturePileBase* other);
  PicturePileBase(const PicturePileBase* other, unsigned thread_index);

  void Resize(gfx::Size size);
  gfx::Size size() const { return tiling_.total_size(); }
  void SetMinContentsScale(float min_contents_scale);

  void UpdateRecordedRegion();
  const Region& recorded_region() const { return recorded_region_; }

  int num_tiles_x() const { return tiling_.num_tiles_x(); }
  int num_tiles_y() const { return tiling_.num_tiles_y(); }
  gfx::Rect tile_bounds(int x, int y) const { return tiling_.TileBounds(x, y); }
  bool HasRecordingAt(int x, int y);
  bool CanRaster(float contents_scale, gfx::Rect content_rect);

  static void ComputeTileGridInfo(gfx::Size tile_grid_size,
                                  SkTileGridPicture::TileGridInfo* info);

  void SetTileGridSize(gfx::Size tile_grid_size);
  TilingData& tiling() { return tiling_; }

  scoped_ptr<base::Value> AsValue() const;

 protected:
  struct CC_EXPORT PictureInfo {
    PictureInfo();
    ~PictureInfo();

    bool Invalidate();
    PictureInfo CloneForThread(int thread_index) const;

    scoped_refptr<Picture> picture;
  };

  typedef std::pair<int, int> PictureMapKey;
  typedef base::hash_map<PictureMapKey, PictureInfo> PictureMap;

  virtual ~PicturePileBase();

  int num_raster_threads() { return num_raster_threads_; }
  int buffer_pixels() const { return tiling_.border_texels(); }
  void Clear();

  gfx::Rect PaddedRect(const PictureMapKey& key);
  gfx::Rect PadRect(gfx::Rect rect);

  // A picture pile is a tiled set of pictures. The picture map is a map of tile
  // indices to picture infos.
  PictureMap picture_map_;
  TilingData tiling_;
  Region recorded_region_;
  float min_contents_scale_;
  SkTileGridPicture::TileGridInfo tile_grid_info_;
  SkColor background_color_;
  int slow_down_raster_scale_factor_for_debug_;
  bool contents_opaque_;
  bool show_debug_picture_borders_;
  bool clear_canvas_with_debug_color_;
  int num_raster_threads_;

 private:
  void SetBufferPixels(int buffer_pixels);

  friend class base::RefCounted<PicturePileBase>;
  DISALLOW_COPY_AND_ASSIGN(PicturePileBase);
};

}  // namespace cc

#endif  // CC_RESOURCES_PICTURE_PILE_BASE_H_
