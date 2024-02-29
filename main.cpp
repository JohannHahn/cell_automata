/*
* TODO:	Fullscreen the field
* TODO:	minimal controls on top? Only button controls?
*/

#include "raygui.h"
#include "raylib/src/raylib.h"
#include "cell_automata.h"
#include <cinttypes>
#include <cmath>
#include <cstring>
#include <iostream>
#include <cassert>
#include <string>
#include "gui.h"

#define COLOR_FROM_U32(c) *(Color*)&c

enum defined_automata {
    ELEM, GOL, PARTICLE 
};

enum control_state {
    HIDE, SHOW, CONTROL_STATE_MAX
};
control_state control_state = HIDE;

enum cell_type {
    EMPTY_CELL, SAND_CELL, ELEM_CELL, GOL_CELL, CELL_TYPE_MAX
};

u32 cell_colors[CELL_TYPE_MAX] = {0xFF181818, 0xFF11AAFF, 0xFFFFFFFF, 0xFFCAFACA};

struct Cell {
    cell_type type; 
    bool in_freefall;
    // how far can the element move sideways after falling
    //int side_range;
    bool operator==(Cell c) {
	return this->type == c.type && this->in_freefall == c.in_freefall;
    }
    friend std::ostream& operator<<(std::ostream& os, const Cell& cell) {
	os << "type = " << cell.type << "\nin_freefall = " << cell.in_freefall << "\n";
	os << "color = " << cell_colors[cell.type] << "\n";
	return os;
    }
};

const size_t fill_values_size = CELL_TYPE_MAX - 1;
Cell fill_values[fill_values_size] = {{SAND_CELL, true}};
Cell empty_cell = {EMPTY_CELL, false};

bool mouse_draw = true;
bool debugging = false;

u32 alive_col = 0xFFFF1111;
u32 dead_col  = 0xFF181818;

KeyboardKey autoplay_key = KEY_SPACE;
KeyboardKey next_frame_key = KEY_RIGHT;

int cell_cols = 100;
int cell_rows = 100;

const int max_cols = 1000;
const int max_rows = 1000;
const int min_cols = 10;
const int min_rows = 10;

float window_width = 1000;
float window_height = 1000;

float min_dim = std::min(window_width, window_height);
Rectangle view_area = {0, 0, window_width*0.9f, window_height};
float cell_width = view_area.width / (float)cell_cols;
float cell_height = view_area.height / (float)cell_rows;
Rectangle control_area = {view_area.width, 0, window_width - view_area.width, window_height};
int controls_num_widgets = 10;
Layout control_layout = Layout(control_area, VERTICAL, controls_num_widgets, 5);
int control_index = 0;

defined_automata automat_type_selection = PARTICLE;

bool autoplay = false;
double seconds_passed = 0.f;
float max_fps = 100.f;
float target_fps = 60;

Cell_Automat<Cell>* active_automat;
float next_cell_cols = 0;
float next_cell_rows = 0;
u64 next_one_dim_ruleset = 0;

Cell_Automat<Cell> gol_automat;
Cell_Automat<Cell> elementary_automat;
Cell_Automat<Cell> sand_automat;

Texture txt;

Rectangle brush_view_rec = {0.f, 0.f, 1.f, 1.f};
float brush_width = 1.f;
float brush_height = 1.f;


void sand_rules_func(Cell_Automat<Cell>& automat) {	
    for (int y = 0; y < automat.height; ++y) {
	for (int x = 0; x < automat.width; ++x) {
	    int index = INDEX_AUTOMAT(x, y);
	    int next_row_index = INDEX_AUTOMAT(x, y + 1);
	    Cell* cell = &automat.cells[index];
	    // fall down
	    if (cell->type == SAND_CELL && cell->in_freefall
		&& !automat.move_if_empty(index, next_row_index, *cell)) {
		bool side1 = false; bool side2 = false;
		int x_dif = GetRandomValue(0, 1) ? 1 : -1;
		int x_dif2 = x_dif == 1 ? -1 : 1;
		side1 = automat.move_if_empty(index, INDEX_AUTOMAT(x + x_dif, y + 1), *cell);	
		if (!side1) side2 = automat.move_if_empty(index, INDEX_AUTOMAT(x + x_dif2, y + 1), *cell);
		if (!side1 && !side2) {
		    automat.empty[index] = *cell; 
		    cell->in_freefall = false;
		}
	    }
	    if (cell->type == SAND_CELL && !cell->in_freefall) {
		bool side1 = false; bool side2 = false;
		int x_dif = GetRandomValue(0, 1) ? 1 : -1;
		int x_dif2 = x_dif == 1 ? -1 : 1;
		side1 = automat.move_if_empty(index, INDEX_AUTOMAT(x + x_dif, y + 1), *cell);	
		if (!side1) side2 = automat.move_if_empty(index, INDEX_AUTOMAT(x + x_dif2, y + 1), *cell);
		if (!side1 && !side2) {
		    automat.empty[index] = *cell; 
		}
	    }
	    else if (cell->type == SAND_CELL && !cell->in_freefall) {
		if (automat.move_if_empty(index, next_row_index, *cell)) {
		    cell->in_freefall = true;
		}
		else automat.empty[index] = *cell; 
	    }
	}
    }
}

