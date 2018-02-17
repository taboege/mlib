/*
 * M*LIB - SNAPSHOT Module
 *
 * Copyright (c) 2017-2018, Patrick Pelissier
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * + Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * + Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef __M_SNAPSHOT
#define __M_SNAPSHOT

#include "m-atomic.h"
#include "m-core.h"
#include "m-genint.h"

// For compatibility with previous version
#define SNAPSHOT_DEF SNAPSHOT_SRSW_DEF

/* Define a SRSW snapshot and it function
   USAGE: SNAPSHOT_SRSW_DEF(name, type[, oplist]) */
#define SNAPSHOT_SRSW_DEF(name, ...)                                    \
  SNAPSHOTI_SRSW_DEF(M_IF_NARGS_EQ1(__VA_ARGS__)                        \
                     ((name, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__) ), \
                      (name, __VA_ARGS__ )))

/* Define a MRSW snapshot and it function
   USAGE: SNAPSHOT_MRSW_DEF(name, type[, oplist]) */
#define SNAPSHOT_MRSW_DEF(name, ...)                                    \
  SNAPSHOTI_MRSW_DEF(M_IF_NARGS_EQ1(__VA_ARGS__)                        \
                     ((name, __VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__) ), \
                      (name, __VA_ARGS__ )))

/* Define the oplist of a snapshot.
   USAGE: SNAPSHOT_OPLIST(name[, oplist]) */
#define SNAPSHOT_OPLIST(...)                                            \
  SNAPSHOTI_OPLIST(M_IF_NARGS_EQ1(__VA_ARGS__)                          \
                   ((__VA_ARGS__, M_GLOBAL_OPLIST_OR_DEF(__VA_ARGS__) ), \
                    (__VA_ARGS__ )))


/********************************** INTERNAL ************************************/

// deferred
#define SNAPSHOTI_OPLIST(arg) SNAPSHOTI_OPLIST2 arg

/* Define the oplist of a snapshot */
#define SNAPSHOTI_OPLIST2(name, oplist)					\
  (INIT(M_C(name, _init)),						\
   INIT_SET(M_C(name, _init_set)),					\
   SET(M_C(name, _set)),						\
   CLEAR(M_C(name, _clear)),						\
   TYPE(M_C(name, _t)),							\
   SUBTYPE(M_C(name_, type_t))						\
   ,OPLIST(oplist)                                                      \
   ,M_IF_METHOD(INIT_MOVE, oplist)(INIT_MOVE(M_C(name, _init_move)),)	\
   ,M_IF_METHOD(MOVE, oplist)(MOVE(M_C(name, _move)),)			\
   )


/******************************** INTERNAL SRSW **********************************/

/* Flag defining the atomic state of a snapshot:
 * - r: Index of the read buffer Range [0..2]
 * - w: Index of the write buffer  Range [0..2]
 * - f: Next index of the write buffer when a shot is taken Range [0..2]
 * - b: Boolean indicating that the read buffer shall be updated
 */
#define SNAPSHOTI_SRSW_FLAG(r, w, f, b)					\
  ((unsigned char)( ( (r) << 4) | ((w) << 2) | ((f)) | ((b) << 6)))
#define SNAPSHOTI_SRSW_R(flags)			\
  (((flags) >> 4) & 0x03u)
#define SNAPSHOTI_SRSW_W(flags)			\
  (((flags) >> 2) & 0x03u)
#define SNAPSHOTI_SRSW_F(flags)			\
  (((flags) >> 0) & 0x03u)
#define SNAPSHOTI_SRSW_B(flags)			\
  (((flags) >> 6) & 0x01u)

/* NOTE: Due to atomic_load only accepting non-const pointer,
   we can't have any const in the interface. */
#define SNAPSHOTI_SRSW_FLAGS_CONTRACT(flags)                            \
  assert(SNAPSHOTI_SRSW_R(flags) != SNAPSHOTI_SRSW_W(flags)             \
	 && SNAPSHOTI_SRSW_R(flags) != SNAPSHOTI_SRSW_F(flags)          \
	 && SNAPSHOTI_SRSW_W(flags) != SNAPSHOTI_SRSW_F(flags))
