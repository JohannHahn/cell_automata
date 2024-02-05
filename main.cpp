#include "raylib/src/raylib.h"
#include "cell_automata.h"
#include <iostream>
#include <cinttypes>
#include <cassert>

typedef uint32_t u32;

#define COLOR_FROM_U32(c) *(Color*)&c

u32 alive_col = 0xFF111111;
u32 dead_col  = 0xFFABABAB;
size_t cells_width = 500;
size_t cells_height = 500;
float window_width = 900;
float window_height = 600;
float min_dim = std::min(window_width, window_height);
Rectangle view_area = {0, 0, min_dim, min_dim};

void rules(Cell_Automat<u32>& automat) {
    for (int y = 0; y < automat.height; ++y) {
	for (int x = 0; x < automat.width; ++x) {
	    int index = x + y * automat.width;
	    u32 input_val = automat.cells[index];
	    assert(input_val == automat.one || input_val == automat.zero && "wrong color");

	    int neighbours = 0; 
	    for (int i = 0; i < automat.num_neighbors; ++i) {
		int new_index = index + automat.neighbour_mask[i];
		if (new_index >= 0 && new_index < automat.size 
		    && automat.cells[new_index] != automat.zero) {
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


int main() {
    InitWindow(window_width, window_height, "hi");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(5);

    Image h = GenImageColor(cells_width, cells_height, COLOR_FROM_U32(alive_col)); 
    u32* pixels = (u32*)h.data;
    for (int i = 0; i < cells_width*cells_height; ++i) {
	if (GetRandomValue(0,1)) {
	    SetPixelColor(&pixels[i], COLOR_FROM_U32(dead_col), h.format);
	}
    }
    SetPixelColor(&pixels[101], COLOR_FROM_U32(dead_col), h.format);
    SetPixelColor(&pixels[(int)window_width + 101], COLOR_FROM_U32(dead_col), h.format);
    Texture txt = LoadTextureFromImage(h);
    Cell_Automat<u32> cell_automat(TWO_DIM, cells_width, cells_height, 
				   dead_col, alive_col, (u32*)h.data, rules);
    UnloadImage(h);

    while (!WindowShouldClose()) {
	if(IsWindowResized()) {
	    resize();
	}
	BeginDrawing();
	DrawTexturePro(txt, {0, 0, (float)txt.width, (float)txt.height}, view_area, {0, 0}, 0, WHITE);
	DrawFPS(0, 0);
	EndDrawing();

	UpdateTexture(txt, cell_automat.cells);
	cell_automat.apply_rules();
    }

    UnloadTexture(txt);
    CloseWindow();
    return 0;
}
