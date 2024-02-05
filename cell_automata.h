#pragma once
#include <iostream>


enum Automata_Type {
    ONE_DIM, TWO_DIM, AUTOMATA_TYPE_MAX
};

#define INDEX(x, y, width) x + y * width


template<typename T> class Cell_Automat {
public:
    Cell_Automat(Automata_Type type, size_t width, size_t height, T zero_value, T one_value, T* new_input = NULL,
		 void (*rules) (Cell_Automat& automat) = NULL
		 ):
	type(type), zero(zero_value), one(one_value), width(width), height(height), rules(rules) {

	size = width * height;
	cells = new T[size];
	empty = new T[size];
	if(new_input) {
	    memcpy(cells, new_input, sizeof(T) * size);
	}
	else {
	    set_buf(cells, size, zero);
	}
	set_buf(empty, size, zero);

	num_neighbors = neighbourhood_sizes[type];
	neighbour_mask = new int[num_neighbors];
	int index = 0;
	for (int y = -1; y <= 1; ++y) {
	    for (int x = -1; x <= 1; ++x) {
		int neighbor = INDEX(x, y, width);
		neighbour_mask[index++] = neighbor;
	    }
	}

    };
    ~Cell_Automat() {
	delete[] cells;
	delete[] empty;
	delete[] neighbour_mask;
    };

    static constexpr int neighbourhood_sizes[AUTOMATA_TYPE_MAX] = {2, 9};
    size_t size;
    size_t width;
    size_t height;
    size_t num_neighbors; 
    int* neighbour_mask;
    T* empty;
    T* cells;
    T zero;
    T one;
    Automata_Type type;

    void set_input(T* new_input) {
	memcpy(cells, new_input, sizeof(T) * size);
    }
    void apply_rules() {
	rules(*this);
	switch_buffers();
    }
    static void set_buf(T* buf, size_t size, T val) {
	for(int i = 0; i < size; ++i) {
	    buf[i] = val;
	}
    }
private:
    void (*rules) (Cell_Automat& automat);
    void switch_buffers() {
	T* h = cells;
	cells = empty;
	empty = h;
    }
};
