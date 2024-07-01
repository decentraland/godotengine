/**************************************************************************/
/*  portable_compressed_texture.cpp                                       */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "portable_compressed_texture.h"

#include "core/io/marshalls.h"
#include "scene/resources/bit_map.h"

void PortableCompressedTexture2D::_set_data(const Vector<uint8_t> &p_data) {
	return;
}

PortableCompressedTexture2D::CompressionMode PortableCompressedTexture2D::get_compression_mode() const {
	return compression_mode;
}
Vector<uint8_t> PortableCompressedTexture2D::_get_data() const {
	return compressed_buffer;
}

void PortableCompressedTexture2D::create_from_image(const Ref<Image> &p_image, CompressionMode p_compression_mode, bool p_normal_map, float p_lossy_quality) {
	ERR_FAIL_COND(p_image.is_null() || p_image->is_empty());

	ERR_FAIL_COND_MSG(!p_image->is_compressed(), "Image is not compressed");

	//Ref<Image> copy = Image::create_from_data(p_image->get_width(), p_image->get_height(), false, p_image->get_format(), p_image->get_data());

	//ERR_FAIL_COND_MSG(p_image.is_null(), "ERR_INVALID_PARAMETER");

	size = p_image->get_size();

	if (texture.is_valid()) {
		RID new_texture = RS::get_singleton()->texture_2d_create(p_image);
		RS::get_singleton()->texture_replace(texture, new_texture);
	} else {
		texture = RS::get_singleton()->texture_2d_create(p_image);
	}

	if (size.width || size.height) {
		RenderingServer::get_singleton()->texture_set_size_override(texture, size.width, size.height);
	}

	format = p_image->get_format();

	image_stored = true;
	alpha_cache.unref();

	notify_property_list_changed();
	emit_changed();
}

Image::Format PortableCompressedTexture2D::get_format() const {
	return format;
}

Ref<Image> PortableCompressedTexture2D::get_image() const {
	if (image_stored) {
		return RenderingServer::get_singleton()->texture_2d_get(texture);
	} else {
		return Ref<Image>();
	}
}

int PortableCompressedTexture2D::get_width() const {
	return size.width;
}

int PortableCompressedTexture2D::get_height() const {
	return size.height;
}

RID PortableCompressedTexture2D::get_rid() const {
	if (!texture.is_valid()) {
		texture = RS::get_singleton()->texture_2d_placeholder_create();
	}
	return texture;
}

bool PortableCompressedTexture2D::has_alpha() const {
	return false;
}

void PortableCompressedTexture2D::draw(RID p_canvas_item, const Point2 &p_pos, const Color &p_modulate, bool p_transpose) const {
	if (size.width == 0 || size.height == 0) {
		return;
	}
	RenderingServer::get_singleton()->canvas_item_add_texture_rect(p_canvas_item, Rect2(p_pos, size), texture, false, p_modulate, p_transpose);
}

void PortableCompressedTexture2D::draw_rect(RID p_canvas_item, const Rect2 &p_rect, bool p_tile, const Color &p_modulate, bool p_transpose) const {
	if (size.width == 0 || size.height == 0) {
		return;
	}
	RenderingServer::get_singleton()->canvas_item_add_texture_rect(p_canvas_item, p_rect, texture, p_tile, p_modulate, p_transpose);
}

void PortableCompressedTexture2D::draw_rect_region(RID p_canvas_item, const Rect2 &p_rect, const Rect2 &p_src_rect, const Color &p_modulate, bool p_transpose, bool p_clip_uv) const {
	if (size.width == 0 || size.height == 0) {
		return;
	}
	RenderingServer::get_singleton()->canvas_item_add_texture_rect_region(p_canvas_item, p_rect, texture, p_src_rect, p_modulate, p_transpose, p_clip_uv);
}

bool PortableCompressedTexture2D::is_pixel_opaque(int p_x, int p_y) const {
	if (!alpha_cache.is_valid()) {
		Ref<Image> img = get_image();
		if (img.is_valid()) {
			if (img->is_compressed()) { //must decompress, if compressed
				Ref<Image> decom = img->duplicate();
				decom->decompress();
				img = decom;
			}
			alpha_cache.instantiate();
			alpha_cache->create_from_image_alpha(img);
		}
	}

	if (alpha_cache.is_valid()) {
		int aw = int(alpha_cache->get_size().width);
		int ah = int(alpha_cache->get_size().height);
		if (aw == 0 || ah == 0) {
			return true;
		}

		int x = p_x * aw / size.width;
		int y = p_y * ah / size.height;

		x = CLAMP(x, 0, aw);
		y = CLAMP(y, 0, ah);

		return alpha_cache->get_bit(x, y);
	}

	return true;
}

