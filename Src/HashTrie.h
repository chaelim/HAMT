/**
 *      File: HashTrie.h
 *    Author: CS Lim
 *   Purpose: Templates for CPU Cache-aware compact and high performance hash table
 *   History:
 *  2012/5/3: File Created
 *
 *  References:
 *      - Ideal Hash Trees by Phil Bagwell (This is main paper the orignal ideas came from)
 *      - Ideal Hash Tries: an implementation in C++
 *          http://www.altdevblogaday.com/2011/03/22/ideal-hash-tries-an-implementation-in-c/
 */

#ifndef __HASH_TRIE_H__
#define __HASH_TRIE_H__

#include <stdint.h>
#include <string.h>
#include <assert.h>

#if _MSC_VER
#include <intrin.h>
#endif

// To use POPCNT CPU instruction (in SSE4.2), set SSE42_POPCNT 1
#define SSE42_POPCNT 0

#if _MSC_VER
    #define COMPILER_CHECK(expr, msg)  typedef char COMPILE_ERROR_##msg[1][(expr)]
#else
    #define COMPILER_CHECK(expr, msg)  typedef char COMPILE_ERROR_##msg[1][(expr)?1:-1]
#endif

//===========================================================================
// Typedefs
//===========================================================================
typedef unsigned char   uint8;
typedef uint16_t        uint16;
typedef uint32_t        uint32;
typedef uint64_t        uint64;
typedef uintptr_t       uint_ptr;
typedef signed char     int8;
typedef int16_t         int16;
typedef int32_t         int32;
typedef int64_t         int64;
typedef intptr_t        int_ptr;


//===========================================================================
//    Macros
//===========================================================================
#define  HASH_TRIE(type, key)              THashTrie< type, key >

//===========================================================================
// Helpers for bit operations
//===========================================================================
#if _MSC_VER

    #include <stdlib.h>
    #define ROTL32(x, y)    _rotl(x,y)
    #define ROTL64(x, y)    _rotl64(x,y)
    #define ROTR32(x, y)    _rotr(x,y)
    #define ROTR64(x, y)    _rotr64(x,y)

#else

    inline uint32 rotl32 (uint32 x, int8 r) {
        return (x << r) | (x >> (32 - r));
    }

    inline uint64 rotl64 (uint64 x, int8 r) {
        return (x << r) | (x >> (64 - r));
    }

    inline uint64 rotr32 (uint32 x, int8 r) {
        return (x >> r) | (x << (32 - r))
    }

    inline uint64 rotr64 (uint64 x, int8 r) {
        return (x >> r) | (x << (64 - r))
    }

    #define ROTL32(x, y)    rotl32(x, y)
    #define ROTL64(x, y)    rotl64(x, y)
    #define ROTR32(x, y)    rotr32(x, y)
    #define ROTR64(x, y)    rotr64(x, y)

#endif // if _MSC_VER


/****************************************************************************
*
*   Some bit twiddling helpers
*
*   GetBitCount function
*        from http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
*
**/

#if _MSC_VER && SSE42_POPCNT

inline uint32 GetBitCount(uint32 v) {
    return __popcnt(v);
}

#else

inline uint32 GetBitCount(uint32 v) {
    v = v - ((v >> 1) & 0x55555555);                    // reuse input as temporary
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);     // temp
    return ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count
}

#endif

#if _MSC_VER && SSE42_POPCNT && WIN64

inline uint32 GetBitCount (uint64 v) {
    return __popcnt64(v);
}

#else

inline uint32 GetBitCount(uint64 v) {
    uint64 c;
    c = v - ((v >> 1) & 0x5555555555555555ull);
    c = ((c >> 2) & 0x3333333333333333ull) + (c & 0x3333333333333333ull);
    c = ((c >> 4) + c) & 0x0F0F0F0F0F0F0F0Full;
    c = ((c >> 8) + c) & 0x00FF00FF00FF00FFull;
    c = ((c >> 16) + c) & 0x0000FFFF0000FFFFull;
    return uint32((c >> 32) + c) /* & 0x00000000FFFFFFFFull*/;
}
#endif

