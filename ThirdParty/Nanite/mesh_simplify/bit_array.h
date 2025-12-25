#pragma once
#include "../utils/types.h"

class BitArray{
    uint32* bits;
public:
    BitArray(){bits=nullptr;}
    BitArray(uint32 size);
    ~BitArray(){free();}
    void resize(uint32 size);
    void free(){
        if(bits) delete[] bits;
    }
    void set_false(uint32 idx){
        uint32 x=idx>>5;
        uint32 y=idx&31;
        bits[x]&=~(1<<y);
    }
    void set_true(uint32 idx){
        uint32 x=idx>>5;
        uint32 y=idx&31;
        bits[x]|=(1<<y);
    }
    bool operator[](uint32 idx){
        uint32 x=idx>>5;
        uint32 y=idx&31;
        return (bool)(bits[x]>>y&1);
    }
};