// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#include "cc/tiled_layer.h"

#include "Region.h"
#include "base/basictypes.h"
#include "cc/layer_impl.h"
#include "cc/layer_tree_host.h"
#include "cc/overdraw_metrics.h"
#include "cc/tiled_layer_impl.h"
#include "third_party/khronos/GLES2/gl2.h"

using namespace std;
using WebKit::WebTransformationMatrix;

namespace cc {

class UpdatableTile : public LayerTilingData::Tile {
public:
    static scoped_ptr<UpdatableTile> create(scoped_ptr<LayerUpdater::Texture> texture)
    {
        return make_scoped_ptr(new UpdatableTile(texture.Pass()));
    }

    LayerUpdater::Texture* texture() { return m_texture.get(); }
    PrioritizedTexture* managedTexture() { return m_texture->texture(); }

    bool isDirty() const { return !dirtyRect.isEmpty(); }

    // Reset update state for the current frame. This should occur before painting
    // for all layers. Since painting one layer can invalidate another layer
    // after it has already painted, mark all non-dirty tiles as valid before painting
    // such that invalidations during painting won't prevent them from being pushed.
    void resetUpdateState()
    {
        updateRect = IntRect();
        occluded = false;
        partialUpdate = false;
        validForFrame = !isDirty();
    }

    // This promises to update the tile and therefore also guarantees the tile
    // will be valid for this frame. dirtyRect is copied into updateRect so
    // we can continue to track re-entrant invalidations that occur during painting.
    void markForUpdate()
    {
        validForFrame = true;
        updateRect = dirtyRect;
        dirtyRect = IntRect();
    }

    IntRect dirtyRect;
    IntRect updateRect;
    bool partialUpdate;
    bool validForFrame;
    bool occluded;
    bool isInUseOnImpl;

private:
    explicit UpdatableTile(scoped_ptr<LayerUpdater::Texture> texture)
        : partialUpdate(false)
        , validForFrame(false)
        , occluded(false)
        , isInUseOnImpl(false)
        , m_texture(texture.Pass())
    {
    }

    scoped_ptr<LayerUpdater::Texture> m_texture;

