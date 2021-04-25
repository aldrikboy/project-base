/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

void assets_create()
{
	assets asset_collection;
	asset_collection.images = array_create(sizeof(image));
	asset_collection.fonts = array_create(sizeof(font));
	
	array_reserve(&asset_collection.images, ASSET_IMAGE_COUNT);
	array_reserve(&asset_collection.fonts, ASSET_FONT_COUNT);
	
	asset_collection.queue.queue = array_create(sizeof(asset_task));
	asset_collection.post_process_queue = array_create(sizeof(asset_task));
	
	array_reserve(&asset_collection.queue.queue, ASSET_QUEUE_COUNT);
	array_reserve(&asset_collection.post_process_queue, ASSET_QUEUE_COUNT);
	
	asset_mutex = mutex_create();
	asset_collection.valid = true;
	asset_collection.done_loading_assets = false;
	
	global_asset_collection = asset_collection;
}

inline static bool is_big_endian()
{
	volatile uint32_t i=0x01234567;
    // return 1 for big endian, 0 for little endian.
    return !((*((uint8_t*)(&i))) == 0x67);
}

void assets_stop_if_done()
{
	return;
	if (!global_asset_collection.valid || (global_asset_collection.queue.queue.length == 0 && !global_asset_collection.done_loading_assets))
	{
		//global_asset_collection.done_loading_assets = true;
	}
}

bool assets_do_post_process()
{
	if (!global_asset_collection.valid) return false;

	assets_stop_if_done();
	
	bool result = false;
	
	mutex_lock(&asset_mutex);

	for (int i = 0; i < global_asset_collection.post_process_queue.length; i++)
	{
		asset_task *task = array_at(&global_asset_collection.post_process_queue, i);
		
		if (task->type == ASSET_IMAGE || task->type == ASSET_BITMAP)
		{
			if (task->image->data && task->valid)
			{
				if (current_render_driver() == DRIVER_CPU) { task->image->loaded = true; goto done; }
				
				IMP_glGenTextures(1, &task->image->textureID);
				IMP_glBindTexture(GL_TEXTURE_2D, task->image->textureID);
				
				s32 flag = is_big_endian() ? GL_UNSIGNED_INT_8_8_8_8 : 
				GL_UNSIGNED_INT_8_8_8_8_REV;
				
				if (task->type == ASSET_IMAGE) {
					IMP_glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA8, task->image->width, 
								 task->image->height, 0,  GL_RGBA, flag, task->image->data);
				}
				else {
					IMP_glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA8, task->image->width, 
								 task->image->height, 0,  GL_BGRA, flag, task->image->data);
				}
				
				IMP_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				IMP_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				task->image->loaded = true;
				IMP_glBindTexture(GL_TEXTURE_2D, 0);
			}
		}
		else if (task->type == ASSET_FONT)
		{
			if (task->valid)
			{
				if (current_render_driver() == DRIVER_CPU) { task->font->loaded = true; goto done; }
				
				for (s32 i = TEXT_CHARSET_START; i < TEXT_CHARSET_END; i++)
				{
					glyph *g = &task->font->glyphs[i];
					
					IMP_glGenTextures(1, &g->textureID);
					IMP_glBindTexture(GL_TEXTURE_2D, g->textureID);
					
					IMP_glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
					IMP_glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
                    IMP_glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
                    IMP_glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
					IMP_glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
					IMP_glTexImage2D( GL_TEXTURE_2D, 0, GL_ALPHA, g->width,g->height,
								 0, GL_ALPHA, GL_UNSIGNED_BYTE, g->bitmap );
				}
				
				task->font->loaded = true;
			}
		}
		
		done:
		result = true;
	}
	array_clear(&global_asset_collection.post_process_queue);	
	mutex_unlock(&asset_mutex);
	
	return result;
}

bool assets_queue_worker_load_bitmap(image *image)
{
#ifdef MODE_DEVELOPER
	u64 stamp = platform_get_time(TIME_FULL, TIME_US);
#endif
	
#pragma pack(push, 1)
	typedef struct {
		unsigned short type;                 /* Magic identifier            */
		unsigned int size;                       /* File size in bytes          */
		unsigned int reserved;
		unsigned int offset;                     /* Offset to image data, bytes */
	} HEADER;
	typedef struct {
		unsigned int size;               /* Header size in bytes      */
		int width,height;                /* Width and height of image */
		unsigned short planes;       /* Number of colour planes   */
		unsigned short bits;         /* Bits per pixel            */
		unsigned int compression;        /* Compression type          */
		unsigned int imagesize;          /* Image size in bytes       */
		int xresolution,yresolution;     /* Pixels per meter          */
		unsigned int ncolours;           /* Number of colours         */
		unsigned int importantcolours;   /* Important colours         */
	} INFOHEADER;
#pragma pack(pop)
	
	HEADER* header = (HEADER*)image->start_addr;
	INFOHEADER* info = (INFOHEADER*)(image->start_addr+sizeof(HEADER));
	
	image->data = image->start_addr+header->offset;
	image->width = info->width;
	image->height = info->height;
	image->channels = info->bits/8;
	
	debug_print_elapsed(stamp, "loaded bitmap in");
	
	return image->data != 0;
}

