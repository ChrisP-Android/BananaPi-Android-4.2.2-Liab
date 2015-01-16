#ifndef CDX_ATOMIC_H
#define CDX_ATOMIC_H

//#include <CdxTypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    volatile long counter;
} cdx_atomic_t;

/*引用计数+ 1 ，返回结果的数值*/
static inline long CdxAtomicInc(cdx_atomic_t *ref) 
{
    return __sync_add_and_fetch(&ref->counter, 1);
}

/*引用计数- 1 ，返回结果的数值*/
static inline long CdxAtomicDec(cdx_atomic_t *ref)
{
    return __sync_sub_and_fetch(&ref->counter, 1);
}

/*设置引用计数为val，返回设置前数值*/
static inline long CdxAtomicSet(cdx_atomic_t *ref, long val)
{
    return __sync_lock_test_and_set(&ref->counter, val);
}

/*读取引用计数数值*/
static inline long CdxAtomicRead(cdx_atomic_t *ref)
{
    return __sync_or_and_fetch(&ref->counter, 0);
}

#ifdef __cplusplus
}
#endif

#endif
