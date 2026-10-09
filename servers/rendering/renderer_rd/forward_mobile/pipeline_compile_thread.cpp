/**************************************************************************/
/*  pipeline_compile_thread.cpp                                           */
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
/**************************************************************************/

#include "scene_shader_forward_mobile.h"

#include "core/object/worker_thread_pool.h"
#include "core/profiling/profiling.h"

using namespace RendererSceneRenderImplementation;

bool SceneShaderForwardMobile::ShaderData::_enqueue_pipeline(const PipelineKey &p_pipeline_key, uint32_t p_hash, RS::PipelineSource p_source) {
	PipelineCompileThread *compile_thread = SceneShaderForwardMobile::singleton->pipeline_compile_thread;
	// Pool threads (resource loaders) keep compiling inline: they publish before anyone looks for the pipeline.
	if (compile_thread == nullptr || WorkerThreadPool::get_singleton()->get_thread_index() != -1) {
		return false;
	}

	PipelineCreateParams params;
	if (!_build_pipeline_params(p_pipeline_key, params)) {
		return false;
	}

	bool high_priority = p_source == RS::PIPELINE_SOURCE_DRAW || p_source == RS::PIPELINE_SOURCE_SPECIALIZATION;
	compile_thread->enqueue(this, params, p_hash, high_priority);
	return true;
}

void SceneShaderForwardMobile::ShaderData::_promote_pipeline(uint32_t p_hash) {
	PipelineCompileThread *compile_thread = SceneShaderForwardMobile::singleton->pipeline_compile_thread;
	if (compile_thread != nullptr) {
		compile_thread->promote(this, p_hash);
	}
}

void SceneShaderForwardMobile::ShaderData::_clear_pipelines() {
	PipelineCompileThread *compile_thread = SceneShaderForwardMobile::singleton->pipeline_compile_thread;
	if (compile_thread != nullptr) {
		compile_thread->unregister_owner(this);
	}
	pipeline_hash_map.clear_pipelines();
}

void SceneShaderForwardMobile::ShaderData::_free_version() {
	if (version.is_null()) {
		return;
	}

	PipelineCompileThread *compile_thread = SceneShaderForwardMobile::singleton->pipeline_compile_thread;
	if (compile_thread != nullptr) {
		compile_thread->unregister_owner(this);
	}

	if (compile_thread == nullptr || !compile_thread->retire_version_if_busy(version)) {
		MutexLock lock(SceneShaderForwardMobile::singleton_mutex);
		SceneShaderForwardMobile::singleton->shader.version_free(version);
	}
	version = RID();
}

void SceneShaderForwardMobile::PipelineCompileThread::enqueue(ShaderData *p_owner, const ShaderData::PipelineCreateParams &p_params, uint32_t p_hash, bool p_high_priority) {
	Job job;
	job.hash = p_hash;
	job.version = p_owner->version;
	job.params = p_params;
	job.profile_context = SceneShaderForwardMobile::pipeline_profile_context;
	job.shader_name = p_owner->path.is_empty() ? String("<code>") : p_owner->path;

	{
		MutexLock lock(mutex);
		if (p_owner->async_owner_token == 0) {
			last_token = last_token == UINT32_MAX ? 1 : last_token + 1;
			p_owner->async_owner_token = last_token;
			owners.insert(last_token, p_owner);
		}
		job.owner_token = p_owner->async_owner_token;

		if (p_high_priority) {
			high_queue.push_back(job);
		} else {
			low_index.insert(_low_key(job.owner_token, p_hash), low_queue.push_back(job));
		}
	}

	semaphore.post();
}

void SceneShaderForwardMobile::PipelineCompileThread::promote(ShaderData *p_owner, uint32_t p_hash) {
	MutexLock lock(mutex);
	if (p_owner->async_owner_token == 0) {
		return;
	}

	HashMap<uint64_t, List<Job>::Element *>::Iterator it = low_index.find(_low_key(p_owner->async_owner_token, p_hash));
	if (it == low_index.end()) {
		return;
	}

	high_queue.push_back(it->value->get());
	low_queue.erase(it->value);
	low_index.remove(it);
}

void SceneShaderForwardMobile::PipelineCompileThread::_cancel_jobs_locked(uint32_t p_token) {
	for (List<Job>::Element *E = high_queue.front(); E;) {
		List<Job>::Element *next = E->next();
		if (E->get().owner_token == p_token) {
			high_queue.erase(E);
		}
		E = next;
	}

	for (List<Job>::Element *E = low_queue.front(); E;) {
		List<Job>::Element *next = E->next();
		if (E->get().owner_token == p_token) {
			low_index.erase(_low_key(p_token, E->get().hash));
			low_queue.erase(E);
		}
		E = next;
	}
}