bool assets_queue_worker_load_image(image *image)
{
#ifdef MODE_DEVELOPER
	u64 stamp = platform_get_time(TIME_FULL, TIME_US);
#endif
	
	//stbi_convert_iphone_png_to_rgb(0);
	image->data = stbi_load_from_memory(image->start_addr,
										image->end_addr - image->start_addr,
										&image->width,
										&image->height,
										&image->channels,
										STBI_rgb_alpha);
	
	debug_print_elapsed(stamp, "loaded image in");
	
	return image->data != 0;
}

bool assets_queue_worker_load_font(font *font)
{
#ifdef MODE_DEVELOPER
	u64 stamp = platform_get_time(TIME_FULL, TIME_US);
#endif
	
	unsigned char *ttf_buffer = (unsigned char*)font->start_addr;
	
    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer,0)))
    {
		return false;
	}
	float scale = stbtt_ScaleForPixelHeight(&info, font->size);
	
	for (s32 i = TEXT_CHARSET_START; i < TEXT_CHARSET_END; i++)
	{
		s32 w = 0, h = 0, xoff = 0, yoff = 0;
		
		glyph new_glyph;
		new_glyph.bitmap = stbtt_GetCodepointBitmap(&info, 0, scale, i, &w, &h, &xoff, &yoff);
		new_glyph.width = w;
		new_glyph.height = h;
		new_glyph.xoff = xoff;
		new_glyph.yoff = yoff;
		
		stbtt_GetCodepointHMetrics(&info, i, &new_glyph.advance, &new_glyph.lsb);
		new_glyph.advance *= scale;
		new_glyph.lsb *= scale;
		
		if (i == 'M') font->px_h = -yoff;
		
		font->glyphs[i-TEXT_CHARSET_START] = new_glyph;
	}
	
	font->info = info;
	font->scale = scale;
	
	debug_print_elapsed(stamp, "loaded font in");
	
	return true;
}

void *_assets_queue_worker()
{
	while (global_asset_collection.valid && !global_asset_collection.done_loading_assets)
	{
		thread_sleep(1000);

		if (mutex_trylock(&asset_mutex))
		{
			int queue_length = global_asset_collection.queue.queue.length;
			if (!queue_length) 
			{
				mutex_unlock(&asset_mutex);
				continue;
			}
			
			asset_task *task = array_at(&global_asset_collection.queue.queue, 0);
			asset_task buf = *task;
			array_remove_at(&global_asset_collection.queue.queue, 0);
			mutex_unlock(&asset_mutex);
			
			if (buf.type == ASSET_IMAGE)
			{
				bool result = assets_queue_worker_load_image(buf.image);
				buf.valid = result;
			}
			else if (buf.type == ASSET_BITMAP)
			{
				bool result = assets_queue_worker_load_bitmap(buf.image);
				buf.valid = result;
			}
			else if (buf.type == ASSET_FONT)
			{
				bool result = assets_queue_worker_load_font(buf.font);
				buf.valid = result;
			}
			
			mutex_lock(&asset_mutex);
			
			log_assert(global_asset_collection.post_process_queue.reserved_length > 
				   global_asset_collection.post_process_queue.length, "Attempted to process more assets than specified with constant ASSET_QUEUE_COUNT");
			
			array_push(&global_asset_collection.post_process_queue, &buf);
			mutex_unlock(&asset_mutex);
		}
	}
	
	thread_exit();
	
	return 0;
}

static image* find_image_ref(u8 *start_addr, s32 hash)
{
	for (int i = 0; i < global_asset_collection.images.length; i++)
	{
		image *img_at = array_at(&global_asset_collection.images, i);
		
		if ((start_addr == img_at->start_addr || hash == img_at->path_hash) && img_at->references > 0)
		{
			img_at->references++;
			return img_at;
		}
	}
	return 0;
}

