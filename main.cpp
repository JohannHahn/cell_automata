#include "raylib/src/raylib.h"
#include "cell_automata.h"
#include <iostream>
#include <cassert>

typedef uint32_t u32;

#define COLOR_FROM_U32(c) *(Color*)&c

u32 alive_col = 0xFFFF1111;
u32 dead_col  = 0xFF181818;
size_t cells_width = 50;
size_t cells_height = 50;
float window_width = 900;
float window_height = 600;
float min_dim = std::min(window_width, window_height);
int target_fps = 0;
Rectangle view_area = {0, 0, min_dim, min_dim};
Cell_Automat<u32>* selected_automat;
bool autoplay = false;
KeyboardKey autoplay_key = KEY_SPACE;
KeyboardKey next_frame_key = KEY_RIGHT;

void one_dim_rules(Cell_Automat<u32>& automat) {
    if (automat.generation < automat.height) {
	int ruleset[8] = {0, 1, 1, 0, 1, 1, 1, 0};
	int y = automat.generation;
	for (int x = 0; x < automat.width; ++x) {
	    //std::cout << "index = " << index << "\n";
	    u32 rule_index = 0;
	    u32 index = INDEX(x, y, automat.width);
	    std::string bits;
	    bits += "ruleset bits = ";
	    for (int n_i = 0; n_i < 3; ++n_i) {
		int new_index = INDEX(x, y, automat.width) + automat.neighbour_mask[n_i];
		if (new_index < 0) new_index += automat.width;
		else if (new_index >= automat.size) new_index -= automat.width;
		if (automat.cells[new_index] == automat.one) {
		    bits += "1";
		    rule_index |= (u32)(1) << (2 - n_i);
		}
		else bits += "0";
	    }
	    //rule_index = 7 - rule_index;
	    u32 new_value = ruleset[rule_index] ? automat.one : automat.zero;
	    if (new_value == automat.one) {
		std::cout << "\n---------- report begin ----------------\n";
		std::cout << "x = " << x << ", y = " << y << "\n";
		std::cout << "index = " << index << "\n";
		std::cout << "rule_index = " << rule_index << "\n";
		std::cout << bits << "\n";
		std::cout << "---------- report end----------------\n";
	    }
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
	    assert(input_val == automat.one || input_val == automat.zero && "wrong color");

	    int neighbours = 0; 
	    for (int i = 0; i < automat.num_neighbors; ++i) {
		int new_index = index + automat.neighbour_mask[i];
		if(new_index < 0) new_index += automat.width;
		else if(new_index >= automat.size) new_index -= automat.width;
		if (automat.cells[new_index] != automat.zero) {
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
}

void controls() {
    if (autoplay || IsKeyReleased(next_frame_key) || IsKeyDown(next_frame_key)) {
	selected_automat->apply_rules(); 
    }
    if(IsKeyReleased(autoplay_key)) {
	autoplay = !autoplay;
    }
}


int main() {
    InitWindow(window_width, window_height, "hi");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(target_fps);

    Image h = GenImageColor(cells_width, cells_height, COLOR_FROM_U32(dead_col)); 
    u32* pixels = (u32*)h.data;
    //for (int i = 0; i < cells_width*cells_height; ++i) {
    //    if (i % 5 == 0) {
    //        SetPixelColor(&pixels[i], COLOR_FROM_U32(alive_col), h.format);
    //    }
    //}
    SetPixelColor(&pixels[cells_width / 2], COLOR_FROM_U32(alive_col), h.format);
    //SetPixelColor(&pixels[cells_width / 2 + 2], COLOR_FROM_U32(alive_col), h.format);
    //SetPixelColor(&pixels[(int)window_width + 101], COLOR_FROM_U32(alive_col), h.format);
    Texture txt = LoadTextureFromImage(h);

    Cell_Automat<u32> gol_automat(TWO_DIM, cells_width, cells_height, 
				   dead_col, alive_col, (u32*)h.data, gol_rules);
    Cell_Automat<u32> elem_automat(ONE_DIM, cells_width, cells_height, 
				   dead_col, alive_col, (u32*)h.data, one_dim_rules);
    UnloadImage(h);
    selected_automat = &elem_automat;
    std::cout << "size of automat in bytes = " << sizeof(gol_automat) << "\n";

    while (!WindowShouldClose()) {
	assert(selected_automat && "selected automat is null");
	if (IsWindowResized()) {
	    resize();
	}
	BeginDrawing();
	DrawTexturePro(txt, {0, 0, (float)txt.width, (float)txt.height}, view_area, {0, 0}, 0, WHITE);
	DrawRectangle(view_area.width, 0, window_width - view_area.width, window_height, LIGHTGRAY);
	DrawFPS(view_area.width, 0);
	EndDrawing();

	UpdateTexture(txt, selected_automat->cells);

	controls();
    }

    UnloadTexture(txt);
    CloseWindow();
    return 0;
}
