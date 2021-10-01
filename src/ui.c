void _qui_close_entire_toolbar(qui_widget* el);
void _qui_close_entire_toolbar_item(qui_widget* el);
qui_widget* _qui_find_parent_of_type(qui_widget* widget, qui_widget_type type);
qui_widget* _qui_create_empty_widget(qui_widget* parent);

#include "qui/button.c"
#include "qui/toolbar.c"
#include "qui/toolbar_item.c"
#include "qui/toolbar_item_option.c"
#include "qui/vertical_layout.c"
#include "qui/horizontal_layout.c"
#include "qui/fixed_container.c"
#include "qui/dragbar.c"
#include "qui/itembar.c"
#include "qui/label.c"
#include "qui/separator.c"
#include "qui/dropdown.c"
#include "qui/tabcontrol.c"
#include "qui/tabcontrol_panel.c"
#include "qui/dropdown_option.c"
#include "qui/size_container.c"
#include "qui/flex_container.c"

//////// General setup
qui_widget* qui_setup()
{
	active_ui_style.widget_text = rgb(249, 249, 249);
	active_ui_style.clear_color = rgb(38, 38, 38);
	active_ui_style.collapse_color = rgb(173, 194, 216);

	active_ui_style.widget_background_static = rgb(31, 31, 31);
	active_ui_style.widget_background_interactive_idle = rgb(51, 51, 51);
	active_ui_style.widget_background_interactive_highlighted = rgb(59, 64, 71);
	active_ui_style.widget_background_interactive_hovered = rgb(61, 61, 61);
	active_ui_style.widget_background_interactive_selected_option = rgb(35, 92, 150);

	active_ui_style.widget_border_inner_idle = rgb(72, 72, 72);
	active_ui_style.widget_border_inner_highlighted = rgb(80, 85, 90);
	active_ui_style.widget_border_inner_hovered = rgb(82, 82, 82);

	active_ui_style.widget_border_outter_static = rgb(24, 24, 24);
	active_ui_style.widget_border_outter_idle = rgb(27, 27, 27);
	active_ui_style.widget_border_outter_highlighted = rgb(80, 160, 240);
	active_ui_style.widget_border_outter_hovered = rgb(27, 27, 27);

	active_ui_style.widget_resize_bar_background = active_ui_style.widget_background_static;
	active_ui_style.widget_panel_background = rgb(35, 35, 35);

	qui_widget* wg = mem_alloc(sizeof(qui_widget));
	wg->children = array_create(sizeof(qui_widget*));
	wg->special_children = array_create(sizeof(qui_widget*));
	wg->data = 0;
	wg->type = WIDGET_MAIN;
	wg->x = 0;
	wg->y = 0;
	return wg;
}

//////// General qui functions
qui_widget* _qui_create_empty_widget(qui_widget* parent) {
	qui_widget* wg = mem_alloc(sizeof(qui_widget));
	wg->children = array_create(sizeof(qui_widget*));
	wg->special_children = array_create(sizeof(qui_widget*));
	wg->data = 0;
	wg->width = 0;
	wg->type = 0;
	wg->height = 0;
	wg->x = 0;
	wg->y = 0;
	wg->margin_x = 0;
	wg->margin_y = 0;
	wg->color_background = rgba(0,0,0,0);
	wg->parent = parent;
	array_push(&parent->children, (uint8_t*)&wg);
	return wg;
}

bool _qui_is_widget_popup_type(qui_widget* el) {
	return el->type == WIDGET_TOOLBAR_ITEM_OPTION || el->type == WIDGET_DROPDOWN_OPTION;
}

vec4 get_scissor_within_current_scissor(vec4 current, vec4 area) {
	s32 x = area.x > current.x ? area.x : current.x;
	s32 y = area.y > current.y ? area.y : current.y;
	s32 w = (x + area.w) > (current.x + current.w) ? (current.x + current.w) : (x + area.w);
	s32 h = (y + area.h) > (current.y + current.h) ? (current.y + current.h) : (y + area.h);
	return (vec4){x,y,w-x,h-y};
}

