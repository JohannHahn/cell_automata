#include "raylib/src/raylib.h"
#include "cell_automata.h"
#include <iostream>
#include <cassert>
#include <string>
#include "gui.h"

typedef uint32_t u32;

#define COLOR_FROM_U32(c) *(Color*)&c

u32 alive_col = 0xFFFF1111;
u32 dead_col  = 0xFF181818;

KeyboardKey autoplay_key = KEY_SPACE;
KeyboardKey next_frame_key = KEY_RIGHT;

size_t cells_width = 200;
size_t cells_height = 200;
float window_width = 900;
float window_height = 600;

float min_dim = std::min(window_width, window_height);
int controls_num_widgets = 10;
Rectangle view_area = {0, 0, min_dim, min_dim};
Rectangle control_area = {view_area.width, 0, window_width - view_area.width, window_height};
Layout control_layout = Layout(control_area, VERTICAL, controls_num_widgets, 5);

bool autoplay = false;
double seconds_passed = 0.f;
float initial_fps = 60.f;
float target_fps = initial_fps;
float max_fps = 100.f;

std::vector<Cell_Automat<u32>*> automata;
Cell_Automat<u32>* selected_automat = NULL;
int selected_automat_index = -1;

Texture txt;

void select_automat(int i) {
    assert(i >= 0 && i < automata.size() && "wrong index");
    selected_automat_index = i;
    selected_automat = automata[i];
}

void one_dim_rules(Cell_Automat<u32>& automat) {
    if (automat.generation < automat.height - 1) {
	int ruleset[8] = {1, 0, 1, 0, 0, 1, 0, 1};
	int y = automat.generation;
	for (int x = 0; x < automat.width; ++x) {
	    u32 rule_index = 0;
	    u32 index = INDEX(x, y, automat.width);
	    for (int n_i = 0; n_i < 3; ++n_i) {
		int new_index = INDEX(x, y, automat.width) + automat.neighbour_mask[n_i];
		if (new_index < 0) new_index += automat.width;
		else if (new_index >= automat.size) new_index -= automat.width;
		if (automat.cells[new_index] == automat.one) {
		    rule_index |= (u32)(1) << (2 - n_i);
		}
	    }
	    rule_index = 7 - rule_index;
	    u32 new_value = ruleset[rule_index] ? automat.one : automat.zero;
	    assert(rule_index < 8);
	    assert(rule_index >= 0);
	    automat.cells[INDEX(x, y + 1, automat.width)] = new_value;
	}
    }
}

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
}

void controls() {
    if (selected_automat && autoplay || IsKeyReleased(next_frame_key)) {
	if (seconds_passed >= 1.f / target_fps) { 
	    seconds_passed = 0.f;
	    selected_automat->apply_rules(); 
	    UpdateTexture(txt, selected_automat->cells);
	}
    }

    GuiSlider(control_layout.get_slot(controls_num_widgets - 4), "0", std::to_string(max_fps).c_str(), &target_fps, 0.f, max_fps);

    if (GuiButton(control_layout.get_slot(controls_num_widgets - 3), "next automat")) {
	selected_automat_index++;
	selected_automat_index %= automata.size();
	select_automat(selected_automat_index);
	UpdateTexture(txt, selected_automat->cells);
    }

    GuiToggle(control_layout.get_slot(controls_num_widgets - 2), "Play", &autoplay);
    if (IsKeyReleased(autoplay_key)) {
	autoplay = !autoplay;
    }

    if (GuiButton(control_layout.get_slot(controls_num_widgets - 1), "Restart")) {
	selected_automat->generation = 0;
	selected_automat->randomize_cells();
	UpdateTexture(txt, selected_automat->cells);
    }
}

void add_automat(Cell_Automat<u32>* automat) {
    automata.push_back(automat);
    std::cout << "added automat\n";
}


int main() {
    SetRandomSeed(GetTime());
    InitWindow(window_width, window_height, "hi");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(initial_fps);
    Image h = GenImageColor(cells_width, cells_height, COLOR_FROM_U32(dead_col));
    txt = LoadTextureFromImage(h);
    UnloadImage(h);

    Cell_Automat<u32> gol_automat(TWO_DIM, cells_width, cells_height, 
				   dead_col, alive_col, NULL, gol_rules);
    Cell_Automat<u32> elem_automat(ONE_DIM, cells_width, cells_height, 
				   dead_col, alive_col, NULL, one_dim_rules);
    gol_automat.randomize_cells();
    elem_automat.randomize_cells();
    automata.push_back(&gol_automat);
    automata.push_back(&elem_automat);
    select_automat(0);
    UpdateTexture(txt, selected_automat->cells);

    std::cout << "alive color = " << alive_col << "\ndead color = " << dead_col << "\n";


    while (!WindowShouldClose()) {
	if(target_fps > initial_fps) {
	    SetTargetFPS(target_fps);
	    initial_fps = target_fps;
	}
	double start = GetTime();
	if (IsWindowResized()) {
	    resize();
	}

	BeginDrawing();
	ClearBackground(BLACK);
	controls();
	Rectangle source = selected_automat->type == ONE_DIM ? 
	    Rectangle{0, 0, (float)txt.width, (float)selected_automat->generation} : 
	    Rectangle{0, 0, (float)txt.width, (float)txt.height};
	Rectangle dest = selected_automat->type == ONE_DIM ? 
	    Rectangle{view_area.x, view_area.y, view_area.width, (float)selected_automat->generation * (view_area.height / (float)cells_height)} : 
	    view_area;
	DrawTexturePro(txt, source, dest, {0.f, 0.f}, 0.f, WHITE);

	DrawFPS(view_area.width, 0);
	EndDrawing();
	double end = GetTime();
	seconds_passed += end - start;
    }

    UnloadTexture(txt);
    CloseWindow();
    return 0;
}
