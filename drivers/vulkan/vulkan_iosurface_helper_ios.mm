/**************************************************************************/
/*  vulkan_iosurface_helper_ios.mm                                        */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#ifdef IOS_EXTERNAL_TEXTURE_SUPPORT

#import <Metal/Metal.h>
#import <IOSurface/IOSurfaceRef.h>
#import <CoreVideo/CVPixelBuffer.h>

// Helper function to create a Metal texture from IOSurface with correct storage mode.
// Returns the MTLTexture as void* (caller must cast to id<MTLTexture>).
// The texture is retained - caller must release when done.
extern "C" void* godot_create_metal_texture_from_iosurface(void* p_mtl_device, void* p_iosurface) {
	if (!p_mtl_device || !p_iosurface) {
		return nullptr;
	}

	id<MTLDevice> device = (__bridge id<MTLDevice>)p_mtl_device;
	IOSurfaceRef surface = (IOSurfaceRef)p_iosurface;

	size_t width = IOSurfaceGetWidth(surface);
	size_t height = IOSurfaceGetHeight(surface);
	OSType pixelFormat = IOSurfaceGetPixelFormat(surface);

	// Determine Metal pixel format
	MTLPixelFormat metalFormat = MTLPixelFormatBGRA8Unorm;
	if (pixelFormat == 'BGRA' || pixelFormat == kCVPixelFormatType_32BGRA) {
		metalFormat = MTLPixelFormatBGRA8Unorm;
	} else if (pixelFormat == 'RGBA' || pixelFormat == kCVPixelFormatType_32RGBA) {
		metalFormat = MTLPixelFormatRGBA8Unorm;
	}

	// Create texture descriptor with MTLStorageModeShared (required for IOSurface)
	MTLTextureDescriptor *desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:metalFormat
																					width:width
																				   height:height
																				mipmapped:NO];
	desc.usage = MTLTextureUsageShaderRead;
	desc.storageMode = MTLStorageModeShared; // Required for IOSurface

	// Create texture from IOSurface
	id<MTLTexture> texture = [device newTextureWithDescriptor:desc iosurface:surface plane:0];
	if (!texture) {
		NSLog(@"[Vulkan IOSurface Helper] Failed to create Metal texture from IOSurface");
		return nullptr;
	}

	// Return retained texture
	return (__bridge_retained void*)texture;
}

// Release a Metal texture created by godot_create_metal_texture_from_iosurface
extern "C" void godot_release_metal_texture(void* p_texture) {
	if (p_texture) {
		id<MTLTexture> texture = (__bridge_transfer id<MTLTexture>)p_texture;
		(void)texture; // ARC will release it
	}
}

#endif // IOS_EXTERNAL_TEXTURE_SUPPORT
