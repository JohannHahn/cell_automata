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


int count_neighbours(int x, int y, const u32* pixels) {
    int count = 0;
    for (int y_n = -1; y_n <= 1; ++y_n) {
	for (int x_n = -1; x_n <= 1; ++x_n) {
	    int index = x + x_n + (y + y_n) * cells_width;
	    if(index >= 0 && index < cells_width * cells_height) {
		if (pixels[index] == alive_col) count++; 
	    }
	}
    }
    return count;
}
void rules(const u32* input, u32* output, size_t size, u32* neighbours, size_t neighbours_size){
    for (int y = 0; y < cells_height; ++y) {
	for (int x = 0; x < cells_width; ++x) {
	    int index = x + y * cells_width;
	    u32 input_val = input[index];
	    assert(input_val == alive_col || input_val == dead_col && "wrong color");

	    int neighbours = count_neighbours(x, y, input); 
	    bool alive = (input_val == alive_col);

	    if(alive && neighbours == 3) output[index] = alive_col;
	    else if(alive && neighbours == 2) output[index] = alive_col;
	    else if(alive && neighbours < 2) output[index] = dead_col;
	    else if(alive && neighbours > 3) output[index] = dead_col;
	    else if(!alive && neighbours == 3) output[index] = alive_col;
	    else if(!alive && neighbours != 3) output[index] = dead_col;
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
   // SetTargetFPS(10);

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
    Cell_Automat<u32> cell_automat(cells_width*cells_height, rules, dead_col, (u32*)h.data);
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