template <class T>
inline T ClearNthSetBit(T v, int idx) {
    for (T b = v; b;) {
        T lsb = b & ~(b - 1);
        if (--idx < 0)
            return v ^ lsb;
        b ^= lsb;
    }
    return v;
}

//===========================================================================
//    Hash function foward declarations
//===========================================================================
// MurmurHash3
uint32 MurmurHash3_x86_32(const void * key, int len, uint32_t seed);


//===========================================================================
//    THashKey32
//    (Helper template class to get 32bit integer hash key value for POD types)
//===========================================================================
template <class T>
class THashKey32 {
    static const uint32 MURMUR_HASH3_SEED = sizeof(T);

public:
    THashKey32() : m_key(0) { }
    THashKey32(const T & key) : m_key(key) { }
    inline bool operator== (const THashKey32 & rhs) const { return m_key == rhs.m_key; }
    inline operator T () const { return m_key; }
    inline uint32 GetHash() const;
    inline const T & Get() const { return m_key; }
    inline void Set(const T & key) { m_key = key; }

protected:
    T        m_key;
};

/**
 * Generic Hash function for POD types
 */
template <class T>
inline uint32 THashKey32<T>::GetHash() const {
    return MurmurHash3_x86_32(
        (const void *)&m_value,
        sizeof(m_value),
        MURMUR_HASH3_SEED
    );
}

// Integer hash functions based on
//    http://www.cris.com/~Ttwang/tech/inthash.htm

/**
 * Specialization for 32 bit interger key
 */
template <>
inline uint32 THashKey32<int32>::GetHash() const {
    uint32 key = (uint32)m_key;
    key = ~key + (key << 15); // key = (key << 15) - key - 1;
    key ^= ROTR64(key, 12);
    key += (key << 2);
    key ^= ROTR64(key, 4);
    key *= 2057; // key = (key + (key << 3)) + (key << 11);
    key ^= ROTR64(key, 16);
    return key;
}

/**
 * Specialization for 32 bit interger key
 */
template <>
inline uint32 THashKey32<uint32>::GetHash() const {
    uint32 key = m_key;
    key = ~key + (key << 15); // key = (key << 15) - key - 1;
    key ^= ROTR64(key, 12);
    key += (key << 2);
    key ^= ROTR64(key, 4);
    key *= 2057; // key = (key + (key << 3)) + (key << 11);
    key ^= ROTR64(key, 16);
    return key;
}

/**
 * Specialization for 64 bit interger key (64 bit to 32 bit hash)
 */
template <>
inline uint32 THashKey32<int64>::GetHash() const {
    uint64 key = (uint64)m_key;
    key = (~key) + (key << 18); // key = (key << 18) - key - 1;
    key ^= ROTR64(key, 31);
    key *= 21; // key = (key + (key << 2)) + (key << 4);
    key ^= ROTR64(key, 11);
    key += (key << 6);
    key ^= ROTR64(key, 22);
    return (uint32)key;
}

/**
 * Specialization for 64 bit interger key (64 bit to 32 bit hash)
 */
template <>
inline uint32 THashKey32<uint64>::GetHash() const {
    uint64 key = m_key;
    key = (~key) + (key << 18); // key = (key << 18) - key - 1;
    key ^= ROTR64(key, 31);
    key *= 21; // key = (key + (key << 2)) + (key << 4);
    key ^= ROTR64(key, 11);
    key += (key << 6);
    key ^= ROTR64(key, 22);
    return (uint32)key;
}

//===========================================================================
//    String Helper functions
//===========================================================================

inline size_t StrLen(const char str[]) {
    return strlen(str);
}

inline size_t StrLen(const wchar_t str[]) {
    return wcslen(str);
}

inline int StrCmp(const char str1[], const char str2[]) {
    return strcmp(str1, str2);
}

