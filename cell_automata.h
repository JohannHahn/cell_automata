#pragma once
#include <iostream>


enum Automata_Type {
    ONE_DIM, TWO_DIM, AUTOMATA_TYPE_MAX
};


template<typename T> class Cell_Automat {
public:
    Cell_Automat(Automata_Type type, size_t width, size_t height, 
		 void (*rules) (const T* data, T* output, size_t size, T* neighbours, size_t neighbouts_size),
		 T zero_value, T* new_input = NULL):
	type(type), zero(zero_value), width(width), height(height), rules(rules) {

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

	const int n_size = neighbourhood_size[type];
	neighbour_mask = new T[n_size];

    };
    ~Cell_Automat() {
	delete[] cells;
	delete[] empty;
	delete[] neighbour_mask;
    };

    static constexpr int neighbourhood_size[AUTOMATA_TYPE_MAX] = {2, 9};
    size_t size;
    size_t width;
    size_t height;
    int* neighbour_mask;
    T* empty;
    T* cells;
    T zero;
    Automata_Type type;

    void set_input(T* new_input) {
	memcpy(cells, new_input, sizeof(T) * size);
    }
    void apply_rules() {
	const int n_size = neighbourhood_size[type];
	int neighbours_mask[n_size] = {0};
	get_neighbour_mask(neighbours_mask, n_size);
	rules(cells, empty, size);
	switch_buffers();
    }
    static void set_buf(T* buf, size_t size, T val) {
	for(int i = 0; i < size; ++i) {
	    buf[i] = val;
	}
    }
    void get_neighbour_mask(int* neighbor_mask, int n_size) {
	for(int i = 0; i < n_size; ++i) {
	    
	}
    }

private:


    void (*rules) (const T* input, T* output, size_t size, T* neighbours, size_t neighbours_size);
    void switch_buffers() {
	T* h = cells;
	cells = empty;
	empty = h;
    }
};
