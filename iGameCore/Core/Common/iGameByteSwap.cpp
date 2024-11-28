#include "iGameByteSwap.h"
#include <cstdint>
#include <cstring>
#include <cstdio>

IGAME_NAMESPACE_BEGIN

ByteSwap::ByteSwap() {}
ByteSwap::~ByteSwap() {}

// Swapper template specializations
template <size_t s> struct ByteSwapper;

template <> struct ByteSwapper<1> {
	static inline void Swap(char*) {}
};

template <> struct ByteSwapper<2> {
	static inline void Swap(char* data) {
		uint16_t& ref = *reinterpret_cast<uint16_t*>(data);
		ref = (ref >> 8) | (ref << 8);
	}
};

template <> struct ByteSwapper<4> {
	static inline void Swap(char* data) {
		uint32_t& ref = *reinterpret_cast<uint32_t*>(data);
		ref = (ref >> 24) | ((ref & 0x00FF0000) >> 8) | ((ref & 0x0000FF00) << 8) | (ref << 24);
	}
};

template <> struct ByteSwapper<8> {
	static inline void Swap(char* data) {
		uint64_t& ref = *reinterpret_cast<uint64_t*>(data);
		ref = (ref >> 56) | (ref << 56) |
			((ref & 0x00FF000000000000) >> 40) | ((ref & 0x000000000000FF00) << 40) |
			((ref & 0x0000FF0000000000) >> 24) | ((ref & 0x0000000000FF0000) << 24) |
			((ref & 0x000000FF00000000) >> 8) | ((ref & 0x00000000FF000000) << 8);
	}
};

// Define ByteSwapLE and ByteSwapBE as explicit functions
template <class T>
inline void ByteSwapLE(T* p) {
	ByteSwapper<sizeof(T)>::Swap(reinterpret_cast<char*>(p));
}

template <class T>
inline void ByteSwapBE(T* p) {
	ByteSwapper<sizeof(T)>::Swap(reinterpret_cast<char*>(p));
}

// Swap range function
template <class T>
inline void ByteSwapRange(T* first, size_t num) {
	for (T* p = first; p != first + num; ++p) {
		ByteSwapper<sizeof(T)>::Swap(reinterpret_cast<char*>(p));
	}
}

// Helper function for writing swapped data to a file
template <class T>
inline bool ByteSwapRangeWrite(const T* first, size_t num, FILE* f) {
	for (const T* p = first; p != first + num; ++p) {
		union { T value; char data[sizeof(T)]; } temp = { *p };
		ByteSwapper<sizeof(T)>::Swap(temp.data);
		if (fwrite(temp.data, sizeof(T), 1, f) != 1) return false;
	}
	return true;
}

// Implementation for each endian type
#define IGAME_BYTE_SWAP_IMPL(T) \
void ByteSwap::SwapLE(T* p) { ByteSwapLE(p); } \
void ByteSwap::SwapBE(T* p) { ByteSwapBE(p); } \
void ByteSwap::SwapLERange(T* p, size_t num) { ByteSwapRange(p, num); } \
void ByteSwap::SwapBERange(T* p, size_t num) { ByteSwapRange(p, num); } \
bool ByteSwap::SwapLERangeWrite(const T* p, size_t num, FILE* file) { return ByteSwapRangeWrite(p, num, file); } \
bool ByteSwap::SwapBERangeWrite(const T* p, size_t num, FILE* file) { return ByteSwapRangeWrite(p, num, file); } \
void ByteSwap::SwapLERangeWrite(const T* p, size_t num, std::ostream* os) { for (const T* pData = p; pData != p + num; ++pData) { T temp = *pData; SwapLE(&temp); *os << temp; } } \
void ByteSwap::SwapBERangeWrite(const T* p, size_t num, std::ostream* os) { for (const T* pData = p; pData != p + num; ++pData) { T temp = *pData; SwapBE(&temp); *os << temp; } }