void SceneShaderForwardMobile::PipelineCompileThread::unregister_owner(ShaderData *p_owner) {
	MutexLock lock(mutex);
	if (p_owner->async_owner_token == 0) {
		return;
	}

	// A job already running for this owner finishes; its result goes to pipelines_to_free.
	_cancel_jobs_locked(p_owner->async_owner_token);
	owners.erase(p_owner->async_owner_token);
	p_owner->async_owner_token = 0;
}

bool SceneShaderForwardMobile::PipelineCompileThread::retire_version_if_busy(RID p_version) {
	MutexLock lock(mutex);
	if (running_version != p_version) {
		return false;
	}

	version_graveyard.push_back(p_version);
	return true;
}

void SceneShaderForwardMobile::PipelineCompileThread::drain_deferred_frees() {
	LocalVector<RID> versions;
	LocalVector<RID> pipelines;
	{
		MutexLock lock(mutex);
		for (uint32_t i = 0; i < version_graveyard.size();) {
			if (version_graveyard[i] != running_version) {
				versions.push_back(version_graveyard[i]);
				version_graveyard.remove_at_unordered(i);
			} else {
				i++;
			}
		}
		pipelines = pipelines_to_free;
		pipelines_to_free.clear();
	}

	for (const RID &pipeline : pipelines) {
		RD::get_singleton()->free_rid(pipeline);
	}

	if (!versions.is_empty()) {
		MutexLock lock(SceneShaderForwardMobile::singleton_mutex);
		for (const RID &version : versions) {
			SceneShaderForwardMobile::singleton->shader.version_free(version);
		}
	}
}

void SceneShaderForwardMobile::PipelineCompileThread::_thread_func(void *p_self) {
	Thread::set_name("PipelineCompileThread");
	static_cast<PipelineCompileThread *>(p_self)->_run();
}

void SceneShaderForwardMobile::PipelineCompileThread::_run() {
	while (true) {
		semaphore.wait();

		Job job;
		{
			MutexLock lock(mutex);
			if (exit) {
				break;
			}

			if (!high_queue.is_empty()) {
				job = high_queue.front()->get();
				high_queue.pop_front();
			} else if (!low_queue.is_empty()) {
				job = low_queue.front()->get();
				low_index.erase(_low_key(job.owner_token, job.hash));
				low_queue.pop_front();
			} else {
				continue; // Posted for a job that was cancelled.
			}
			running_version = job.version;
		}

		RID pipeline;
		{
			GodotProfileZoneStr("PipelineCompileThread::compile",
					vformat("shader=%s version=%d ubershader=%d src=%d mesh=%d surface=%d material=%d",
							job.shader_name, (int)job.params.key.version, (int)job.params.key.ubershader,
							job.profile_context.source, job.profile_context.mesh_rid, job.profile_context.surface_index,
							job.profile_context.material_rid));
			const ShaderData::PipelineCreateParams &p = job.params;
			pipeline = RD::get_singleton()->render_pipeline_create(p.shader, p.key.framebuffer_format_id, p.key.vertex_format_id, p.primitive, p.raster_state, p.multisample_state, p.depth_stencil_state, p.blend_state, 0, p.key.render_pass, p.specialization_constants);
		}

		MutexLock lock(mutex);
		running_version = RID();
		HashMap<uint32_t, ShaderData *>::Iterator owner = owners.find(job.owner_token);
		if (owner != owners.end()) {
			owner->value->pipeline_hash_map.async_pipeline_finished(job.hash, pipeline);
		} else if (pipeline.is_valid()) {
			pipelines_to_free.push_back(pipeline);
		}
	}
}

SceneShaderForwardMobile::PipelineCompileThread::PipelineCompileThread() {
	thread.start(_thread_func, this);
}

SceneShaderForwardMobile::PipelineCompileThread::~PipelineCompileThread() {
	{
		MutexLock lock(mutex);
		exit = true;
	}
	semaphore.post();
	thread.wait_to_finish();

	for (KeyValue<uint32_t, ShaderData *> &E : owners) {
		E.value->async_owner_token = 0;
	}
	owners.clear();
	high_queue.clear();
	low_queue.clear();
	low_index.clear();
	drain_deferred_frees();
}

void SceneShaderForwardMobile::set_async_pipeline_compilation(bool p_enabled) {
#ifdef THREADS_ENABLED
	if (p_enabled && pipeline_compile_thread == nullptr) {
		pipeline_compile_thread = memnew(PipelineCompileThread);
	} else if (!p_enabled && pipeline_compile_thread != nullptr) {
		PipelineCompileThread *compile_thread = pipeline_compile_thread;
		pipeline_compile_thread = nullptr;
		memdelete(compile_thread);
	}
#endif
}

void SceneShaderForwardMobile::drain_async_pipeline_frees() {
	if (pipeline_compile_thread != nullptr) {
		pipeline_compile_thread->drain_deferred_frees();
	}
}