static font* find_font_ref(u8 *start_addr, s32 hash, s16 size)
{
	for (int i = 0; i < global_asset_collection.fonts.length; i++)
	{
		font *font_at = array_at(&global_asset_collection.fonts, i);
		
		if ((start_addr == font_at->start_addr || hash == font_at->path_hash) && font_at->size == size && font_at->references > 0)
		{
			font_at->references++;
			return font_at;
		}
	}
	return 0;
}

static u32 hash_path(char* str)
{
	unsigned long hash = 5381;
    int c;
    while ((c = *str++) && c) hash = ((hash << 5) + hash) + c;
    return hash;
}

static image empty_image()
{
	image new_image;
	new_image.loaded = false;
	new_image.start_addr = 0;
	new_image.end_addr = 0;
	new_image.references = 1;
	new_image.path_hash = UNDEFINED_PATH_HASH;
	return new_image;
}

static font empty_font()
{
	font new_font;
	new_font.size = 0;
	new_font.loaded = false;
	new_font.start_addr = 0;
	new_font.end_addr = 0;
	new_font.references = 1;
	new_font.path_hash = UNDEFINED_PATH_HASH;
	return new_font;
}

static asset_task add_font_to_queue(font font)
{
	// NOTE(Aldrik): we should never realloc the image array because pointers will be invalidated.
	log_assert(CAN_ADD_NEW_FONT(), "Attempted to process more fonts than specified with constant ASSET_FONT_COUNT");

	int index = array_push(&global_asset_collection.fonts, &font);
	
	asset_task task;
	task.type = ASSET_FONT;
	task.font = array_at(&global_asset_collection.fonts, index);
	
	mutex_lock(&asset_mutex);
	array_push(&global_asset_collection.queue.queue, &task);
	mutex_unlock(&asset_mutex);

	return task;
}

static asset_task add_image_to_queue(image img, bool is_bitmap)
{
	// NOTE(Aldrik): we should never realloc the image array because pointers will be invalidated.
	log_assert(CAN_ADD_NEW_IMAGE(), "Attempted to process more images than specified with constant ASSET_IMAGE_COUNT");

	int index = array_push(&global_asset_collection.images, &img);
	
	asset_task task;
	task.type = is_bitmap ? ASSET_BITMAP : ASSET_IMAGE;
	task.image = array_at(&global_asset_collection.images, index);
	
	mutex_lock(&asset_mutex);
	array_push(&global_asset_collection.queue.queue, &task);
	mutex_unlock(&asset_mutex);

	return task;
}

////////////////////////////////////////////////////
// Loading
////////////////////////////////////////////////////
font* assets_load_font_from_file(char* path, s16 size)
{
	u32 hash = hash_path(path);
	font* ref = find_font_ref(UNDEFINED_START_ADDR, hash, size);
	if (ref) return ref;

	platform_set_active_directory(binary_path);
	file_content content = platform_read_file_content(path, "rb");

	font new_font = empty_font();
	new_font.size = size;
	new_font.path_hash = hash;
	new_font.start_addr = content.content;
	new_font.end_addr = content.content + content.content_length - 1;

	return add_font_to_queue(new_font).font;
}

font* assets_load_font(u8 *start_addr, u8 *end_addr, s16 size)
{
	font* ref = find_font_ref(start_addr, UNDEFINED_PATH_HASH, size);
	if (ref) return ref;
	
	font new_font = empty_font();
	new_font.start_addr = start_addr;
	new_font.end_addr = end_addr;
	new_font.size = size;

	return add_font_to_queue(new_font).font;
}

image* assets_load_image_from_file(char* path)
{
	u32 hash = hash_path(path);
	image* ref = find_image_ref(UNDEFINED_START_ADDR, hash);
	if (ref) return ref;

	platform_set_active_directory(binary_path);
	file_content content = platform_read_file_content(path, "rb");

	image new_image = empty_image();
	new_image.path_hash = hash;
	new_image.start_addr = content.content;
	new_image.end_addr = content.content + content.content_length - 1;

	return add_image_to_queue(new_image, false).image;
}

image* assets_load_image(u8 *start_addr, u8 *end_addr)
{
	image* ref = find_image_ref(start_addr, UNDEFINED_PATH_HASH);
	if (ref) return ref;
	
	image new_image = empty_image();
	new_image.start_addr = start_addr;
	new_image.end_addr = end_addr;

	return add_image_to_queue(new_image, false).image;
}