IGAME_BYTE_SWAP_IMPL(float)
IGAME_BYTE_SWAP_IMPL(double)
IGAME_BYTE_SWAP_IMPL(char)
IGAME_BYTE_SWAP_IMPL(short)
IGAME_BYTE_SWAP_IMPL(int)
IGAME_BYTE_SWAP_IMPL(long)
IGAME_BYTE_SWAP_IMPL(long long)
IGAME_BYTE_SWAP_IMPL(signed char)
IGAME_BYTE_SWAP_IMPL(unsigned char)
IGAME_BYTE_SWAP_IMPL(unsigned short)
IGAME_BYTE_SWAP_IMPL(unsigned int)
IGAME_BYTE_SWAP_IMPL(unsigned long)
IGAME_BYTE_SWAP_IMPL(unsigned long long)
#undef IGAME_BYTE_SWAP_IMPL

void ByteSwap::Swap2LE(void* p) {
	uint16_t* data = static_cast<uint16_t*>(p);
	*data = (*data >> 8) | (*data << 8); // 交换字节
}

void ByteSwap::Swap4LE(void* p) {
	uint32_t* data = static_cast<uint32_t*>(p);
	*data = (*data >> 24) |
		((*data & 0x00FF0000) >> 8) |
		((*data & 0x0000FF00) << 8) |
		(*data << 24); // 交换字节
}

void ByteSwap::Swap8LE(void* p) {
	uint64_t* data = static_cast<uint64_t*>(p);
	*data = (*data >> 56) |
		((*data & 0x00FF000000000000) >> 40) |
		((*data & 0x0000FF0000000000) >> 24) |
		((*data & 0x000000FF00000000) >> 8) |
		((*data & 0x00000000FF000000) << 8) |
		((*data & 0x0000000000FF0000) << 24) |
		((*data & 0x000000000000FF00) << 40) |
		(*data << 56); // 交换字节
}

void ByteSwap::Swap2LERange(void* p, size_t num) {
	return;// 小端不需要交换
}

void ByteSwap::Swap4LERange(void* p, size_t num) {
	uint32_t* data = static_cast<uint32_t*>(p);
	uint32_t temp = 0;
	for (size_t i = 0; i < num; ++i) {
		temp = data[i];
		data[i] = (temp & 0x000000FF) << 24 | (temp & 0x0000FF00) << 8 |
			(temp & 0x00FF0000) >> 8 | (temp & 0xFF000000) >> 24;
	}
}

void ByteSwap::Swap8LERange(void* p, size_t num) {
	uint64_t* data = static_cast<uint64_t*>(p);
	uint64_t temp = 0;
	for (size_t i = 0; i < num; ++i) {
		temp = data[i];
		data[i] = (temp & 0x00000000000000FF) << 56 | (temp & 0x000000000000FF00) << 40 |
			(temp & 0x00000000FF000000) << 24 | (temp & 0x0000FF0000000000) << 8 |
			(temp & 0x00FF000000000000) >> 8 | (temp & 0xFF00000000000000) >> 24 |
			(temp & 0x00000000FF000000) >> 8 | (temp & 0x0000FF0000000000) >> 40;
	}
}

bool ByteSwap::SwapWrite2LERange(const void* p, size_t num, FILE* f) {
	const uint16_t* data = static_cast<const uint16_t*>(p);
	for (size_t i = 0; i < num; ++i) {
		// 小端不需要交换
		if (fwrite(&data[i], sizeof(uint16_t), 1, f) != 1) {
			return false;
		}
	}
	return true;
}

bool ByteSwap::SwapWrite4LERange(const void* p, size_t num, FILE* f) {
	const uint32_t* data = static_cast<const uint32_t*>(p);
	uint32_t temp=0;
	for (size_t i = 0; i < num; ++i) {
		temp = data[i];
		temp = (temp & 0x000000FF) << 24 | (temp & 0x0000FF00) << 8 |
			(temp & 0x00FF0000) >> 8 | (temp & 0xFF000000) >> 24;
		if (fwrite(&temp, sizeof(uint32_t), 1, f) != 1) {
			return false;
		}
	}
	return true;
}