inline int StrCmp(const wchar_t str1[], const wchar_t str2[]) {
    return wcscmp(str1, str2);
}

inline int StrCmpI(const char str1[], const char str2[]) {
    return _stricmp(str1, str2);
}

inline int StrCmpI(const wchar_t str1[], const wchar_t str2[]) {
    return _wcsicmp (str1, str2);
}

inline char * StrDup(const char str[]) {
    return _strdup(str);
}

inline wchar_t * StrDup(const wchar_t str[]) {
    return _wcsdup(str);
}


//===========================================================================
//    TStrCmp and TStrCmpI
//===========================================================================
template<class CharType>
class TStrCmp {
public:
    static int StrCmp(const CharType str1[], const CharType str2[]) {
        return ::StrCmp(str1, str2);
    }
};

template<class CharType>
class TStrCmpI {
public:
    static int StrCmp(const CharType str1[], const CharType str2[]) {
        return ::StrCmpI(str1, str2);
    }
};


//===========================================================================
//    THashKeyStr
//===========================================================================
template<class CharType, class Cmp>
class THashKeyStrPtr;

template<class CharType, class Cmp = TStrCmp<CharType> >
class THashKeyStr {
public:
    bool operator== (const THashKeyStr & rhs) const {
        return (Cmp::StrCmp(m_str, rhs.m_str) == 0);
    }

    uint32 GetHash () const {
        if (m_str != NULL) {
            size_t strLen = StrLen(m_str);
            return MurmurHash3_x86_32(
                (const void *)m_str,
                sizeof(CharType) * strLen,
                strLen    // use string length as seed value
            );
        }
        else
            return 0;
    }
    const CharType * GetString () const { return m_str; }

protected:
    THashKeyStr() : m_str(NULL) { }
    virtual ~THashKeyStr() { }

    const CharType * m_str;

    friend class THashKeyStrPtr<CharType, Cmp>;
};

template<class CharType, class Cmp = TStrCmp<CharType> >
class THashKeyStrCopy : public THashKeyStr<CharType, Cmp> {
public:
    THashKeyStrCopy() { }
    THashKeyStrCopy(const CharType str[]) { SetString(str); }
    ~THashKeyStrCopy() {
        free(const_cast<CharType *>(THashKeyStr<CharType>::m_str));
    }

    void SetString(const CharType str[]) {
        free(const_cast<CharType *>(THashKeyStr<CharType>::m_str));
        THashKeyStr<CharType>::m_str = str ? StrDup(str) : NULL;
    }
};

template<class CharType, class Cmp = TStrCmp<CharType> >
class THashKeyStrPtr : public THashKeyStr<CharType, Cmp> {
public:
    THashKeyStrPtr() { }
    THashKeyStrPtr(const CharType str[]) { SetString(str); }
    THashKeyStrPtr(const THashKeyStr<CharType, Cmp> & rhs) {
        THashKeyStr<CharType, Cmp>::m_str = rhs.m_str;
    }
    THashKeyStrPtr & operator=(const THashKeyStr<CharType, Cmp> & rhs) {
        THashKeyStr<CharType, Cmp>::m_str = rhs.m_str;
        return *this;
    }
    THashKeyStrPtr & operator=(const THashKeyStrPtr<CharType, Cmp> & rhs) {
        THashKeyStr<CharType, Cmp>::m_str = rhs.m_str;
        return *this;
    }

    void SetString (const CharType str[]) {
        THashKeyStr<CharType, Cmp>::m_str = str;
    }
};

// Typedefs for convenience
typedef THashKeyStrCopy<wchar_t>                        CHashKeyStr;
typedef THashKeyStrPtr <wchar_t>                        CHashKeyStrPtr;
typedef THashKeyStrCopy<char>                           CHashKeyStrAnsiChar;
typedef THashKeyStrPtr <char>                           CHashKeyStrPtrAnsiChar;

