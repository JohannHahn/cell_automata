#include "raylib/src/raylib.h"
#include "cell_automata.h"
#include <iostream>
#include <cstdint>

typedef uint32_t u32;

Image img = GenImageColor(800, 600, BLACK);
Texture txt;

void rules(const u32* input, u32* output, size_t size) {
    std::cout << "in rules\n";
    std::cout << "img.data pointer = " << img.data << ", output = " << output << "\n";
    for (int i = 0; i < size; ++i) {
	output[i] = GetRandomValue(0x000000FF, 0xFFFFFFFF);	
    }
}
int main() {
    Cell_Automat<u32> cell_automat((u32*)img.data, 800*600, rules);
    InitWindow(800, 600, "hi");
    //SetTraceLogLevel(LOG_ERROR);
    while (!WindowShouldClose()) {
	UnloadTexture(txt);	
	txt = LoadTextureFromImage(img);

	BeginDrawing();
	DrawTexture(txt, 0, 0, WHITE);
	EndDrawing();

	cell_automat.apply_rules();
    }
    return 0;
}
