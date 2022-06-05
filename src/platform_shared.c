/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

void platform_get_name_from_path(char *buffer, char *path)
{
	buffer[0] = 0;
	
	s32 len = strlen(path);
	if (len == 1)
	{
		return;
	}
	
	char *path_end = path + len;
#ifdef OS_LINUX
	while (*path_end != '/' && path_end >= path)
	{
		--path_end;
	}
#endif
#ifdef OS_WIN
	while (*path_end != '\\' && path_end >= path)
	{
		--path_end;
	}
#endif
	
	string_copyn(buffer, path_end+1, MAX_INPUT_LENGTH);
}

void platform_get_directory_from_path(char *buffer, char *path)
{
	buffer[0] = 0;
	
	s32 len = strlen(path);
	if (len == 1)
	{
		return;
	}
	
	char *path_end = path + len;
#ifdef OS_LINUX
	while (*path_end != '/' && path_end >= path)
	{
		--path_end;
	}
#endif
#ifdef OS_WIN
	while (*path_end != '\\' && path_end >= path)
	{
		--path_end;
	}
#endif
	
	s32 offset = path_end - path;
	char ch = path[offset+1];
	path[offset+1] = 0;
	string_copyn(buffer, path, MAX_INPUT_LENGTH);
	path[offset+1] = ch;
}

void platform_autocomplete_path(char *buffer, bool want_dir)
{
	char dir[MAX_INPUT_LENGTH]; 
	char name[MAX_INPUT_LENGTH]; 
	platform_get_directory_from_path(dir, buffer);
	platform_get_name_from_path(name, buffer);
	
	// nothing to autocomplete
	if (name[0] == 0)
	{
		return;
	}
	
	// create filter
	string_appendn(name, "*", MAX_INPUT_LENGTH);
	
	array files = array_create(sizeof(found_file));
	array filters = string_split(name);
	bool is_cancelled = false;
	platform_list_files_block(&files, dir, filters, false, 0, want_dir, &is_cancelled, 0);
	
	s32 index_to_take = -1;
	if (want_dir)
	{
		for (s32 i = 0; i < files.length; i++)
		{
			found_file *file = array_at(&files, i);
			
			if (platform_directory_exists(file->path))
			{
				index_to_take = i;
				break;
			}
		}
	}
	else
	{
		index_to_take = 0;
	}
	
	array_destroy(&filters);
	
	if (files.length > 0 && index_to_take != -1)
	{
		found_file *file = array_at(&files, index_to_take);
		string_copyn(buffer, file->path, MAX_INPUT_LENGTH);
	}
	
	for (s32 i = 0; i < files.length; i++)
	{
		found_file *match = array_at(&files, i);
		mem_free(match->matched_filter);
		mem_free(match->path);
	}
	array_destroy(&files);
}

void *platform_list_files_thread(void *args)
{
	list_file_args *info = args;
	
	array filters = string_split(info->pattern);
	platform_list_files_block(info->list, info->start_dir, filters, info->recursive, info->bucket, info->include_directories, info->is_cancelled, info->info);
	
	mutex_lock(&info->list->mutex);
	*(info->state) = true;
	mutex_unlock(&info->list->mutex);
	
	array_destroy(&filters);
	
	return 0;
}

void platform_list_files(array *list, char *start_dir, char *filter, bool recursive, memory_bucket *bucket, bool *is_cancelled, bool *state, search_info *info)
{
	list_file_args *args = memory_bucket_reserve(bucket, sizeof(list_file_args));
	args->list = list;
	args->start_dir = start_dir;
	args->pattern = filter;
	args->recursive = recursive;
	args->state = state;
	args->include_directories = 0;
	args->bucket = bucket;
	args->is_cancelled = is_cancelled;
	args->info = info;
	
	thread thr = thread_start(platform_list_files_thread, args);
	thread_detach(&thr);
}

void platform_open_file_dialog(file_dialog_type type, char *buffer, char *file_filter, char *start_path, char *save_file_extension)
{
	open_dialog_args *args = mem_alloc(sizeof(open_dialog_args));
	args->buffer = buffer;
	args->type = type;
	args->file_filter = file_filter;
	args->start_path = start_path;
	args->default_save_file_extension = save_file_extension;
	
	thread thr;
	thr.valid = false;
	
	while (!thr.valid)
		thr = thread_start(platform_open_file_dialog_block, args);
	thread_detach(&thr);
}

void destroy_found_file_array(array *found_files)
{
	for (s32 i = 0; i < found_files->length; i++)
	{
		found_file *f = array_at(found_files, i);
		mem_free(f->matched_filter);
		mem_free(f->path);
	}
	array_destroy(found_files);
}

char *platform_get_file_extension(char *path)
{
	while(*path != '.' && *path)
	{
		path++;
	}
	return path;
}

s32 platform_filter_matches(array *filters, char *string, char **matched_filter)
{
	for (s32 i = 0; i < filters->length; i++)
	{
		char *filter = array_at(filters, i);
		if (string_match(filter, string))
		{
			*matched_filter = filter;
			return strlen(filter);
		}
	}
	return -1;
}