typedef THashKeyStrCopy<wchar_t, TStrCmpI<wchar_t> >    CHashKeyStrI;
typedef THashKeyStrPtr <wchar_t, TStrCmpI<wchar_t> >    CHashKeyStrPtrI;
typedef THashKeyStrCopy<char, TStrCmpI<char> >          CHashKeyStrAnsiCharI;
typedef THashKeyStrPtr <char, TStrCmpI<char> >          CHashKeyStrPtrAnsiCharI;


/****************************************************************************
*
*   THashTrie
*
*   Template class for HAMT (Hash Array Mapped Trie)
*
**/

template <class T, class K>
class THashTrie {
private:
    // Use the least significant bit as reference marker
    static const uint_ptr   AMT_MARK_BIT    = 1;    // Using LSB for marking AMT (sub-trie) data structure
    static const uint32     HASH_INDEX_BITS = 5;
    static const uint32     HASH_INDEX_MASK = (1 << HASH_INDEX_BITS) - 1;
    // Ceiling to 8 bits boundary to use all 32 bits.
    static const uint32     MAX_HASH_BITS   = ((sizeof(uint32) * 8 + 7) / HASH_INDEX_BITS) * HASH_INDEX_BITS;
    static const uint32     MAX_HAMT_DEPTH  = MAX_HASH_BITS / HASH_INDEX_BITS;

private:
    // Each Node entry in the hash table is either terminal node
    // (a T pointer) or AMT data structure.
    //
    //        If a node pointer's LSB == 1 then AMT
    //            else T object pointer
    //
    // A one bit in the bit map represents a valid arc, while a zero an empty arc.
    // The pointers in the table are kept in sorted order and correspond to
    // the order of each one bit in the bit map.

    struct ArrayMappedTrie {
        uint32  m_bitmap;
        T *     m_subHash[1];
        // Do not add more data below
        // New data should be added before m_subHash

        inline T ** Lookup(uint32 hashIndex);
        inline T ** LookupLinear(const K & key);

        static T ** Alloc1(
            uint32    bitIndex,
            T **    slotToReplace
        );
        static T ** Alloc2(
            uint32  hashIndex,
            T *     node,
            uint32  oldHashIndex,
            T *     oldNode,
            T **    slotToReplace
        );
        static T ** Alloc2Linear(
            T *     node,
            T *     oldNode,
            T **    slotToReplace
        );

        static ArrayMappedTrie * Insert(
            ArrayMappedTrie * amt,
            uint32      hashIndex,
            T *         node,
            T **        slotToReplace
        );
        static ArrayMappedTrie * AppendLinear(
            ArrayMappedTrie *   amt,
            T *         node,
            T **        slotToReplace
        );
        static ArrayMappedTrie * Resize(
            ArrayMappedTrie * amt,
            int         oldSize,
            int         deltasize,
            int         idx
        );

        static void ClearAll(ArrayMappedTrie * amt, uint32 depth=0);
        static void DestroyAll(ArrayMappedTrie * amt, uint32 depth=0);
    };

    // Root Hash Table
    T *         m_root;
    uint32      m_count;

public:
    THashTrie() : m_root(NULL), m_count(0) { }
    ~THashTrie() { Clear(); }

public:
    void Add(T * node);
    T * Find(const K & key);
    T * Remove(const K & key);
    bool Empty();
    uint32 GetCount() { return m_count; }
    void Clear();        // Destruct HAMT data structures only
    void Destroy();    // Destruct HAMT data structures as well as containing objects
};


//===========================================================================
//    THashTrie<T, K>::ArrayMappedTrie Implementation
//===========================================================================

// helpers to search for a given entry
// this function counts bits in order to return the correct slot for a given hash
template<class T, class K>
T ** THashTrie<T, K>::ArrayMappedTrie::Lookup(uint32 hashIndex) {
    assert(hashIndex < (1 << HASH_INDEX_BITS));
    uint32 bitPos = (uint32)1 << hashIndex;
    if ((m_bitmap & bitPos) == 0)
        return NULL;
    else
        return &m_subHash[GetBitCount(m_bitmap & (bitPos - 1))];
}