void PortableCompressedTexture2D::set_size_override(const Size2 &p_size) {
	size = p_size;
	RenderingServer::get_singleton()->texture_set_size_override(texture, size.width, size.height);
}

Size2 PortableCompressedTexture2D::get_size_override() const {
	return size;
}

void PortableCompressedTexture2D::set_path(const String &p_path, bool p_take_over) {
	if (texture.is_valid()) {
		RenderingServer::get_singleton()->texture_set_path(texture, p_path);
	}

	Resource::set_path(p_path, p_take_over);
}

bool PortableCompressedTexture2D::keep_all_compressed_buffers = false;

void PortableCompressedTexture2D::set_keep_all_compressed_buffers(bool p_keep) {
	keep_all_compressed_buffers = p_keep;
}

bool PortableCompressedTexture2D::is_keeping_all_compressed_buffers() {
	return keep_all_compressed_buffers;
}

void PortableCompressedTexture2D::set_keep_compressed_buffer(bool p_keep) {
	keep_compressed_buffer = p_keep;
	if (!p_keep) {
		compressed_buffer.clear();
	}
}

bool PortableCompressedTexture2D::is_keeping_compressed_buffer() const {
	return keep_compressed_buffer;
}

void PortableCompressedTexture2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("create_from_image", "image", "compression_mode", "normal_map", "lossy_quality"), &PortableCompressedTexture2D::create_from_image, DEFVAL(false), DEFVAL(0.8));
	ClassDB::bind_method(D_METHOD("get_format"), &PortableCompressedTexture2D::get_format);
	ClassDB::bind_method(D_METHOD("get_compression_mode"), &PortableCompressedTexture2D::get_compression_mode);

	ClassDB::bind_method(D_METHOD("set_size_override", "size"), &PortableCompressedTexture2D::set_size_override);
	ClassDB::bind_method(D_METHOD("get_size_override"), &PortableCompressedTexture2D::get_size_override);

	ClassDB::bind_method(D_METHOD("set_keep_compressed_buffer", "keep"), &PortableCompressedTexture2D::set_keep_compressed_buffer);
	ClassDB::bind_method(D_METHOD("is_keeping_compressed_buffer"), &PortableCompressedTexture2D::is_keeping_compressed_buffer);

	ClassDB::bind_method(D_METHOD("_set_data", "data"), &PortableCompressedTexture2D::_set_data);
	ClassDB::bind_method(D_METHOD("_get_data"), &PortableCompressedTexture2D::_get_data);

	ClassDB::bind_static_method("PortableCompressedTexture2D", D_METHOD("set_keep_all_compressed_buffers", "keep"), &PortableCompressedTexture2D::set_keep_all_compressed_buffers);
	ClassDB::bind_static_method("PortableCompressedTexture2D", D_METHOD("is_keeping_all_compressed_buffers"), &PortableCompressedTexture2D::is_keeping_all_compressed_buffers);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "_set_data", "_get_data");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "size_override", PROPERTY_HINT_NONE, "suffix:px"), "set_size_override", "get_size_override");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "keep_compressed_buffer"), "set_keep_compressed_buffer", "is_keeping_compressed_buffer");

	BIND_ENUM_CONSTANT(COMPRESSION_MODE_LOSSLESS);
	BIND_ENUM_CONSTANT(COMPRESSION_MODE_LOSSY);
	BIND_ENUM_CONSTANT(COMPRESSION_MODE_BASIS_UNIVERSAL);
	BIND_ENUM_CONSTANT(COMPRESSION_MODE_S3TC);
	BIND_ENUM_CONSTANT(COMPRESSION_MODE_ETC2);
	BIND_ENUM_CONSTANT(COMPRESSION_MODE_BPTC);
}

PortableCompressedTexture2D::PortableCompressedTexture2D() {}

PortableCompressedTexture2D::~PortableCompressedTexture2D() {
	if (texture.is_valid()) {
		ERR_FAIL_NULL(RenderingServer::get_singleton());
		RenderingServer::get_singleton()->free(texture);
	}
}