void update_texture() {
    Image h = GenImageColor(active_automat->width, active_automat->height, 
			    COLOR_FROM_U32(cell_colors[EMPTY_CELL]));
    u32* pixels = (u32*)h.data;
    for (int i = 0; i < active_automat->size; ++i) {
	if (active_automat->cells[i].type != EMPTY_CELL) {
	    pixels[i] = cell_colors[active_automat->cells[i].type];
	}
    }
    UpdateTexture(txt, pixels);
    UnloadImage(h);
}

void resize() {
    window_width = GetScreenWidth();
    window_height = GetScreenHeight();
    min_dim = std::min(window_width, window_height);
    if (control_state == HIDE) view_area = {0, 0, window_width*0.9f, window_height};
    else if (control_state == SHOW) view_area = {0, 0, min_dim, min_dim};
    bool vertical = min_dim == window_width && !(window_width == window_height);
    std::cout << "vertical = " << vertical << "\n";
    control_area = {.x = vertical ? 0 : view_area.width, 
		    .y = vertical ? view_area.height : 0, 
		    .width = vertical ? window_width : window_width - view_area.width, 
		    .height = vertical ? window_height - view_area.height : window_height};
    control_layout = Layout(control_area, VERTICAL, controls_num_widgets, 5);
    cell_width = view_area.width / (float)cell_cols;
    cell_height = view_area.height / (float)cell_rows;
    brush_view_rec.width = cell_width * brush_width;
    brush_view_rec.height = cell_height * brush_height;
    control_layout.set_spacing(min_dim / 30.f);
}

Rectangle get_next_control_slot(bool spaced = true) {
    return control_layout.get_slot(control_index++, spaced);
}

void control_current_automat() {
    // info about current layout
    std::string table_body = active_automat->type == ONE_DIM ? "1D elementary" : "2D Game of life"; table_body += '\0';
    table_body += std::to_string(active_automat->width); table_body += '\0';
    table_body += std::to_string(active_automat->height); table_body += '\0';
    Gui::table(get_next_control_slot(), 3, 1, "Type\0Width\0Height", table_body.c_str());; 
    

    Layout ruleset_info_layout = Layout(get_next_control_slot(), SLICE_VERT, 0.1f, 1.f);
    Layout ruleset_label_layout = Layout(ruleset_info_layout.get_slot(0), HORIZONTAL, 2, 1.f);
    GuiDrawText("Ruleset:", ruleset_label_layout.get_slot(0, true), TEXT_ALIGN_LEFT, WHITE);
    std::string ruleset_str = active_automat->type == TWO_DIM ? "Conway's game of life" : std::to_string(active_automat->one_dim_rules);
    GuiDrawText(ruleset_str.c_str(), ruleset_label_layout.get_slot(1, true), TEXT_ALIGN_LEFT, WHITE);
    // input one dimensional rules as binary
    if (active_automat->type == ONE_DIM) {
	bool secret_view = true;
	Layout ruleset_layout = Layout(ruleset_info_layout.get_slot(1), HORIZONTAL, 8, 5.f);
	for(int i = 0; i < 8; ++i) {
	    std::string binary = "000";
	    Layout vert_layout = Layout(ruleset_layout.get_slot(i), VERTICAL, 2);
	    int bit = BIT_AT(7 - i, active_automat->one_dim_rules);
	    std::string bit_str; 
	    bit_str += '0' + bit;
	    for (int j = 0; j < 3; ++j) {
		binary[2-j] = '0' + BIT_AT(j, i);
	    }
	    binary += "\nflip";
	    GuiDrawText(bit_str.c_str(), vert_layout.get_slot(0), TEXT_ALIGN_MIDDLE, WHITE);
	    if (GuiButton(vert_layout.get_slot(1), binary.c_str())) {
		if (bit) {
		    BIT_RESET(7 - i, active_automat->one_dim_rules);
		}
		else {
		    BIT_SET(7 - i, active_automat->one_dim_rules);
		}
	    }
	}
    }
    GuiSlider(get_next_control_slot(), "0", std::to_string(max_fps).c_str(), &target_fps, 0.f, max_fps);
    GuiToggle(get_next_control_slot(), autoplay ? "Stop" : "Play", &autoplay);

    if (GuiButton(get_next_control_slot(), "Restart")) {
	active_automat->generation = 0;
	memcpy(active_automat->cells, active_automat->initial_cells, active_automat->size);
    }
}