template<class T, class K>
T ** THashTrie<T, K>::ArrayMappedTrie::LookupLinear(const K & key) {
    // Linear search
    T ** cur = m_subHash;
    T ** end = m_subHash + m_bitmap;
    for (; cur < end; cur++) {
        if (**cur == key)
            return cur;
    }
    // Not found
    return NULL;
}

template<class T, class K>
T ** THashTrie<T, K>::ArrayMappedTrie::Alloc1(uint32 bitIndex, T ** slotToReplace) {
    // Assert (0 <= bitIndex && bitIndex < 31);
    ArrayMappedTrie * amt = (ArrayMappedTrie *)malloc(sizeof(ArrayMappedTrie));
    amt->m_bitmap = 1 << bitIndex;
    *slotToReplace = (T *)((uint_ptr)amt | AMT_MARK_BIT);
    return amt->m_subHash;
}

template<class T, class K>
T ** THashTrie<T, K>::ArrayMappedTrie::Alloc2(
    uint32      hashIndex,
    T *         node,
    uint32      oldHashIndex,
    T *         oldNode,
    T **        slotToReplace
) {
    // Allocates a node with room for 2 elements
    ArrayMappedTrie * amt = (ArrayMappedTrie *)malloc(
          sizeof(ArrayMappedTrie)
        + sizeof(T *)
    );
    amt->m_bitmap = \
        ((uint32)1 << hashIndex) | ((uint32)1 << oldHashIndex);

    // Sort them in order and return new node
    if (hashIndex < oldHashIndex) {
        amt->m_subHash[0] = node;
        amt->m_subHash[1] = oldNode;
    }
    else {
        amt->m_subHash[0] = oldNode;
        amt->m_subHash[1] = node;
    }

    *slotToReplace = (T *)((uint_ptr)amt | AMT_MARK_BIT);;
    return amt->m_subHash;
}

template<class T, class K>
T ** THashTrie<T, K>::ArrayMappedTrie::Alloc2Linear(
    T *        node,
    T *        oldNode,
    T **    slotToReplace
) {
    // Allocates a node with room for 2 elements
    ArrayMappedTrie * amt = (ArrayMappedTrie *)malloc(
          sizeof(ArrayMappedTrie)
        + sizeof(T *)
    );
    amt->m_bitmap = 2;    // Number of entry in the linear search array
    amt->m_subHash[0] = node;
    amt->m_subHash[1] = oldNode;
    *slotToReplace = (T *)((uint_ptr)amt | AMT_MARK_BIT);
    return amt->m_subHash;
}

template<class T, class K>
typename THashTrie<T, K>::ArrayMappedTrie *
THashTrie<T, K>::ArrayMappedTrie::Insert(
    ArrayMappedTrie * amt,
    uint32      hashIndex,
    T *         node,
    T **        slotToReplace
) {
    uint32 bitPos = (uint32)1 << hashIndex;
    assert((amt->m_bitmap & bitPos) == 0);

    uint32 numBitsBelow = GetBitCount(amt->m_bitmap & (bitPos-1));
    amt = Resize(amt, GetBitCount(amt->m_bitmap), 1, numBitsBelow);
    amt->m_bitmap |= bitPos;
    amt->m_subHash[numBitsBelow] = node;
    *slotToReplace = (T *)((uint_ptr)amt | AMT_MARK_BIT);
    return amt;
}

template<class T, class K>
typename THashTrie<T, K>::ArrayMappedTrie *
THashTrie<T, K>::ArrayMappedTrie::AppendLinear(
    ArrayMappedTrie * amt,
    T *         node,
    T **        slotToReplace
) {
    amt = Resize(amt, amt->m_bitmap, 1, amt->m_bitmap);
    amt->m_subHash[amt->m_bitmap] = node;
    amt->m_bitmap++;
    *slotToReplace = (T *)((uint_ptr)amt | AMT_MARK_BIT);
    return amt;
}