bool ByteSwap::SwapWrite8LERange(const void* p, size_t num, FILE* f) {
	const uint64_t* data = static_cast<const uint64_t*>(p);
	uint64_t temp=0;
	for (size_t i = 0; i < num; ++i) {
		temp = data[i];
		temp = (temp & 0x00000000000000FF) << 56 | (temp & 0x000000000000FF00) << 40 |
			(temp & 0x00000000FF000000) << 24 | (temp & 0x0000FF0000000000) << 8 |
			(temp & 0x00FF000000000000) >> 8 | (temp & 0xFF00000000000000) >> 24 |
			(temp & 0x00000000FF000000) >> 8 | (temp & 0x0000FF0000000000) >> 40;
		if (fwrite(&temp, sizeof(uint64_t), 1, f) != 1) {
			return false;
		}
	}
	return true;
}

void ByteSwap::SwapWrite2LERange(const void* p, size_t num, std::ostream* os) {
	const uint16_t* data = static_cast<const uint16_t*>(p);
	for (size_t i = 0; i < num; ++i) {
		// 小端不需要交换
		*os << data[i];
	}
}

void ByteSwap::SwapWrite4LERange(const void* p, size_t num, std::ostream* os) {
	const uint32_t* data = static_cast<const uint32_t*>(p);
	uint32_t temp=0;
	for (size_t i = 0; i < num; ++i) {
		temp = data[i];
		temp = (temp & 0x000000FF) << 24 | (temp & 0x0000FF00) << 8 |
			(temp & 0x00FF0000) >> 8 | (temp & 0xFF000000) >> 24;
		*os << temp;
	}
}

void ByteSwap::SwapWrite8LERange(const void* p, size_t num, std::ostream* os) {
	const uint64_t* data = static_cast<const uint64_t*>(p);
	uint64_t temp=0;
	for (size_t i = 0; i < num; ++i) {
		temp = data[i];
		temp = (temp & 0x00000000000000FF) << 56 | (temp & 0x000000000000FF00) << 40 |
			(temp & 0x00000000FF000000) << 24 | (temp & 0x0000FF0000000000) << 8 |
			(temp & 0x00FF000000000000) >> 8 | (temp & 0xFF00000000000000) >> 24 |
			(temp & 0x00000000FF000000) >> 8 | (temp & 0x0000FF0000000000) >> 40;
		*os << temp;
	}
}

void ByteSwap::Swap2BE(void* p) {
	uint16_t* data = static_cast<uint16_t*>(p);
	*data = (*data >> 8) | (*data << 8); // 交换字节
}

void ByteSwap::Swap4BE(void* p) {
	uint32_t* data = static_cast<uint32_t*>(p);
	*data = (*data >> 24) |
		((*data & 0x00FF0000) >> 8) |
		((*data & 0x0000FF00) << 8) |
		(*data << 24); // 交换字节
}

void ByteSwap::Swap8BE(void* p) {
	uint64_t* data = static_cast<uint64_t*>(p);
	*data = (*data >> 56) |
		((*data & 0x00FF000000000000) >> 40) |
		((*data & 0x0000FF0000000000) >> 24) |
		((*data & 0x000000FF00000000) >> 8) |
		((*data & 0x00000000FF000000) << 8) |
		((*data & 0x0000000000FF0000) << 24) |
		((*data & 0x000000000000FF00) << 40) |
		(*data << 56); // 交换字节
}

// Implement Swap2BERange and others
void ByteSwap::Swap2BERange(void* p, size_t num) {
	uint16_t* data = static_cast<uint16_t*>(p);
	uint16_t temp=0;
	for (size_t i = 0; i < num; ++i) {
		temp = data[i];
		data[i] = (temp >> 8) | (temp << 8); // 交换字节
	}
}

