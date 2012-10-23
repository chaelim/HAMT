// ideal hash trie implementation by alex evans, 2011
// see http://altdevblogaday.org/?p=2311 for more info

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <sys/types.h>

/////////////////////////////// define some useful types and timing macros

typedef unsigned char u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef signed char s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#ifdef WIN32
#include <conio.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static u64 GetMicroTime()
{
	static u64 hz=0;
	static u64 hzo=0;
	if (!hz) {
		QueryPerformanceFrequency((LARGE_INTEGER*)&hz);
		QueryPerformanceCounter((LARGE_INTEGER*)&hzo);
	}
	
	u64 t;
	QueryPerformanceCounter((LARGE_INTEGER*)&t);
	return ((t-hzo)*1000000)/hz;
}
#else
#include <sys/time.h>
static u64 GetMicroTime()
{
	timeval t;
	gettimeofday(&t,NULL);
	return t.tv_sec * 1000000ull + t.tv_usec;
}
#endif

///////////////////////// some bit twiddling helpers we need to maintain our bitmask ///////////////////////// 
// CountSetBits adapted from http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
inline u32 CountSetBits (u32 v)
{   
	v = v - ((v >> 1) & 0x55555555);                    // reuse input as temporary
	v = (v & 0x33333333) + ((v >> 2) & 0x33333333);     // temp
	return ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count
}

inline u32 CountSetBits (u64 v)
{
	u64 c;
	c = v - ((v >> 1) & 0x5555555555555555ull);
	c = ((c >> 2) & 0x3333333333333333ull) + (c & 0x3333333333333333ull);
	c = ((c >> 4) + c) & 0x0F0F0F0F0F0F0F0Full;
	c = ((c >> 8) + c) & 0x00FF00FF00FF00FFull;
	c = ((c >> 16) + c) & 0x0000FFFF0000FFFFull;
	return u32((c >> 32) + c)/* & 0x00000000FFFFFFFFull*/;
}

template <class T> inline T ClearNthSetBit (T v, int idx)
{	
	for (T b=v; b;)
	{
		T lsb = b & ~(b - 1);
		if (--idx < 0)
			return v ^ lsb;
		b ^= lsb;
	}
	return v;
}