// memory allocation all in this function. (re)allocates n,
// copies old m_data, and inserts space at index 'idx'
template<class T, class K>
typename THashTrie<T, K>::ArrayMappedTrie *
THashTrie<T,K>::ArrayMappedTrie::Resize(
    ArrayMappedTrie * amt,
    int        oldSize,
    int        deltaSize,
    int        idx
) {
    assert(deltaSize != 0);
    int newSize = oldSize + deltaSize;
    assert(newSize > 0);

    // if it shrinks then (idx + deltasize, idx) will be removed
    if (deltaSize < 0) {
        memmove(
            amt->m_subHash + idx,
            amt->m_subHash + idx - deltaSize,
            (newSize - idx) * sizeof(T *)
        );
    }

    amt = (ArrayMappedTrie *)realloc(
        amt,
        sizeof(ArrayMappedTrie)
            + (newSize - 1) * sizeof(T *)
    );

    // If it grows then (idx, idx + deltasize) will be inserted
    if (deltaSize > 0) {
        memmove(
            amt->m_subHash + idx + deltaSize,
            amt->m_subHash + idx,
            (oldSize - idx) * sizeof(T *)
        ); // shuffle tail to make room
    }

    return amt;
}


/*
 * Destroy HashTrie including pertaining sub-tries
 * NOTE: It does NOT destroy the objects in leaf nodes.
 */
template<class T, class K>
void THashTrie<T, K>::ArrayMappedTrie::ClearAll(
    ArrayMappedTrie * amt,
    uint32 depth
) {
    // If this is a leaf node, do nothing
    if (((uint_ptr)amt & AMT_MARK_BIT) == 0)
        return;

    amt = (ArrayMappedTrie *)((uint_ptr)amt & (~AMT_MARK_BIT));
    if (depth < MAX_HAMT_DEPTH) {
        T ** cur = amt->m_subHash;
        T ** end = amt->m_subHash + GetBitCount(amt->m_bitmap);
        for (; cur < end; cur++)
            ClearAll((ArrayMappedTrie *)*cur, depth + 1);
    }
    free(amt);
}

/*
 * Destroy HashTrie including pertaining sub-tries and containing objects
 * NOTE: It DOES destory (delete) the objects in leaf nodes.
 */
template<class T, class K>
void THashTrie<T, K>::ArrayMappedTrie::DestroyAll(
    ArrayMappedTrie * amt,
    uint32 depth
) {
    // If this is a leaf node just destroy the conatining object T
    if (((uint_ptr)amt & AMT_MARK_BIT) == 0) {
        delete ((T *)amt);
        return;
    }

    amt = (ArrayMappedTrie *)((uint_ptr)amt & (~AMT_MARK_BIT));
    if (depth < MAX_HAMT_DEPTH) {
        T ** cur = amt->m_subHash;
        T ** end = amt->m_subHash + GetBitCount(amt->m_bitmap);
        for (; cur < end; cur++)
            DestroyAll((ArrayMappedTrie *)*cur, depth + 1);
    }
    else {
        T ** cur = amt->m_subHash;
        T ** end = amt->m_subHash + amt->m_bitmap;
        for (; cur < end; cur++)
            delete ((T * )*cur);
    }

    free(amt);
}


#if _MSC_VER
inline bool HasAMTMarkBit (uint_ptr ptr) {
    return _bittest((const long *)&ptr, 0) != 0;
}
#else
inline bool HasAMTMarkBit (uint_ptr ptr) {
    return (ptr & 1) != 0;
}
#endif

//===========================================================================
//    THashTrie<T, K> Implementation
//===========================================================================

