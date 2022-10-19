#pragma once

#include <stdlib.h>
#include <string.h>

struct DataStore
{
public:
    DataStore(int startingSize)
    {
        m_data = (char*)malloc(startingSize);
        m_totalSize = startingSize;
        m_size = 0;
    }
    ~DataStore()
    {
        #ifdef _DEBUG
            if(m_size) __debugbreak(); // somehow didn't clean up this data store
        #endif
    }
    void PushInt(int i) { Push((char*)&i, sizeof(int)); }
    void PushFloat(float f) { Push((char*)&f, sizeof(float)); }
    void Push(char* data, int size)
    {
        while (m_size + size > m_totalSize)
        {
            Grow();
        }
        memcpy(m_data + m_size, data, size);
        m_size += size;
    }

    char* GetData() { return m_data; }
    int   GetSize() { return m_size; }
    void  Free()    { free(m_data); m_size = 0; }
private:
    void Grow()
    {
        m_data = (char*)realloc(m_data, m_totalSize * 2);
        m_totalSize = m_totalSize * 2;
    }
    char* m_data;
    int   m_size;
    int   m_totalSize;
};

struct ReadDataStore
{
public:
    ReadDataStore(char* data, int size)
    {
        m_data = data;
        m_size = size;
    }
    ~ReadDataStore()
    {
    #ifdef _DEBUG
        if (m_size) __debugbreak(); // somehow didn't clean up this data store
    #endif
    }
    int   GetInt()   { return *(int*)  (Get(sizeof(int))); }
    float GetFloat() { return *(float*)(Get(sizeof(float))); }
    char* Get(int size)
    {
    #ifdef _DEBUG
        if(m_size < size) { __debugbreak(); }
    #endif
        char* ret = m_data;
        m_data += size;
        m_size -= size;
        return ret;
    }
private:
    char* m_data;
    int   m_size;
};