void controls() {
    control_index = 0;
    if (autoplay || IsKeyReleased(next_frame_key)) {
	if (seconds_passed >= 1.f / target_fps) { 
	    seconds_passed = 0.f;
	    active_automat->apply_rules(); 
	}
    }

    if (IsKeyReleased(autoplay_key)) {
	autoplay = !autoplay;
    }
    if (IsKeyReleased(KEY_ONE)) {
	active_automat = &elementary_automat;
	autoplay = false;
    }
    if (IsKeyReleased(KEY_TWO)) {
	active_automat = &gol_automat;
	autoplay = false;
    }
    if (IsKeyReleased(KEY_THREE)) {
	active_automat = &sand_automat;
	autoplay = false;
    }
    if (IsKeyReleased(KEY_R)) {
	active_automat->randomize_cells();
    }
    if (IsKeyReleased(KEY_DELETE)) {
	active_automat->clear_cells();
    }

    if (mouse_draw && active_automat->is_initialized()) {
	Vector2 mouse_pos = GetMousePosition();
	if (CheckCollisionPointRec(mouse_pos, view_area)) {
	    brush_view_rec.x = floor(mouse_pos.x - cell_width / 2.f); 
	    brush_view_rec.y = floor(mouse_pos.y - cell_height / 2.f);
	    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
		
		Vector2 mouse_pos_projected = {mouse_pos.x / view_area.width * active_automat->width, mouse_pos.y / view_area.height * active_automat->height};
		active_automat->cells[INDEX((int)mouse_pos_projected.x, (int)mouse_pos_projected.y, active_automat->width)] = active_automat->selected_fill;
	    }
	    DrawRectangleLinesEx(brush_view_rec, 2.f, WHITE);
	}
    }
    // ui elements
    if (control_state == HIDE) {
	DrawRectangleRec(control_area, GRAY);
	if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), control_area)) {
	    std::cout << "Hi\n";
	    control_state = SHOW;
	    resize();
	}
	return;
    }
    control_current_automat();
    if (GuiButton(get_next_control_slot(), "randomize buffer")) {
	active_automat->randomize_cells();
    }
    if (GuiButton(get_next_control_slot(), "erase buffer")) {
	active_automat->clear_cells();
    }

    Rectangle mouse_checkbox_rec = get_next_control_slot();
    mouse_checkbox_rec.width /= 5.f;
    GuiCheckBox(mouse_checkbox_rec, "Mouse drawing", &mouse_draw);
}

void draw_view_area() {
    Rectangle source = {0, 0, (float)txt.width, (float)txt.height};
    DrawTexturePro(txt, source, view_area, {0.f, 0.f}, 0.f, WHITE);
}

int main() {
    SetRandomSeed(GetTime());
    InitWindow(window_width, window_height, "hi");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(max_fps);
    Image h = GenImageColor(cell_cols, cell_rows, COLOR_FROM_U32(dead_col));
    txt = LoadTextureFromImage(h);
    UnloadImage(h);
    
    gol_automat.init(TWO_DIM, cell_cols, cell_rows, {EMPTY_CELL, 0}, fill_values, 1);
    gol_automat.set_rules_gol();
    std::cout << "after gol init\n";
    elementary_automat.init(ONE_DIM, cell_cols, cell_rows, {EMPTY_CELL, 0}, fill_values, 1);
    elementary_automat.set_ruleset_bin("00011110");
    sand_automat.init(TWO_DIM, cell_cols, cell_rows, 
		      {EMPTY_CELL, false}, fill_values, fill_values_size);
    sand_automat.set_rules2D(sand_rules_func);
    active_automat = &gol_automat;

    update_texture();
    control_layout.set_spacing(min_dim / 50.f);

    while (!WindowShouldClose()) {
	double start = GetTime();

	if (IsWindowResized()) resize();

	BeginDrawing();
	ClearBackground(BLACK);

	draw_view_area();
	controls();

	update_texture();
	EndDrawing();

	double end = GetTime();
	seconds_passed += end - start;
    }

    UnloadTexture(txt);
    CloseWindow();
    return 0;
}