template<class T, class K>
inline void THashTrie<T, K>::Add(T * node) {
    // If hash trie is empty just add value/pair node and set it as root
    if (Empty()) {
        m_root = node;
        m_count++;
        return;
    }

    // Get hash value
    uint32 hash = node->GetHash();
    uint32 bitShifts = 0;
    T ** slot = &m_root;    // First slot is the root node
    for (;;) {
        // Leaf node (a T node pointer)?
        if (!HasAMTMarkBit((uint_ptr)*slot)) {
            // Replace if a node already exists with same key.
            // Caller is responsible for checking if a different object
            // with same key already exists and prevent memory leak.
            if (**slot == *node) {
                *slot = node;
                return;
            }

            // Hash collision detected:
            //      Replace this leaf with an AMT node to resolve the collision.
            //    The existing key must be replaced with a sub-hash table and
            //    the next 5 bit hash of the existing key computed. If there is still
            //    a collision then this process is repeated until no collision occurs.
            //    The existing key is then inserted in the new sub-hash table and
            //    the new key added.

            T *        oldNode = *slot;
            uint32    oldHash = oldNode->GetHash() >> bitShifts;

            // As long as the hashes match, we have to create single element
            // AMT internal nodes. this loop is hopefully nearly always run 0 time.
            while (bitShifts < MAX_HASH_BITS && (oldHash & HASH_INDEX_MASK) == (hash & HASH_INDEX_MASK)) {
                slot = ArrayMappedTrie::Alloc1(hash & HASH_INDEX_MASK, slot);
                bitShifts += HASH_INDEX_BITS;
                hash     >>= HASH_INDEX_BITS;
                oldHash  >>= HASH_INDEX_BITS;
            }

            if (bitShifts < MAX_HASH_BITS) {
                ArrayMappedTrie::Alloc2(
                    hash & HASH_INDEX_MASK,
                    node,
                    oldHash & HASH_INDEX_MASK,
                    oldNode,
                    slot
                );
            }
            else {
                // Consumed all hash bits, alloc and init a linear search table
                ArrayMappedTrie::Alloc2Linear(
                    node,
                    oldNode,
                    slot
                );
            }

            m_count++;
            break;
        }

        //
        // It's an Array Mapped Trie (sub-trie)
        //
        ArrayMappedTrie * amt = (ArrayMappedTrie *)((uint_ptr)*slot & (~AMT_MARK_BIT));
        T ** childSlot;
        if (bitShifts >= MAX_HASH_BITS) {
            // Consumed all hash bits. Add to the linear search array.
            childSlot = amt->LookupLinear(*node);
            if (childSlot == NULL) {
                ArrayMappedTrie::AppendLinear(amt, node, slot);
                m_count++;
            }
            else
                *slot = node;     // If the same key node already exists then replace
            break;
        }

        childSlot = amt->Lookup(hash & HASH_INDEX_MASK);
        if (childSlot == NULL) {
            amt = ArrayMappedTrie::Insert(
                amt,
                hash & HASH_INDEX_MASK,
                node,
                slot
            );
            m_count++;
            break;
        }

        // Go to next sub-trie level
        slot = childSlot;
        bitShifts += HASH_INDEX_BITS;
        hash     >>= HASH_INDEX_BITS;
    }    // for (;;)
}

template<class T, class K>
T * THashTrie<T, K>::Find(const K & key) {
    // Hash trie is empty?
    if (Empty())
        return NULL;

    // Get hash value
    uint32 hash = key.GetHash();
    uint32 bitShifts = 0;
    const T * slot = m_root;    // First slot is the root node
    for (;;) {
        // Leaf node (a T node pointer)?
        if (((uint_ptr)slot & AMT_MARK_BIT) == 0)
            return (*slot == key) ? (T *)slot : NULL;

        //
        // It's an Array Mapped Trie (sub-trie)
        //
        ArrayMappedTrie * amt = (ArrayMappedTrie *)((uint_ptr)slot & (~AMT_MARK_BIT));
        if (bitShifts >= MAX_HASH_BITS) {
            // Consumed all hash bits. Run linear search.
            T ** linearSlot = amt->LookupLinear(key);
            return (linearSlot != NULL) ? *(linearSlot) : NULL;
        }

        T ** childSlot = amt->Lookup(hash & HASH_INDEX_MASK);
        if (childSlot == NULL)
            return NULL;

        // Go to next sub-trie level
        slot = *childSlot;
        bitShifts += HASH_INDEX_BITS;
        hash     >>= HASH_INDEX_BITS;
    }

    // !!! SHOULD NEVER BE HERE !!!
    assert(false);
}

