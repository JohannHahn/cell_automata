#pragma once
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <cassert>

typedef uint64_t u64;

enum Automata_Type {
    ONE_DIM, TWO_DIM, AUTOMATA_TYPE_MAX
};

#define INDEX(x, y, width) (x) + ((y) * (width))
#define BIT_AT(i, map) ((map) >> (i) & u64(1))
#define BIT_SET(i, map) ((map) |= (u64(1) << (i)))
#define BIT_RESET(i, map) ((map) &= ~(u64(1) << (i))) 

template<typename T> class Cell_Automat {
public:
    Cell_Automat() {
	std::cout << "CELL_AUTOMAT: empty automat created, initialize with init()\n";
        init(AUTOMATA_TYPE_MAX, 0, 0, 0, T(), T());		
    }

    Cell_Automat(Automata_Type type, size_t width, size_t height, T zero_value, T one_value,
		 void (*rules) (Cell_Automat& automat) = NULL, T* input = NULL,
		const char* rules_one_dim_str = NULL, int rules_one_dim_int = -1) {
	init(type, width, height, zero_value, one_value, rules, input, rules_one_dim_str, rules_one_dim_int);
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
    int* neighbour_mask = NULL;
    T* empty;
    T* cells;
    // state before the simulation started
    T* initial_cells;
    T zero;
    T one;
    Automata_Type type;
    u64 one_dim_rules = 0;

    void init(Automata_Type type, size_t width, size_t height, T zero, T one,
		 void (*rules) (Cell_Automat& automat) = NULL, T* input = NULL, 
	      const char* one_dim_rules_str = NULL, int one_dim_rules_int = -1) {
	size = width * height;
	this->width = width;
	this->height = height;
	this->zero = zero;
	this->one = one;
	this->type = type;
	cells = new T[size];
	initial_cells = new T[size];
	empty = new T[size];
	if(input) {
	    memcpy(cells, input, sizeof(T) * size);
	    memcpy(initial_cells, input, sizeof(T) * size);
	}
	else {
	    set_buf(cells, size, zero);
	    set_buf(initial_cells, size, zero);
	}
	set_buf(empty, size, zero);
	setup_neighborhood();
	srand(time(NULL));
	switch (type) {
	    case ONE_DIM:
		if(one_dim_rules_str) {
		    set_ruleset_bin(one_dim_rules_str);
		}
		if(one_dim_rules_int >= 0) {
		    set_ruleset_dec(one_dim_rules_int);
		}
	    break;
	    case TWO_DIM:
		if (rules) this->rules = rules;
	    break;
	    case AUTOMATA_TYPE_MAX:
	    break;
	    default:
	    assert(0 && "unreachable");
	}
	std::cout << "finished initializing automat\n";
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
	std::cout << "new ruleset = " << one_dim_rules << "\n";
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
	std::cout << "new ruleset = " << one_dim_rules << "\n";
    }
    void clear_cells() {
	set_buf(cells, size, zero);
    }

    void randomize_cells() {
	int limit = size;
	if (type == ONE_DIM) limit = width;
	for (int i = 0; i < size; ++i) {
	    if (i < limit && rand() / (RAND_MAX / 2)) {
		cells[i] = one;
	    }
	    else cells[i] = zero;
	}
	memcpy(initial_cells, cells, sizeof(T) * size);
    }

    void set_cells(T* new_input) {
	memcpy(cells, new_input, sizeof(T) * size);
	memcpy(initial_cells, new_input, sizeof(T) * size);
    }

    void apply_rules() {
	if (type != ONE_DIM) {
	    rules(*this);
	    switch_buffers();
	}
	else {
	    one_dim_rules_func();
	    if (generation < height - 1) generation++;
	}
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


    void one_dim_rules_func() {
	if (generation < height - 1) {
	    int y = generation;
	    for (int x = 0; x < width; ++x) {
		T rule_index = 0;
		T index = INDEX(x, y, width);
		for (int n_i = 0; n_i < 3; ++n_i) {
		    int new_index = INDEX(x, y, width) + neighbour_mask[n_i];
		    if (new_index < 0) new_index += width;
		    else if (new_index >= size) new_index -= width;
		    if (cells[new_index] == one) {
			rule_index |= (T)(1) << (2 - n_i);
		    }
		}
		T new_value = BIT_AT(rule_index, one_dim_rules) ? one : zero;
		assert(rule_index < 8);
		assert(rule_index >= 0);
		cells[INDEX(x, y + 1, width)] = new_value;
	    }
	}
    }
};