    DISALLOW_COPY_AND_ASSIGN(UpdatableTile);
};

TiledLayer::TiledLayer()
    : Layer()
    , m_textureFormat(GL_INVALID_ENUM)
    , m_skipsDraw(false)
    , m_failedUpdate(false)
    , m_tilingOption(AutoTile)
{
    m_tiler = LayerTilingData::create(IntSize(), LayerTilingData::HasBorderTexels);
}

TiledLayer::~TiledLayer()
{
}

scoped_ptr<LayerImpl> TiledLayer::createLayerImpl()
{
    return TiledLayerImpl::create(id()).PassAs<LayerImpl>();
}

void TiledLayer::updateTileSizeAndTilingOption()
{
    DCHECK(layerTreeHost());

    const IntSize& defaultTileSize = layerTreeHost()->settings().defaultTileSize;
    const IntSize& maxUntiledLayerSize = layerTreeHost()->settings().maxUntiledLayerSize;
    int layerWidth = contentBounds().width();
    int layerHeight = contentBounds().height();

    const IntSize tileSize(min(defaultTileSize.width(), layerWidth), min(defaultTileSize.height(), layerHeight));

    // Tile if both dimensions large, or any one dimension large and the other
    // extends into a second tile but the total layer area isn't larger than that
    // of the largest possible untiled layer. This heuristic allows for long skinny layers
    // (e.g. scrollbars) that are Nx1 tiles to minimize wasted texture space but still avoids
    // creating very large tiles.
    const bool anyDimensionLarge = layerWidth > maxUntiledLayerSize.width() || layerHeight > maxUntiledLayerSize.height();
    const bool anyDimensionOneTile = (layerWidth <= defaultTileSize.width() || layerHeight <= defaultTileSize.height())
                                      && (layerWidth * layerHeight) <= (maxUntiledLayerSize.width() * maxUntiledLayerSize.height());
    const bool autoTiled = anyDimensionLarge && !anyDimensionOneTile;

    bool isTiled;
    if (m_tilingOption == AlwaysTile)
        isTiled = true;
    else if (m_tilingOption == NeverTile)
        isTiled = false;
    else
        isTiled = autoTiled;

    IntSize requestedSize = isTiled ? tileSize : contentBounds();
    const int maxSize = layerTreeHost()->rendererCapabilities().maxTextureSize;
    IntSize clampedSize = requestedSize.shrunkTo(IntSize(maxSize, maxSize));
    setTileSize(clampedSize);
}

void TiledLayer::updateBounds()
{
    IntSize oldBounds = m_tiler->bounds();
    IntSize newBounds = contentBounds();
    if (oldBounds == newBounds)
        return;
    m_tiler->setBounds(newBounds);

    // Invalidate any areas that the new bounds exposes.
    Region oldRegion(IntRect(IntPoint(), oldBounds));
    Region newRegion(IntRect(IntPoint(), newBounds));
    newRegion.subtract(oldRegion);
    Vector<WebCore::IntRect> rects = newRegion.rects();
    for (size_t i = 0; i < rects.size(); ++i)
        invalidateContentRect(rects[i]);
}

void TiledLayer::setTileSize(const IntSize& size)
{
    m_tiler->setTileSize(size);
}

void TiledLayer::setBorderTexelOption(LayerTilingData::BorderTexelOption borderTexelOption)
{
    m_tiler->setBorderTexelOption(borderTexelOption);
}

bool TiledLayer::drawsContent() const
{
    if (!Layer::drawsContent())
        return false;

    bool hasMoreThanOneTile = m_tiler->numTilesX() > 1 || m_tiler->numTilesY() > 1;
    if (m_tilingOption == NeverTile && hasMoreThanOneTile)
        return false;

    return true;
}

bool TiledLayer::needsContentsScale() const
{
    return true;
}

IntSize TiledLayer::contentBounds() const
{
    return IntSize(lroundf(bounds().width() * contentsScale()), lroundf(bounds().height() * contentsScale()));
}

void TiledLayer::setTilingOption(TilingOption tilingOption)
{
    m_tilingOption = tilingOption;
}

void TiledLayer::setIsMask(bool isMask)
{
    setTilingOption(isMask ? NeverTile : AutoTile);
}

void TiledLayer::pushPropertiesTo(LayerImpl* layer)
{
    Layer::pushPropertiesTo(layer);

    TiledLayerImpl* tiledLayer = static_cast<TiledLayerImpl*>(layer);

    tiledLayer->setSkipsDraw(m_skipsDraw);
    tiledLayer->setTilingData(*m_tiler);
    Vector<UpdatableTile*> invalidTiles;

    for (LayerTilingData::TileMap::const_iterator iter = m_tiler->tiles().begin(); iter != m_tiler->tiles().end(); ++iter) {
        int i = iter->first.first;
        int j = iter->first.second;
        UpdatableTile* tile = static_cast<UpdatableTile*>(iter->second);
        // FIXME: This should not ever be null.
        if (!tile)
            continue;

        tile->isInUseOnImpl = false;

        if (!tile->managedTexture()->haveBackingTexture()) {
            // Evicted tiles get deleted from both layers
            invalidTiles.append(tile);
            continue;
        }

        if (!tile->validForFrame) {
            // Invalidated tiles are set so they can get different debug colors.
            tiledLayer->pushInvalidTile(i, j);
            continue;
        }

        tiledLayer->pushTileProperties(i, j, tile->managedTexture()->resourceId(), tile->opaqueRect(), tile->managedTexture()->contentsSwizzled());
        tile->isInUseOnImpl = true;
    }
    for (Vector<UpdatableTile*>::const_iterator iter = invalidTiles.begin(); iter != invalidTiles.end(); ++iter)
        m_tiler->takeTile((*iter)->i(), (*iter)->j());
}

PrioritizedTextureManager* TiledLayer::textureManager() const
{
    if (!layerTreeHost())
        return 0;
    return layerTreeHost()->contentsTextureManager();
}

void TiledLayer::setLayerTreeHost(LayerTreeHost* host)
{
    if (host && host != layerTreeHost()) {
        for (LayerTilingData::TileMap::const_iterator iter = m_tiler->tiles().begin(); iter != m_tiler->tiles().end(); ++iter) {
            UpdatableTile* tile = static_cast<UpdatableTile*>(iter->second);
            // FIXME: This should not ever be null.
            if (!tile)
                continue;
            tile->managedTexture()->setTextureManager(host->contentsTextureManager());
        }
    }
    Layer::setLayerTreeHost(host);
}

UpdatableTile* TiledLayer::tileAt(int i, int j) const
{
    return static_cast<UpdatableTile*>(m_tiler->tileAt(i, j));
}

UpdatableTile* TiledLayer::createTile(int i, int j)
{
    createUpdaterIfNeeded();

    scoped_ptr<UpdatableTile> tile(UpdatableTile::create(updater()->createTexture(textureManager())));
    tile->managedTexture()->setDimensions(m_tiler->tileSize(), m_textureFormat);

    UpdatableTile* addedTile = tile.get();
    m_tiler->addTile(tile.PassAs<LayerTilingData::Tile>(), i, j);

    addedTile->dirtyRect = m_tiler->tileRect(addedTile);

    // Temporary diagnostic crash.
    if (!addedTile)
        CRASH();
    if (!tileAt(i, j))
        CRASH();

    return addedTile;
}

void TiledLayer::setNeedsDisplayRect(const FloatRect& dirtyRect)
{
    float contentsWidthScale = static_cast<float>(contentBounds().width()) / bounds().width();
    float contentsHeightScale = static_cast<float>(contentBounds().height()) / bounds().height();
    FloatRect scaledDirtyRect(dirtyRect);
    scaledDirtyRect.scale(contentsWidthScale, contentsHeightScale);
    IntRect dirty = enclosingIntRect(scaledDirtyRect);
    invalidateContentRect(dirty);
    Layer::setNeedsDisplayRect(dirtyRect);
}

void TiledLayer::setUseLCDText(bool useLCDText)
{
    Layer::setUseLCDText(useLCDText);

    LayerTilingData::BorderTexelOption borderTexelOption;
#if OS(ANDROID)
    // Always want border texels and GL_LINEAR due to pinch zoom.
    borderTexelOption = LayerTilingData::HasBorderTexels;
#else
    borderTexelOption = useLCDText ? LayerTilingData::NoBorderTexels : LayerTilingData::HasBorderTexels;
#endif
    setBorderTexelOption(borderTexelOption);
}

void TiledLayer::invalidateContentRect(const IntRect& contentRect)
{
    updateBounds();
    if (m_tiler->isEmpty() || contentRect.isEmpty() || m_skipsDraw)
        return;

    for (LayerTilingData::TileMap::const_iterator iter = m_tiler->tiles().begin(); iter != m_tiler->tiles().end(); ++iter) {
        UpdatableTile* tile = static_cast<UpdatableTile*>(iter->second);
        DCHECK(tile);
        // FIXME: This should not ever be null.
        if (!tile)
            continue;
        IntRect bound = m_tiler->tileRect(tile);
        bound.intersect(contentRect);
        tile->dirtyRect.unite(bound);
    }
}

// Returns true if tile is dirty and only part of it needs to be updated.
bool TiledLayer::tileOnlyNeedsPartialUpdate(UpdatableTile* tile)
{
    return !tile->dirtyRect.contains(m_tiler->tileRect(tile));
}

// Dirty tiles with valid textures needs buffered update to guarantee that
// we don't modify textures currently used for drawing by the impl thread.
bool TiledLayer::tileNeedsBufferedUpdate(UpdatableTile* tile)
{
    if (!tile->managedTexture()->haveBackingTexture())
        return false;

    if (!tile->isDirty())
        return false;

    if (!tile->isInUseOnImpl)
        return false;

    return true;
}


bool TiledLayer::updateTiles(int left, int top, int right, int bottom, TextureUpdateQueue& queue, const OcclusionTracker* occlusion, RenderingStats& stats, bool& didPaint)
{
    didPaint = false;
    createUpdaterIfNeeded();

    bool ignoreOcclusions = !occlusion;
    if (!haveTexturesForTiles(left, top, right, bottom, ignoreOcclusions)) {
        m_failedUpdate = true;
        return false;
    }

    IntRect paintRect = markTilesForUpdate(left, top, right, bottom, ignoreOcclusions);

    if (occlusion)
        occlusion->overdrawMetrics().didPaint(paintRect);

    if (paintRect.isEmpty())
        return true;

    didPaint = true;
    updateTileTextures(paintRect, left, top, right, bottom, queue, occlusion, stats);
    return true;
}

void TiledLayer::markOcclusionsAndRequestTextures(int left, int top, int right, int bottom, const OcclusionTracker* occlusion)
{
    // There is some difficult dependancies between occlusions, recording occlusion metrics
    // and requesting memory so those are encapsulated in this function:
    // - We only want to call requestLate on unoccluded textures (to preserve
    //   memory for other layers when near OOM).
    // - We only want to record occlusion metrics if all memory requests succeed.

    int occludedTileCount = 0;
    bool succeeded = true;
    for (int j = top; j <= bottom; ++j) {
        for (int i = left; i <= right; ++i) {
            UpdatableTile* tile = tileAt(i, j);
            DCHECK(tile); // Did setTexturePriorities get skipped?
            // FIXME: This should not ever be null.
            if (!tile)
                continue;
            DCHECK(!tile->occluded); // Did resetUpdateState get skipped? Are we doing more than one occlusion pass?
            IntRect visibleTileRect = intersection(m_tiler->tileBounds(i, j), visibleContentRect());
            if (occlusion && occlusion->occluded(this, visibleTileRect)) {
                tile->occluded = true;
                occludedTileCount++;
            } else {
                succeeded &= tile->managedTexture()->requestLate();
            }
        }
    }

    if (!succeeded)
        return;

    // FIXME: Remove the loop and just pass the count!
    for (int i = 0; i < occludedTileCount; i++)
        occlusion->overdrawMetrics().didCullTileForUpload();
}

bool TiledLayer::haveTexturesForTiles(int left, int top, int right, int bottom, bool ignoreOcclusions)
{
    for (int j = top; j <= bottom; ++j) {
        for (int i = left; i <= right; ++i) {
            UpdatableTile* tile = tileAt(i, j);
            DCHECK(tile); // Did setTexturePriorites get skipped?
            // FIXME: This should not ever be null.
            if (!tile)
                continue;

            // Ensure the entire tile is dirty if we don't have the texture.
            if (!tile->managedTexture()->haveBackingTexture())
                tile->dirtyRect = m_tiler->tileRect(tile);

            // If using occlusion and the visible region of the tile is occluded,
            // don't reserve a texture or update the tile.
            if (tile->occluded && !ignoreOcclusions)
                continue;

            if (!tile->managedTexture()->canAcquireBackingTexture())
                return false;
        }
    }
    return true;
}

IntRect TiledLayer::markTilesForUpdate(int left, int top, int right, int bottom, bool ignoreOcclusions)
{
    IntRect paintRect;
    for (int j = top; j <= bottom; ++j) {
        for (int i = left; i <= right; ++i) {
            UpdatableTile* tile = tileAt(i, j);
            DCHECK(tile); // Did setTexturePriorites get skipped?
            // FIXME: This should not ever be null.
            if (!tile)
                continue;
            if (tile->occluded && !ignoreOcclusions)
                continue;
            paintRect.unite(tile->dirtyRect);
            tile->markForUpdate();
        }
    }
    return paintRect;
}

void TiledLayer::updateTileTextures(const IntRect& paintRect, int left, int top, int right, int bottom, TextureUpdateQueue& queue, const OcclusionTracker* occlusion, RenderingStats& stats)
{
    // The updateRect should be in layer space. So we have to convert the paintRect from content space to layer space.
    m_updateRect = FloatRect(paintRect);
    float widthScale = bounds().width() / static_cast<float>(contentBounds().width());
    float heightScale = bounds().height() / static_cast<float>(contentBounds().height());
    m_updateRect.scale(widthScale, heightScale);

    // Calling prepareToUpdate() calls into WebKit to paint, which may have the side
    // effect of disabling compositing, which causes our reference to the texture updater to be deleted.
    // However, we can't free the memory backing the SkCanvas until the paint finishes,
    // so we grab a local reference here to hold the updater alive until the paint completes.
    scoped_refptr<LayerUpdater> protector(updater());
    IntRect paintedOpaqueRect;
    updater()->prepareToUpdate(paintRect, m_tiler->tileSize(), 1 / widthScale, 1 / heightScale, paintedOpaqueRect, stats);

    for (int j = top; j <= bottom; ++j) {
        for (int i = left; i <= right; ++i) {
            UpdatableTile* tile = tileAt(i, j);
            DCHECK(tile); // Did setTexturePriorites get skipped?
            // FIXME: This should not ever be null.
            if (!tile)
                continue;

            IntRect tileRect = m_tiler->tileBounds(i, j);

            // Use updateRect as the above loop copied the dirty rect for this frame to updateRect.
            const IntRect& dirtyRect = tile->updateRect;
            if (dirtyRect.isEmpty())
                continue;

            // Save what was painted opaque in the tile. Keep the old area if the paint didn't touch it, and didn't paint some
            // other part of the tile opaque.
            IntRect tilePaintedRect = intersection(tileRect, paintRect);
            IntRect tilePaintedOpaqueRect = intersection(tileRect, paintedOpaqueRect);
            if (!tilePaintedRect.isEmpty()) {
                IntRect paintInsideTileOpaqueRect = intersection(tile->opaqueRect(), tilePaintedRect);
                bool paintInsideTileOpaqueRectIsNonOpaque = !tilePaintedOpaqueRect.contains(paintInsideTileOpaqueRect);
                bool opaquePaintNotInsideTileOpaqueRect = !tilePaintedOpaqueRect.isEmpty() && !tile->opaqueRect().contains(tilePaintedOpaqueRect);

                if (paintInsideTileOpaqueRectIsNonOpaque || opaquePaintNotInsideTileOpaqueRect)
                    tile->setOpaqueRect(tilePaintedOpaqueRect);
            }

            // sourceRect starts as a full-sized tile with border texels included.
            IntRect sourceRect = m_tiler->tileRect(tile);
            sourceRect.intersect(dirtyRect);
            // Paint rect not guaranteed to line up on tile boundaries, so
            // make sure that sourceRect doesn't extend outside of it.
            sourceRect.intersect(paintRect);

            tile->updateRect = sourceRect;

            if (sourceRect.isEmpty())
                continue;

            const IntPoint anchor = m_tiler->tileRect(tile).location();

            // Calculate tile-space rectangle to upload into.
            IntSize destOffset(sourceRect.x() - anchor.x(), sourceRect.y() - anchor.y());
            if (destOffset.width() < 0)
                CRASH();
            if (destOffset.height() < 0)
                CRASH();

            // Offset from paint rectangle to this tile's dirty rectangle.
            IntPoint paintOffset(sourceRect.x() - paintRect.x(), sourceRect.y() - paintRect.y());
            if (paintOffset.x() < 0)
                CRASH();
            if (paintOffset.y() < 0)
                CRASH();
            if (paintOffset.x() + sourceRect.width() > paintRect.width())
                CRASH();
            if (paintOffset.y() + sourceRect.height() > paintRect.height())
                CRASH();

            tile->texture()->update(queue, sourceRect, destOffset, tile->partialUpdate, stats);
            if (occlusion)
                occlusion->overdrawMetrics().didUpload(WebTransformationMatrix(), sourceRect, tile->opaqueRect());

        }
    }
}

namespace {
// This picks a small animated layer to be anything less than one viewport. This
// is specifically for page transitions which are viewport-sized layers. The extra
// 64 pixels is due to these layers being slightly larger than the viewport in some cases.
bool isSmallAnimatedLayer(TiledLayer* layer)
{
    if (!layer->drawTransformIsAnimating() && !layer->screenSpaceTransformIsAnimating())
        return false;
    IntSize viewportSize = layer->layerTreeHost() ? layer->layerTreeHost()->deviceViewportSize() : IntSize();
    IntRect contentRect(IntPoint::zero(), layer->contentBounds());
    return contentRect.width() <= viewportSize.width() + 64
        && contentRect.height() <= viewportSize.height() + 64;
}

// FIXME: Remove this and make this based on distance once distance can be calculated
// for offscreen layers. For now, prioritize all small animated layers after 512
// pixels of pre-painting.
void setPriorityForTexture(const IntRect& visibleRect,
                           const IntRect& tileRect,
                           bool drawsToRoot,
                           bool isSmallAnimatedLayer,
                           PrioritizedTexture* texture)
{
    int priority = PriorityCalculator::lowestPriority();
    if (!visibleRect.isEmpty())
        priority = PriorityCalculator::priorityFromDistance(visibleRect, tileRect, drawsToRoot);
    if (isSmallAnimatedLayer)
        priority = PriorityCalculator::maxPriority(priority, PriorityCalculator::smallAnimatedLayerMinPriority());
    if (priority != PriorityCalculator::lowestPriority())
        texture->setRequestPriority(priority);
}
}

void TiledLayer::setTexturePriorities(const PriorityCalculator& priorityCalc)
{
    updateBounds();
    resetUpdateState();

    if (m_tiler->hasEmptyBounds())
        return;

    bool drawsToRoot = !renderTarget()->parent();
    bool smallAnimatedLayer = isSmallAnimatedLayer(this);

    // Minimally create the tiles in the desired pre-paint rect.
    IntRect createTilesRect = idlePaintRect();
    if (!createTilesRect.isEmpty()) {
        int left, top, right, bottom;
        m_tiler->contentRectToTileIndices(createTilesRect, left, top, right, bottom);
        for (int j = top; j <= bottom; ++j) {
            for (int i = left; i <= right; ++i) {
                if (!tileAt(i, j))
                    createTile(i, j);
            }
        }
    }

    // Also, minimally create all tiles for small animated layers and also
    // double-buffer them since we have limited their size to be reasonable.
    IntRect doubleBufferedRect = visibleContentRect();
    if (smallAnimatedLayer)
        doubleBufferedRect = IntRect(IntPoint::zero(), contentBounds());

    // Create additional textures for double-buffered updates when needed.
    // These textures must stay alive while the updated textures are incrementally
    // uploaded, swapped atomically via pushProperties, and finally deleted
    // after the commit is complete, after which they can be recycled.
    if (!doubleBufferedRect.isEmpty()) {
        int left, top, right, bottom;
        m_tiler->contentRectToTileIndices(doubleBufferedRect, left, top, right, bottom);
        for (int j = top; j <= bottom; ++j) {
            for (int i = left; i <= right; ++i) {
                UpdatableTile* tile = tileAt(i, j);
                if (!tile)
                    tile = createTile(i, j);
                // We need an additional texture if the tile needs a buffered-update and it's not a partial update.
                // FIXME: Decide if partial update should be allowed based on cost
                // of update. https://bugs.webkit.org/show_bug.cgi?id=77376
                if (!layerTreeHost() || !layerTreeHost()->bufferedUpdates() || !tileNeedsBufferedUpdate(tile))
                    continue;
                if (tileOnlyNeedsPartialUpdate(tile) && layerTreeHost()->requestPartialTextureUpdate()) {
                    tile->partialUpdate = true;
                    continue;
                }

                IntRect tileRect = m_tiler->tileRect(tile);
                tile->dirtyRect = tileRect;
                LayerUpdater::Texture* backBuffer = tile->texture();
                setPriorityForTexture(visibleContentRect(), tile->dirtyRect, drawsToRoot, smallAnimatedLayer, backBuffer->texture());
                scoped_ptr<PrioritizedTexture> frontBuffer = PrioritizedTexture::create(backBuffer->texture()->textureManager(),
                                                                                        backBuffer->texture()->size(),
                                                                                        backBuffer->texture()->format());
                // Swap backBuffer into frontBuffer and add it to delete after commit queue.
                backBuffer->swapTextureWith(frontBuffer);
                layerTreeHost()->deleteTextureAfterCommit(frontBuffer.Pass());
            }
        }
    }

    // Now update priorities on all tiles we have in the layer, no matter where they are.
    for (LayerTilingData::TileMap::const_iterator iter = m_tiler->tiles().begin(); iter != m_tiler->tiles().end(); ++iter) {
        UpdatableTile* tile = static_cast<UpdatableTile*>(iter->second);
        // FIXME: This should not ever be null.
        if (!tile)
            continue;
        IntRect tileRect = m_tiler->tileRect(tile);
        setPriorityForTexture(visibleContentRect(), tileRect, drawsToRoot, smallAnimatedLayer, tile->managedTexture());
    }
}

Region TiledLayer::visibleContentOpaqueRegion() const
{
    if (m_skipsDraw)
        return Region();
    if (contentsOpaque())
        return visibleContentRect();
    return m_tiler->opaqueRegionInContentRect(visibleContentRect());
}

void TiledLayer::resetUpdateState()
{
    m_skipsDraw = false;
    m_failedUpdate = false;

    LayerTilingData::TileMap::const_iterator end = m_tiler->tiles().end();
    for (LayerTilingData::TileMap::const_iterator iter = m_tiler->tiles().begin(); iter != end; ++iter) {
        UpdatableTile* tile = static_cast<UpdatableTile*>(iter->second);
        // FIXME: This should not ever be null.
        if (!tile)
            continue;
        tile->resetUpdateState();
    }
}

void TiledLayer::update(TextureUpdateQueue& queue, const OcclusionTracker* occlusion, RenderingStats& stats)
{
    DCHECK(!m_skipsDraw && !m_failedUpdate); // Did resetUpdateState get skipped?
    updateBounds();
    if (m_tiler->hasEmptyBounds() || !drawsContent())
        return;

    bool didPaint = false;

    // Animation pre-paint. If the layer is small, try to paint it all
    // immediately whether or not it is occluded, to avoid paint/upload
    // hiccups while it is animating.
    if (isSmallAnimatedLayer(this)) {
        int left, top, right, bottom;
        m_tiler->contentRectToTileIndices(IntRect(IntPoint::zero(), contentBounds()), left, top, right, bottom);
        updateTiles(left, top, right, bottom, queue, 0, stats, didPaint);
        if (didPaint)
            return;
        // This was an attempt to paint the entire layer so if we fail it's okay,
        // just fallback on painting visible etc. below.
        m_failedUpdate = false;
    }

    if (visibleContentRect().isEmpty())
        return;

    // Visible painting. First occlude visible tiles and paint the non-occluded tiles.
    int left, top, right, bottom;
    m_tiler->contentRectToTileIndices(visibleContentRect(), left, top, right, bottom);
    markOcclusionsAndRequestTextures(left, top, right, bottom, occlusion);
    m_skipsDraw = !updateTiles(left, top, right, bottom, queue, occlusion, stats, didPaint);
    if (m_skipsDraw)
        m_tiler->reset();
    if (m_skipsDraw || didPaint)
        return;

    // If we have already painting everything visible. Do some pre-painting while idle.
    IntRect idlePaintContentRect = idlePaintRect();
    if (idlePaintContentRect.isEmpty())
        return;

    // Prepaint anything that was occluded but inside the layer's visible region.
    if (!updateTiles(left, top, right, bottom, queue, 0, stats, didPaint) || didPaint)
        return;

    int prepaintLeft, prepaintTop, prepaintRight, prepaintBottom;
    m_tiler->contentRectToTileIndices(idlePaintContentRect, prepaintLeft, prepaintTop, prepaintRight, prepaintBottom);

    // Then expand outwards from the visible area until we find a dirty row or column to update.
    while (left > prepaintLeft || top > prepaintTop || right < prepaintRight || bottom < prepaintBottom) {
        if (bottom < prepaintBottom) {
            ++bottom;
            if (!updateTiles(left, bottom, right, bottom, queue, 0, stats, didPaint) || didPaint)
                return;
        }
        if (top > prepaintTop) {
            --top;
            if (!updateTiles(left, top, right, top, queue, 0, stats, didPaint) || didPaint)
                return;
        }
        if (left > prepaintLeft) {
            --left;
            if (!updateTiles(left, top, left, bottom, queue, 0, stats, didPaint) || didPaint)
                return;
        }
        if (right < prepaintRight) {
            ++right;
            if (!updateTiles(right, top, right, bottom, queue, 0, stats, didPaint) || didPaint)
                return;
        }
    }
}

bool TiledLayer::needsIdlePaint()
{
    // Don't trigger more paints if we failed (as we'll just fail again).
    if (m_failedUpdate || visibleContentRect().isEmpty() || m_tiler->hasEmptyBounds() || !drawsContent())
        return false;

    IntRect idlePaintContentRect = idlePaintRect();
    if (idlePaintContentRect.isEmpty())
        return false;

    int left, top, right, bottom;
    m_tiler->contentRectToTileIndices(idlePaintContentRect, left, top, right, bottom);

    for (int j = top; j <= bottom; ++j) {
        for (int i = left; i <= right; ++i) {
            UpdatableTile* tile = tileAt(i, j);
            DCHECK(tile); // Did setTexturePriorities get skipped?
            if (!tile)
                continue;

            bool updated = !tile->updateRect.isEmpty();
            bool canAcquire = tile->managedTexture()->canAcquireBackingTexture();
            bool dirty = tile->isDirty() || !tile->managedTexture()->haveBackingTexture();
            if (!updated && canAcquire && dirty)
                return true;
        }
    }
    return false;
}

IntRect TiledLayer::idlePaintRect()
{
    // Don't inflate an empty rect.
    if (visibleContentRect().isEmpty())
        return IntRect();

    // FIXME: This can be made a lot larger now! We should increase
    //        this slowly while insuring it doesn't cause any perf issues.
    IntRect prepaintRect = visibleContentRect();
    prepaintRect.inflateX(m_tiler->tileSize().width());
    prepaintRect.inflateY(m_tiler->tileSize().height() * 2);
    IntRect contentRect(IntPoint::zero(), contentBounds());
    prepaintRect.intersect(contentRect);

    return prepaintRect;
}

}