template<class T, class K>
T * THashTrie<T, K>::Remove(const K & key) {
    T ** slots[MAX_HAMT_DEPTH + 2];
    slots[0] = &m_root;

    ArrayMappedTrie * amts[MAX_HAMT_DEPTH + 2];
    amts[0] = NULL;

    uint32 hash = key.GetHash();
    //
    // First find the leaf node that we want to delete
    //
    int depth = 0;
    for (; depth <= MAX_HAMT_DEPTH; ++depth, hash >>= HASH_INDEX_BITS) {
        // Leaf node?
        if (((uint_ptr)*slots[depth] & AMT_MARK_BIT) == 0) {
            amts[depth] = NULL;
            if (!(**slots[depth] == key))
                return NULL;
            break;
        }
        else {
            // It's an AMT node
            ArrayMappedTrie * amt = amts[depth] = \
                (ArrayMappedTrie *)((uint_ptr)*slots[depth] & (~AMT_MARK_BIT));
            slots[depth + 1] = (depth >= MAX_HAMT_DEPTH) ? \
                    amt->LookupLinear(key) : amt->Lookup(hash & HASH_INDEX_MASK);
            if (slots[depth + 1] == NULL)
                return NULL;
        }
    }

    // Get the node will be returned
    T * ret = *slots[depth];

    // we are going to have to delete an entry from the internal node at amts[depth]
    while (--depth >= 0) {
        int oldsize = depth >= MAX_HAMT_DEPTH ? \
                (int)(amts[depth]->m_bitmap) :
                (int)(GetBitCount(amts[depth]->m_bitmap));
        int oldidx  = (int)(slots[depth + 1] - amts[depth]->m_subHash);

        // the second condition is that the remaining entry is a leaf
        if (oldsize == 2 && ((uint_ptr)(amts[depth]->m_subHash[!oldidx]) & AMT_MARK_BIT) == 0) {
            // we no longer need this node; just fold the remaining entry,
            // which must be a leaf, into the parent and free this node
            *(slots[depth]) = amts[depth]->m_subHash[!oldidx];
            free(amts[depth]);
            break;
        }

        // resize this node down by a bit, and update the m_usedBitMap bitfield
        if (oldsize > 1) {
            ArrayMappedTrie * amt = ArrayMappedTrie::Resize(amts[depth], oldsize, -1, oldidx);
            amt->m_bitmap = (depth >= MAX_HAMT_DEPTH) ? \
                (amt->m_bitmap - 1) : ClearNthSetBit(amt->m_bitmap, oldidx);
            *(slots[depth]) = (T *)((uint_ptr)amt | AMT_MARK_BIT);    // update the parent slot to point to the resized node
            break;
        }
        free(amts[depth]);    // oldsize==1. delete this node, and then loop to kill the parent too!
    }

    // No node exists in the HashTrie any more
    if (depth < 0)
        m_root = NULL;

    m_count--;
    return ret;
}

template<class T, class K>
inline bool THashTrie<T, K>::Empty() {
    return m_root == NULL;
}

template<class T, class K>
inline void THashTrie<T, K>::Clear() {
    if (Empty())
        return;

    ArrayMappedTrie::ClearAll((ArrayMappedTrie *)m_root);

    // HashTrie is now empty
    m_root = NULL;
}


template<class T, class K>
inline void THashTrie<T, K>::Destroy() {
    if (Empty())
        return;

    ArrayMappedTrie::DestroyAll((ArrayMappedTrie *)m_root);

    // HashTrie is now empty
    m_root = NULL;
}

#endif // if __HASH_TRIE_H__