#define SNAPSHOTI_SRSW_CONTRACT(snap)	do {                            \
    assert((snap) != NULL);						\
    unsigned char f = atomic_load (&(snap)->flags);                     \
    SNAPSHOTI_SRSW_FLAGS_CONTRACT(f);                                   \
  } while (0)

// Defered evaluation (TBC if it really helps).
#define SNAPSHOTI_SRSW_DEF(arg)	SNAPSHOTI_SRSW_DEF2 arg

// This is basically an atomic triple buffer (Lock Free)
// between a produced thread and a consummer thread.
#define SNAPSHOTI_SRSW_MAX_BUFFER             3

#define SNAPSHOTI_SRSW_DEF2(name, type, oplist)				\
                                                                        \
  /* Create an aligned type to avoid false sharing between threads */   \
  typedef struct M_C(name, _aligned_type_s) {                           \
    type         x;							\
    char align[M_ALIGN_FOR_CACHELINE_EXCLUSION > sizeof(type) ? M_ALIGN_FOR_CACHELINE_EXCLUSION - sizeof(type) : 1]; \
  } M_C(name, _aligned_type_t);                                         \
                                                                        \
  typedef struct M_C(name, _s) {					\
    M_C(name, _aligned_type_t)  data[SNAPSHOTI_SRSW_MAX_BUFFER];        \
    atomic_uchar flags;                                                 \
  } M_C(name, _t)[1];							\
                                                                        \
  typedef type M_C(name, _type_t);                                      \
                                                                        \
  static inline void M_C(name, _init)(M_C(name, _t) snap)               \
  {									\
    assert(snap != NULL);						\
    for(int i = 0; i < SNAPSHOTI_SRSW_MAX_BUFFER; i++) {                \
      M_GET_INIT oplist(snap->data[i].x);                               \
    }									\
    atomic_init (&snap->flags, SNAPSHOTI_SRSW_FLAG(0, 1, 2, 0));        \
    SNAPSHOTI_SRSW_CONTRACT(snap);                                      \
  }									\
                                                                        \
  static inline void M_C(name, _clear)(M_C(name, _t) snap)		\
  {									\
    SNAPSHOTI_SRSW_CONTRACT(snap);                                      \
    for(int i = 0; i < SNAPSHOTI_SRSW_MAX_BUFFER; i++) {                \
      M_GET_CLEAR oplist(snap->data[i].x);				\
    }									\
  }									\
                                                                        \
  static inline void M_C(name, _init_set)(M_C(name, _t) snap,		\
					  M_C(name, _t) org)		\
  {									\
    SNAPSHOTI_SRSW_CONTRACT(org);                                       \
    assert(snap != NULL && snap != org);				\
    for(int i = 0; i < SNAPSHOTI_SRSW_MAX_BUFFER; i++) {                \
      M_GET_INIT_SET oplist(snap->data[i].x, org->data[i].x);		\
    }									\
    atomic_init (&snap->flags, atomic_load(&org->flags));		\
    SNAPSHOTI_SRSW_CONTRACT(snap);                                      \
  }									\
                                                                        \
  static inline void M_C(name, _set)(M_C(name, _t) snap,		\
				     M_C(name, _t) org)			\
  {									\
    SNAPSHOTI_SRSW_CONTRACT(snap);                                      \
    SNAPSHOTI_SRSW_CONTRACT(org);                                       \
    for(int i = 0; i < SNAPSHOTI_SRSW_MAX_BUFFER; i++) {                \
      M_GET_SET oplist(snap->data[i].x, org->data[i].x);                \
    }									\
    atomic_init (&snap->flags, atomic_load(&org->flags));		\
    SNAPSHOTI_SRSW_CONTRACT(snap);                                      \
  }									\
                                                                        \
  M_IF_METHOD(INIT_MOVE, oplist)(					\
    static inline void M_C(name, _init_move)(M_C(name, _t) snap,        \
					     M_C(name, _t) org)		\
    {									\
      SNAPSHOTI_SRSW_CONTRACT(org);                                     \
      assert(snap != NULL && snap != org);				\
      for(int i = 0; i < SNAPSHOTI_SRSW_MAX_BUFFER; i++) {              \
	M_GET_INIT_MOVE oplist(snap->data[i].x, org->data[i].x);        \
      }									\
      atomic_store (&snap->flags, atomic_load(&org->flags));            \
      atomic_store (&org->flags, SNAPSHOTI_SRSW_FLAG(0,0,0,0) );        \
      SNAPSHOTI_SRSW_CONTRACT(snap);                                    \
    }									\
    ,) /* IF_METHOD (INIT_MOVE) */					\
                                                                        \
   M_IF_METHOD(MOVE, oplist)(                                           \
     static inline void M_C(name, _move)(M_C(name, _t) snap,            \
					 M_C(name, _t) org)		\
     {									\
       SNAPSHOTI_SRSW_CONTRACT(snap);					\
       SNAPSHOTI_SRSW_CONTRACT(org);                                    \
       assert(snap != org);						\
       for(int i = 0; i < SNAPSHOTI_SRSW_MAX_BUFFER; i++) {             \
	 M_GET_MOVE oplist(snap->data[i].x, org->data[i].x);		\
       }								\
       atomic_store (&snap->flags, atomic_load(&org->flags));           \
       atomic_store (&org->flags, SNAPSHOTI_SRSW_FLAG(0,0,0,0) );       \
       SNAPSHOTI_SRSW_CONTRACT(snap);					\
     }									\
     ,) /* IF_METHOD (MOVE) */						\
                                                                        \
                                                                        \
  static inline type *M_C(name, _write)(M_C(name, _t) snap)             \
  {									\
    SNAPSHOTI_SRSW_CONTRACT(snap);                                      \
    unsigned char nextFlags, origFlags = atomic_load (&snap->flags);	\
    /* Atomic CAS operation */                                          \
    do {								\
      /* Swap F and W buffer, setting exchange flag */                  \
      nextFlags = SNAPSHOTI_SRSW_FLAG(SNAPSHOTI_SRSW_R(origFlags),      \
                                      SNAPSHOTI_SRSW_F(origFlags),      \
                                      SNAPSHOTI_SRSW_W(origFlags), 1);  \
      /* exponential backoff is not needed as there can't be more       \
         than 2 threads which try to update the data. */                \
    } while (!atomic_compare_exchange_weak (&snap->flags, &origFlags,	\
					    nextFlags));		\
    /* Return new write buffer for new updating */                      \
    return &snap->data[SNAPSHOTI_SRSW_W(nextFlags)].x;                  \
  }									\
                                                                        \
  static inline const type *M_C(name, _read)(M_C(name, _t) snap)	\
  {									\
    SNAPSHOTI_SRSW_CONTRACT(snap);                                      \
    unsigned char nextFlags, origFlags = atomic_load (&snap->flags);	\
    /* Atomic CAS operation */                                          \
    do {								\
      /* If no exchange registered, do nothing and keep the same */     \
      if (!SNAPSHOTI_SRSW_B(origFlags)) {                               \
        nextFlags = origFlags;                                          \
	break;								\
      }                                                                 \
      /* Swap R and F buffer, clearing exchange flag */                 \
      nextFlags = SNAPSHOTI_SRSW_FLAG(SNAPSHOTI_SRSW_F(origFlags),      \
                                      SNAPSHOTI_SRSW_W(origFlags),      \
                                      SNAPSHOTI_SRSW_R(origFlags), 0);  \
      /* exponential backoff is not needed as there can't be more       \
         than 2 threads which try to update the data. */                \
    } while (!atomic_compare_exchange_weak (&snap->flags, &origFlags,	\
					    nextFlags));		\
    /* Return current read buffer */                                    \
    return M_CONST_CAST(type, &snap->data[SNAPSHOTI_SRSW_R(nextFlags)].x); \
  }									\
                                                                        \
  static inline bool M_C(name, _updated_p)(M_C(name, _t) snap)          \
  {									\
    SNAPSHOTI_SRSW_CONTRACT(snap);                                      \
    unsigned char flags = atomic_load (&snap->flags);                   \
    return SNAPSHOTI_SRSW_B(flags);                                     \
  }									\
                                                                        \
  static inline type *M_C(name, _get_write_buffer)(M_C(name, _t) snap)	\
  {									\
    SNAPSHOTI_SRSW_CONTRACT(snap);                                      \
    unsigned char flags = atomic_load(&snap->flags);			\
    return &snap->data[SNAPSHOTI_SRSW_W(flags)].x;                      \
  }									\
                                                                        \
  static inline const type *M_C(name, _get_read_buffer)(M_C(name, _t) snap) \
  {									\
    SNAPSHOTI_SRSW_CONTRACT(snap);                                      \
    unsigned char flags = atomic_load(&snap->flags);			\
    return  M_CONST_CAST(type, &snap->data[SNAPSHOTI_SRSW_R(flags)].x);	\
  }									\
  

