#ifndef IGAME_BYTE_SWAP_H
#define IGAME_BYTE_SWAP_H

#include "iGameObject.h"
#include <ostream>
#include "iGameType.h"

IGAME_NAMESPACE_BEGIN

class ByteSwap : public Object {
public:
	I_OBJECT(ByteSwap);
	static Pointer New() { return new ByteSwap; }

	// Type-safe swap functions for Little and Big Endian storage
#define IGAME_BYTE_SWAP_DECL(T) \
    static void SwapLE(T* p); \
    static void SwapBE(T* p); \
    static void SwapLERange(T* p, size_t num); \
    static void SwapBERange(T* p, size_t num); \
    static bool SwapLERangeWrite(const T* p, size_t num, FILE* file); \
    static bool SwapBERangeWrite(const T* p, size_t num, FILE* file); \
    static void SwapLERangeWrite(const T* p, size_t num, std::ostream* os); \
    static void SwapBERangeWrite(const T* p, size_t num, std::ostream* os);

	IGAME_BYTE_SWAP_DECL(float);
	IGAME_BYTE_SWAP_DECL(double);
	IGAME_BYTE_SWAP_DECL(char);
	IGAME_BYTE_SWAP_DECL(short);
	IGAME_BYTE_SWAP_DECL(int);
	IGAME_BYTE_SWAP_DECL(long);
	IGAME_BYTE_SWAP_DECL(long long);
	IGAME_BYTE_SWAP_DECL(signed char);
	IGAME_BYTE_SWAP_DECL(unsigned char);
	IGAME_BYTE_SWAP_DECL(unsigned short);
	IGAME_BYTE_SWAP_DECL(unsigned int);
	IGAME_BYTE_SWAP_DECL(unsigned long);
	IGAME_BYTE_SWAP_DECL(unsigned long long);
#undef IGAME_BYTE_SWAP_DECL
	// Byte swap functions for specific sizes in computer Endian type
	static void Swap2(void* p);
	static void Swap4(void* p);
	static void Swap8(void* p);
	// Byte swap functions for specific ranges in computer Endian type
	static void Swap2Range(void* p, size_t num);
	static void Swap4Range(void* p, size_t num);
	static void Swap8Range(void* p, size_t num);
	// Byte swap and write functions for specific ranges in computer Endian type
	static bool SwapWrite2Range(const void* p, size_t num, FILE* f);
	static bool SwapWrite4Range(const void* p, size_t num, FILE* f);
	static bool SwapWrite8Range(const void* p, size_t num, FILE* f);
	static void SwapWrite2Range(const void* p, size_t num, std::ostream* os);
	static void SwapWrite4Range(const void* p, size_t num, std::ostream* os);
	static void SwapWrite8Range(const void* p, size_t num, std::ostream* os);

	// Byte swap functions for specific sizes in Little Endian
	static void Swap2LE(void* p);
	static void Swap4LE(void* p);
	static void Swap8LE(void* p);

	// Byte swap functions for specific ranges in Little Endian
	static void Swap2LERange(void* p, size_t num);
	static void Swap4LERange(void* p, size_t num);
	static void Swap8LERange(void* p, size_t num);

	// Byte swap and write functions for specific ranges in Little Endian
	static bool SwapWrite2LERange(const void* p, size_t num, FILE* f);
	static bool SwapWrite4LERange(const void* p, size_t num, FILE* f);
	static bool SwapWrite8LERange(const void* p, size_t num, FILE* f);
	static void SwapWrite2LERange(const void* p, size_t num, std::ostream* os);
	static void SwapWrite4LERange(const void* p, size_t num, std::ostream* os);
	static void SwapWrite8LERange(const void* p, size_t num, std::ostream* os);

	// Byte swap functions for specific sizes in Big Endian
	static void Swap2BE(void* p);
	static void Swap4BE(void* p);
	static void Swap8BE(void* p);

	// Byte swap functions for specific ranges in Big Endian
	static void Swap2BERange(void* p, size_t num);
	static void Swap4BERange(void* p, size_t num);
	static void Swap8BERange(void* p, size_t num);

	// Byte swap and write functions for specific ranges in Big Endian
	static bool SwapWrite2BERange(const void* p, size_t num, FILE* f);
	static bool SwapWrite4BERange(const void* p, size_t num, FILE* f);
	static bool SwapWrite8BERange(const void* p, size_t num, FILE* f);
	static void SwapWrite2BERange(const void* p, size_t num, std::ostream* os);
	static void SwapWrite4BERange(const void* p, size_t num, std::ostream* os);
	static void SwapWrite8BERange(const void* p, size_t num, std::ostream* os);

	// Generic byte swap for a buffer with arbitrary word size
	static void SwapVoidRange(void* buffer, size_t numWords, size_t wordSize);

protected:
	ByteSwap();
	~ByteSwap() override;
};

IGAME_NAMESPACE_END

#endif // IGAME_BYTE_SWAP_H