int NextPowerOf2(int x)
{
	x -= 1;
	x |= x >> 16;
	x |= x >> 8;
	x |= x >> 4;
	x |= x >> 2;
	x |= x >> 1;
	return x + 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////  
/* this class defines a THashTrie - a type of hash table that works by having a trie of nodes,
   each of which has at most 64 children. each child is either a data value, or another trie
   node. the children are indexed by picking off 6 bits of the object's key at a time.

   the data structure is very cache friendly, fast, and has low memory overhead: 1 bit per entry.
   it should be faster than any binary tree, and a little slower than a big hash table, but without
   any need to ever re-hash (ie no lumpy performance)
   it should also use less memory than both the binary trees (only 1-2 bit per entry overhead) and
   the hash table (which has to be kept relatively un-loaded to be fast).
   delete is also efficient and doesn't require the use of tombstones and compaction.
   
   the API is simple
	T *Get(T &root, const T &k)
	T *Set(T &root, const T &k)
	void Delete(T &root, const T &k) // yes I implemented delete! 
	 
   this implementation has some particular features that users should be aware of:   
   * up to 30 bits of the hash are m_usedBitMap to walk the trie; after that, collisions are handled with
	 a brute force linear list. with a good hash function, collisions should be rare enough that
	 this is fast.
   * as a side effect of this, at most 30/6 = 5 pointer hops are followed to find any key.
   * currently the THashTrie allocates memory using malloc() and free(), and grows its arrays from
	 size 1 to 64 *in steps of 1*. This favours low memory waste over speed / avoiding fragmentation. 
	 it should be trivial to replace the functions Resize() and Capacity() to change this behaviour.
   * the type T is expected to be a pointer, or other small POD type (for example a pair of ints)
	 that can be safely & efficiently copied around.
   * the root of the tree is stored as a T. this is nice: a THashTrie with 0 or 1 entries entails
	 no memory allocation, and is just stored in the root itself: memory cost is sizeof(T).
	 this does however set some restrictions on T, detailed below.
   * the type T need not have any operators or base type, but should have these global functions 
	 overloaded for it:
 
   HashTrieGetHash - return the hash of a given T (at least 30 bits with good distribution)
   HashTrieEqual   - compare the full keys of two Ts, not just the hashes.
   HashTrieReplace - copy new T over the top of an existing old T. should de-allocate old T.
			this is your chance to handle the case that Set() is called with duplicate keys.
   
   * the type T needs to be able to be aliased with a single pointer, in other words
	 sizeof(T) must be >= sizeof(void*), and must have 1 bit of spare storage to indicate if 
	 a given bit of memory is a T, or a pointer. if T is a pointer type, you need do nothing,
	 and the 'is pointer' bit is stored in the bottom bit of the T.
	 if T is something else, you must also overload these 3 additional global functions for T:
 
 void HashTrieSetNode(T&dst, THashTrie<T> *d); put pointer d into the memory for dst and mark it as a pointer
 THashTrie<T>* HashTrieGetNode(const T&src)    return previously stored pointer, or null if src is not marked
 bool HashTrieIsEmpty(const T&src)            return true if src is equal to the empty tree
 void HashTrieDelete(T &dst)		clear dst to the empty state (eg m_usedBitMap to set the root to the empty tree)

	* for more info , read the code ;-)
	* this hasn't been tested in production code, it was just written for fun.
 */

#if _MSC_VER
	#define COMPILER_CHECK(expr, msg)  typedef char COMPILE_ERROR_##msg[1][(expr)]
#else
	#define COMPILER_CHECK(expr, msg)  typedef char COMPILE_ERROR_##msg[1][(expr)?1:-1]
#endif


#if 0	// this chooses between 64-way fanout and 32-way fanout. choose based on cache line size and ISA... and measurement ;)
typedef u64 FANOUT_T;
#define FANOUT_SHIFT 6
#else
typedef u32 FANOUT_T;
#define FANOUT_SHIFT 5
#endif

//===========================================================================
// Constants
//===========================================================================
#define FANOUT_BITS		(1 << FANOUT_SHIFT)
#define FANOUT_MASK		(FANOUT_BITS - 1)
#define MAX_DEPTH		(30 / FANOUT_SHIFT)

template <typename T> class THashTrie;

// provide specialisations of these if your T is not pointer-like, or you dont want to use the bottom bit as a flag
template <class T>
inline void HashTrieSetNode (T & dst, THashTrie<T> * d)
{
	dst = (T)((size_t)d | 1);
}

template <class T>
inline THashTrie<T> * HashTrieGetNode (const T & src)
{
	size_t p = (size_t)src;
	return (p & 1) ? (THashTrie<T>*)(p - 1) : 0;
}

template <class T>
inline bool HashTrieIsEmpty (const T & src)
{
	return src != NULL;
}

template <class T>
inline void HashTrieDelete (T & dst)
{
	dst = T();
};

template <class T>
inline T * HashTrieReplace (T & dst, const T & src)
{
	HashTrieDelete(dst);
	dst = src;
	return &dst;
}

template <typename T>
class THashTrie
{
protected:
	// Node is THashTrie Node type
	typedef THashTrie<T> Node;

	// Array Mapped Trie:
	//	One bit in a bit map for each node that represents a valid branch
	//	in the trie (an entry in 'm_data').
	FANOUT_T	m_usedBitMap;  
	
	// we store the children here, tightly packed according to 'm_usedBitMap'
	// the THashTrie structure is malloced such that the m_data array is
	// big enough to hold CountSetBits(m_usedBitMap) entries.
	T			m_data[1];      

private:
	// helpers to search for a given entry 
	T * Lookup (u32 index) // this function counts bits in order to return the correct slot for a given hash 
	{ 	
		FANOUT_T bit = FANOUT_T(1) << (index & FANOUT_MASK);
		if ((m_usedBitMap & bit) == 0)
			return 0;
		return m_data + CountSetBits(m_usedBitMap & (bit-1));
	}

	T * LookupLinear (const T & k) // this one treats the m_data as an unsorted array and scans.
	{
		for (u32 c1 = 0; c1 < m_usedBitMap; ++c1)
			if (HashTrieEqual(m_data[c1], k))
				return m_data + c1;
		return 0;
	}

public:
	static inline T * Get (T & root, const T & k) { return Lookup(root, k, true); }
	static inline T * Set (T & root, const T & k) { return Lookup(root, k, false); }

	static T * Lookup (T & root, const T & k, bool get)
	{
		COMPILER_CHECK(sizeof(T) >= sizeof(void *), DataMustBelargerThanPointerSize);

		if (HashTrieIsEmpty(root)) // special case: empty trie 
			return get ? 0 : HashTrieReplace(root, k);

		T * slot = &root;	// slot holds the slot in the parent node, ie the place we are going to update if we do an insert
		Node * n;				// the last internal node we visited
		u32 k_hash = HashTrieGetHash(k); // the hash we are looking for

		// walk through the hashed key FANOUT_SHIFT bits at a time
		int depth = 0;
		for (;;) {
			if (!(n = HashTrieGetNode(*slot)))  // is this an inner node or a value?
			{   // it's a leaf node 
				if (HashTrieEqual(*slot,k))
					return get ? slot : HashTrieReplace(*slot, k);
				if (get)
					return 0;

				// we have a hash collision - replace this leaf with an internal node to resolve the collision
				T oldval = *slot;
				u32 old_hash = HashTrieGetHash(oldval) >> depth;
				// as long as the hashes match, we have to create single element internal nodes.
				// this loop is hopefully nearly always run 0 times
				while (depth < 30 && (old_hash & FANOUT_MASK) == (k_hash & FANOUT_MASK)) {
					slot = Alloc1(k_hash,slot);
					depth	  += FANOUT_SHIFT;
					k_hash	 >>= FANOUT_SHIFT;
					old_hash >>= FANOUT_SHIFT;
				}
				// finally we create our collision-resolving internal node with just 2 slots: the old and the new value
				return (depth>=30) ? Alloc2Linear(k,oldval,slot) : Alloc2(k_hash,k,old_hash,oldval,slot);
			}
			// else internal node - find our child and continue the walk
			if (depth >= 30)
				break; // nodes above bit 30 are a linear list walk - handled outside the main loop

			T *child_slot = n->Lookup(k_hash);
			if (!child_slot) 
				return get ? 0 : Insert(n,k_hash,k,slot);
			slot = child_slot;

			depth   += FANOUT_SHIFT;
			k_hash >>= FANOUT_SHIFT;
		}

		// we've run out of tree! switch to linear list search. 30 bit hash collisions are hopefully very, very rare.
		T *child_slot = n->LookupLinear(k);
		if (child_slot) return get ? child_slot : HashTrieReplace(*child_slot, k);
		 return get ? 0 : Append(n,k,slot);
	}

	
	static bool Delete(T &root, const T &k)
	{
		T *slots[MAX_DEPTH+2]; slots[0]=&root;
		Node *nodes[MAX_DEPTH+2]; nodes[0]=0;
		u32 k_hash = HashTrieGetHash(k);
		int depth; 
		for (depth=0;depth<=MAX_DEPTH;++depth,k_hash>>=FANOUT_SHIFT)
		{
			if (!(nodes[depth]=HashTrieGetNode(*slots[depth])))
			{   // leaf node
				if (!HashTrieEqual(*slots[depth],k))
					return false;
				break;
			} else    // inner node
				if (!(slots[depth+1] = ((depth>=MAX_DEPTH) ? nodes[depth]->LookupLinear(k) : nodes[depth]->Lookup(k_hash)))) 
					return false;
		}			
		// now we've found the leaf we want to delete, we have to go back up the tree deleting unnecessary nodes.
		assert(HashTrieEqual(*slots[depth],k));
		for (HashTrieDelete(*slots[depth]);--depth>=0;)
		{	// we are going to have to delete an entry from the internal node at nodes[depth]
			int oldsize=(depth>=MAX_DEPTH)?(int)(nodes[depth]->m_usedBitMap):(int)(CountSetBits(nodes[depth]->m_usedBitMap));
			int oldidx =(int)(slots[depth+1]-nodes[depth]->m_data);
			if (oldsize==2 && !HashTrieGetNode(nodes[depth]->m_data[!oldidx])) // the second condition is that the remaining entry is a leaf
			{	// we no longer need this node; just fold the remaining entry, which must be a leaf, into the parent and free this node								
				*slots[depth] = nodes[depth]->m_data[!oldidx];
				free(nodes[depth]);
				return true;
			}
			if (oldsize>1) 
			{	// resize this node down by a bit, and update the m_usedBitMap bitfield
				Node *n=Resize(nodes[depth],oldsize,-1,oldidx);				
				n->m_usedBitMap =(depth>=MAX_DEPTH)?(n->m_usedBitMap-1):ClearNthSetBit(n->m_usedBitMap,oldidx);
				HashTrieSetNode(*slots[depth],n); // update the parent slot to point to the resized node
				return true;
			}			
			HashTrieDelete(*slots[depth]);
			free(nodes[depth]);	// oldsize==1. delete this node, and then loop to kill the parent too!
		}
		return true;
	}
		
	///////////////////////// helper functions - mainly memory allocation & shuffling memory around. joy
	static void DebugPrint(const T &root, int depth=0, u32 hash_so_far=0)
	{
		if (HashTrieIsEmpty(root)) return;
		Node *n = HashTrieGetNode(root);
		u32 c2=0;
		if (n) 
		{
			if (depth>=30) for (u32 c1=0;c1<n->m_usedBitMap;++c1) DebugPrint(n->m_data[c1],depth+FANOUT_SHIFT);
			else   		   for (u32 c1=0;c1<64;++c1) 
			{   u64 bit=FANOUT_T(1)<<c1;
				if (n->m_usedBitMap & bit) DebugPrint(n->m_data[c2++],depth+FANOUT_SHIFT,hash_so_far + (c1<<depth));
			}
		}
		else  
		{
			u64 mask=(FANOUT_T(1)<<depth)-1;
			assert(hash_so_far == (HashTrieGetHash(root) & mask)); // this asserts that the tree is layed out as we expect
			::DebugPrint(root);
		}
	}

	static inline int Capacity(int c) { return c; }			// growth policy enshrined in this function. could eg round c up to next power of 2.
	//static inline int Capacity(int c) { return (c<4)?4:NextPowerOf2(c); }			// ...like this

	static Node * Resize(Node*n, int oldsize, int deltasize, int idx) // memory allocation all in this function. (re)allocates n, copies old m_data, and inserts space at index 'idx'
	{
		Node *newn=n; int newsize=oldsize+deltasize;
		int oldcapacity=Capacity(oldsize);
		int newcapacity=Capacity(newsize);
		if (n==0 || oldcapacity!=newcapacity) newn=(Node*)malloc(sizeof(Node)+(newcapacity-1)*sizeof(T));
		if (n)
		{
			if (deltasize>0)
				memmove(newn->m_data+idx+deltasize,n->m_data+idx,sizeof(T)*(oldsize-idx)); // shuffle tail to make room
			else
				memmove(newn->m_data+idx,n->m_data+idx-deltasize,sizeof(T)*(newsize-idx)); // shuffle tail to make room
			if (n!=newn) 
			{   // copy over old unchanged m_data & free old version
				newn->m_usedBitMap=n->m_usedBitMap;
				memcpy(newn->m_data,n->m_data,sizeof(T)*idx);
				free(n); 
			}
		}		
		return newn;
	}

	// allocation helper functions that use Resize to insert new nodes in various ways; all return a pointer to the new slot
	static T * Insert (Node *n, u32 index, const T &newval, T *slot_to_replace) 
	{
		FANOUT_T bit=FANOUT_T(1)<<(index&FANOUT_MASK);
		assert(0==(n->m_usedBitMap&bit));
		u32 numbitsbelow = CountSetBits(n->m_usedBitMap & (bit-1));
		n=Resize(n,CountSetBits(n->m_usedBitMap), 1, numbitsbelow);
		n->m_usedBitMap |= bit;
		n->m_data[numbitsbelow]=newval;
		HashTrieSetNode(*slot_to_replace,n);
		return n->m_data+numbitsbelow;
	}
	static T * Append(Node*&n, T newval, T*slot_to_replace)
	{
		n=Resize(n,int(n->m_usedBitMap),1, int(n->m_usedBitMap));
		n->m_data[n->m_usedBitMap]=newval;
		HashTrieSetNode(*slot_to_replace,n);
		return n->m_data+n->m_usedBitMap++;
	}

	static T * Alloc1(u32 index, T*slot_to_replace) 
	{ 
		Node*n=Resize(0,0,1,0);
		n->m_usedBitMap=FANOUT_T(1)<<(index&FANOUT_MASK); 
		HashTrieSetNode(*slot_to_replace,n); 
		return n->m_data; // warning: does not set the m_data yet... caller must do that.
	}

	static T * Alloc2(u32 newindex, const T &newval, u32 oldindex, const T &oldval, T*slot_to_replace)
	{   // allocates a node with room for 2 elements, and sorts them by index.
		newindex&=FANOUT_MASK; oldindex&=FANOUT_MASK;
		assert(newindex!=oldindex);
		Node*n=Resize(0,0,2,0); 
		n->m_usedBitMap=(FANOUT_T(1)<<newindex)|(FANOUT_T(1)<<oldindex); 
		if (newindex<oldindex)
		{
			n->m_data[0]=newval;n->m_data[1]=oldval;
			HashTrieSetNode(*slot_to_replace,n);
			return n->m_data;
		}
		else
		{
			n->m_data[0] = oldval;
			n->m_data[1] = newval;
			HashTrieSetNode(*slot_to_replace,n);
			return n->m_data + 1;
		}
	}
	static T *Alloc2Linear(const T & newval, const T & oldval, T * slot_to_replace)
	{   // allocates a node with room for 2 elements, to be m_usedBitMap as a linear list.
		Node * n = Resize(0, 0, 2, 0);
		n->m_usedBitMap = 2;
		n->m_data[0] = newval;
		n->m_data[1] = oldval;
		HashTrieSetNode(*slot_to_replace, n);
		return n->m_data;
	}
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// example program, that also compares performance (ROUGHLY!) with stl set and hash_set
// TODO: make hash_set actually work. gah.

#define TEST_SIZE 1000000
#define TEST_TRIE
//#define TEST_SET
//#define TEST_HASHSET // TODO: does not work? 

#ifdef TEST_SET
#include <set>
#endif
#ifdef TEST_HASHSET
#include <hash_set>
#endif


// from http://www.cris.com/~Ttwang/tech/inthash.htm
#define ror64(x, k) (((x) >> (k)) | ((x) << (64-(k))))

u32 GetHash (u64 key)
{
	key = (~key) + (key << 18); // key = (key << 18) - key - 1;
	key ^= ror64(key,31);
	key *= 21; // key = (key + (key << 2)) + (key << 4);
	key ^= ror64(key,11);
	key += (key << 6);
	key ^= ror64(key,22);
	return (u32)key;
}

// from murmurhash2
inline u64 murmurmix (u64 h, u64 k)
{
	const u64 m = 0xc6a4a7935bd1e995ull;
	const int r = 47;
	k *= m;
	k ^= k >> r;
	k *= m;
	h ^= k;
	h *= m;
	return h;
}

u32  HashTrieGetHash(u64 kv)                { return GetHash(kv); }
bool HashTrieEqual(u64 a, u64 b)            { return a == b; }
void DebugPrint(u64 a)						{ printf("%llx ",(long long unsigned int)(a & 0xfff)); }

u32  HashTrieGetHash(void* kv)                { return 11; } // test a really bad hash function
bool HashTrieEqual(void*a, void*b)            { return a==b; }


struct hashseteq {   
	const static size_t bucket_size = FANOUT_BITS; 
	bool operator()(u64 a, u64 b) const { return HashTrieEqual(a,b); } 
	size_t operator()(u64 a) const { return HashTrieGetHash(a); } 
};

int TestHashTrie1 ()
{
	u64 root=0;    
	u32 c1; 
	u64 t0;

	//_getch();
#ifdef TEST_TRIE

#if 0
	{
		// test hash collision edge case with size_t whose hash function is 'return 11';
		// so everything ends up in a linear list 
		void* root=0;
		THashTrie<void*>::Set(root,(void*) 100);
		THashTrie<void*>::Set(root,(void*) 200);
		THashTrie<void*>::Set(root,(void*) 300);
		THashTrie<void*>::Set(root,(void*) 400);
		assert(THashTrie<void*>::Delete(root,(void*) 400));
		assert(THashTrie<void*>::Delete(root,(void*) 300));
		assert(THashTrie<void*>::Delete(root,(void*) 200));
		assert(THashTrie<void*>::Delete(root,(void*) 100));
		assert(root==0);
	}
#endif

	/////////////////////////////////////////////////////////////////////////
	for (t0=GetMicroTime(), c1=0; c1<TEST_SIZE; ++c1)
	{
		u64 randy = murmurmix(12345, c1*2) * 2 + 2; // we use the bottom bit to indicate internal nodes, and 0 means null.
		THashTrie<u64>::Set(root, randy);
		//THashTrie<u64>::DebugPrint(root); printf("\n");
	}
	printf("trie insert   %8d %8dusec\n",c1,int(GetMicroTime()-t0));
	//_getch();
	for (int iter=0;iter<3;++iter)
	{
		for (t0=GetMicroTime(),c1=0;c1<TEST_SIZE*2;++c1)
		{
			u64 randy=murmurmix(12345,c1)*2+2; // we use the bottom bit to indicate internal nodes, and 0 means null.
			THashTrie<u64>::Get(root, randy);
		}
		printf("trie get      %8d %8dusec\n",c1,int(GetMicroTime()-t0));
	}
	for (t0=GetMicroTime(),c1=0;c1<TEST_SIZE;++c1)
	{		
		u64 randy=murmurmix(12345,c1*2)*2+2; // we use the bottom bit to indicate internal nodes, and 0 means null.
		THashTrie<u64>::Delete(root, randy);
		//THashTrie<u64>::DebugPrint(root); printf("\n");
	}
	assert(root==0);
	printf("trie delete   %8d %8dusec\n",c1,int(GetMicroTime()-t0));
#endif

#ifdef TEST_SET
	/////////////////////////////////////////////////////////////////////////
	std::set<u64> myset;
	for (t0=GetMicroTime(),c1=0;c1<TEST_SIZE;++c1)
	{
		u64 randy=murmurmix(12345,c1*2)*2+2; // we use the bottom bit to indicate internal nodes, and 0 means null.
		myset.insert(randy);
	}
	printf("set insert    %8d %8dusec\n",c1,int(GetMicroTime()-t0));
	//_getch();
	for (int iter=0;iter<3;++iter)
	{
	
		for (t0=GetMicroTime(),c1=0;c1<TEST_SIZE*2;++c1)
		{
			u64 randy=murmurmix(12345,c1)*2+2; // we use the bottom bit to indicate internal nodes, and 0 means null.
			std::set<u64>::iterator i=myset.find(randy);
		}
		printf("set get       %8d %8dusec\n",c1,int(GetMicroTime()-t0));
	}
	for (t0=GetMicroTime(),c1=0;c1<TEST_SIZE;++c1)
	{		
		u64 randy=murmurmix(12345,c1*2)*2+2; // we use the bottom bit to indicate internal nodes, and 0 means null.
		myset.erase(myset.find(randy));		
	}    
	printf("set delete    %8d %8dusec\n",c1,int(GetMicroTime()-t0));    
	assert(myset.empty());
#endif

#ifdef TEST_HASHSET
	/////////////////////////////////////////////////////////////////////////

	typedef std::hash_set<u64, hashseteq> hashsettype ;
	hashsettype hashset;
	
	for (t0=GetMicroTime(),c1=0;c1<TEST_SIZE;++c1)
	{
		u64 randy=murmurmix(12345,c1*2)*2+2; // we use the bottom bit to indicate internal nodes, and 0 means null.
		hashset.insert(randy);
	}
	printf("hset insert   %8d %8dusec\n",c1,int(GetMicroTime()-t0));
	//_getch();
	for (int iter=0;iter<3;++iter)
	{
	
		for (t0=GetMicroTime(),c1=0;c1<TEST_SIZE*2;++c1)
		{
			u64 randy=murmurmix(12345,c1)*2+2; // we use the bottom bit to indicate internal nodes, and 0 means null.
			hashsettype::iterator i=hashset.find(randy);
		}
		printf("hset get      %8d %8dusec\n",c1,int(GetMicroTime()-t0));
	}
	for (t0=GetMicroTime(),c1=0;c1<TEST_SIZE;++c1)
	{		
		u64 randy=murmurmix(12345,c1*2)*2+2; // we use the bottom bit to indicate internal nodes, and 0 means null.
		hashset.erase(hashset.find(randy));		
	}    
	printf("hset delete   %8d %8dusec\n",c1,int(GetMicroTime()-t0));    
	assert(hashset.empty());
#endif

	return 0;
}