/******************************** INTERNAL MRSW **********************************/

#define SNAPSHOTI_MRSW_INT_FLAG(w, n) ( ((w) << 1) | (n) )
#define SNAPSHOTI_MRSW_INT_FLAG_W(f)  ((f) >> 1)
#define SNAPSHOTI_MRSW_INT_FLAG_N(f)  ((f) & 1)

// 2 more buffer than the number of readers are needed
#define SNAPSHOTI_MRSW_EXTRA_BUFFER 2

// due to genint
#define SNAPSHOTI_MRSW_MAX_READER (4096-SNAPSHOTI_MRSW_EXTRA_BUFFER)

// structure to handle MRSW snapshot but return a unique index in a buffer array.
typedef struct snapshot_mrsw_int_s {
  atomic_uint lastNext;
  unsigned int currentWrite;
  size_t n;
  atomic_uint *cptTab;
  genint_t freeList;
} snapshot_mrsw_int_t[1];

#define SNAPSHOTI_MRSW_INT_CONTRACT(s) do {                             \
    assert (s != NULL);                                                 \
    assert (s->n > 0 && s->n <= SNAPSHOTI_MRSW_MAX_READER);             \
    assert (s->currentWrite < s->n + SNAPSHOTI_MRSW_EXTRA_BUFFER);      \
    assert (SNAPSHOTI_MRSW_INT_FLAG_W(atomic_load(&s->lastNext))        \
            <= s->n + SNAPSHOTI_MRSW_EXTRA_BUFFER);                     \
    assert (s->cptTab != NULL);                                         \
    assert (atomic_load(&s->cptTab[s->currentWrite]) == 1);             \
  } while (0)

