//
// Created by Sumzeek on 23/02/2024.
//

#include "hash_table.h"
#include <assert.h>
#include <memory.h>

HashTable::HashTable(uint32 _index_size){
    hash=nullptr,next_index=nullptr;
    resize(_index_size);
}

HashTable::HashTable(uint32 _hash_size,uint32 _index_size){
    hash=nullptr,next_index=nullptr;
    resize(_hash_size,_index_size);
}

HashTable::~HashTable(){
    free();
}

void HashTable::resize(uint32 _index_size){
    resize(lower_nearest_2_power(_index_size),_index_size);
}

void HashTable::resize(uint32 _hash_size,uint32 _index_size){
    free();
    assert((_hash_size&(_hash_size-1))==0);

    hash_size=_hash_size;
    hash_mask=hash_size-1;
    index_size=_index_size;
    hash=new uint32[hash_size];
    next_index=new uint32[index_size];
    memset(hash,0xff,hash_size*4);
}

void HashTable::resize_index(uint32 _index_size){
    uint32* indexs=new uint32[_index_size];
    memcpy(indexs,next_index,sizeof(uint32)*index_size);
    delete[] next_index;
    next_index=indexs;
    index_size=_index_size;
}

void HashTable::clear(){
    memset(hash,0xff,hash_size*4);
}