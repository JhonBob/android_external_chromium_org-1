# This file is read into the GN build.

# Files are relative to third_party/skia.
{
  # Shared defines for all builds.
  'skia_feature_defines': [
    'GR_GL_CUSTOM_SETUP_HEADER="GrGLConfig_chrome.h"',
    'GR_GL_IGNORE_ES3_MSAA=0',
    'SK_ATTR_DEPRECATED=SK_NOTHING_ARG1',
    'SK_DEFERRED_CANVAS_USES_FACTORIES=1',
    'SK_DISABLE_OFFSETIMAGEFILTER_OPTIMIZATION',
    'SK_ENABLE_INST_COUNT=0',
    'SK_ENABLE_LEGACY_API_ALIASING=1',
    'SK_HIGH_QUALITY_IS_LANCZOS',
    'SK_IGNORE_BLURRED_RRECT_OPT',
    'SK_IGNORE_QUAD_RR_CORNERS_OPT',
    'SK_SUPPORT_LEGACY_GETCLIPTYPE',
    'SK_SUPPORT_LEGACY_GETTOPDEVICE',
    'SK_SUPPORT_LEGACY_GETTOTALCLIP',
    'SK_SUPPORT_LEGACY_LAYERRASTERIZER_API=1',
    'SK_SUPPORT_LEGACY_PUBLICEFFECTCONSTRUCTORS=1',
    'SK_SUPPORT_LEGACY_N32_NAME',
    'SK_USE_DISCARDABLE_SCALEDIMAGECACHE',
    'SK_WILL_NEVER_DRAW_PERSPECTIVE_TEXT',
  ],

  'skia_gpu_sources': [
    'include/gpu/GrBackendEffectFactory.h',
    'include/gpu/GrClipData.h',
    'include/gpu/GrColor.h',
    'include/gpu/GrConfig.h',
    'include/gpu/GrContext.h',
    'include/gpu/GrContextFactory.h',
    'include/gpu/GrCoordTransform.h',
    'include/gpu/GrEffect.h',
    'include/gpu/GrEffectStage.h',
    'include/gpu/GrEffectUnitTest.h',
    'include/gpu/GrFontScaler.h',
    'include/gpu/GrGlyph.h',
    'include/gpu/GrKey.h',
    'include/gpu/GrPaint.h',
    'include/gpu/GrPathRendererChain.h',
    'include/gpu/GrPoint.h',
    'include/gpu/GrRect.h',
    'include/gpu/GrRenderTarget.h',
    'include/gpu/GrResource.h',
    'include/gpu/GrSurface.h',
    'include/gpu/GrTBackendEffectFactory.h',
    'include/gpu/GrTexture.h',
    'include/gpu/GrTextureAccess.h',
    'include/gpu/GrTypes.h',
    'include/gpu/GrUserConfig.h',

    'include/gpu/gl/GrGLConfig.h',
    'include/gpu/gl/GrGLExtensions.h',
    'include/gpu/gl/GrGLFunctions.h',
    'include/gpu/gl/GrGLInterface.h',

    'src/gpu/GrAAHairLinePathRenderer.cpp',
    'src/gpu/GrAAHairLinePathRenderer.h',
    'src/gpu/GrAAConvexPathRenderer.cpp',
    'src/gpu/GrAAConvexPathRenderer.h',
    'src/gpu/GrAARectRenderer.cpp',
    'src/gpu/GrAARectRenderer.h',
    'src/gpu/GrAddPathRenderers_default.cpp',
    'src/gpu/GrAllocator.h',
    'src/gpu/GrAllocPool.h',
    'src/gpu/GrAllocPool.cpp',
    'src/gpu/GrAtlas.cpp',
    'src/gpu/GrAtlas.h',
    'src/gpu/GrBinHashKey.h',
    'src/gpu/GrBitmapTextContext.cpp',
    'src/gpu/GrBitmapTextContext.h',
    'src/gpu/GrBlend.cpp',
    'src/gpu/GrBlend.h',
    'src/gpu/GrBufferAllocPool.cpp',
    'src/gpu/GrBufferAllocPool.h',
    'src/gpu/GrCacheID.cpp',
    'src/gpu/GrClipData.cpp',
    'src/gpu/GrContext.cpp',
    'src/gpu/GrDefaultPathRenderer.cpp',
    'src/gpu/GrDefaultPathRenderer.h',
    'src/gpu/GrDistanceFieldTextContext.h',
    'src/gpu/GrDistanceFieldTextContext.cpp',
    'src/gpu/GrDrawState.cpp',
    'src/gpu/GrDrawState.h',
    'src/gpu/GrDrawTarget.cpp',
    'src/gpu/GrDrawTarget.h',
    'src/gpu/GrDrawTargetCaps.h',
    'src/gpu/GrEffect.cpp',
    'src/gpu/GrGeometryBuffer.h',
    'src/gpu/GrClipMaskCache.h',
    'src/gpu/GrClipMaskCache.cpp',
    'src/gpu/GrClipMaskManager.h',
    'src/gpu/GrClipMaskManager.cpp',
    'src/gpu/GrGpu.cpp',
    'src/gpu/GrGpu.h',
    'src/gpu/GrGpuFactory.cpp',
    'src/gpu/GrIndexBuffer.h',
    'src/gpu/GrInOrderDrawBuffer.cpp',
    'src/gpu/GrInOrderDrawBuffer.h',
    'src/gpu/GrLayerCache.cpp',
    'src/gpu/GrLayerCache.h',
    'src/gpu/GrMemoryPool.cpp',
    'src/gpu/GrMemoryPool.h',
    'src/gpu/GrOrderedSet.h',
    'src/gpu/GrOvalRenderer.cpp',
    'src/gpu/GrOvalRenderer.h',
    'src/gpu/GrPaint.cpp',
    'src/gpu/GrPath.cpp',
    'src/gpu/GrPath.h',
    'src/gpu/GrPathRendererChain.cpp',
    'src/gpu/GrPathRenderer.cpp',
    'src/gpu/GrPathRenderer.h',
    'src/gpu/GrPathUtils.cpp',
    'src/gpu/GrPathUtils.h',
    'src/gpu/GrPictureUtils.h',
    'src/gpu/GrPictureUtils.cpp',
    'src/gpu/GrPlotMgr.h',
    'src/gpu/GrRectanizer.cpp',
    'src/gpu/GrRectanizer.h',
    'src/gpu/GrRectanizer_skyline.cpp',
    'src/gpu/GrRedBlackTree.h',
    'src/gpu/GrRenderTarget.cpp',
    'src/gpu/GrReducedClip.cpp',
    'src/gpu/GrReducedClip.h',
    'src/gpu/GrResource.cpp',
    'src/gpu/GrResourceCache.cpp',
    'src/gpu/GrResourceCache.h',
    'src/gpu/GrStencil.cpp',
    'src/gpu/GrStencil.h',
    'src/gpu/GrStencilAndCoverPathRenderer.cpp',
    'src/gpu/GrStencilAndCoverPathRenderer.h',
    'src/gpu/GrStencilBuffer.cpp',
    'src/gpu/GrStencilBuffer.h',
    'src/gpu/GrTBSearch.h',
    'src/gpu/GrTraceMarker.cpp',
    'src/gpu/GrTraceMarker.h',
    'src/gpu/GrTracing.h',
    'src/gpu/GrSWMaskHelper.cpp',
    'src/gpu/GrSWMaskHelper.h',
    'src/gpu/GrSoftwarePathRenderer.cpp',
    'src/gpu/GrSoftwarePathRenderer.h',
    'src/gpu/GrSurface.cpp',
    'src/gpu/GrTemplates.h',
    'src/gpu/GrTextContext.cpp',
    'src/gpu/GrTextContext.h',
    'src/gpu/GrTextStrike.cpp',
    'src/gpu/GrTextStrike.h',
    'src/gpu/GrTextStrike_impl.h',
    'src/gpu/GrTexture.cpp',
    'src/gpu/GrTextureAccess.cpp',
    'src/gpu/GrTHashTable.h',
    'src/gpu/GrVertexBuffer.h',

    'src/gpu/effects/Gr1DKernelEffect.h',
    'src/gpu/effects/GrConfigConversionEffect.cpp',
    'src/gpu/effects/GrConfigConversionEffect.h',
    'src/gpu/effects/GrBezierEffect.cpp',
    'src/gpu/effects/GrBezierEffect.h',
    'src/gpu/effects/GrConvolutionEffect.cpp',
    'src/gpu/effects/GrConvolutionEffect.h',
    'src/gpu/effects/GrConvexPolyEffect.cpp',
    'src/gpu/effects/GrConvexPolyEffect.h',
    'src/gpu/effects/GrBicubicEffect.cpp',
    'src/gpu/effects/GrBicubicEffect.h',
    'src/gpu/effects/GrCustomCoordsTextureEffect.cpp',
    'src/gpu/effects/GrCustomCoordsTextureEffect.h',
    'src/gpu/effects/GrDistanceFieldTextureEffect.cpp',
    'src/gpu/effects/GrDistanceFieldTextureEffect.h',
    'src/gpu/effects/GrOvalEffect.cpp',
    'src/gpu/effects/GrOvalEffect.h',
    'src/gpu/effects/GrRRectEffect.cpp',
    'src/gpu/effects/GrRRectEffect.h',
    'src/gpu/effects/GrSimpleTextureEffect.cpp',
    'src/gpu/effects/GrSimpleTextureEffect.h',
    'src/gpu/effects/GrSingleTextureEffect.cpp',
    'src/gpu/effects/GrSingleTextureEffect.h',
    'src/gpu/effects/GrTextureDomain.cpp',
    'src/gpu/effects/GrTextureDomain.h',
    'src/gpu/effects/GrTextureStripAtlas.cpp',
    'src/gpu/effects/GrTextureStripAtlas.h',

    'src/gpu/gl/GrGLBufferImpl.cpp',
    'src/gpu/gl/GrGLBufferImpl.h',
    'src/gpu/gl/GrGLCaps.cpp',
    'src/gpu/gl/GrGLCaps.h',
    'src/gpu/gl/GrGLContext.cpp',
    'src/gpu/gl/GrGLContext.h',
    'src/gpu/gl/GrGLCreateNativeInterface_none.cpp',
    'src/gpu/gl/GrGLDefaultInterface_none.cpp',
    'src/gpu/gl/GrGLDefines.h',
    'src/gpu/gl/GrGLEffect.h',
    'src/gpu/gl/GrGLVertexEffect.h',
    'src/gpu/gl/GrGLExtensions.cpp',
    'src/gpu/gl/GrGLIndexBuffer.cpp',
    'src/gpu/gl/GrGLIndexBuffer.h',
    'src/gpu/gl/GrGLInterface.cpp',
    'src/gpu/gl/GrGLIRect.h',
    'src/gpu/gl/GrGLNoOpInterface.cpp',
    'src/gpu/gl/GrGLNoOpInterface.h',
    'src/gpu/gl/GrGLPath.cpp',
    'src/gpu/gl/GrGLPath.h',
    'src/gpu/gl/GrGLProgram.cpp',
    'src/gpu/gl/GrGLProgram.h',
    'src/gpu/gl/GrGLProgramDesc.cpp',
    'src/gpu/gl/GrGLProgramDesc.h',
    'src/gpu/gl/GrGLProgramEffects.cpp',
    'src/gpu/gl/GrGLProgramEffects.h',
    'src/gpu/gl/GrGLRenderTarget.cpp',
    'src/gpu/gl/GrGLRenderTarget.h',
    'src/gpu/gl/GrGLShaderBuilder.cpp',
    'src/gpu/gl/GrGLShaderBuilder.h',
    'src/gpu/gl/GrGLShaderVar.h',
    'src/gpu/gl/GrGLSL.cpp',
    'src/gpu/gl/GrGLSL.h',
    'src/gpu/gl/GrGLSL_impl.h',
    'src/gpu/gl/GrGLStencilBuffer.cpp',
    'src/gpu/gl/GrGLStencilBuffer.h',
    'src/gpu/gl/GrGLTexture.cpp',
    'src/gpu/gl/GrGLTexture.h',
    'src/gpu/gl/GrGLUtil.cpp',
    'src/gpu/gl/GrGLUtil.h',
    'src/gpu/gl/GrGLUniformManager.cpp',
    'src/gpu/gl/GrGLUniformManager.h',
    'src/gpu/gl/GrGLUniformHandle.h',
    'src/gpu/gl/GrGLVertexArray.cpp',
    'src/gpu/gl/GrGLVertexArray.h',
    'src/gpu/gl/GrGLVertexBuffer.cpp',
    'src/gpu/gl/GrGLVertexBuffer.h',
    'src/gpu/gl/GrGpuGL.cpp',
    'src/gpu/gl/GrGpuGL.h',
    'src/gpu/gl/GrGpuGL_program.cpp',

    # Sk files
    'include/gpu/SkGpuDevice.h',
    'include/gpu/SkGr.h',
    'include/gpu/SkGrPixelRef.h',
    'include/gpu/SkGrTexturePixelRef.h',

    'include/gpu/gl/SkGLContextHelper.h',

    'src/gpu/SkGpuDevice.cpp',
    'src/gpu/SkGr.cpp',
    'src/gpu/SkGrFontScaler.cpp',
    'src/gpu/SkGrPixelRef.cpp',
    'src/gpu/SkGrTexturePixelRef.cpp',

    'src/image/SkImage_Gpu.cpp',
    'src/image/SkSurface_Gpu.cpp',

    'src/gpu/gl/SkGLContextHelper.cpp'
  ],

  'skia_core_sources': [
    'src/core/ARGB32_Clamp_Bilinear_BitmapShader.h',
    'src/core/SkAAClip.cpp',
    'src/core/SkAnnotation.cpp',
    'src/core/SkAdvancedTypefaceMetrics.cpp',
    'src/core/SkAlphaRuns.cpp',
    'src/core/SkAntiRun.h',
    'src/core/SkBBoxHierarchy.h',
    'src/core/SkBBoxRecord.cpp',
    'src/core/SkBBoxRecord.h',
    'src/core/SkBBoxHierarchyRecord.cpp',
    'src/core/SkBBoxHierarchyRecord.h',
    'src/core/SkBitmap.cpp',
    'src/core/SkBitmapDevice.cpp',
    'src/core/SkBitmapFilter.h',
    'src/core/SkBitmapFilter.cpp',
    'src/core/SkBitmapHeap.cpp',
    'src/core/SkBitmapHeap.h',
    'src/core/SkBitmapProcShader.cpp',
    'src/core/SkBitmapProcShader.h',
    'src/core/SkBitmapProcState.cpp',
    'src/core/SkBitmapProcState.h',
    'src/core/SkBitmapProcState_matrix.h',
    'src/core/SkBitmapProcState_matrixProcs.cpp',
    'src/core/SkBitmapProcState_sample.h',
    'src/core/SkBitmapScaler.h',
    'src/core/SkBitmapScaler.cpp',
    'src/core/SkBitmapShader16BilerpTemplate.h',
    'src/core/SkBitmapShaderTemplate.h',
    'src/core/SkBitmap_scroll.cpp',
    'src/core/SkBlitBWMaskTemplate.h',
    'src/core/SkBlitMask_D32.cpp',
    'src/core/SkBlitRow_D16.cpp',
    'src/core/SkBlitRow_D32.cpp',
    'src/core/SkBlitter.h',
    'src/core/SkBlitter.cpp',
    'src/core/SkBlitter_A8.cpp',
    'src/core/SkBlitter_ARGB32.cpp',
    'src/core/SkBlitter_RGB16.cpp',
    'src/core/SkBlitter_Sprite.cpp',
    'src/core/SkBuffer.cpp',
    'src/core/SkCanvas.cpp',
    'src/core/SkChunkAlloc.cpp',
    'src/core/SkClipStack.cpp',
    'src/core/SkColor.cpp',
    'src/core/SkColorFilter.cpp',
    'src/core/SkColorTable.cpp',
    'src/core/SkComposeShader.cpp',
    'src/core/SkConfig8888.cpp',
    'src/core/SkConfig8888.h',
    'src/core/SkConvolver.cpp',
    'src/core/SkConvolver.h',
    'src/core/SkCoreBlitters.h',
    'src/core/SkCubicClipper.cpp',
    'src/core/SkCubicClipper.h',
    'src/core/SkData.cpp',
    'src/core/SkDataTable.cpp',
    'src/core/SkDebug.cpp',
    'src/core/SkDeque.cpp',
    'src/core/SkDevice.cpp',
    'src/core/SkDeviceLooper.cpp',
    'src/core/SkDeviceProfile.cpp',
    'src/lazy/SkDiscardableMemoryPool.cpp',
    'src/lazy/SkDiscardablePixelRef.cpp',
    'src/core/SkDistanceFieldGen.cpp',
    'src/core/SkDistanceFieldGen.h',
    'src/core/SkDither.cpp',
    'src/core/SkDraw.cpp',
    'src/core/SkDrawLooper.cpp',
    'src/core/SkDrawProcs.h',
    'src/core/SkEdgeBuilder.cpp',
    'src/core/SkEdgeClipper.cpp',
    'src/core/SkEdge.cpp',
    'src/core/SkEdge.h',
    'src/core/SkError.cpp',
    'src/core/SkErrorInternals.h',
    'src/core/SkFilterProc.cpp',
    'src/core/SkFilterProc.h',
    'src/core/SkFilterShader.cpp',
    'src/core/SkFlattenable.cpp',
    'src/core/SkFlattenableSerialization.cpp',
    'src/core/SkFloat.cpp',
    'src/core/SkFloat.h',
    'src/core/SkFloatBits.cpp',
    'src/core/SkFontHost.cpp',
    'src/core/SkFontDescriptor.cpp',
    'src/core/SkFontDescriptor.h',
    'src/core/SkFontStream.cpp',
    'src/core/SkFontStream.h',
    'src/core/SkGeometry.cpp',
    'src/core/SkGlyphCache.cpp',
    'src/core/SkGlyphCache.h',
    'src/core/SkGlyphCache_Globals.h',
    'src/core/SkGraphics.cpp',
    'src/core/SkInstCnt.cpp',
    'src/core/SkImageFilter.cpp',
    'src/core/SkImageInfo.cpp',
    'src/core/SkLineClipper.cpp',
    'src/core/SkMallocPixelRef.cpp',
    'src/core/SkMask.cpp',
    'src/core/SkMaskFilter.cpp',
    'src/core/SkMaskGamma.cpp',
    'src/core/SkMaskGamma.h',
    'src/core/SkMath.cpp',
    'src/core/SkMatrix.cpp',
    'src/core/SkMatrixClipStateMgr.cpp',
    'src/core/SkMatrixClipStateMgr.h',
    'src/core/SkMessageBus.h',
    'src/core/SkMetaData.cpp',
    'src/core/SkMipMap.cpp',
    'src/core/SkOffsetTable.h',
    'src/core/SkPackBits.cpp',
    'src/core/SkPaint.cpp',
    'src/core/SkPaintOptionsAndroid.cpp',
    'src/core/SkPaintPriv.cpp',
    'src/core/SkPaintPriv.h',
    'src/core/SkPath.cpp',
    'src/core/SkPathEffect.cpp',
    'src/core/SkPathHeap.cpp',
    'src/core/SkPathHeap.h',
    'src/core/SkPathMeasure.cpp',
    'src/core/SkPathRef.cpp',
    'src/core/SkPicture.cpp',
    'src/core/SkPictureFlat.cpp',
    'src/core/SkPictureFlat.h',
    'src/core/SkPicturePlayback.cpp',
    'src/core/SkPicturePlayback.h',
    'src/core/SkPictureRecord.cpp',
    'src/core/SkPictureRecord.h',
    'src/core/SkPictureShader.cpp',
    'src/core/SkPictureShader.h',
    'src/core/SkPictureStateTree.cpp',
    'src/core/SkPictureStateTree.h',
    'src/core/SkPixelRef.cpp',
    'src/core/SkPoint.cpp',
    'src/core/SkProcSpriteBlitter.cpp',
    'src/core/SkPtrRecorder.cpp',
    'src/core/SkQuadClipper.cpp',
    'src/core/SkQuadClipper.h',
    'src/core/SkQuadTree.cpp',
    'src/core/SkQuadTree.h',
    'src/core/SkQuadTreePicture.cpp',
    'src/core/SkQuadTreePicture.h',
    'src/core/SkRasterClip.cpp',
    'src/core/SkRasterizer.cpp',
    'src/core/SkReadBuffer.cpp',
    'src/core/SkRect.cpp',
    'src/core/SkRefDict.cpp',
    'src/core/SkRegion.cpp',
    'src/core/SkRegionPriv.h',
    'src/core/SkRegion_path.cpp',
    'src/core/SkRRect.cpp',
    'src/core/SkRTree.h',
    'src/core/SkRTree.cpp',
    'src/core/SkScaledImageCache.cpp',
    'src/core/SkScalar.cpp',
    'src/core/SkScalerContext.cpp',
    'src/core/SkScalerContext.h',
    'src/core/SkScan.cpp',
    'src/core/SkScan.h',
    'src/core/SkScanPriv.h',
    'src/core/SkScan_AntiPath.cpp',
    'src/core/SkScan_Antihair.cpp',
    'src/core/SkScan_Hairline.cpp',
    'src/core/SkScan_Path.cpp',
    'src/core/SkShader.cpp',
    'src/core/SkSpriteBlitter_ARGB32.cpp',
    'src/core/SkSpriteBlitter_RGB16.cpp',
    'src/core/SkSinTable.h',
    'src/core/SkSpriteBlitter.h',
    'src/core/SkSpriteBlitterTemplate.h',
    'src/core/SkStream.cpp',
    'src/core/SkString.cpp',
    'src/core/SkStringUtils.cpp',
    'src/core/SkStroke.h',
    'src/core/SkStroke.cpp',
    'src/core/SkStrokeRec.cpp',
    'src/core/SkStrokerPriv.cpp',
    'src/core/SkStrokerPriv.h',
    'src/core/SkTextFormatParams.h',
    'src/core/SkTileGrid.cpp',
    'src/core/SkTileGrid.h',
    'src/core/SkTileGridPicture.cpp',
    'src/core/SkTLList.h',
    'src/core/SkTLS.cpp',
    'src/core/SkTraceEvent.h',
    'src/core/SkTSearch.cpp',
    'src/core/SkTSort.h',
    'src/core/SkTypeface.cpp',
    'src/core/SkTypefaceCache.cpp',
    'src/core/SkTypefaceCache.h',
    'src/core/SkUnPreMultiply.cpp',
    'src/core/SkUtils.cpp',
    'src/core/SkValidatingReadBuffer.cpp',
    'src/core/SkWriteBuffer.cpp',
    'src/core/SkWriter32.cpp',
    'src/core/SkXfermode.cpp',
    'src/doc/SkDocument.cpp',
    'src/image/SkImage.cpp',
    'src/image/SkImagePriv.cpp',
    'src/image/SkImage_Codec.cpp',
    'src/image/SkImage_Picture.cpp',
    'src/image/SkImage_Raster.cpp',
    'src/image/SkSurface.cpp',
    'src/image/SkSurface_Base.h',
    'src/image/SkSurface_Picture.cpp',
    'src/image/SkSurface_Raster.cpp',
    'src/pipe/SkGPipeRead.cpp',
    'src/pipe/SkGPipeWrite.cpp',
    'include/core/SkAdvancedTypefaceMetrics.h',
    'include/core/SkBitmap.h',
    'include/core/SkBitmapDevice.h',
    'include/core/SkBlitRow.h',
    'include/core/SkBounder.h',
    'include/core/SkCanvas.h',
    'include/core/SkChecksum.h',
    'include/core/SkChunkAlloc.h',
    'include/core/SkClipStack.h',
    'include/core/SkColor.h',
    'include/core/SkColorFilter.h',
    'include/core/SkColorPriv.h',
    'include/core/SkColorShader.h',
    'include/core/SkComposeShader.h',
    'include/core/SkData.h',
    'include/core/SkDeque.h',
    'include/core/SkDevice.h',
    'include/core/SkDeviceProperties.h',
    'include/core/SkDither.h',
    'include/core/SkDraw.h',
    'include/core/SkDrawFilter.h',
    'include/core/SkDrawLooper.h',
    'include/core/SkEndian.h',
    'include/core/SkError.h',
    'include/core/SkFixed.h',
    'include/core/SkFlattenable.h',
    'include/core/SkFlattenableSerialization.h',
    'include/core/SkFloatBits.h',
    'include/core/SkFloatingPoint.h',
    'include/core/SkFontHost.h',
    'include/core/SkGeometry.h',
    'include/core/SkGraphics.h',
    'include/core/SkImage.h',
    'include/core/SkImageDecoder.h',
    'include/core/SkImageEncoder.h',
    'include/core/SkImageFilter.h',
    'include/core/SkImageInfo.h',
    'include/core/SkInstCnt.h',
    'include/core/SkMallocPixelRef.h',
    'include/core/SkMask.h',
    'include/core/SkMaskFilter.h',
    'include/core/SkMath.h',
    'include/core/SkMatrix.h',
    'include/core/SkMetaData.h',
    'include/core/SkOnce.h',
    'include/core/SkOSFile.h',
    'include/core/SkPackBits.h',
    'include/core/SkPaint.h',
    'include/core/SkPath.h',
    'include/core/SkPathEffect.h',
    'include/core/SkPathMeasure.h',
    'include/core/SkPathRef.h',
    'include/core/SkPicture.h',
    'include/core/SkPixelRef.h',
    'include/core/SkPoint.h',
    'include/core/SkPreConfig.h',
    'include/core/SkRasterizer.h',
    'include/core/SkReader32.h',
    'include/core/SkRect.h',
    'include/core/SkRefCnt.h',
    'include/core/SkRegion.h',
    'include/core/SkRRect.h',
    'include/core/SkScalar.h',
    'include/core/SkShader.h',
    'include/core/SkStream.h',
    'include/core/SkString.h',
    'include/core/SkStringUtils.h',
    'include/core/SkStrokeRec.h',
    'include/core/SkSurface.h',
    'include/core/SkTArray.h',
    'include/core/SkTDArray.h',
    'include/core/SkTDStack.h',
    'include/core/SkTDict.h',
    'include/core/SkTInternalLList.h',
    'include/core/SkTileGridPicture.h',
    'include/core/SkTRegistry.h',
    'include/core/SkTSearch.h',
    'include/core/SkTemplates.h',
    'include/core/SkThread.h',
    'include/core/SkTime.h',
    'include/core/SkTLazy.h',
    'include/core/SkTypeface.h',
    'include/core/SkTypes.h',
    'include/core/SkUnPreMultiply.h',
    'include/core/SkUnitMapper.h',
    'include/core/SkUtils.h',
    'include/core/SkWeakRefCnt.h',
    'include/core/SkWriter32.h',
    'include/core/SkXfermode.h',
    'src/lazy/SkCachingPixelRef.cpp',
    'src/lazy/SkCachingPixelRef.h',
    'include/pathops/SkPathOps.h',
    'src/pathops/SkAddIntersections.cpp',
    'src/pathops/SkDCubicIntersection.cpp',
    'src/pathops/SkDCubicLineIntersection.cpp',
    'src/pathops/SkDCubicToQuads.cpp',
    'src/pathops/SkDLineIntersection.cpp',
    'src/pathops/SkDQuadImplicit.cpp',
    'src/pathops/SkDQuadIntersection.cpp',
    'src/pathops/SkDQuadLineIntersection.cpp',
    'src/pathops/SkIntersections.cpp',
    'src/pathops/SkOpAngle.cpp',
    'src/pathops/SkOpContour.cpp',
    'src/pathops/SkOpEdgeBuilder.cpp',
    'src/pathops/SkOpSegment.cpp',
    'src/pathops/SkPathOpsBounds.cpp',
    'src/pathops/SkPathOpsCommon.cpp',
    'src/pathops/SkPathOpsCubic.cpp',
    'src/pathops/SkPathOpsDebug.cpp',
    'src/pathops/SkPathOpsLine.cpp',
    'src/pathops/SkPathOpsOp.cpp',
    'src/pathops/SkPathOpsPoint.cpp',
    'src/pathops/SkPathOpsQuad.cpp',
    'src/pathops/SkPathOpsRect.cpp',
    'src/pathops/SkPathOpsSimplify.cpp',
    'src/pathops/SkPathOpsTriangle.cpp',
    'src/pathops/SkPathOpsTypes.cpp',
    'src/pathops/SkPathWriter.cpp',
    'src/pathops/SkQuarticRoot.cpp',
    'src/pathops/SkReduceOrder.cpp',
    'src/pathops/SkAddIntersections.h',
    'src/pathops/SkDQuadImplicit.h',
    'src/pathops/SkIntersectionHelper.h',
    'src/pathops/SkIntersections.h',
    'src/pathops/SkLineParameters.h',
    'src/pathops/SkOpAngle.h',
    'src/pathops/SkOpContour.h',
    'src/pathops/SkOpEdgeBuilder.h',
    'src/pathops/SkOpSegment.h',
    'src/pathops/SkOpSpan.h',
    'src/pathops/SkPathOpsBounds.h',
    'src/pathops/SkPathOpsCommon.h',
    'src/pathops/SkPathOpsCubic.h',
    'src/pathops/SkPathOpsCurve.h',
    'src/pathops/SkPathOpsDebug.h',
    'src/pathops/SkPathOpsLine.h',
    'src/pathops/SkPathOpsPoint.h',
    'src/pathops/SkPathOpsQuad.h',
    'src/pathops/SkPathOpsRect.h',
    'src/pathops/SkPathOpsTriangle.h',
    'src/pathops/SkPathOpsTypes.h',
    'src/pathops/SkPathWriter.h',
    'src/pathops/SkQuarticRoot.h',
    'src/pathops/SkReduceOrder.h',
  ],

  'skia_effects_sources': [
    'src/effects/Sk1DPathEffect.cpp',
    'src/effects/Sk2DPathEffect.cpp',
    'src/effects/SkArithmeticMode.cpp',
    'src/effects/SkAvoidXfermode.cpp',
    'src/effects/SkBicubicImageFilter.cpp',
    'src/effects/SkBitmapSource.cpp',
    'src/effects/SkBlurDrawLooper.cpp',
    'src/effects/SkBlurMask.cpp',
    'src/effects/SkBlurMask.h',
    'src/effects/SkBlurImageFilter.cpp',
    'src/effects/SkBlurMaskFilter.cpp',
    'src/effects/SkColorFilters.cpp',
    'src/effects/SkColorFilterImageFilter.cpp',
    'src/effects/SkColorMatrix.cpp',
    'src/effects/SkColorMatrixFilter.cpp',
    'src/effects/SkComposeImageFilter.cpp',
    'src/effects/SkCornerPathEffect.cpp',
    'src/effects/SkDashPathEffect.cpp',
    'src/effects/SkDiscretePathEffect.cpp',
    'src/effects/SkDisplacementMapEffect.cpp',
    'src/effects/SkDropShadowImageFilter.cpp',
    'src/effects/SkEmbossMask.cpp',
    'src/effects/SkEmbossMask.h',
    'src/effects/SkEmbossMask_Table.h',
    'src/effects/SkEmbossMaskFilter.cpp',
    'src/effects/SkGpuBlurUtils.h',
    'src/effects/SkGpuBlurUtils.cpp',
    'src/effects/SkKernel33MaskFilter.cpp',
    'src/effects/SkLayerDrawLooper.cpp',
    'src/effects/SkLayerRasterizer.cpp',
    'src/effects/SkLerpXfermode.cpp',
    'src/effects/SkLightingImageFilter.cpp',
    'src/effects/SkLumaColorFilter.cpp',
    'src/effects/SkMagnifierImageFilter.cpp',
    'src/effects/SkMatrixConvolutionImageFilter.cpp',
    'src/effects/SkMatrixImageFilter.cpp',
    'src/effects/SkMergeImageFilter.cpp',
    'src/effects/SkMorphologyImageFilter.cpp',
    'src/effects/SkOffsetImageFilter.cpp',
    'src/effects/SkPaintFlagsDrawFilter.cpp',
    'src/effects/SkPerlinNoiseShader.cpp',
    'src/effects/SkPictureImageFilter.cpp',
    'src/effects/SkPixelXorXfermode.cpp',
    'src/effects/SkPorterDuff.cpp',
    'src/effects/SkRectShaderImageFilter.cpp',
    'src/effects/SkStippleMaskFilter.cpp',
    'src/effects/SkTableColorFilter.cpp',
    'src/effects/SkTableMaskFilter.cpp',
    'src/effects/SkTestImageFilters.cpp',
    'src/effects/SkTileImageFilter.cpp',
    'src/effects/SkTransparentShader.cpp',
    'src/effects/SkXfermodeImageFilter.cpp',

    'src/effects/gradients/SkBitmapCache.cpp',
    'src/effects/gradients/SkBitmapCache.h',
    'src/effects/gradients/SkClampRange.cpp',
    'src/effects/gradients/SkClampRange.h',
    'src/effects/gradients/SkRadialGradient_Table.h',
    'src/effects/gradients/SkGradientShader.cpp',
    'src/effects/gradients/SkGradientShaderPriv.h',
    'src/effects/gradients/SkLinearGradient.cpp',
    'src/effects/gradients/SkLinearGradient.h',
    'src/effects/gradients/SkRadialGradient.cpp',
    'src/effects/gradients/SkRadialGradient.h',
    'src/effects/gradients/SkTwoPointRadialGradient.cpp',
    'src/effects/gradients/SkTwoPointRadialGradient.h',
    'src/effects/gradients/SkTwoPointConicalGradient.cpp',
    'src/effects/gradients/SkTwoPointConicalGradient.h',
    'src/effects/gradients/SkTwoPointConicalGradient_gpu.cpp',
    'src/effects/gradients/SkTwoPointConicalGradient_gpu.h',
    'src/effects/gradients/SkSweepGradient.cpp',
    'src/effects/gradients/SkSweepGradient.h',

    'include/effects/Sk1DPathEffect.h',
    'include/effects/Sk2DPathEffect.h',
    'include/effects/SkXfermodeImageFilter.h',
    'include/effects/SkArithmeticMode.h',
    'include/effects/SkAvoidXfermode.h',
    'include/effects/SkBitmapSource.h',
    'include/effects/SkBlurDrawLooper.h',
    'include/effects/SkBlurImageFilter.h',
    'include/effects/SkBlurMaskFilter.h',
    'include/effects/SkColorMatrix.h',
    'include/effects/SkColorMatrixFilter.h',
    'include/effects/SkColorFilterImageFilter.h',
    'include/effects/SkCornerPathEffect.h',
    'include/effects/SkDashPathEffect.h',
    'include/effects/SkDiscretePathEffect.h',
    'include/effects/SkDisplacementMapEffect.h',
    'include/effects/SkDrawExtraPathEffect.h',
    'include/effects/SkDropShadowImageFilter.h',
    'include/effects/SkEmbossMaskFilter.h',
    'include/effects/SkGradientShader.h',
    'include/effects/SkKernel33MaskFilter.h',
    'include/effects/SkLayerDrawLooper.h',
    'include/effects/SkLayerRasterizer.h',
    'include/effects/SkLerpXfermode.h',
    'include/effects/SkLightingImageFilter.h',
    'include/effects/SkOffsetImageFilter.h',
    'include/effects/SkMorphologyImageFilter.h',
    'include/effects/SkPaintFlagsDrawFilter.h',
    'include/effects/SkPerlinNoiseShader.h',
    'include/effects/SkPixelXorXfermode.h',
    'include/effects/SkPorterDuff.h',
    'include/effects/SkRectShaderImageFilter.h',
    'include/effects/SkStippleMaskFilter.h',
    'include/effects/SkTableColorFilter.h',
    'include/effects/SkTableMaskFilter.h',
    'include/effects/SkTileImageFilter.h',
    'include/effects/SkTransparentShader.h',
    'include/effects/SkMagnifierImageFilter.h',
  ],

  'skia_pdf_sources': [
    'include/pdf/SkPDFDevice.h',
    'include/pdf/SkPDFDocument.h',

    'src/pdf/SkPDFCatalog.cpp',
    'src/pdf/SkPDFCatalog.h',
    'src/pdf/SkPDFDevice.cpp',
    'src/pdf/SkPDFDocument.cpp',
    'src/pdf/SkPDFFont.cpp',
    'src/pdf/SkPDFFont.h',
    'src/pdf/SkPDFFontImpl.h',
    'src/pdf/SkPDFFormXObject.cpp',
    'src/pdf/SkPDFFormXObject.h',
    'src/pdf/SkPDFGraphicState.cpp',
    'src/pdf/SkPDFGraphicState.h',
    'src/pdf/SkPDFImage.cpp',
    'src/pdf/SkPDFImage.h',
    'src/pdf/SkPDFPage.cpp',
    'src/pdf/SkPDFPage.h',
    'src/pdf/SkPDFResourceDict.cpp',
    'src/pdf/SkPDFResourceDict.h',
    'src/pdf/SkPDFShader.cpp',
    'src/pdf/SkPDFShader.h',
    'src/pdf/SkPDFStream.cpp',
    'src/pdf/SkPDFStream.h',
    'src/pdf/SkPDFTypes.cpp',
    'src/pdf/SkPDFTypes.h',
    'src/pdf/SkPDFUtils.cpp',
    'src/pdf/SkPDFUtils.h',
    'src/pdf/SkTSet.h',
  ],

  'skia_library_sources': [
    'src/core/SkFlate.cpp',  # this should likely be moved into src/utils in skia

    'include/images/SkImageRef_GlobalPool.h',
    'include/images/SkImageRef.h',
    'include/images/SkMovie.h',
    'include/images/SkPageFlipper.h',
    'include/ports/SkTypeface_win.h',
    'include/utils/mac/SkCGUtils.h',
    'include/utils/SkDeferredCanvas.h',
    'include/utils/SkEventTracer.h',
    'include/utils/SkMatrix44.h',
    'include/utils/SkNullCanvas.h',
    'include/utils/SkNWayCanvas.h',
    'include/utils/SkPictureUtils.h',
    'include/utils/SkProxyCanvas.h',
    'include/utils/SkRTConf.h',
    'src/fonts/SkFontMgr_fontconfig.cpp',
    'src/images/SkScaledBitmapSampler.cpp',
    'src/images/SkScaledBitmapSampler.h',
    'src/ports/SkFontConfigInterface_android.cpp',
    'src/ports/SkFontConfigInterface_direct.cpp',
    'src/ports/SkFontConfigParser_android.cpp',
    'src/ports/SkFontHost_fontconfig.cpp',
    'src/ports/SkFontHost_FreeType_common.cpp',
    'src/ports/SkFontHost_FreeType_common.h',
    'src/ports/SkFontHost_FreeType.cpp',
    'src/ports/SkFontHost_linux.cpp',
    'src/ports/SkFontHost_mac.cpp',
    'src/ports/SkFontHost_win.cpp',
    'src/ports/SkFontHost_win_dw.cpp',
    'src/ports/SkFontMgr_default_gdi.cpp',
    'src/ports/SkGlobalInitialization_chromium.cpp',
    'src/ports/SkImageDecoder_empty.cpp',
    'src/ports/SkOSFile_posix.cpp',
    'src/ports/SkOSFile_stdio.cpp',
    'src/ports/SkOSFile_win.cpp',
    'src/ports/SkThread_win.cpp',
    'src/ports/SkTime_Unix.cpp',
    'src/ports/SkTLS_pthread.cpp',
    'src/ports/SkTLS_win.cpp',
    'src/sfnt/SkOTTable_name.cpp',
    'src/sfnt/SkOTTable_name.h',
    'src/sfnt/SkOTUtils.cpp',
    'src/sfnt/SkOTUtils.h',
    'src/utils/debugger/SkDebugCanvas.cpp',
    'src/utils/debugger/SkDebugCanvas.h',
    'src/utils/debugger/SkDrawCommand.cpp',
    'src/utils/debugger/SkDrawCommand.h',
    'src/utils/debugger/SkObjectParser.cpp',
    'src/utils/debugger/SkObjectParser.h',
    'src/utils/mac/SkCreateCGImageRef.cpp',
    'src/utils/SkBase64.cpp',
    'src/utils/SkBase64.h',
    'src/utils/SkBitmapHasher.cpp',
    'src/utils/SkBitmapHasher.h',
    'src/utils/SkBitSet.cpp',
    'src/utils/SkBitSet.h',
    'src/utils/SkBoundaryPatch.cpp',
    'src/utils/SkFrontBufferedStream.cpp',
    'src/utils/SkCamera.cpp',
    'src/utils/SkCanvasStack.h',
    'src/utils/SkCanvasStack.cpp',
    'src/utils/SkCanvasStateUtils.cpp',
    'src/utils/SkCubicInterval.cpp',
    'src/utils/SkCullPoints.cpp',
    'src/utils/SkDeferredCanvas.cpp',
    'src/utils/SkDumpCanvas.cpp',
    'src/utils/SkEventTracer.cpp',
    'src/utils/SkFloatUtils.h',
    'src/utils/SkGatherPixelRefsAndRects.cpp',
    'src/utils/SkGatherPixelRefsAndRects.h',
    'src/utils/SkInterpolator.cpp',
    'src/utils/SkLayer.cpp',
    'src/utils/SkMatrix44.cpp',
    'src/utils/SkMD5.cpp',
    'src/utils/SkMD5.h',
    'src/utils/SkMeshUtils.cpp',
    'src/utils/SkNinePatch.cpp',
    'src/utils/SkNWayCanvas.cpp',
    'src/utils/SkNullCanvas.cpp',
    'src/utils/SkOSFile.cpp',
    'src/utils/SkParse.cpp',
    'src/utils/SkParseColor.cpp',
    'src/utils/SkParsePath.cpp',
    'src/utils/SkPictureUtils.cpp',
    'src/utils/SkPathUtils.cpp',
    'src/utils/SkProxyCanvas.cpp',
    'src/utils/SkSHA1.cpp',
    'src/utils/SkSHA1.h',
    'src/utils/SkRTConf.cpp',
    'src/utils/SkThreadUtils.h',
    'src/utils/SkThreadUtils_pthread.cpp',
    'src/utils/SkThreadUtils_pthread.h',
    'src/utils/SkThreadUtils_pthread_linux.cpp',
    'src/utils/SkThreadUtils_pthread_mach.cpp',
    'src/utils/SkThreadUtils_pthread_other.cpp',
    'src/utils/SkThreadUtils_win.cpp',
    'src/utils/SkThreadUtils_win.h',
    'src/utils/SkTFitsIn.h',
    'src/utils/SkTLogic.h',
    'src/utils/SkUnitMappers.cpp',

    #mac
    'include/utils/mac/SkCGUtils.h',
    'src/utils/mac/SkCreateCGImageRef.cpp',

    #windows
    'include/utils/win/SkAutoCoInitialize.h',
    'include/utils/win/SkHRESULT.h',
    'include/utils/win/SkIStream.h',
    'include/utils/win/SkTScopedComPtr.h',
    'src/utils/win/SkAutoCoInitialize.cpp',
    'src/utils/win/SkDWrite.h',
    'src/utils/win/SkDWrite.cpp',
    'src/utils/win/SkDWriteFontFileStream.cpp',
    'src/utils/win/SkDWriteFontFileStream.h',
    'src/utils/win/SkDWriteGeometrySink.cpp',
    'src/utils/win/SkDWriteGeometrySink.h',
    'src/utils/win/SkHRESULT.cpp',
    'src/utils/win/SkIStream.cpp',
    'src/utils/win/SkWGL_win.cpp',

    #testing
    'src/fonts/SkGScalerContext.cpp',
    'src/fonts/SkGScalerContext.h',
  ],
}