static inline void snapshot_mrsw_int_init(snapshot_mrsw_int_t s, size_t n)
{
  assert (s != NULL);
  assert (n >= 1 && n <= SNAPSHOTI_MRSW_MAX_READER);
  s->n = n;
  n += SNAPSHOTI_MRSW_EXTRA_BUFFER;
  atomic_uint *ptr = M_MEMORY_REALLOC (atomic_uint, NULL, n);
  if (M_UNLIKELY (ptr == NULL)) {
    M_MEMORY_FULL(sizeof (atomic_uint) * n);
    return;
  }
  s->cptTab = ptr;
  for(size_t i = 0; i < n; i++)
    atomic_init(&s->cptTab[i], 0U);
  genint_init (s->freeList, n);
  // Get a free buffer and set it as available for readers
  unsigned int w = genint_pop(s->freeList);
  atomic_store(&s->cptTab[w], 1);
  atomic_init(&s->lastNext, SNAPSHOTI_MRSW_INT_FLAG(w, true));
  // Get working buffer
  s->currentWrite = genint_pop(s->freeList);
  atomic_store(&s->cptTab[s->currentWrite], 1);
  SNAPSHOTI_MRSW_INT_CONTRACT(s);
}

static inline void snapshot_mrsw_int_clear(snapshot_mrsw_int_t s)
{
  SNAPSHOTI_MRSW_INT_CONTRACT(s);
  M_MEMORY_FREE (s->cptTab);
  genint_clear(s->freeList);
  s->cptTab = NULL;
  s->n = 0;
}

