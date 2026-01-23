/**************************************************************************/
/*  test_gltf_png_gamma.h                                                 */
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

#pragma once

#include "test_gltf.h"

#ifdef TOOLS_ENABLED

#include "editor/file_system/editor_file_system.h"
#include "scene/resources/image_texture.h"

namespace TestGltf {

// Per glTF 2.0 specification:
// "Any colorspace information (such as ICC profiles, intents, gamma values, etc.)
// from PNG or JPEG images MUST be ignored."
//
// This test verifies that PNG gAMA chunks are ignored during glTF import.
// Two PNGs with identical pixel values (0, 136, 0) but different gAMA chunks
// should result in identical textures after import.

TEST_CASE("[SceneTree][Node][Editor] glTF PNG gamma chunk is ignored per spec") {
	init("gltf_png_gamma_test", "res://");

	EditorFileSystem *efs = memnew(EditorFileSystem);
	EditorResourcePreview *erp = memnew(EditorResourcePreview);

	ERR_PRINT_OFF
	Node *loaded = gltf_import("res://gamma_test.gltf");
	ERR_PRINT_ON

	CHECK_MESSAGE(loaded != nullptr, "glTF import failed.");

	// Find both mesh instances
	MeshInstance3D *mesh_no_gamma = Object::cast_to<MeshInstance3D>(loaded->find_child("Cube_NoGamma", true, true));
	MeshInstance3D *mesh_with_gamma = Object::cast_to<MeshInstance3D>(loaded->find_child("Cube_WithGamma", true, true));

	CHECK_MESSAGE(mesh_no_gamma != nullptr, "Cube_NoGamma mesh not found.");
	CHECK_MESSAGE(mesh_with_gamma != nullptr, "Cube_WithGamma mesh not found.");

	// Get materials from both meshes
	Ref<StandardMaterial3D> material_no_gamma = mesh_no_gamma->get_active_material(0);
	Ref<StandardMaterial3D> material_with_gamma = mesh_with_gamma->get_active_material(0);

	CHECK_MESSAGE(material_no_gamma.is_valid(), "Material_NoGamma not found.");
	CHECK_MESSAGE(material_with_gamma.is_valid(), "Material_WithGamma not found.");

	// Get textures from both materials
	Ref<Texture2D> texture_no_gamma = material_no_gamma->get_texture(StandardMaterial3D::TextureParam::TEXTURE_ALBEDO);
	Ref<Texture2D> texture_with_gamma = material_with_gamma->get_texture(StandardMaterial3D::TextureParam::TEXTURE_ALBEDO);

	CHECK_MESSAGE(texture_no_gamma.is_valid(), "Texture without gamma not found.");
	CHECK_MESSAGE(texture_with_gamma.is_valid(), "Texture with gamma not found.");

	// Get pixel values from both textures
	Ref<Image> image_no_gamma = texture_no_gamma->get_image();
	Ref<Image> image_with_gamma = texture_with_gamma->get_image();

	CHECK_MESSAGE(image_no_gamma.is_valid(), "Image without gamma is null.");
	CHECK_MESSAGE(image_with_gamma.is_valid(), "Image with gamma is null.");

	Color pixel_no_gamma = image_no_gamma->get_pixel(0, 0);
	Color pixel_with_gamma = image_with_gamma->get_pixel(0, 0);

	// Both pixels should be identical (0, 136/255, 0) = approximately (0, 0.533, 0)
	// The expected value is 136/255 = 0.5333...
	// If gamma is applied incorrectly, pixel_with_gamma will be different (darker).
	float expected_green = 136.0f / 255.0f;

	// Check that both images have the expected green value (within floating point tolerance)
	CHECK_MESSAGE(Math::is_equal_approx(pixel_no_gamma.r, 0.0f), "Pixel without gamma: red should be 0.");
	CHECK_MESSAGE(Math::is_equal_approx(pixel_no_gamma.g, expected_green, 0.01f), "Pixel without gamma: green should be ~0.533.");
	CHECK_MESSAGE(Math::is_equal_approx(pixel_no_gamma.b, 0.0f), "Pixel without gamma: blue should be 0.");

	CHECK_MESSAGE(Math::is_equal_approx(pixel_with_gamma.r, 0.0f), "Pixel with gamma: red should be 0.");
	CHECK_MESSAGE(Math::is_equal_approx(pixel_with_gamma.g, expected_green, 0.01f), "Pixel with gamma: green should be ~0.533 (gamma MUST be ignored per glTF spec).");
	CHECK_MESSAGE(Math::is_equal_approx(pixel_with_gamma.b, 0.0f), "Pixel with gamma: blue should be 0.");

	// The key assertion: both pixels must be identical
	// This will FAIL before the fix is applied because Godot applies the PNG gamma
	CHECK_MESSAGE(pixel_no_gamma == pixel_with_gamma, "Both textures must have identical pixels (PNG gamma MUST be ignored per glTF 2.0 spec).");

	memdelete(loaded);
	memdelete(erp);
	memdelete(efs);
}

} // namespace TestGltf

#endif // TOOLS_ENABLED
