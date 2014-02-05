/**
 *      File: main.cpp
 *    Author: CS Lim
 *   Purpose: Simple AMT test program
 *   History:
 *  2012/5/3: File Created
 *
 */

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
u64 GetMicroTime() {
    u64 hz;
    QueryPerformanceFrequency((LARGE_INTEGER*)&hz);
    
    u64 t;
    QueryPerformanceCounter((LARGE_INTEGER*)&t);
    return (t * 1000000) / hz;
}
#else
#include <sys/time.h>
u64 GetMicroTime() {
    timeval t;
    gettimeofday(&t,NULL);
    return t.tv_sec * 1000000ull + t.tv_usec;
}
#endif

void TestHashTrie ()
{
    struct CTest : THashKey32<uint32> {
        CTest (uint32 data) : THashKey32<uint32>(data), m_data(data) { }
        uint32    m_data;
    };

    struct CTestStr : CHashKeyStrAnsiChar {
        CTestStr (const char str[]) : CHashKeyStrAnsiChar(str) { }
        uint32    m_data;
    };

    HASH_TRIE(CTest, THashKey32<uint32>)        test_int32;
    HASH_TRIE(CTestStr, CHashKeyStrAnsiChar)    test_str;

    printf("32 bit integer test...\n");
    printf("1) Add %d entries:    ", MAX_TEST_ENTRIES);
    u64 t0 = GetMicroTime();
    for (int i = 0; i < MAX_TEST_ENTRIES; i++) {
        CTest * test = new CTest(i);
        test_int32.Add(test);
    }
    printf("   %10u usec\n", int(GetMicroTime() - t0));

    printf("2) Find %d entries:   ", MAX_TEST_ENTRIES);
    t0 = GetMicroTime();
    for (int i = 0; i < MAX_TEST_ENTRIES; i++) {
        CTest * find = test_int32.Find(THashKey32<uint32>(i));
        assert(find->Get() == i);
    }
    printf("   %10u usec\n", int(GetMicroTime() - t0));

    printf("3) Remove %d entries: ", MAX_TEST_ENTRIES);
    t0 = GetMicroTime();
    for (int i = 0; i < MAX_TEST_ENTRIES; i++) {
        CTest * removed = test_int32.Remove(THashKey32<uint32>(i));
        assert(removed != 0);
        assert(removed->Get() == i);
        delete removed;
    }
    printf("   %10u usec\n\n", int(GetMicroTime() - t0));

    printf("ANSI string test...\n");
    printf("1) Add %d entries:    ", MAX_TEST_ENTRIES);
    t0 = GetMicroTime();
    for (int i = 0; i < MAX_TEST_ENTRIES; i++) {
        // String Hash test
        char buffer[16];
        sprintf_s(buffer, "%d", i);
        CTestStr * test = new CTestStr(buffer);
        test_str.Add(test);
    }
    printf("   %10u usec\n", int(GetMicroTime() - t0));

    printf("2) Find %d entries:   ", MAX_TEST_ENTRIES);
    t0 = GetMicroTime();
    for (int i = 0; i < MAX_TEST_ENTRIES; i++) {
        char buffer[16];
        sprintf_s(buffer, "%d", i);
        CTestStr * find = test_str.Find(CHashKeyStrAnsiChar(buffer));
        assert(strcmp(find->GetString(), buffer) == 0);
    }
    printf("   %10u usec\n", int(GetMicroTime() - t0));

    printf("3) Remove %d entries: ", MAX_TEST_ENTRIES);
    t0 = GetMicroTime();
    for (int i = 0; i < MAX_TEST_ENTRIES; i++) {
        char buffer[16];
        sprintf_s(buffer, "%d", i);
        CTestStr * removed2 = test_str.Remove(CHashKeyStrAnsiChar(buffer));
        assert(removed2 != 0);
        assert(strcmp(removed2->GetString(), buffer) == 0);
        delete removed2;
    }
    printf("   %10u usec\n\n", int(GetMicroTime() - t0));
}

int main (int argc, int argv[]) {
    TestHashTrie();

    printf("Hit any key to exit.");
    _getch();
}