static inline unsigned int snapshot_mrsw_int_get_write_idx(const snapshot_mrsw_int_t s)
{
  SNAPSHOTI_MRSW_INT_CONTRACT(s);
  return s->currentWrite;
}

static inline unsigned int snapshot_mrsw_int_write(snapshot_mrsw_int_t s)
{
  SNAPSHOTI_MRSW_INT_CONTRACT(s);
  unsigned int idx = s->currentWrite;
  unsigned int newNext, previous = atomic_load(&s->lastNext);
  do {
    newNext = SNAPSHOTI_MRSW_INT_FLAG(idx, true);
  } while (!atomic_compare_exchange_weak(&s->lastNext, &previous, newNext));
  if (SNAPSHOTI_MRSW_INT_FLAG_N(previous)) {
    // Reuse previous buffer as it was not used by any reader
    s->currentWrite = SNAPSHOTI_MRSW_INT_FLAG_W(previous);
  } else {
    // Free the write index
    idx = SNAPSHOTI_MRSW_INT_FLAG_W(previous);
    unsigned int c = atomic_fetch_sub(&s->cptTab[idx], 1);
    assert (c != 0 && c <= s->n + 1);
    // Get a new buffer.
    if (c != 1)
      idx = genint_pop(s->freeList);
    s->currentWrite = idx;
    assert (idx < s->n + SNAPSHOTI_MRSW_EXTRA_BUFFER);
    assert (atomic_load(&s->cptTab[idx]) == 0);
    atomic_store(&s->cptTab[idx], 1);
  }
  SNAPSHOTI_MRSW_INT_CONTRACT(s);
  return s->currentWrite;
}

static inline unsigned int snapshot_mrsw_int_read_start(snapshot_mrsw_int_t s)
{
  SNAPSHOTI_MRSW_INT_CONTRACT(s);
  unsigned int previous = atomic_load(&s->lastNext);
  unsigned int idx;
  while (true) {
    idx = SNAPSHOTI_MRSW_INT_FLAG_W(previous);
    unsigned int newNext = SNAPSHOTI_MRSW_INT_FLAG(idx, false);
    // Reserve the index
    unsigned int c = atomic_fetch_add(&s->cptTab[idx], 1);
    assert (c <= s->n + 1);
    // Try to ack it
    if (atomic_compare_exchange_weak(&s->lastNext, &previous, newNext))
      break;
    // If we have been preempted by another read thread
    if (idx == SNAPSHOTI_MRSW_INT_FLAG_W(previous)) {
      // This is still ok if the index has not changed
      assert (SNAPSHOTI_MRSW_INT_FLAG_N(previous) == false);
      break;
    }
    // Free the reserved index as we failed it to ack it
    c = atomic_fetch_sub(&s->cptTab[idx], 1);
    assert (c != 0 && c <= s->n+1);
    if (c == 1)
      genint_push(s->freeList, idx);
  }
  SNAPSHOTI_MRSW_INT_CONTRACT(s);
  return idx;
}

static inline void snapshot_mrsw_int_read_end(snapshot_mrsw_int_t s, unsigned int idx)
{
  SNAPSHOTI_MRSW_INT_CONTRACT(s);
  assert (idx < s->n + SNAPSHOTI_MRSW_EXTRA_BUFFER);
  // Decrement reference counter of the buffer
  unsigned int c = atomic_fetch_sub(&s->cptTab[idx], 1);
  assert (c != 0 && c <= s->n+1);
  if (c == 1) {
    // Buffer no longer used by any reader thread.
    // Push back index in free list
    genint_push(s->freeList, idx);
  }
  SNAPSHOTI_MRSW_INT_CONTRACT(s);
}

#define SNAPSHOTI_MRSW_CONTRACT(snap) do {                              \
    assert (snap != NULL);                                              \
    assert (snap->data != NULL);                                        \
  } while (0)


