#include "raygui.h"
#include "raylib.h"
#include "cell_automata.h"
#include <cinttypes>
#include <cstring>
#include <iostream>
#include <cassert>
#include <string>
#include "gui.h"

typedef uint32_t u32;

#define COLOR_FROM_U32(c) *(Color*)&c

enum control_view_type {
    CURRENT, NEXT, CONTROL_VIEW_MAX
};
control_view_type control_window_type = CURRENT;
bool mouse_draw = true;

u32 alive_col = 0xFFFF1111;
u32 dead_col  = 0xFF181818;

KeyboardKey autoplay_key = KEY_SPACE;
KeyboardKey next_frame_key = KEY_RIGHT;

size_t cell_cols = 200;
size_t cell_rows = 200;

float window_width = 900;
float window_height = 600;

float min_dim = std::min(window_width, window_height);
Rectangle view_area = {0, 0, min_dim, min_dim};
float cell_width = view_area.width / (float)cell_cols;
float cell_height = view_area.height / (float)cell_rows;
Rectangle control_area = {view_area.width, 0, window_width - view_area.width, window_height};
int controls_num_widgets = 10;
Layout control_layout = Layout(control_area, VERTICAL, controls_num_widgets, 5);
int control_index = 0;

bool autoplay = false;
double seconds_passed = 0.f;
float max_fps = 100.f;
float target_fps = 60;

Cell_Automat<u32> active_automat;

Texture txt;

Rectangle brush_view_rec = {0.f, 0.f, 1.f, 1.f};
float brush_width = 1.f;
float brush_height = 1.f;

void gol_rules(Cell_Automat<u32>& automat) {
    for (int y = 0; y < automat.height; ++y) {
	for (int x = 0; x < automat.width; ++x) {
	    int index = x + y * automat.width;
	    u32 input_val = automat.cells[index];
	    if (input_val != automat.one && input_val != automat.zero) {
		std::cout << "wrong colors! input_val = " << input_val << "\n";
		std::cout << "alive color = " << alive_col << "\ndead color = " << dead_col << "\n";
	    }

	    int neighbours = 0; 
	    for (int i = 0; i < automat.num_neighbors; ++i) {
		int new_index = index + automat.neighbour_mask[i];
		if(new_index < 0) new_index += automat.width;
		else if(new_index >= automat.size) new_index -= automat.width;
		if (new_index != index && automat.cells[new_index] != automat.zero) {
		    neighbours++;
		}
	    }

	    bool alive = (input_val == automat.one);
	    if(alive && neighbours == 3) automat.empty[index] = automat.one;
	    else if(alive && neighbours == 2) automat.empty[index] = automat.one;
	    else if(alive && neighbours < 2) automat.empty[index] = automat.zero;
	    else if(alive && neighbours > 3) automat.empty[index] = automat.zero;
	    else if(!alive && neighbours == 3) automat.empty[index] = automat.one;
	    else if(!alive && neighbours != 3) automat.empty[index] = automat.zero;
	    else {
		assert(0 && "unreachable");
	    }
	}
    }
}

