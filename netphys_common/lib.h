#pragma once

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <immintrin.h>

#define alignup(p, i)      ( ((uintptr_t)p + (uintptr_t)i - 1) & ~((uintptr_t)i - 1) )
//https://graphics.stanford.edu/~seander/bithacks.html#ConditionalSetOrClearBitsWithoutBranching
#define set_or_clear_mask(condition, bits, mask) (bits ^= (-(condition) ^ bits) & mask)

struct DataStore
{
public:
    DataStore(int startingSize = 1024)
    {
        m_data = (char*)malloc(startingSize);
        m_totalSize = startingSize;
        m_size = 0;
    }
    ~DataStore()
    {
        assert(!m_size); // should have called free() before this went out scope
    }
    char* PushInt(int i)            { return Push((char*)&i, sizeof(int)); }
    char* PushUint(unsigned int i)  { return Push((char*)&i, sizeof(unsigned int)); }
    char* PushFloat(float f)        { return Push((char*)&f, sizeof(float)); }
    char* PushDouble(double f)      { return Push((char*)&f, sizeof(double)); }
    char* Push(char* data, int size)
    {
        assert(!m_finalized);
        while (m_size + size > m_totalSize)
        {
            Grow();
        }
        char* ret = m_data;
        memcpy(m_data + m_size, data, size);
        m_size += size;
        return ret;
    }

    int           GetInt()    { return *(int*)Get(sizeof(int)); }
    unsigned int  GetUint()   { return *(int*)Get(sizeof(unsigned int)); }
    float         GetFloat()  { return *(float*)Get(sizeof(float)); }
    double        GetDouble() { return *(double*)Get(sizeof(double)); }
    char*         Get(int size)
    {
        assert(m_finalized);
        char* ret = m_data + m_size;
        m_size += size;
        assert(m_size <= m_totalSize);
        return ret;
    }

    // call this after put all the data.  resets for read, and prevents future writes
    void  Finalize()
    { 
        m_finalized = true;
        m_totalSize = m_size;
        m_size = 0;
    }
    char* GetData() { assert(m_finalized); return m_data; }
    int   GetSize() { assert(m_finalized); return m_totalSize; }
    void  Free()    { free(m_data); m_data = nullptr; m_size = 0; m_totalSize = 0; }
private:
    void Grow()
    {
        m_data = (char*)realloc(m_data, m_totalSize * 2);
        m_totalSize = m_totalSize * 2;
    }
    char* m_data;
    int   m_size;
    int   m_totalSize;
    bool  m_finalized = false;
};
//******************************************************************************
template <class T>
class CyclicalList
{
public:
    CyclicalList();
    virtual ~CyclicalList();

public:
    bool Add(T* node);
    bool Remove(T* node);
    T* GetFirst(void);
    T* GetNext(T* node);

private:
    T* m_head;
};


//******************************************************************************
// Constructor
//******************************************************************************
template < class T > CyclicalList<T>::CyclicalList() { m_head = nullptr; }
template < class T > CyclicalList<T>::~CyclicalList() {}

//******************************************************************************
// Public interface
//******************************************************************************
template < class T >
bool CyclicalList<T>::Add(T* node)
{
    if (m_head == nullptr)
    {
        m_head = node;
        m_head->m_prev = node;
        m_head->m_next = node;
    }
    else
    {
        node->m_prev = m_head->m_prev;
        node->m_next = m_head->m_prev->m_next;
        m_head->m_prev->m_next = node;
        m_head->m_prev = node;
    }
    return true;
}
//******************************************************************************
template < class T >
bool CyclicalList<T>::Remove(T* node)
{
    if (node == nullptr)
    {
        return false;
    }
    if (node == m_head)
    {
        m_head = GetNext(node);
    }
    if (node->m_prev != node)
    {
        node->m_prev->m_next = node->m_next;
    }
    if (node->m_next != node)
    {
        node->m_next->m_prev = node->m_prev;
    }
    node->m_prev = nullptr;
    node->m_next = nullptr;
    return true;
}
//******************************************************************************
template < class T > T* CyclicalList<T>::GetFirst()
{
    return m_head;
}
//******************************************************************************
template < class T > T* CyclicalList<T>::GetNext(T* node)
{
    return node->m_next == m_head ? nullptr : node->m_next;
}
//******************************************************************************







//
// block allocator.  supports Get and Free.  maximum 64 segments.
//
template<typename T>
class BlockAllocator
{
public:
    T* Get()
    {
        if (!m_freeSegmentMask)
        {
            GetNewSegment();
        }
        unsigned long index;
        _BitScanReverse(&index, m_freeSegmentMask); // get the highest bit set
        Segment& s = m_segments[index];
        T* ret = (T*)(s.nextFree);
        s.nextFree = s.nextFree->next;

        set_or_clear_mask(s.nextFree != nullptr, m_freeSegmentMask, (1ull << index));

        return ret;
    }
    void Free(T* ptr)
    {
        for (size_t i = 0; i < m_segments.size(); i++)
        {
            Segment& s = m_segments[i];
            char* start = ((char*)(s.data));
            char* end = ((char*)(s.data)) + (m_blocksPerSegment * m_blockSize);
            if ((char*)ptr >= start && (char*)ptr < end)
            {
                ((Block*)ptr)->next = s.nextFree;
                s.nextFree = ((Block*)(ptr));

                m_freeSegmentMask |= (1ull << i);

                return;
            }
        }
        assert(false); // not good, we couldnt find this ptr to free
    }
    void Init(unsigned int blockSize, unsigned int numBlocksPerSegment)
    {
        m_blockSize = alignup(sizeof(T) * blockSize, sizeof(Block*)); // make sure each block can store a ptr
        m_blocksPerSegment = numBlocksPerSegment;
        m_segments.reserve(64);
        GetNewSegment();
    }
    // general purpose allocator.. one object per block
    BlockAllocator(unsigned int numBlocksPerSegment)
    {
        Init(1, numBlocksPerSegment);
    }
    // use this if you want multiple objects per block, like 4 floats
    BlockAllocator(unsigned int blockSize, unsigned int numBlocksPerSegment)
    {
        Init(blockSize, numBlocksPerSegment);
    }
    ~BlockAllocator()
    {
        for (Segment& s : m_segments)
        {
            free(s.data);
        }
    }
private:
    struct Block
    {
        Block* next;
    };
    struct Segment
    {
        T* data;
        Block* nextFree;
        Segment(T* _data) : data(_data), nextFree((Block*)_data) {}
    };

    std::vector<Segment> m_segments;
    int m_blockSize;
    int m_blocksPerSegment;
    int m_numSegments = 0;
    unsigned long m_freeSegmentMask = 0;
    void GetNewSegment()
    {
        char* segmentMemory = (char*)calloc(m_blockSize, m_blocksPerSegment);
        m_segments.push_back(Segment((T*)segmentMemory));

        Block* iter = (Block*)segmentMemory;
        for (int i = 0; i < m_blocksPerSegment - 1; i++)
        {
            iter->next = (Block*)((char*)iter + m_blockSize);
            iter = iter->next;
        }
        iter->next = nullptr;

        assert(m_numSegments < 32); // increase block size, not intended to have this many segments
        assert(!m_freeSegmentMask); // not supposed to have free segments if we're calling this

        m_freeSegmentMask |= (1ull << m_numSegments);
        m_numSegments++;
    }

};