// Defered evaluation (TBC if it really helps).
#define SNAPSHOTI_MRSW_DEF(arg)	SNAPSHOTI_MRSW_DEF2 arg

#define SNAPSHOTI_MRSW_DEF2(name, type, oplist)                         \
                                                                        \
  /* Create an aligned type to avoid false sharing between threads */   \
  typedef struct M_C(name, _aligned_type_s) {                           \
    type         x;							\
    char align[M_ALIGN_FOR_CACHELINE_EXCLUSION > sizeof(type) ? M_ALIGN_FOR_CACHELINE_EXCLUSION - sizeof(type) : 1]; \
  } M_C(name, _aligned_type_t);                                         \
                                                                        \
  typedef struct M_C(name, _s) {					\
    M_C(name, _aligned_type_t)  *data;                                  \
    snapshot_mrsw_int_t          core;                                  \
  } M_C(name, _t)[1];							\
                                                                        \
  typedef type M_C(name, _type_t);                                      \
                                                                        \
  static inline void M_C(name, _init)(M_C(name, _t) snap, size_t nReader) \
  {									\
    assert (snap != NULL);						\
    assert (nReader > 0 && nReader <= SNAPSHOTI_MRSW_MAX_READER);       \
    snap->data = M_GET_REALLOC oplist (M_C(name, _aligned_type_t),      \
                                       NULL,                            \
                                       nReader+SNAPSHOTI_MRSW_EXTRA_BUFFER); \
    for(size_t i = 0; i < nReader + SNAPSHOTI_MRSW_EXTRA_BUFFER; i++) { \
      M_GET_INIT oplist(snap->data[i].x);                               \
    }									\
    snapshot_mrsw_int_init(snap->core, nReader);                        \
    SNAPSHOTI_MRSW_CONTRACT(snap);                                      \
  }									\
                                                                        \
  static inline void M_C(name, _clear)(M_C(name, _t) snap)		\
  {									\
    SNAPSHOTI_MRSW_CONTRACT(snap);                                      \
    for(int i = 0; i < SNAPSHOTI_SRSW_MAX_BUFFER; i++) {                \
      M_GET_CLEAR oplist(snap->data[i].x);				\
    }									\
    snapshot_mrsw_int_clear(snap->core);                                \
  }									\
                                                                        \
  static inline type *M_C(name, _write)(M_C(name, _t) snap)             \
  {									\
    SNAPSHOTI_MRSW_CONTRACT(snap);                                      \
    const unsigned int idx = snapshot_mrsw_int_write(snap->core);       \
    return &snap->data[idx].x;                                          \
  }									\
                                                                        \
  static inline const type *M_C(name, _read_start)(M_C(name, _t) snap)	\
  {									\
    SNAPSHOTI_MRSW_CONTRACT(snap);                                      \
    const unsigned int idx = snapshot_mrsw_int_read_start(snap->core);  \
    return M_CONST_CAST(type, &snap->data[idx].x);                      \
  }									\
                                                                        \
  static inline void M_C(name, _read_end)(M_C(name, _t) snap, const type *old) \
  {									\
    SNAPSHOTI_MRSW_CONTRACT(snap);                                      \
    assert (old != NULL);                                               \
    const M_C(name, _aligned_type_t) *oldx;                             \
    oldx = M_CTYPE_FROM_FIELD(M_C(name, _aligned_type_t), old,          \
                              type, x);                                 \
    assert (oldx >= snap->data);                                        \
    assert (oldx < snap->data + snap->core->n+SNAPSHOTI_MRSW_EXTRA_BUFFER); \
    const unsigned int idx = oldx - snap->data;                         \
    snapshot_mrsw_int_read_end(snap->core, idx);                        \
  }									\
                                                                        \
  static inline type *M_C(name, _get_write_buffer)(M_C(name, _t) snap)	\
  {									\
    SNAPSHOTI_MRSW_CONTRACT(snap);                                      \
    unsigned int idx = snapshot_mrsw_int_get_write_idx(snap->core);     \
    return &snap->data[idx].x;                                          \
  }									\
                                                                        \
  //TODO: _set_, _init_set

#endif