s32 scissor_index = 0;
vec4 scissor_stack[100] = {0};
void _qui_render_widget(platform_window* window, qui_widget* el, bool draw_special) {
	bool is_special = _qui_is_widget_popup_type(el);
	if (is_special != draw_special) return;

	scissor_stack[scissor_index] = scissor_index == 0 ? 
		(vec4){el->x, el->y, el->width, el->height} : 
		get_scissor_within_current_scissor(scissor_stack[scissor_index-1], (vec4){el->x, el->y, el->width, el->height});

	renderer->render_set_scissor(window, scissor_stack[scissor_index].x, 
		scissor_stack[scissor_index].y, 
		scissor_stack[scissor_index].w, 
		scissor_stack[scissor_index].h);

	scissor_index++;

	if (el->type == WIDGET_BUTTON) _qui_render_button(el);
	if (el->type == WIDGET_TOOLBAR) _qui_render_toolbar(el);
	if (el->type == WIDGET_TOOLBAR_ITEM) _qui_render_toolbar_item(el);
	if (el->type == WIDGET_TOOLBAR_ITEM_OPTION) _qui_render_toolbar_item_option(el);
	if (el->type == WIDGET_LABEL) _qui_render_label(el);
	if (el->type == WIDGET_DROPDOWN) _qui_render_dropdown(el);
	if (el->type == WIDGET_DROPDOWN_OPTION) _qui_render_dropdown_option(el);
	if (el->type == WIDGET_TABCONTROL) _qui_render_tabcontrol(el);
	if (el->type == WIDGET_TABCONTROL_PANEL) _qui_render_tabcontrol_panel(el);

	if (el->type == WIDGET_VERTICAL_LAYOUT/* || el->type == WIDGET_MAIN*/) _qui_render_vertical_layout(el);
	if (el->type == WIDGET_FIXED_CONTAINER) _qui_render_fixed_container(el);
	if (el->type == WIDGET_SIZE_CONTAINER) _qui_render_size_container(el);
	if (el->type == WIDGET_FLEX_CONTAINER) _qui_render_flex_container(el);
	if (el->type == WIDGET_HORIZONTAL_LAYOUT) _qui_render_horizontal_layout(el);
	for (s32 i = 0; i < el->children.length; i++) {
		qui_widget* w = *(qui_widget**)array_at(&el->children, i);
		_qui_render_widget(window, w, draw_special);
	}
	scissor_index--;
}

qui_widget* _qui_find_parent_of_type(qui_widget* widget, qui_widget_type type) {
	qui_widget* parent = widget->parent;
	while (parent) {
		if (parent->type == type) return parent;
		parent = parent->parent;
	}
	return 0;
}

void _qui_update_widget(qui_widget* el, bool update_special) {
	bool is_special = _qui_is_widget_popup_type(el);
	if (is_special != update_special) return;
	for (s32 i = 0; i < el->children.length; i++) {
		qui_widget* w = *(qui_widget**)array_at(&el->children, i);
		_qui_update_widget(w, update_special);
	}
	if (el->type == WIDGET_BUTTON) _qui_update_button(el);
	if (el->type == WIDGET_TOOLBAR) _qui_update_toolbar(el);
	if (el->type == WIDGET_TOOLBAR_ITEM) _qui_update_toolbar_item(el);
	if (el->type == WIDGET_TOOLBAR_ITEM_OPTION) _qui_update_toolbar_item_option(el);
	if (el->type == WIDGET_LABEL) _qui_update_label(el);
	if (el->type == WIDGET_DROPDOWN) _qui_update_dropdown(el);
	if (el->type == WIDGET_DROPDOWN_OPTION) _qui_update_dropdown_option(el);
	if (el->type == WIDGET_TABCONTROL) _qui_update_tabcontrol(el);
	if (el->type == WIDGET_TABCONTROL_PANEL) _qui_update_tabcontrol_panel(el);

	if (el->type == WIDGET_VERTICAL_LAYOUT/* || el->type == WIDGET_MAIN*/) _qui_update_vertical_layout(el);
	if (el->type == WIDGET_FIXED_CONTAINER) _qui_update_fixed_container(el);
	if (el->type == WIDGET_SIZE_CONTAINER) _qui_update_size_container(el);
	if (el->type == WIDGET_FLEX_CONTAINER) _qui_update_flex_container(el);
	if (el->type == WIDGET_HORIZONTAL_LAYOUT) _qui_update_horizontal_layout(el);
}

void qui_render(platform_window* window, qui_widget* el) {
	renderer->render_clear(window, active_ui_style.clear_color);
	renderer->set_render_depth(1);
	scissor_stack[0] = (vec4){0,0,0,0};
	scissor_index = 0;
	renderer->render_reset_scissor();

	for (s32 i = 0; i < el->children.length; i++) {
		qui_widget* w = *(qui_widget**)array_at(&el->children, i);
		_qui_render_widget(window, w, false);
	}
	renderer->set_render_depth(1);
	scissor_stack[0] = (vec4){0,0,0,0};
	scissor_index = 0;
	renderer->render_reset_scissor();

	for (s32 i = 0; i < el->special_children.length; i++) {
		qui_widget* w = *(qui_widget**)array_at(&el->special_children, i);
		_qui_render_widget(window, w, true);
	}
}

void qui_update(platform_window* window, qui_widget* el) {
	el->width = window->width;
	el->height = window->height;

	// Update popup types first.
	for (s32 i = 0; i < el->special_children.length; i++) {
		qui_widget* w = *(qui_widget**)array_at(&el->special_children, i);
		_qui_update_widget(w, true);
	}

	// If mouse press was not handled by popup types, they should be closed.
	if (is_left_clicked_peak()) {
		for (s32 i = 0; i < el->special_children.length; i++) {
			qui_widget* w = *(qui_widget**)array_at(&el->special_children, i);
			if (w->type == WIDGET_TOOLBAR_ITEM_OPTION) _qui_close_entire_toolbar(w);
			if (w->type == WIDGET_DROPDOWN_OPTION) _qui_close_dropdown(w);
		}
	}

	// Update everything else.
	for (s32 i = 0; i < el->children.length; i++) {
		qui_widget* w = *(qui_widget**)array_at(&el->children, i);
		_qui_update_widget(w, false);
	}
}