void _qui_update_horizontal_layout(qui_widget* el) {
	if (el->parent) {
		el->x = el->parent->x;
		el->y = el->parent->y;
		el->width = el->parent->width;
		el->height = el->parent->height;
	}

	layout_widget* data = (layout_widget*)el->data;

	// Calculate size for flex elements.
	s32 fixed_width = 0;
	s32 flex_size = 0;
	for (s32 i = 0; i < el->children.length; i++) {
		qui_widget* w = *(qui_widget**)array_at(&el->children, i);
		if (w->type != WIDGET_FLEX_CONTAINER) {
			fixed_width += w->width;
			fixed_width += w->margin_x*2;
		}
		else {
			qui_flex_container* data = (qui_flex_container*)w->data;
			flex_size += data->flex;
		}
	}

	// Resize flex elements here.
	s32 width_remaining_for_flex_containers = el->width - fixed_width;
	data->size_left_for_flex = width_remaining_for_flex_containers;
	if (flex_size) {
		s32 width_per_flex = width_remaining_for_flex_containers / flex_size;
		s32 rogue_pixels = width_remaining_for_flex_containers - (width_per_flex*flex_size);
		for (s32 i = 0; i < el->children.length; i++) {
			qui_widget* w = *(qui_widget**)array_at(&el->children, i);
			if (w->type == WIDGET_FLEX_CONTAINER) {
				qui_flex_container* data = (qui_flex_container*)w->data;
				w->width = (width_per_flex*data->flex);
				if (i == 0) w->width += rogue_pixels;
			}
		}
	}

	// Set position of elements.
	s32 offsetx = 0;
	for (s32 i = 0; i < el->children.length; i++) {
		qui_widget* w = *(qui_widget**)array_at(&el->children, i);
		w->x = el->x + offsetx + w->margin_x;
		w->y = el->y + w->margin_y;
		w->height = el->height - w->margin_y*2;
		offsetx += w->width + w->margin_x*2;
	}
	data->size = el->width;
	data->fixed_size = fixed_width;
}


void _qui_render_horizontal_layout(qui_widget* el) {
	//renderer->render_rectangle(el->x, el->y, el->width, el->height, rgb(255,0,0));
}

qui_widget* qui_create_horizontal_layout(qui_widget* qui)
{
	log_assert(qui, "Horizontal layout must have a parent widget");
	qui_widget* wg = _qui_create_empty_widget(qui);
	layout_widget* data = mem_alloc(sizeof(layout_widget));
	data->fixed_size = 0;
	data->size_left_for_flex = 0;
	wg->data = (u8*)data;
	wg->type = WIDGET_HORIZONTAL_LAYOUT;
	return wg;
}