void ByteSwap::Swap4BERange(void* p, size_t num) {
	uint32_t* data = static_cast<uint32_t*>(p);
	uint32_t temp=0;
	for (size_t i = 0; i < num; ++i) {
		temp = data[i];
		data[i] = (temp >> 24) | ((temp & 0x00FF0000) >> 8) | ((temp & 0x0000FF00) << 8) | (temp << 24);
	}
}

void ByteSwap::Swap8BERange(void* p, size_t num) {
	uint64_t* data = static_cast<uint64_t*>(p);
	uint64_t temp=0;
	for (size_t i = 0; i < num; ++i) {
		temp = data[i];
		data[i] = (temp >> 56) | (temp << 56) |
			((temp & 0x00FF000000000000) >> 40) | ((temp & 0x000000000000FF00) << 40) |
			((temp & 0x0000FF0000000000) >> 24) | ((temp & 0x0000000000FF0000) << 24) |
			((temp & 0x000000FF00000000) >> 8) | ((temp & 0x00000000FF000000) << 8);
	}
}

bool ByteSwap::SwapWrite2BERange(const void* p, size_t num, FILE* f) {
	const uint16_t* data = static_cast<const uint16_t*>(p);
	uint16_t temp=0;
	for (size_t i = 0; i < num; ++i) {
		temp = data[i];
		temp = (temp >> 8) | (temp << 8); // 交换字节
		if (fwrite(&temp, sizeof(uint16_t), 1, f) != 1) {
			return false;
		}
	}
	return true;
}

bool ByteSwap::SwapWrite4BERange(const void* p, size_t num, FILE* f) {
	const uint32_t* data = static_cast<const uint32_t*>(p);
	uint32_t temp=0;
	for (size_t i = 0; i < num; ++i) {
		temp = data[i];
		temp = (temp >> 24) | ((temp & 0x00FF0000) >> 8) | ((temp & 0x0000FF00) << 8) | (temp << 24);
		if (fwrite(&temp, sizeof(uint32_t), 1, f) != 1) {
			return false;
		}
	}
	return true;
}

bool ByteSwap::SwapWrite8BERange(const void* p, size_t num, FILE* f) {
	const uint64_t* data = static_cast<const uint64_t*>(p);
	uint64_t temp=0;
	for (size_t i = 0; i < num; ++i) {
		temp = data[i];
		temp = (temp >> 56) | (temp << 56) |
			((temp & 0x00FF000000000000) >> 40) | ((temp & 0x000000000000FF00) << 40) |
			((temp & 0x0000FF0000000000) >> 24) | ((temp & 0x0000000000FF0000) << 24) |
			((temp & 0x000000FF00000000) >> 8) | ((temp & 0x00000000FF000000) << 8);
		if (fwrite(&temp, sizeof(uint64_t), 1, f) != 1) {
			return false;
		}
	}
	return true;
}

void ByteSwap::SwapWrite2BERange(const void* p, size_t num, std::ostream* os) {
	const uint16_t* data = static_cast<const uint16_t*>(p);
	uint16_t temp=0;
	for (size_t i = 0; i < num; ++i) {
		temp = data[i];
		temp = (temp >> 8) | (temp << 8); // 交换字节
		*os << temp;
	}
}

void ByteSwap::SwapWrite4BERange(const void* p, size_t num, std::ostream* os) {
	const uint32_t* data = static_cast<const uint32_t*>(p);
	uint32_t temp=0;
	for (size_t i = 0; i < num; ++i) {
		temp = data[i];
		temp = (temp >> 24) | ((temp & 0x00FF0000) >> 8) | ((temp & 0x0000FF00) << 8) | (temp << 24);
		*os << temp;
	}
}

