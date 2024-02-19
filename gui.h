#pragma once
#include <iostream>
#include <vector>
#include <unordered_map>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include <cassert>

typedef std::vector<Rectangle> rec_list;

template <class T> 
struct list {
    T* data;
    size_t size;
    size_t capacity;
};

enum layout_type {
    VERTICAL, HORIZONTAL, SLICE_VERT, SLICE_HOR, LAYOUT_TYPE_MAX
};

class Layout {
public:

    Layout(Rectangle boundary, int type, int slot_count, float spacing = 0.f) 
    : boundary(boundary), type((layout_type)type), spacing(spacing) {
	assert(type < SLICE_VERT && "this constructor is not for slice types");
	precompute_slots(slot_count);
    };

    Layout(Rectangle boundary, int type, float slice_ratio, float spacing = 0.f) 
	:boundary(boundary), type((layout_type)type), slice_ratio(slice_ratio), spacing(spacing) {
	assert(type > HORIZONTAL && "this constructor is for slice types");
	Rectangle rec1, rec2;
	slice_rec(boundary, slice_ratio, type, rec1, rec2);
	slot_list.push_back(rec1);
	slot_list.push_back(rec2);
    }

    void resize_slots() {
	if(type < SLICE_VERT) {
	    int slot_count = slot_list.size();
	    for(int slot_index = 0; slot_index < slot_list.size(); ++slot_index) {
		slot_list[slot_index] = get_default_slot_rec(type, slot_index, slot_count);
	    }
	}
	else {
	    slice_rec(boundary, slice_ratio, type, slot_list[0], slot_list[1]);
	}
	
    }

    Rectangle get_default_slot_rec(int layout_type, int slot_index, int slot_count) {
	Rectangle slot;
	    switch (layout_type) {
	    case VERTICAL:
		slot.x = boundary.x;
		slot.y = boundary.y + (boundary.height / (float)slot_count) * (float)slot_index;
		slot.width = boundary.width;
		slot.height = boundary.height / (float)slot_count;
		break;
	    case HORIZONTAL:
		slot.x = boundary.x + boundary.width / (float)slot_count * (float)slot_index;
		slot.y = boundary.y;
		slot.width = boundary.width / (float)slot_count;
		slot.height = boundary.height;
		break;
	    case SLICE_VERT:
	    case SLICE_HOR:
	    case LAYOUT_TYPE_MAX:
		assert(false && "unreachable");
		break;
	    }
	return slot;
    }

    Rectangle get_slot(int slot_index, bool spaced = false) {
	slot_index %= slot_list.size();
	Rectangle slot = slot_list[slot_index];
	if(spaced) {
	    Rectangle slot_nospace = slot;
	    slot.width -= spacing * 2.f;
	    slot.height -= spacing * 2.f;
	    center_rec(slot_nospace, slot);
	}
	return slot;
    }

    void resize(Rectangle new_boundary) {
	boundary = new_boundary;
	resize_slots();
    }

    void resize_from_point(Vector2 new_point) {
	Rectangle new_boundary = resize_rec(boundary, new_point.x, new_point.y, type);
	resize(new_boundary);
    }

    Rectangle get_boundary() {
	return boundary;
    }

    Rectangle resize_rec(Rectangle rec, float new_x, float new_y, int type) {
	if(type == VERTICAL) {
	    rec.width -= new_x - rec.x;
	    rec.x = new_x;
	}
	else if(type == HORIZONTAL) {
	    rec.height -= new_y - rec.y;
	    rec.y = new_y;
	}
	return rec;
    }

    void center_rec(const Rectangle& boundary, Rectangle& to_center) {
	to_center.x = boundary.x + boundary.width / 2.f - to_center.width / 2.f;
	to_center.y = boundary.y + boundary.height / 2.f - to_center.height / 2.f;
    }

    void slice_rec(const Rectangle rec, float slice_ratio, int type, Rectangle& out1, Rectangle& out2) {
	assert(type > HORIZONTAL);
	float width = type == SLICE_VERT ? rec.width : rec.width * slice_ratio;
	float height = type == SLICE_HOR ? rec.height : rec.height * slice_ratio;
	out1 = Rectangle{rec.x, rec.y, width, height};

	float x2 = type == SLICE_VERT ? out1.x : out1.x + width;
	float y2 = type == SLICE_HOR ? out1.y : out1.y + height;
	float width2 = type == SLICE_VERT ? rec.width : rec.width * (1.f - slice_ratio);
	float height2 = type == SLICE_HOR ? rec.height : rec.height * (1.f - slice_ratio);
	out2 = Rectangle{x2, y2, width2, height2};
    }

    void set_spacing(float spacing) {
	this->spacing = spacing;
    }

    float get_spacing() {
	return spacing;
    }

    void print_rec(Rectangle rec) {
	std::cout << "rectangle: x = " << rec.x << ", y = " << rec.y 
	    << ", width = " << rec.width << ", height = " << rec.height << "\n";
    }

    void draw() {
	int slot_count = slot_list.size();
	for(int slot = 0; slot < slot_count; ++slot) {
	    //GuiGroupBox(get_slot(slot), "");
	    DrawRectangleLinesEx(get_slot(slot, false), 1, BLACK);
	}
    }
private:

    Rectangle boundary; 
    rec_list slot_list;
    layout_type type;
    //int current_slot = 0;
    float spacing = 0.f;
    float slice_ratio = 0.f;

