#pragma once
#include <cinttypes>

template<typename T> class Cell_Automat {
public:
    Cell_Automat() {};
    Cell_Automat(T* data, size_t size, void (*rules) (const T* data, T* output, size_t size)):
	data(data), size(size), rules(rules){};
    size_t size;
    T* data;
    void (*rules) (const T* input, T* output, size_t size);
    void apply_rules() {
	rules(data, data, size);
    }
};
