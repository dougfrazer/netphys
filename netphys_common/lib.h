#pragma once

#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
class LIST
{
public:
    LIST();
    virtual ~LIST();

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
template < class T > LIST<T>::LIST() { m_head = nullptr; }
template < class T > LIST<T>::~LIST() {}

//******************************************************************************
// Public interface
//******************************************************************************
template < class T >
bool LIST<T>::Add(T* node)
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
bool LIST<T>::Remove(T* node)
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
template < class T > T* LIST<T>::GetFirst()
{
    return m_head;
}
//******************************************************************************
template < class T > T* LIST<T>::GetNext(T* node)
{
    return node->m_next == m_head ? nullptr : node->m_next;
}
//******************************************************************************