image* assets_load_bitmap_from_file(char* path)
{
	u32 hash = hash_path(path);
	image* ref = find_image_ref(UNDEFINED_START_ADDR, hash);
	if (ref) return ref;

	platform_set_active_directory(binary_path);
	file_content content = platform_read_file_content(path, "rb");

	image new_image = empty_image();
	new_image.path_hash = hash;
	new_image.start_addr = content.content;
	new_image.end_addr = content.content + content.content_length - 1;

	return add_image_to_queue(new_image, true).image;
}

image* assets_load_bitmap(u8 *start_addr, u8 *end_addr)
{
	image* ref = find_image_ref(start_addr, UNDEFINED_PATH_HASH);
	if (ref) return ref;

	image new_image = empty_image();
	new_image.start_addr = start_addr;
	new_image.end_addr = end_addr;

	return add_image_to_queue(new_image, true).image;
}

////////////////////////////////////////////////////
// Cleaning up
////////////////////////////////////////////////////
void assets_destroy_bitmap(image *image_to_destroy)
{
	if (image_to_destroy->references == 1)
	{
		if (current_render_driver() == DRIVER_GL)
		{
			IMP_glBindTexture(GL_TEXTURE_2D, 0);
			IMP_glDeleteTextures(1, &image_to_destroy->textureID);
		}
		
		image_to_destroy->references = 0;
	}
	else
	{
		image_to_destroy->references--;
	}
}

void assets_destroy_font(font *font_to_destroy)
{
	if (font_to_destroy->references == 1)
	{
		if (current_render_driver() == DRIVER_GL)
		{
			for (s32 i = TEXT_CHARSET_START; i < TEXT_CHARSET_END; i++)
			{
				glyph g = font_to_destroy->glyphs[i];
				IMP_glBindTexture(GL_TEXTURE_2D, 0);
				IMP_glDeleteTextures(1, &g.textureID);
			}
		}
		
		font_to_destroy->references = 0;
	}
	else
	{
		font_to_destroy->references--;
	}
}

void assets_destroy_image(image *image_to_destroy)
{
	if (image_to_destroy->references == 1)
	{
		if (current_render_driver() == DRIVER_CPU)
		{
			IMP_glBindTexture(GL_TEXTURE_2D, 0);
			IMP_glDeleteTextures(1, &image_to_destroy->textureID);
		}
		
		image_to_destroy->references = 0;
	}
	else
	{
		image_to_destroy->references--;
	}
}


////////////////////////////////////////////////////
// Extra
////////////////////////////////////////////////////
void _assets_switch_render_method()
{
	for (int i = 0; i < global_asset_collection.images.length; i++)
	{
		image *img_at = array_at(&global_asset_collection.images, i);
		
		if (current_render_driver() == DRIVER_GL)
		{
			asset_task task;
			task.type = ASSET_IMAGE;
			task.image = img_at;
			task.valid = true;
			array_push(&global_asset_collection.post_process_queue, &task);
		}
		else
		{
			IMP_glBindTexture(GL_TEXTURE_2D, 0);
			IMP_glDeleteTextures(1, &img_at->textureID);
		}
	}
	
	for (int i = 0; i < global_asset_collection.fonts.length; i++)
	{
		font *font_at = array_at(&global_asset_collection.fonts, i);
		
		if (current_render_driver() == DRIVER_GL)
		{
			asset_task task;
			task.type = ASSET_FONT;
			task.font = font_at;
			task.valid = true;
			
			array_push(&global_asset_collection.post_process_queue, &task);
		}
		else
		{
			for (s32 i = TEXT_CHARSET_START; i < TEXT_CHARSET_END; i++)
			{
				glyph g = font_at->glyphs[i];
				IMP_glBindTexture(GL_TEXTURE_2D, 0);
				IMP_glDeleteTextures(1, &g.textureID);
			}
		}
	}
}

void assets_destroy()
{
	global_asset_collection.valid = false;
	global_asset_collection.done_loading_assets = true;
	thread_sleep(30000); // wait 30ms for all asset threads to finish.
	mutex_lock(&asset_mutex);
	{
		if (array_exists(&global_asset_collection.images)) array_destroy(&global_asset_collection.images);
		if (array_exists(&global_asset_collection.fonts)) array_destroy(&global_asset_collection.fonts);
		
		if (array_exists(&global_asset_collection.queue.queue)) array_destroy(&global_asset_collection.queue.queue);
		if (array_exists(&global_asset_collection.post_process_queue)) array_destroy(&global_asset_collection.post_process_queue);

		mem_free(binary_path);
	}
	mutex_unlock(&asset_mutex);
	mutex_destroy(&asset_mutex);
}
