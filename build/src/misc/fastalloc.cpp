//---------------------------------------------------------------------------
//
// "華和梨" for あれ以外の何か以外の何か
// 小オブジェクト用アロケータ
//
// インタプリタは std::string や std::vector の生成・破棄で小さなメモリ確保を
// 大量に繰り返すため、OSのヒープではなく固定サイズのフリーリストで確保する。
// グローバルの operator new / delete を置き換える。
//
//  - MinGW (静的リンクのshiori.dll / kosui.exe) のみ有効。
//    他の環境ではこのファイルは何も定義しない。
//  - 確保済みのスラブは解放しない (フリーリストに戻して再利用するのみ)。
//  - 小さなブロックの先頭にサイズクラスを入れる。大きなブロックはmallocへ。
//  - kosuiのDSSTP受信スレッドから呼ばれる可能性があるのでスピンロックで保護。
//
// gcc.mak で FASTALLOC=no にすると、このファイルはビルドされない。
//
//---------------------------------------------------------------------------
#include "config.h"
//---------------------------------------------------------------------------
#if defined(__MINGW32__) && defined(__GNUC__) && (__GNUC__ >= 4)
//---------------------------------------------------------------------------
#include <new>
#include <cstdlib>
#include <cstddef>
//---------------------------------------------------------------------------
// C++11以降は例外指定の書き方が違う
#if __cplusplus >= 201103L
#	define KAWARI_THROW_BAD_ALLOC
#	define KAWARI_NOTHROW noexcept
#else
#	define KAWARI_THROW_BAD_ALLOC throw(std::bad_alloc)
#	define KAWARI_NOTHROW throw()
#endif
//---------------------------------------------------------------------------
namespace {
//---------------------------------------------------------------------------
// ブロック先頭のヘッダ。malloc同等のアラインメントを保つため、ポインタ2個分。
const std::size_t HEADER_SIZE = sizeof(void *) * 2;
// サイズクラスの刻み
const std::size_t GRAIN = 16;
// ヘッダ込みでこのサイズ以下のブロックをフリーリストで管理する
const std::size_t MAX_SMALL = 512;
const std::size_t CLASS_COUNT = MAX_SMALL / GRAIN + 1;
// スラブ(mallocでまとめて確保する単位)
const std::size_t SLAB_SIZE = 64 * 1024;
// mallocに任せたブロックのクラス番号
const std::size_t CLASS_LARGE = 0;
//---------------------------------------------------------------------------
struct TFreeNode {
	TFreeNode *next;
};

TFreeNode *FreeList[CLASS_COUNT];
char *SlabCurrent = 0;
char *SlabEnd = 0;
volatile long LockFlag = 0;
//---------------------------------------------------------------------------
inline void Lock(void)
{
	unsigned int spin = 0;
	while (__sync_lock_test_and_set(&LockFlag, 1)) {
		// ロック保持中のスレッドが切り替わった場合に備えて、たまにCPUを譲る
		if ((++spin & 0xff) == 0) ::SwitchToThread();
	}
}
//---------------------------------------------------------------------------
inline void Unlock(void)
{
	__sync_lock_release(&LockFlag);
}
//---------------------------------------------------------------------------
void *Allocate(std::size_t size)
{
	std::size_t total = size + HEADER_SIZE;
	if (total < size) return 0;		// オーバーフロー

	if (total > MAX_SMALL) {
		char *block = static_cast<char *>(std::malloc(total));
		if (!block) return 0;
		*reinterpret_cast<std::size_t *>(block) = CLASS_LARGE;
		return block + HEADER_SIZE;
	}

	std::size_t cls = (total + GRAIN - 1) / GRAIN;
	char *block;

	Lock();
	TFreeNode *node = FreeList[cls];
	if (node) {
		FreeList[cls] = node->next;
		block = reinterpret_cast<char *>(node);
	} else {
		std::size_t block_size = cls * GRAIN;
		if (static_cast<std::size_t>(SlabEnd - SlabCurrent) < block_size) {
			char *slab = static_cast<char *>(std::malloc(SLAB_SIZE));
			if (!slab) {
				Unlock();
				return 0;
			}
			// 残りは捨てる(最大 MAX_SMALL バイト)
			SlabCurrent = slab;
			SlabEnd = slab + SLAB_SIZE;
		}
		block = SlabCurrent;
		SlabCurrent += block_size;
	}
	Unlock();

	*reinterpret_cast<std::size_t *>(block) = cls;
	return block + HEADER_SIZE;
}
//---------------------------------------------------------------------------
void Release(void *ptr)
{
	if (!ptr) return;

	char *block = static_cast<char *>(ptr) - HEADER_SIZE;
	std::size_t cls = *reinterpret_cast<std::size_t *>(block);
	if (cls == CLASS_LARGE) {
		std::free(block);
		return;
	}

	TFreeNode *node = reinterpret_cast<TFreeNode *>(block);
	Lock();
	node->next = FreeList[cls];
	FreeList[cls] = node;
	Unlock();
}
//---------------------------------------------------------------------------
// new_handlerを呼びながら確保を試みる
void *AllocateOrHandle(std::size_t size)
{
	for (;;) {
		void *p = Allocate(size);
		if (p) return p;

		std::new_handler handler = std::set_new_handler(0);
		std::set_new_handler(handler);
		if (!handler) return 0;
		handler();
	}
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
void *operator new(std::size_t size) KAWARI_THROW_BAD_ALLOC
{
	void *p = AllocateOrHandle(size);
	if (!p) throw std::bad_alloc();
	return p;
}
//---------------------------------------------------------------------------
void *operator new[](std::size_t size) KAWARI_THROW_BAD_ALLOC
{
	void *p = AllocateOrHandle(size);
	if (!p) throw std::bad_alloc();
	return p;
}
//---------------------------------------------------------------------------
void *operator new(std::size_t size, const std::nothrow_t &) KAWARI_NOTHROW
{
	return AllocateOrHandle(size);
}
//---------------------------------------------------------------------------
void *operator new[](std::size_t size, const std::nothrow_t &) KAWARI_NOTHROW
{
	return AllocateOrHandle(size);
}
//---------------------------------------------------------------------------
void operator delete(void *ptr) KAWARI_NOTHROW
{
	Release(ptr);
}
//---------------------------------------------------------------------------
void operator delete[](void *ptr) KAWARI_NOTHROW
{
	Release(ptr);
}
//---------------------------------------------------------------------------
void operator delete(void *ptr, const std::nothrow_t &) KAWARI_NOTHROW
{
	Release(ptr);
}
//---------------------------------------------------------------------------
void operator delete[](void *ptr, const std::nothrow_t &) KAWARI_NOTHROW
{
	Release(ptr);
}
//---------------------------------------------------------------------------
#endif
