#include "raylib/src/raylib.h"
#include "cell_automata.h"
#include <iostream>
#include <cstdint>

typedef uint32_t u32;

Image* img;

void rules(const u32* input, u32* output, size_t size) {
    for (size_t i = 0; i < size; ++i) {
	output[i] = GetRandomValue(0x000000FF, 0xFFFFFFFF);	
    }
}
int main() {
    std::cout << "hi\n";

    InitWindow(800, 600, "hi");

    Image h = GenImageColor(800, 600, BLACK); 
    img = &h;
    Texture txt = LoadTextureFromImage(*img);

    Cell_Automat<u32> cell_automat((u32*)img->data, 800*600, rules);

    while (!WindowShouldClose()) {

	BeginDrawing();
	DrawTexture(txt, 0, 0, WHITE);
	EndDrawing();

	cell_automat.apply_rules();
	UpdateTexture(txt, img->data);
    }
    return 0;
}