void _platform_destroy_shared()
{
	_settings_destroy();
	localization_destroy();
	assets_destroy();
}

void _platform_init_shared(int argc, char **argv, char* config_path)
{
	_lib_loader_init();

	// SDL2 audio.
	{
		if (SDL_Init(SDL_INIT_AUDIO) < 0) {
			log_info("Audio setup failed.");
		}
		if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 512) < 0) {
			log_info("Failed to open audio mixer.");
		}
		if (Mix_AllocateChannels(NUM_AUDIO_CHANNELS) < 0) {
			log_info("Failed to allocate audio channels.");
		}
		log_info("Done setting up audio.");
	}

	// get fullpath of the directory the exe is residing in
	binary_path = platform_get_full_path(argv[0]);
	
	char buf[MAX_PATH_LENGTH];
	platform_get_directory_from_path(buf, binary_path);
	string_copyn(binary_path, buf, MAX_INPUT_LENGTH);

	assets_create();
	for (s32 i = 0; i < ASSET_WORKER_COUNT; i++) {
		thread asset_queue_worker_thread = thread_start(_assets_queue_worker, NULL);
		thread_detach(&asset_queue_worker_thread);
	}


	// Load builtin assets
	assets_load_font(mono_ttf, mono_ttf+mono_ttf_len, 16);
	assets_load_bitmap(close_bmp, close_bmp+close_bmp_len);
	assets_load_bitmap(info_bmp, info_bmp+info_bmp_len);
	
	localization_init();

	_settings_init(config_path);
	set_render_driver(DRIVER_CPU); // Default to CPU
}

u64 __last_stamp = 0;
bool platform_keep_running(platform_window *window)
{
	__last_stamp = platform_get_time(TIME_FULL, TIME_US);

	return window->is_open;
}

void _platform_register_window(platform_window* window) {
	if (!array_exists(&window_registry)) {
		window_registry = array_create(sizeof(platform_window*));
		array_reserve(&window_registry, 10);
	}

	array_push(&window_registry, (uint8_t*)&window);
}

void _platform_unregister_window(platform_window* window) {
	array_remove_by(&window_registry, (uint8_t*)&window);
}

void _switch_render_method(bool use_gpu)
{
	set_render_driver(use_gpu ? DRIVER_GL : DRIVER_CPU);

	for (s32 i = 0; i < window_registry.length; i++) {
		platform_window* w = *(platform_window**)array_at(&window_registry, i);
		platform_setup_backbuffer(w);
	}
	platform_setup_renderer();
	_assets_switch_render_method();
}

void platform_handle_events()
{
	__last_stamp = platform_get_time(TIME_FULL, TIME_NS);

	bool _use_gpu = settings_get_number_or_default("USE_GPU", 0);
	
	// USE_GPU setting changed..
	if (current_render_driver() != (_use_gpu ? DRIVER_GL : DRIVER_CPU)) {
		_switch_render_method(_use_gpu);
	}

	bool redraw_all = false;

	if (assets_do_post_process())
		redraw_all = true;
	
	for (s32 i = 0; i < window_registry.length; i++) {
		platform_window* w = *(platform_window**)array_at(&window_registry, i);

		_global_keyboard = w->keyboard;
		_global_mouse = w->mouse;
		_global_camera = w->camera;

		platform_window_make_current(w); // This should be called whether we are drawing or not.
		_platform_handle_events_for_window(w);

		if (w->do_draw || redraw_all) {
			w->do_draw = false;
			
			u64 update_start = platform_get_time(TIME_FULL, TIME_NS);

			platform_set_cursor(w, CURSOR_DEFAULT);
			renderer->render_clear(w, rgb(255,255,255));
			_camera_apply_transformations(w, &_global_camera);
			renderer->render_reset_scissor();

			if (w->ui)
			{
				// Fix issue with text alignment on startup after assets are done loading.
				if (redraw_all)
				{		
					w->do_draw = true;
					for (int i = 0; i < 2; i++)
					{
						qui_update(w, w->ui);
						qui_render(w, w->ui);
					}
				}

				qui_update(w, w->ui);
				qui_render(w, w->ui);
			}

            if (w->update_func) w->update_func(w);	

			// Update delta.
			{
				u64 current_stamp = platform_get_time(TIME_FULL, TIME_NS);
				u64 diff = current_stamp - update_start;
				float diff_ms = diff / 1000000000.0f;
				update_delta = diff_ms;
			}

		    platform_window_swap_buffers(w);
        }

		w->keyboard = _global_keyboard;
		w->mouse = _global_mouse;
		w->camera = _global_camera;

		if (!w->is_open) {
			if (w->close_func) w->close_func(w);
			platform_destroy_window(w);
		}
	}

	{
		u64 current_stamp = platform_get_time(TIME_FULL, TIME_NS);
		u64 diff = current_stamp - __last_stamp;
		float diff_us = diff / 1000.0f;
		s64 toSleepUS = ((1000000/30.0f)-diff_us);
		if (toSleepUS < 0) toSleepUS = 0;
		thread_sleep(toSleepUS);
	}
}