void ByteSwap::SwapWrite8BERange(const void* p, size_t num, std::ostream* os) {
	const uint64_t* data = static_cast<const uint64_t*>(p);
	uint64_t temp=0;
	for (size_t i = 0; i < num; ++i) {
		temp = data[i];
		temp = (temp >> 56) | (temp << 56) |
			((temp & 0x00FF000000000000) >> 40) | ((temp & 0x000000000000FF00) << 40) |
			((temp & 0x0000FF0000000000) >> 24) | ((temp & 0x0000000000FF0000) << 24) |
			((temp & 0x000000FF00000000) >> 8) | ((temp & 0x00000000FF000000) << 8);
		*os << temp;
	}
}

// Swap functions for different byte sizes
void ByteSwap::Swap2(void* p) {
	if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_BIG) {
		ByteSwap::Swap2BE(p);
	}
	else if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_LITTLE) {
		ByteSwap::Swap2LE(p);
	}
}

void ByteSwap::Swap4(void* p) {
	if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_BIG) {
		ByteSwap::Swap4BE(p);
	}
	else if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_LITTLE) {
		ByteSwap::Swap4LE(p);
	}
}

void ByteSwap::Swap8(void* p) {
	if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_BIG) {
		ByteSwap::Swap8BE(p);
	}
	else if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_LITTLE) {
		ByteSwap::Swap8LE(p);
	}
}

void ByteSwap::Swap2Range(void* p, size_t num) {
	if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_BIG) {
		ByteSwap::Swap2BERange(p, num);
	}
	else if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_LITTLE) {
		ByteSwap::Swap2LERange(p, num);
	}
}


void ByteSwap::Swap4Range(void* p, size_t num) {
	if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_BIG) {
		ByteSwap::Swap4BERange(p, num);
	}
	else if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_LITTLE) {
		ByteSwap::Swap4LERange(p, num);
	}
}


void ByteSwap::Swap8Range(void* p, size_t num) {
	if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_BIG) {
		ByteSwap::Swap8BERange(p, num);
	}
	else if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_LITTLE) {
		ByteSwap::Swap8LERange(p, num);
	}
}

bool ByteSwap::SwapWrite2Range(const void* p, size_t num, FILE* f) {
	if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_BIG) {
		return ByteSwap::SwapWrite2BERange(p, num, f);
	}
	else if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_LITTLE) {
		return ByteSwap::SwapWrite2LERange(p, num, f);
	}
	return false;
}

bool ByteSwap::SwapWrite4Range(const void* p, size_t num, FILE* f) {
	if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_BIG) {
		return ByteSwap::SwapWrite4BERange(p, num, f);
	}
	else if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_LITTLE) {
		return ByteSwap::SwapWrite4LERange(p, num, f);
	}
	return false;
}
bool ByteSwap::SwapWrite8Range(const void* p, size_t num, FILE* f) {
	if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_BIG) {
		return ByteSwap::SwapWrite8BERange(p, num, f);
	}
	else if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_LITTLE) {
		return ByteSwap::SwapWrite8LERange(p, num, f);
	}
	return false;
}

void ByteSwap::SwapWrite2Range(const void* p, size_t num, std::ostream* os) {
	if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_BIG) {
		ByteSwap::SwapWrite2BERange(p, num, os);
	}
	else if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_LITTLE) {
		ByteSwap::SwapWrite2LERange(p, num, os);
	}
}
void ByteSwap::SwapWrite4Range(const void* p, size_t num, std::ostream* os) {
	if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_BIG) {
		ByteSwap::SwapWrite4BERange(p, num, os);
	}
	else if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_LITTLE) {
		ByteSwap::SwapWrite4LERange(p, num, os);
	}
}

void ByteSwap::SwapWrite8Range(const void* p, size_t num, std::ostream* os) {
	if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_BIG) {
		ByteSwap::SwapWrite8BERange(p, num, os);
	}
	else if (IGAME_ABI_ENDIAN_ID == IGAME_ABI_ENDIAN_ID_LITTLE) {
		ByteSwap::SwapWrite8LERange(p, num, os);
	}
}
IGAME_NAMESPACE_END