void resize() {
    window_width = GetScreenWidth();
    window_height = GetScreenHeight();
    min_dim = std::min(window_width, window_height);
    view_area = {0, 0, min_dim, min_dim};
    bool vertical = min_dim == window_width;
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

void control_current_automat() {
    if (autoplay || IsKeyReleased(next_frame_key)) {
	if (seconds_passed >= 1.f / target_fps) { 
	    seconds_passed = 0.f;
	    active_automat.apply_rules(); 
	    UpdateTexture(txt, active_automat.cells);
	}
    }
    // info about current layout
    Layout info_layout = Layout(control_layout.get_slot(control_index++, true), HORIZONTAL, 2, 1.f);
    float text_height = info_layout.get_slot(0).height;
    std::string type = active_automat.type == ONE_DIM ? "Type:\n Elementary\n One dimensional" : "Type:\n Two dimensional"; type += "\n";
    GuiDrawText(type.c_str(), info_layout.get_slot(0, true), TEXT_ALIGN_LEFT, WHITE);
    std::string dims = "Width = " + std::to_string(active_automat.width) + "\nHeight = " + std::to_string(active_automat.height) + "\n";
    GuiDrawText(dims.c_str(), info_layout.get_slot(1, true), TEXT_ALIGN_LEFT, WHITE);
    
    info_layout = Layout(control_layout.get_slot(control_index++, true), HORIZONTAL, 2, 1.f);
    GuiDrawText("Ruleset:", info_layout.get_slot(0, true), TEXT_ALIGN_LEFT, WHITE);
    std::string ruleset_str = active_automat.type == TWO_DIM ? "Conway's game of life" : "";
    GuiDrawText(ruleset_str.c_str(), info_layout.get_slot(1, true), TEXT_ALIGN_LEFT, WHITE);
    // input one dimensional rules as binary
    if (active_automat.type == ONE_DIM) {
	bool secret_view = true;
	Layout ruleset_layout = Layout(control_layout.get_slot(control_index++), HORIZONTAL, 8, 5.f);
	for(int i = 0; i < 8; ++i) {
	    std::string binary = "000";
	    Layout vert_layout = Layout(ruleset_layout.get_slot(i), VERTICAL, 2);
	    int bit = BIT_AT(7 - i, active_automat.one_dim_rules);
	    std::string bit_str; 
	    bit_str += '0' + bit;
	    for (int j = 0; j < 3; ++j) {
		binary[2-j] = '0' + BIT_AT(j, i);
	    }
	    binary += "\nflip";
	    GuiDrawText(bit_str.c_str(), vert_layout.get_slot(0), TEXT_ALIGN_MIDDLE, WHITE);
	    if (GuiButton(vert_layout.get_slot(1), binary.c_str())) {
		if (bit) {
		    BIT_RESET(7 - i, active_automat.one_dim_rules);
		}
		else {
		    BIT_SET(7 - i, active_automat.one_dim_rules);
		}
	    }
	}
    }
}

void control_window_selection() {
    Layout top_row_layout = Layout(control_layout.get_slot(control_index++, true), HORIZONTAL, CONTROL_VIEW_MAX, 5.f);
    if (GuiButton(top_row_layout.get_slot(0, true), "Edit current automat")) {
	control_window_type = CURRENT;
    }
    if (GuiButton(top_row_layout.get_slot(1, true), "Prepare next automat")) {
	control_window_type = NEXT;
    }
    //top_row_layout.draw();
}
 
void controls() {
    control_index = 0;
    control_window_selection();

    if (control_window_type == CURRENT) {
	control_current_automat();
    }
    if (GuiButton(control_layout.get_slot(control_index++, true), "erase")) {
	active_automat.clear_cells();
	UpdateTexture(txt, active_automat.cells);
    }
    GuiSlider(control_layout.get_slot(control_index++, true), "0", std::to_string(max_fps).c_str(), &target_fps, 0.f, max_fps);

    bool autoplay_prev = autoplay;
    GuiToggle(control_layout.get_slot(control_index++, true), "Play", &autoplay);
    if (IsKeyReleased(autoplay_key)) {
	autoplay = !autoplay;
    }
    if (autoplay_prev != autoplay) {
	// one dimensional automat reached max height
	if (active_automat.type == ONE_DIM && active_automat.generation == active_automat.height - 1) {
	    active_automat.generation = 0;
	}
    }

    if (GuiButton(control_layout.get_slot(control_index++, true), "Restart")) {
	active_automat.generation = 0;
	memcpy(active_automat.cells, active_automat.initial_cells, active_automat.size);
	//autoplay = false;
	UpdateTexture(txt, active_automat.cells);
    }

    GuiCheckBox(control_layout.get_slot(control_index++, true), "Mouse drawing", &mouse_draw);
    if (mouse_draw) {
	Vector2 mouse_pos = GetMousePosition();
	if (CheckCollisionPointRec(mouse_pos, view_area)) {
	    brush_view_rec.x = floor(mouse_pos.x - cell_width / 2.f); 
	    brush_view_rec.y = floor(mouse_pos.y - cell_height / 2.f);
	    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
		
		Vector2 mouse_pos_projected = {mouse_pos.x / view_area.width * cell_cols, mouse_pos.y / view_area.height * cell_rows};
		active_automat.cells[INDEX((int)mouse_pos_projected.x, (int)mouse_pos_projected.y, active_automat.width)] = active_automat.one;
		UpdateTexture(txt, active_automat.cells);
	    }
	    DrawRectangleLinesEx(brush_view_rec, 2.f, WHITE);
	}
    }
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

    active_automat.init(ONE_DIM, cell_cols, cell_rows, dead_col, alive_col);
    active_automat.randomize_cells();
    active_automat.set_ruleset_bin("1101");

    UpdateTexture(txt, active_automat.cells);

    std::cout << "alive color = " << alive_col << "\ndead color = " << dead_col << "\n";
    control_layout.set_spacing(min_dim / 50.f);

    while (!WindowShouldClose()) {
	double start = GetTime();

	if (IsWindowResized()) {
	    resize();
	}

	BeginDrawing();
	ClearBackground(BLACK);

	draw_view_area();
	controls();

	EndDrawing();

	double end = GetTime();
	seconds_passed += end - start;
    }

    UnloadTexture(txt);
    CloseWindow();
    return 0;
}
