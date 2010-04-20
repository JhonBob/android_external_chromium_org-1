/**************************************************************************
 * 
 * Copyright 2005 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "main/glheader.h"
#include "main/context.h"
#include "main/enums.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/mtypes.h"

#include "swrast_setup/swrast_setup.h"
#include "swrast/swrast.h"
#include "tnl/tnl.h"
#include "brw_context.h"
#include "brw_fallback.h"
#include "intel_chipset.h"
#include "intel_fbo.h"
#include "intel_regions.h"

#include "glapi/glapi.h"

#define FILE_DEBUG_FLAG DEBUG_FALLBACKS

static GLboolean do_check_fallback(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   GLcontext *ctx = &brw->intel.ctx;
   GLuint i;

   if (brw->intel.no_rast) {
      DBG("FALLBACK: rasterization disabled\n");
      return GL_TRUE;
   }

   /* _NEW_RENDERMODE
    */
   if (ctx->RenderMode != GL_RENDER) {
      DBG("FALLBACK: render mode\n");
      return GL_TRUE;
   }

   /* _NEW_TEXTURE:
    */
   for (i = 0; i < BRW_MAX_TEX_UNIT; i++) {
      struct gl_texture_unit *texUnit = &ctx->Texture.Unit[i];
      if (texUnit->_ReallyEnabled) {
	 struct intel_texture_object *intelObj = intel_texture_object(texUnit->_Current);
	 struct gl_texture_image *texImage = intelObj->base.Image[0][intelObj->firstLevel];
	 if (texImage->Border) {
	    DBG("FALLBACK: texture border\n");
	    return GL_TRUE;
	 }
      }
   }
   
   /* _NEW_STENCIL 
    */
   if (ctx->Stencil._Enabled &&
       (ctx->DrawBuffer->Name == 0 && !brw->intel.hw_stencil)) {
      DBG("FALLBACK: stencil\n");
      return GL_TRUE;
   }

   /* _NEW_BUFFERS */
   if (IS_965(intel->intelScreen->deviceID) &&
       !IS_G4X(intel->intelScreen->deviceID)) {
      for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++) {
	 struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[i];
	 struct intel_renderbuffer *irb = intel_renderbuffer(rb);

	 /* The original gen4 hardware couldn't set up WM surfaces pointing
	  * at an offset within a tile, which can happen when rendering to
	  * anything but the base level of a texture or the +X face/0 depth.
	  * This was fixed with the 4 Series hardware.
	  *
	  * For these original chips, you would have to make the depth and
	  * color destination surfaces include information on the texture
	  * type, LOD, face, and various limits to use them as a destination.
	  * I would have done this, but there's also a nasty requirement that
	  * the depth and the color surfaces all be of the same LOD, which
	  * may be a worse requirement than this alignment.  (Also, we may
	  * want to just demote the texture to untiled, instead).
	  */
	 if (irb->region && irb->region->tiling != I915_TILING_NONE &&
	     (irb->region->draw_offset & 4095)) {
	    DBG("FALLBACK: non-tile-aligned destination for tiled FBO\n");
	    return GL_TRUE;
	 }
      }
   }

   return GL_FALSE;
}

static void check_fallback(struct brw_context *brw)
{
   brw->intel.Fallback = do_check_fallback(brw);
}

const struct brw_tracked_state brw_check_fallback = {
   .dirty = {
      .mesa = _NEW_BUFFERS | _NEW_RENDERMODE | _NEW_TEXTURE | _NEW_STENCIL,
      .brw  = 0,
      .cache = 0
   },
   .prepare = check_fallback
};




/**
 * Called by the INTEL_FALLBACK() macro.
 * NOTE: this is a no-op for the i965 driver.  The brw->intel.Fallback
 * field is treated as a boolean, not a bitmask.  It's only set in a
 * couple of places.
 */
void intelFallback( struct intel_context *intel, GLuint bit, GLboolean mode )
{
}



