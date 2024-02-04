#pragma once
#include <iostream>

enum Automata_Type {
    ONE_DIM, TWO_DIM
};

template<typename T> class Cell_Automat {
public:
    Cell_Automat(size_t size, void (*rules) (const T* data, T* output, size_t size), T zero_value, T* new_input = NULL):
	zero(zero_value), size(size), rules(rules) {
	cells = new T[size];
	empty = new T[size];
	if(new_input) {
	    memcpy(cells, new_input, sizeof(T) * size);
	}
	else {
	    set_buf(cells, size, zero);
	}
	set_buf(empty, size, zero);
    };
    ~Cell_Automat() {
	delete[] cells;
	delete[] empty;
    };

    size_t size;
    T* empty;
    T* cells;

    void set_input(T* new_input) {
	memcpy(cells, new_input, sizeof(T) * size);
    }
    void apply_rules() {
	rules(cells, empty, size);
	switch_buffers();
    }
    static void set_buf(T* buf, size_t size, T val) {
	for(int i = 0; i < size; ++i) {
	    buf[i] = val;
	}
    }

private:

    T zero;

    void (*rules) (const T* input, T* output, size_t size);
    void switch_buffers() {
	T* h = cells;
	cells = empty;
	empty = h;
    }
};
