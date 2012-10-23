#include <stdio.h>
#include <conio.h>
#include <stdint.h>
#include <assert.h>
#include "..\Src\HashTrie.h"

extern int TestHashTrie1 ();

typedef unsigned char u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef signed char s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

const uint32 MAX_TEST_ENTRIES = 1000000;

#ifdef WIN32
#include <conio.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
//===========================================================================
// Timing functions
//===========================================================================
u64 GetMicroTime()
{
    u64 hz;
    QueryPerformanceFrequency((LARGE_INTEGER*)&hz);
    
    u64 t;
    QueryPerformanceCounter((LARGE_INTEGER*)&t);
    return (t * 1000000) / hz;
}
#else
#include <sys/time.h>
u64 GetMicroTime()
{
    timeval t;
    gettimeofday(&t,NULL);
    return t.tv_sec * 1000000ull + t.tv_usec;
}
#endif

void TestHashTrie ()
{
    struct  CTest : THashKey32<uint32>
    {
        CTest (uint32 data) : THashKey32<uint32>(data), m_data(data) { }
        uint32    m_data;
    };

    struct CTestStr : CHashKeyStrAnsiChar
    {
        CTestStr (const char str[]) : CHashKeyStrAnsiChar(str) { }
        uint32    m_data;
    };

    HASH_TRIE(CTest, THashKey32<uint32>)        test_int32;
    HASH_TRIE(CTestStr, CHashKeyStrAnsiChar)    test_str;

    u64 t0 = GetMicroTime();
    for (int i = 0; i < MAX_TEST_ENTRIES; i++)
    {
        CTest * test = new CTest(i);
        test_int32.Add(test);
        CTest * find = test_int32.Find(THashKey32<uint32>(i));
        assert(test == find);

        // String Hash test
        char buffer[16];
        sprintf_s(buffer, "%d", i);
        CTestStr * test2 = new CTestStr(buffer);
        test_str.Add(test2);
        CTestStr * find2 = test_str.Find(CHashKeyStrAnsiChar(buffer));
        assert(test2 == find2);
    }

    for (int i = 0; i < MAX_TEST_ENTRIES; i++)
    {
        CTest * removed = test_int32.Remove(THashKey32<uint32>(i));
        assert(removed != 0);
        assert(removed->Get() == i);
        delete removed;

        char buffer[16];
        sprintf_s(buffer, "%d", i);
        CTestStr * removed2 = test_str.Remove(CHashKeyStrAnsiChar(buffer));
        assert(removed2 != 0);
        assert(strcmp(removed2->GetString(), buffer) == 0);
        delete removed2;
    }

    printf("trie test   %8d %10uusec\n", MAX_TEST_ENTRIES, int(GetMicroTime() - t0));
}

int main (int argc, int argv[])
{
    TestHashTrie();
    _getch();
}