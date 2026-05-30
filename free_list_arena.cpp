#include "free_list_arena.hpp"
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <cstdint>
#include <new>

static const size_t MIN_BLOCK_SIZE = sizeof(FreeListArena::Header) + 16;

FreeListArena::FreeListArena(size_t size)
    : start_(static_cast<char*>(std::malloc(size + (size_t)sizeof(Header))))
    , capacity_(size + (size_t)sizeof(Header))
    , freeList_(nullptr)
{
    if (!start_) throw std::bad_alloc();
    addToFreeList(start_, capacity_); //Explicit type conversion only, Badmaev programming school
}

FreeListArena::~FreeListArena() {
    free(start_);
}

void FreeListArena::addToFreeList(void* ptr, size_t blockSize) 
{
    Header* block = static_cast<Header*>(ptr);
    block->size = blockSize - sizeof(Header);
    block->next = freeList_;

    freeList_ = block;
}

size_t FreeListArena::alignUp(size_t n, size_t alignment) 
{
    return (n + alignment - 1) & ~(alignment - 1);
}

void* FreeListArena::allocate(size_t bytes, size_t alignment)
{
    Header* prev = nullptr;
    Header* curr = freeList_;

    while (curr) 
    {
        uintptr_t dataStart = reinterpret_cast<uintptr_t>(curr) + sizeof(Header);
        size_t misalignment = (alignment - (dataStart % alignment)) % alignment;
        size_t needed = bytes + misalignment;

        if (curr->size >= needed) 
        {
            if (prev)
                prev->next = curr->next;
            else
                freeList_ = curr->next;

            size_t remaining = curr->size - needed;
            if (remaining > MIN_BLOCK_SIZE) 
            {
                Header* newFree = reinterpret_cast<Header*>(reinterpret_cast<uintptr_t>(curr) + sizeof(Header) + needed);
                newFree->size = remaining - sizeof(Header);
                newFree->next = freeList_;

                freeList_ = newFree;
            }
            char* userPtr = reinterpret_cast<uintptr_t>(curr) + sizeof(Header) + misalignment;
            return userPtr;
        }
        prev = curr;
        curr = curr->next;
    }
    return nullptr;
}

void FreeListArena::deallocate(void* ptr, size_t size)//the user must input the pointer TO THE BEGINNING OF THE ALLOCATED DATA ARRAY, that means TO THE BEGINNING OF THE HEADER
{
    if (!ptr) return;
    if (ptr < start_ || ptr >= start_ + capacity_) return;

    Header* block = reinterpret_cast<Header*>(static_cast<uintptr_t>(ptr));
    block->next = freeList_;
    block->size = size - sizeof(Header);

    freeList_ = block;
    // TODO: once coalesce is ready it MUST be called here
}

void FreeListArena::coalesce() 
{
    if (!freeList_) return;

    std::vector<Header*> blocks;
    for (Header* h = freeList_; h != nullptr; h = h->next) 
    {
        // FIX: validate pointer is within arena bounds
        if (reinterpret_cast<uintptr_t>(h) < start_ ||
            reinterpret_cast<uintptr_t>(h) >= start_ + capacity_)
        {
            continue;
        }
        blocks.push_back(h);
    }

    if (blocks.size() < 2) return;

    std::sort(blocks.begin(), blocks.end(),
              [](Header* a, Header* b) { return a < b; });

    std::vector<Header*> merged;
    Header* cur = blocks[0];
    for (size_t i = 1; i < blocks.size(); ++i) 
    {
        Header* nxt = blocks[i];
        char* curEnd = reinterpret_cast<uintptr_t>(cur) + sizeof(Header) + cur->size;

        if (curEnd == reinterpret_cast<uintptr_t>(nxt)) 
        {
            cur->size += sizeof(Header) + nxt->size;
        } 
        else 
        {
            merged.push_back(cur);
            cur = nxt;
        }
    }
    merged.push_back(cur);

    // FIX: rebuild freeList_ from merged vector with proper next pointers
    freeList_ = nullptr;
    if (!merged.empty()) 
    {
        freeList_ = merged[0];

        for (size_t i = 0; i < merged.size() - 1; ++i) 
        {
            merged[i]->next = merged[i + 1];
        }

        merged.back()->next = nullptr;
    }
}

void FreeListArena::reset() 
{
    freeList_ = nullptr;
    addToFreeList(start_, capacity_);
}

size_t FreeListArena::getFreeBlocksCount() const 
{
    size_t count = 0;

    for (Header* h = freeList_; h; h = h->next)
        ++count;

    return count;
}