    void precompute_slots(int slot_count) {
	for(int slot_index = 0; slot_index < slot_count; ++slot_index) {
	    Rectangle slot = get_default_slot_rec(type, slot_index, slot_count);
	    slot_list.push_back(slot);
	}

    }
};

namespace Gui {
    struct Node;
    typedef std::vector<Node> node_list;
    struct Node{
        bool* open;
        Rectangle boundary;
        const char* label;
        node_list children;
        Node* parent;
    };

    void table(Rectangle boundary, int num_cols, int num_rows, 
               const char* header_values, const char* body_values, void(*on_click)(void* data) = NULL); 
    void begin_tree(Rectangle boundary);
    void end_tree();
    bool tree_node(const char* label, bool default_open = false, bool* button_click = NULL);
    void tree_push(const char* label, bool open);
    void tree_pop();
    void draw_tree(Node* head, int depth = 0);
    const char* read_word(const char* words, int word);
    bool contains(const std::unordered_map<std::string, bool>& map, const char* key); 

static Node tree;
static Node* tree_head;
static Layout tree_layout = Layout({0}, VERTICAL, 0);
static std::vector<Rectangle> tree_stack;
static std::unordered_map<std::string, bool> tree_state;
static int tree_node_count = 0;
static float tree_toggle_scale_factor = 0.9f;


bool contains(const std::unordered_map<std::string, bool>& map, const char* key) {
    for(std::pair<std::string, bool> pair : map) {
	if(pair.first == key) return true;
    }
    return false;
}

const char* read_word(const char* words, int word) {
    int cursor = 0;
    int word_found = 0;
    while(word_found < word) {
	if(words[cursor] == '\0') {
	    word_found++;
	}
	cursor++;
    } 
    return words + cursor;
}

void table(Rectangle boundary, int num_cols, int num_rows, 
		const char* header_values, const char* body_values, void(*on_click)(void* data)) {
    Layout table_layout = Layout(boundary, SLICE_VERT, 0.2f);
    Layout header = Layout(table_layout.get_slot(0), HORIZONTAL, num_cols);
    Layout row_layout = Layout(table_layout.get_slot(1), VERTICAL, num_rows);
    Color header_background_col = ColorAlpha(GRAY, 0.3f);

    for(int i = 0; i < num_cols; ++i) {
	Rectangle slot = header.get_slot(i);
	DrawRectangleRec(slot, header_background_col);
	GuiTextBox(slot, (char*)read_word(header_values, i), (int)(header.get_boundary().height), false);	
    }
    for(int i = 0; i < num_rows; ++i) {
	Layout single_row = Layout(row_layout.get_slot(i), HORIZONTAL, num_cols);
	int text_size = (int)(single_row.get_boundary().height);
	for(int j = 0; j < num_cols; ++j) {
	    Rectangle slot = single_row.get_slot(j);
	    char* word = (char*)read_word(body_values, j + i * num_cols);
	    if(on_click) {
		if(GuiLabelButton(slot, word)) {
		    on_click(word);
		}
	    }
	    else {
		GuiTextBox(slot, word, text_size, false);	
	    }
	    DrawLine(slot.x + slot.width, slot.y, slot.x + slot.width, slot.y + slot.height, BLACK);
	}
    }
    //header.draw();
    row_layout.draw();
}

void begin_tree(Rectangle boundary) {
    tree_layout = Layout(boundary, VERTICAL, 20);
    tree_stack.push_back({0});
    tree_node_count = 0;
}

void end_tree() {
    tree_stack.clear();
}


Rectangle scale_node_boundary(Rectangle parent_boundary) {
    if(parent_boundary.width == 0) return tree_layout.get_slot(0);
    float new_width = parent_boundary.width * tree_toggle_scale_factor;
    float new_x = parent_boundary.x + parent_boundary.width - new_width; 
    parent_boundary.x = new_x; parent_boundary.width = new_width;
    parent_boundary.y = parent_boundary.height * (tree_node_count - 1);
    return parent_boundary;
}

void tree_push(const char* label, bool open) {
    tree_node_count++;
    if(!contains(tree_state, label)) {
	tree_state[label] = open;
    }
}

void tree_pop() {
    tree_stack.pop_back();
}

bool tree_node(const char *label, bool default_open, bool* button_click) {
    tree_push(label, default_open);

    Rectangle parent_boundary = tree_stack[tree_stack.size() - 1];
    Rectangle boundary = scale_node_boundary(parent_boundary);
    Layout node_layout = Layout(boundary, SLICE_HOR, 1.f - tree_toggle_scale_factor);
    bool open = tree_state[label];
    const char* toggle_label = open ? "^" : ">";

    GuiToggle(node_layout.get_slot(0), toggle_label, &open);
    tree_state[label] = open;

    if(GuiButton(node_layout.get_slot(1), label)) {
	if(button_click) {
	    *button_click = true;
	}
    }

    Vector2 horizontal_line_start = {boundary.x, boundary.y + boundary.height/2.f};
    Vector2 vertical_line_start = {parent_boundary.x + parent_boundary.width * (1.f - tree_toggle_scale_factor) / 2.f, horizontal_line_start.y};
    Vector2 vertical_line_end = {vertical_line_start.x, parent_boundary.y + parent_boundary.height};
    if(parent_boundary.width > 0) {
	DrawLineV(horizontal_line_start, vertical_line_start, DARKGRAY);
	DrawLineV(vertical_line_start, vertical_line_end, DARKGRAY);
    }

    tree_stack.push_back(boundary);
    return open;
}

}
