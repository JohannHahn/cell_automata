#pragma once

#include "raylib/src/raylib.h"
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <cassert>
#include <vector>

typedef uint64_t u64;
typedef uint32_t u32;

enum Automata_Type {
    ONE_DIM, TWO_DIM, AUTOMATA_TYPE_MAX
};

#define INDEX(x, y, width) (x) + ((y) * (width))
// requires Cell_Automat object called automat to be in scope
#define INDEX_AUTOMAT(x, y) (x) + ((y) * (automat.width))
#define BIT_AT(i, map) ((map) >> (i) & u64(1))
#define BIT_SET(i, map) ((map) |= (u64(1) << (i)))
#define BIT_RESET(i, map) ((map) &= ~(u64(1) << (i))) 

template<typename T> class Cell_Automat {
typedef void (*rules_func)(Cell_Automat&);
public:
    Cell_Automat() {
	std::cout << "CELL_AUTOMAT: empty automat created, initialize with init()\n";
        init(AUTOMATA_TYPE_MAX, 0, 0, T(), {}, 0);		
    }

    Cell_Automat(Cell_Automat& automat) {
        init(automat);		
    }

    Cell_Automat(Automata_Type type, size_t width, size_t height, T zero_value, T* fill_values, size_t fill_values_size) {
	init(type, width, height, zero_value, fill_values, fill_values_size);
    };

    ~Cell_Automat() {
	delete[] cells;
	delete[] initial_cells;
	delete[] empty;
	delete[] neighbour_mask;
	std::cout << "CELL_AUTOMAT: destructor called\n";
    };

    static constexpr int neighbourhood_sizes[AUTOMATA_TYPE_MAX] = {3, 9};
    size_t size;
    size_t width;
    size_t height;
    size_t num_neighbors; 
    size_t generation = 0;
    rules_func rules;
    int* neighbour_mask = NULL;
    T* empty;
    T* cells;
    // state before the simulation started
    T* initial_cells;
    T zero;
    T selected_fill;
    std::vector<T> fill_values;
    Automata_Type type;
    u64 one_dim_rules = 0;

    void init(const Cell_Automat& automat) {
	init(automat.type, automat.width, automat.height, automat.zero, automat.fill_values.data(), automat.fill_values.size());
	this->rules = automat.rules;
	this->set_cells(automat.cells);
    }
    void init(Automata_Type type, size_t width, size_t height, T zero, T* fill_values, size_t fill_values_size) {
	size = width * height;
	this->width = width;
	this->height = height;
	this->zero = zero;
	this->type = type;
	if (type == AUTOMATA_TYPE_MAX) return;
	assert(fill_values && fill_values_size >= 1);
	for (int i = 0; i < fill_values_size; ++i) {
	    this->fill_values.push_back(fill_values[i]);
	}
	this->selected_fill = this->fill_values[0];

	if (cells) delete[] cells;
	if (initial_cells) delete[] initial_cells;
	if (empty) delete[] empty;
	cells = new T[size];
	initial_cells = new T[size];
	empty = new T[size];

	set_buf(cells, size, zero);
	set_buf(initial_cells, size, zero);
	set_buf(empty, size, zero);
	setup_neighborhood();
	srand(time(NULL));
	switch (type) {
	    case ONE_DIM:
		this->rules = one_dim_rules_func;
	    break;
	    case TWO_DIM:
		this->rules = gol_rules_func;
	    break;
	    case AUTOMATA_TYPE_MAX:
	    break;
	    default:
	    assert(0 && "unreachable");
	}
	std::cout << "init: finished initializing automat\n";
    }

    void setup_neighborhood() {
	assert(type >= 0 && type <= AUTOMATA_TYPE_MAX);
	if (type == AUTOMATA_TYPE_MAX) num_neighbors = 0;
	else num_neighbors = neighbourhood_sizes[type];

	if (neighbour_mask) {
	    delete [] neighbour_mask;
	}
	neighbour_mask = new int[num_neighbors];

	int index = 0;
	switch(type) {
	    case ONE_DIM:
		for(int i = -1; i < 2; ++i) {
		    assert(index < num_neighbors);
		    neighbour_mask[index++] = i;
		}
	    break;
	    case TWO_DIM:
		for (int y = -1; y <= 1; ++y) {
		    for (int x = -1; x <= 1; ++x) {
			assert(index < num_neighbors);
			int neighbor = INDEX(x, y, width);
			neighbour_mask[index++] = neighbor;
		    }
		}
	    break;
	    case AUTOMATA_TYPE_MAX:
		std::cout << "empty automat, no neighbours :(\n";
	    break;
	    default:
		assert(0 && "unreachable");
	}
    }
    void set_ruleset_dec(u64 dec) {
	one_dim_rules = dec;
	std::cout << "new decimal ruleset = " << one_dim_rules << "\n";
    }
    void set_ruleset_bin(const char* rules_string) {
	assert(type == ONE_DIM && "wrong type");
	int i = 0;
	int size = 0;
	while(rules_string[i++] != '\0') {
	    size++; 
	}
	std::cout << "size of ruleset = " << size << "\n";
	i = 0;
	while(size > 0) {
	    size--;
	    if (rules_string[i] == '1') {
		BIT_SET(size, one_dim_rules);
	    }
	    i++;
	}
	std::cout << "new string ruleset = " << one_dim_rules << "\n";
    }

    void print() {
	std::cout << "\n----Automat info--------\n";
	std::cout << "type: " << (type == ONE_DIM ? "1D elementary" : "2D") << "\n";
	std::cout << "width = "  << width << ", height = " << height << "\n";
	std::cout << "empty/dead value = "  << zero << "\n"; 
	for (T value : fill_values) {
	    std::cout << ", alive/one value = " << value  << "\n";
	}
	std::cout << "cells pointer = "  << cells << ", empty/next frame pointer = " << empty << "\n";
	std::cout << "number of neighbours of any cell = "  << num_neighbors << ", neighbourhood mask pointer = " << neighbour_mask << "\n";
	std::cout << "----Automat info end----\n";
    }

    void clear_cells() {
	set_buf(cells, size, zero);
    }

    void randomize_cells() {
	int limit = size;
	if (type == ONE_DIM) limit = width;
	for (int i = 0; i < size; ++i) {
	    if (i < limit && rand() / (RAND_MAX / 2)) {
		cells[i] = fill_values[GetRandomValue(0, fill_values.size() - 1)];
	    }
	    else cells[i] = zero;
	}
	memcpy(initial_cells, cells, sizeof(T) * size);
    }

    void set_cells(T* new_input) {
	memcpy(cells, new_input, sizeof(T) * size);
	memcpy(initial_cells, new_input, sizeof(T) * size);
    }

    void switch_buffers() {
	T* h = cells;
	cells = empty;
	empty = h;
    }

    void apply_rules() {
	rules(*this);
	if (type != ONE_DIM) {
	    switch_buffers();
	    set_buf(empty, size, zero);
	}
	else {
	    if (generation < height - 1) generation++;
	}
    }
    static void set_buf(T* buf, size_t size, T val) {
	for(int i = 0; i < size; ++i) {
	    buf[i] = val;
	}
    }

    void set_rules_gol() {
	rules = gol_rules_func;
    }

    void set_rules2D(rules_func rules) {
	this->rules = rules;
    }

    bool is_initialized() {
	return type != AUTOMATA_TYPE_MAX;
    }


    // rules of conway's game of life
    static void gol_rules_func(Cell_Automat& automat) {
	for (int y = 0; y < automat.height; ++y) {
	    for (int x = 0; x < automat.width; ++x) {
		int index = x + y * automat.width;
		T input_val = automat.cells[index];

		int neighbours = 0; 
		for (int i = 0; i < automat.num_neighbors; ++i) {
		    int new_index = index + automat.neighbour_mask[i];
		    if(new_index < 0) new_index += automat.width;
		    else if(new_index >= automat.size) new_index -= automat.width;
		    if (new_index != index && automat.cells[new_index] != automat.zero) {
			neighbours++;
		    }
		}

		bool alive = (input_val != automat.zero);
		if(alive && neighbours == 3) automat.empty[index] = automat.fill_values[0];
		else if(alive && neighbours == 2) automat.empty[index] = automat.fill_values[0];
		else if(alive && neighbours < 2) automat.empty[index] = automat.zero;
		else if(alive && neighbours > 3) automat.empty[index] = automat.zero;
		else if(!alive && neighbours == 3) automat.empty[index] = automat.fill_values[0];
		else if(!alive && neighbours != 3) automat.empty[index] = automat.zero;
		else {
		    assert(0 && "unreachable");
		}
	    }
	}
    }

    static void one_dim_rules_func(Cell_Automat& automat) {
	if (automat.generation < automat.height - 1) {
	    int y = automat.generation;
	    for (int x = 0; x < automat.width; ++x) {
		u32 rule_index = 0;
		int index = INDEX(x, y, automat.width);
		for (int n_i = 0; n_i < 3; ++n_i) {
		    int new_index = INDEX(x, y, automat.width) + automat.neighbour_mask[n_i];
		    if (new_index < 0) new_index += automat.width;
		    else if (new_index >= automat.size) new_index -= automat.width;
		    if (automat.cells[new_index] == automat.fill_values[0]) {
			rule_index |= (u32)(1) << (2 - n_i);
		    }
		}
		T new_value = BIT_AT(rule_index, automat.one_dim_rules) ? automat.fill_values[0]: automat.zero;
		assert(rule_index < 8);
		assert(rule_index >= 0);
		automat.cells[INDEX(x, y + 1, automat.width)] = new_value;
	    }
	}
    }
    
    bool move_if_empty(int src, int dst, T fill_value) {
	if (dst >= 0 && dst < size) {
	    if (cells[dst] == zero) {
		empty[dst] = fill_value; 
		return true;
	    }
	}
	return false;
    }
};
