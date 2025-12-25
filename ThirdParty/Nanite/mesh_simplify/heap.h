#pragma once
#include "../utils/types.h"

class Heap{
    uint32 heap_size;
    uint32 num_index;
    uint32* heap;
    float* keys;
    uint32* heap_indexes;

    void push_up(uint32 i);
    void push_down(uint32 i);
public:
    Heap();
    Heap(uint32 _num_index);
    ~Heap(){free();}

    void free(){
        heap_size=0,num_index=0;
        delete[] heap;
        delete[] keys;
        delete[] heap_indexes;
        heap=nullptr,keys=nullptr,heap_indexes=nullptr;
    }
    void resize(uint32 _num_index);

    float get_key(uint32 idx);
    void clear();
    bool empty(){return heap_size==0;}
    bool is_present(uint32 idx){return heap_indexes[idx]!=~0u;}
    uint32 top();
    void pop();
    void add(float key,uint32 idx);
    void update(float key,uint32 idx);
    void remove(uint32 idx);
};