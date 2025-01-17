#define NDEBUG

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "m-array.h"
#include "m-list.h"
#include "m-rbtree.h"
#include "m-bptree.h"
#include "m-dict.h"
#include "m-algo.h"
#include "m-mempool.h"
#include "m-string.h"
#include "m-buffer.h"
#include "m-mutex.h"
#include "m-deque.h"
#include "m-concurrent.h"

#include "common.h"

/********************************************************************************************/

ARRAY_DEF(array_uint, unsigned int)

static void test_array(size_t n)
{
  M_LET(a1, a2, ARRAY_OPLIST(array_uint)) {
    for(size_t i = 0; i < n; i++) {
      array_uint_push_back(a1, rand_get() );
      array_uint_push_back(a2, rand_get() );
    }
    unsigned int s = 0;
    for(unsigned long i = 0; i < n; i++) {
      s += *array_uint_cget(a1, i ) * *array_uint_cget(a2, i );
    }
    g_result = s;
  }
}

/********************************************************************************************/

// Two benched mode: with or without mempool
#ifdef USE_MEMPOOL
LIST_DEF(list_uint, unsigned int, (MEMPOOL( list_mpool), MEMPOOL_LINKAGE(static)))
#else
LIST_DEF(list_uint, unsigned int)
#endif

static void test_list (size_t n)
{
#ifdef USE_MEMPOOL
  list_uint_mempool_init(list_mpool);
#endif
  M_LET(a1, a2, LIST_OPLIST(list_uint)) {
    for(size_t i = 0; i < n; i++) {
      list_uint_push_back(a1, rand_get() );
      list_uint_push_back(a2, rand_get() );
    }
    unsigned int s = 0;
    list_uint_it_t it1, it2;
    for(list_uint_it(it1, a1), list_uint_it(it2,a2); !list_uint_end_p(it1); list_uint_next(it1), list_uint_next(it2)) {
      s += *list_uint_cref(it1) * *list_uint_cref(it2);
    }
    g_result = s;
  }
#ifdef USE_MEMPOOL
  list_uint_mempool_clear(list_mpool);
#endif
}

/********************************************************************************************/

// Two benched mode: with or without mempool
#ifdef USE_MEMPOOL
LIST_DUAL_PUSH_DEF(dlist_uint, unsigned int, (MEMPOOL( dlist_mpool), MEMPOOL_LINKAGE(static)))
#else
LIST_DUAL_PUSH_DEF(dlist_uint, unsigned int)
#endif

static void test_dlist (size_t n)
{
#ifdef USE_MEMPOOL
  dlist_uint_mempool_init(dlist_mpool);
#endif
  M_LET(a1, a2, LIST_OPLIST(dlist_uint)) {
    for(size_t i = 0; i < n; i++) {
      dlist_uint_push_back(a1, rand_get() );
      dlist_uint_push_back(a2, rand_get() );
    }
    unsigned int s = 0;
    dlist_uint_it_t it1, it2;
    for(dlist_uint_it(it1, a1), dlist_uint_it(it2,a2); !dlist_uint_end_p(it1); dlist_uint_next(it1), dlist_uint_next(it2)) {
      s += *dlist_uint_cref(it1) * *dlist_uint_cref(it2);
    }
    g_result = s;
  }
#ifdef USE_MEMPOOL
  dlist_uint_mempool_clear(dlist_mpool);
#endif
}

/********************************************************************************************/

#ifdef USE_MEMPOOL
RBTREE_DEF(rbtree_ulong, unsigned long, (MEMPOOL( rbtree_mpool), MEMPOOL_LINKAGE(static)))
#else
RBTREE_DEF(rbtree_ulong, unsigned long)
#endif

static void test_rbtree(size_t n)
{
#ifdef USE_MEMPOOL
  rbtree_ulong_mempool_init(rbtree_mpool);
#endif
  M_LET(tree, RBTREE_OPLIST(rbtree_ulong)) {
    for (size_t i = 0; i < n; i++) {
      rbtree_ulong_push(tree, rand_get());
    }
    rand_init();
    unsigned int s = 0;
    for (size_t i = 0; i < n; i++) {
      unsigned long *p = rbtree_ulong_get(tree, rand_get());
      if (p)
        s += *p;
    }
    g_result = s;
  }
#ifdef USE_MEMPOOL
  rbtree_ulong_mempool_clear(rbtree_mpool);
#endif
}

/********************************************************************************************/

BPTREE_DEF(bptree_ulong, 21, unsigned long, M_DEFAULT_OPLIST)

static void test_bptree(size_t n)
{
  M_LET(tree, BPTREE_OPLIST(bptree_ulong)) {
      for (size_t i = 0; i < n; i++) {
        bptree_ulong_push(tree, rand_get());
      }
      rand_init();
      unsigned int s = 0;
      for (size_t i = 0; i < n; i++) {
        unsigned long *p = bptree_ulong_get(tree, rand_get());
        if (p)
          s += *p;
      }
      g_result = s;
  }
}

/********************************************************************************************/

#ifdef USE_MEMPOOL
DICT_DEF2(dict_ulong, unsigned long, M_OPEXTEND(M_DEFAULT_OPLIST, MEMPOOL(dict_mpool), MEMPOOL_LINKAGE(static)), unsigned long, M_DEFAULT_OPLIST)
#else
DICT_DEF2(dict_ulong, unsigned long, M_DEFAULT_OPLIST, unsigned long, M_DEFAULT_OPLIST)
#endif

static void
test_dict(size_t  n)
{
#ifdef USE_MEMPOOL
  dict_ulong_list_pair_mempool_init(dict_mpool);
#endif
  M_LET(dict, DICT_OPLIST(dict_ulong)) {
    for (size_t i = 0; i < n; i++) {
      unsigned long value = rand_get();
      unsigned long key = rand_get();
      dict_ulong_set_at(dict, key, value);
    }
    rand_init();
    unsigned int s = 0;
    for (size_t i = 0; i < n; i++) {
      unsigned long *p = dict_ulong_get(dict, rand_get());
      if (p)
        s += *p;
    }
    g_result = s;
  }
#ifdef USE_MEMPOOL
  dict_ulong_list_pair_mempool_clear(dict_mpool);
#endif
}

/********************************************************************************************/

static inline bool oor_equal_p(unsigned long k, unsigned char n)
{
  return k == (unsigned long)n;
}

static inline void oor_set(unsigned long *k, unsigned char n)
{
  *k = (unsigned long)n;
}

DICT_OA_DEF2(dict_oa_ulong,
	     unsigned long, M_OPEXTEND(M_DEFAULT_OPLIST, OOR_EQUAL(oor_equal_p), OOR_SET(oor_set M_IPTR)),
	     unsigned long, M_DEFAULT_OPLIST)

static void
test_dict_oa(size_t  n)
{
  M_LET(dict, DICT_OPLIST(dict_oa_ulong)) {
    for (size_t i = 0; i < n; i++) {
      dict_oa_ulong_set_at(dict, rand_get(), rand_get() );
    }
    rand_init();
    unsigned int s = 0;
    for (size_t i = 0; i < n; i++) {
      unsigned long *p = dict_oa_ulong_get(dict, rand_get());
      if (p)
        s += *p;
    }
    g_result = s;
  }
}

/********************************************************************************************/

typedef char char_array_t[256];
static void char_init (char_array_t a) { a[0] = 0; }
static void char_set (char_array_t a, const char_array_t b) { strcpy(a, b); }
static bool char_equal_p (const char_array_t a, const char_array_t b) { return strcmp(a,b)==0; }
static size_t char_hash(const char_array_t a) { return m_core_hash (a, strlen(a)); }
#define M_OPL_char_array_t() (INIT(char_init), INIT_SET(char_set), SET(char_set), CLEAR(char_init), HASH(char_hash), EQUAL(char_equal_p))

#ifdef USE_MEMPOOL
DICT_STOREHASH_DEF2(dict_char, char_array_t, M_OPEXTEND(M_OPL_char_array_t(),MEMPOOL(dict_mpool2), MEMPOOL_LINKAGE(static)), char_array_t, M_OPL_char_array_t())
#else
DICT_STOREHASH_DEF2(dict_char, char_array_t, M_OPL_char_array_t(), char_array_t, M_OPL_char_array_t())
#endif

static void
test_dict_big(size_t  n)
{
#ifdef USE_MEMPOOL
  dict_char_list_pair_mempool_init(dict_mpool2);
#endif
  M_LET(dict, DICT_OPLIST(dict_char)) {
    for (size_t i = 0; i < n; i++) {
      char_array_t s1, s2;
      sprintf(s1, "%u", rand_get());
      sprintf(s2, "%u", rand_get());
      dict_char_set_at(dict, s1, s2);
    }
    rand_init();
    unsigned int s = 0;
    for (size_t i = 0; i < n; i++) {
      char_array_t s1;
      sprintf(s1, "%u", rand_get());
      char_array_t *p = dict_char_get(dict, s1);
      if (p)
        s ++;
    }
    g_result = s;
  }
#ifdef USE_MEMPOOL
  dict_char_list_pair_mempool_clear(dict_mpool2);
#endif
}

/********************************************************************************************/

#ifdef USE_MEMPOOL
DICT_DEF2(dict_str, string_t, M_OPEXTEND(STRING_OPLIST, MEMPOOL(dict_mpool3), MEMPOOL_LINKAGE(static)), string_t, STRING_OPLIST)
#else
DICT_DEF2(dict_str, string_t, STRING_OPLIST, string_t, STRING_OPLIST)
#endif

static void
test_dict_str(size_t  n)
{
#ifdef USE_MEMPOOL
  dict_str_list_pair_mempool_init(dict_mpool3);
#endif
  M_LET(s1, s2, STRING_OPLIST)
  M_LET(dict, DICT_OPLIST(dict_str)) {
    for (size_t i = 0; i < n; i++) {
      string_printf(s1, "%u", rand_get());
      string_printf(s2, "%u", rand_get());
      dict_str_set_at(dict, s1, s2);
    }
    rand_init();
    unsigned int s = 0;
    for (size_t i = 0; i < n; i++) {
      string_printf(s1, "%u", rand_get());
      string_t *p = dict_str_get(dict, s1);
      if (p)
        s ++;
    }
    g_result = s;
  }
#ifdef USE_MEMPOOL
  dict_str_list_pair_mempool_clear(dict_mpool3);
#endif
}

/********************************************************************************************/

ARRAY_DEF(array_float, float)
ALGO_DEF(array_float, ARRAY_OPLIST(array_float))

static void test_sort(size_t n)
{
  M_LET(a1, ARRAY_OPLIST(array_float)) {
    for(size_t i = 0; i < n; i++) {
      array_float_push_back(a1, rand_get() );
    }
    array_float_sort(a1);
    g_result = *array_float_get(a1, 0);
  }
}

static void test_stable_sort(size_t n)
{
  M_LET(a1, ARRAY_OPLIST(array_float)) {
    for(size_t i = 0; i < n; i++) {
      array_float_push_back(a1, rand_get() );
    }
    array_float_special_stable_sort(a1);
    g_result = *array_float_get(a1, 0);
  }
}

/********************************************************************************************/

#define SIZE_LIMIT (UINT_MAX/2)

BUFFER_DEF(buffer_uint, unsigned int, 0, BUFFER_QUEUE|BUFFER_BLOCKING)
buffer_uint_t g_buff_lock;

BUFFER_DEF(buffer_ull, unsigned long long, 0, BUFFER_QUEUE|BUFFER_BLOCKING)
buffer_ull_t g_final_lock;

static void final_lock(void *arg)
{
  size_t *p_n = arg;
  size_t    n = *p_n;
  unsigned long long j, s = 0;
  for(int i = 0; i < n;i++) {
    buffer_ull_pop(&j, g_final_lock);
    s += j;
  }
  g_result = s;
}

static void conso_lock(void *arg)
{
  unsigned int j;
  size_t *p_n = arg;
  size_t n = *p_n;
  unsigned long long s = 0;
  for(int i = 0; i < n;i++) {
    buffer_uint_pop(&j, g_buff_lock);
    s += j;
  }
  buffer_ull_push(g_final_lock, s);
}

static void prod_lock(void *arg)
{
  size_t *p_n = arg;
  size_t n = *p_n;
  size_t r = n;
  for(unsigned int i = 0; i < n;i++) {
    buffer_uint_push(g_buff_lock, r );
    r = r * 31421U + 6927U;
  }
}

static void test_buffer(size_t n)
{
  const int cpu_count   = n > SIZE_LIMIT ? 2 : get_cpu_count();
  const int prod_count  = cpu_count/2;
  const int conso_count = cpu_count - prod_count;
  if (cpu_count < 2) {
    fprintf(stderr, "WARNING: Can not measure Buffer performance.\n");
    return;
  }
  n = n > SIZE_LIMIT ? n - SIZE_LIMIT : n;
  // Init
  buffer_uint_init(g_buff_lock, 64*cpu_count);
  buffer_ull_init (g_final_lock, 64*cpu_count);

  // Create thread
  m_thread_t idx_p[prod_count];
  m_thread_t idx_c[conso_count];
  m_thread_t idx_final;
  for(int i = 0; i < prod_count; i++) {
    m_thread_create (idx_p[i], prod_lock, &n);
  }
  for(int i = 0; i < conso_count; i++) {
    m_thread_create (idx_c[i], conso_lock, &n);
  }
  size_t n2 = conso_count;
  m_thread_create(idx_final, final_lock, &n2);

  // Wait for jobs to be done.
  for(int i = 0; i < prod_count; i++) {
    m_thread_join(idx_p[i]);
  }
  for(int i = 0; i < conso_count; i++) {
    m_thread_join(idx_c[i]);
  }
  m_thread_join(idx_final);

  // Clear & quit
  buffer_ull_clear(g_final_lock);
  buffer_uint_clear(g_buff_lock);
}

/********************************************************************************************/

QUEUE_MPMC_DEF(queue_uint, unsigned int, BUFFER_QUEUE)
queue_uint_t g_buff_mpmc;

QUEUE_MPMC_DEF(queue_ull, unsigned long long, BUFFER_QUEUE)
queue_ull_t g_final_mpmc;

static void final_mpmc(void *arg)
{
  size_t *p_n = arg;
  size_t    n = *p_n;
  unsigned long long j, s = 0;
  for(int i = 0; i < n;i++) {
    while (!queue_ull_pop(&j, g_final_mpmc)) m_thread_yield();
    s += j;
  }
  g_result = s;
}

static void conso_mpmc(void *arg)
{
  unsigned int j;
  size_t *p_n = arg;
  size_t n = *p_n;
  unsigned long long s = 0;
  for(int i = 0; i < n;i++) {
    while (!queue_uint_pop(&j, g_buff_mpmc)) m_thread_yield();
    s += j;
  }
  while (!queue_ull_push(g_final_mpmc, s));
}

static void prod_mpmc(void *arg)
{
  size_t *p_n = arg;
  size_t n = *p_n;
  size_t r = n;
  for(unsigned int i = 0; i < n;i++) {
    while (!queue_uint_push(g_buff_mpmc, r )) m_thread_yield();
    r = r * 31421U + 6927U;
  }
}

static void test_queue(size_t n)
{
  const int cpu_count   = n > SIZE_LIMIT ? 2 : get_cpu_count();
  const int prod_count  = cpu_count/2;
  const int conso_count = cpu_count - prod_count;
  if (cpu_count < 2) {
    fprintf(stderr, "WARNING: Can not measure Queue performance.\n");
    return;
  }
  n = n > SIZE_LIMIT ? n - SIZE_LIMIT : n;
  // Init
  queue_uint_init(g_buff_mpmc, 64*cpu_count);
  queue_ull_init (g_final_mpmc, 64*cpu_count);

  // Create thread
  m_thread_t idx_p[prod_count];
  m_thread_t idx_c[conso_count];
  m_thread_t idx_final;
  for(int i = 0; i < prod_count; i++) {
    m_thread_create (idx_p[i], prod_mpmc, &n);
  }
  for(int i = 0; i < conso_count; i++) {
    m_thread_create (idx_c[i], conso_mpmc, &n);
  }
  size_t n2 = conso_count;
  m_thread_create(idx_final, final_mpmc, &n2);

  // Wait for jobs to be done.
  for(int i = 0; i < prod_count; i++) {
    m_thread_join(idx_p[i]);
  }
  for(int i = 0; i < conso_count; i++) {
    m_thread_join(idx_c[i]);
  }
  m_thread_join(idx_final);

  // Clear & quit
  queue_ull_clear(g_final_mpmc);
  queue_uint_clear(g_buff_mpmc);
}

/********************************************************************************************/

QUEUE_SPSC_DEF(queue_single_uint, unsigned int, BUFFER_QUEUE)
queue_single_uint_t g_buff_spsc;

QUEUE_SPSC_DEF(queue_single_ull, unsigned long long, BUFFER_QUEUE)
queue_single_ull_t g_final_spsc;

static void final_spsc(void *arg)
{
  size_t *p_n = arg;
  size_t    n = *p_n;
  unsigned long long j, s = 0;
  for(int i = 0; i < n;i++) {
    while (!queue_single_ull_pop(&j, g_final_spsc)) m_thread_yield();
    s += j;
  }
  g_result = s;
}

static void conso_spsc(void *arg)
{
  unsigned int j;
  size_t *p_n = arg;
  size_t n = *p_n;
  unsigned long long s = 0;
  for(int i = 0; i < n;i++) {
    while (!queue_single_uint_pop(&j, g_buff_spsc)) m_thread_yield();
    s += j;
  }
  while (!queue_single_ull_push(g_final_spsc, s));
}

static void prod_spsc(void *arg)
{
  size_t *p_n = arg;
  size_t n = *p_n;
  size_t r = n;
  for(unsigned int i = 0; i < n;i++) {
    while (!queue_single_uint_push(g_buff_spsc, r )) m_thread_yield();
    r = r * 31421U + 6927U;
  }
}

static void test_queue_single(size_t n)
{
  const int cpu_count   = 2;
  const int prod_count  = cpu_count/2;
  const int conso_count = cpu_count - prod_count;
  // Init
  queue_single_uint_init(g_buff_spsc, 64*cpu_count);
  queue_single_ull_init (g_final_spsc, 64*cpu_count);

  // Create thread
  m_thread_t idx_p[prod_count];
  m_thread_t idx_c[conso_count];
  m_thread_t idx_final;
  for(int i = 0; i < prod_count; i++) {
    m_thread_create (idx_p[i], prod_spsc, &n);
  }
  for(int i = 0; i < conso_count; i++) {
    m_thread_create (idx_c[i], conso_spsc, &n);
  }
  size_t n2 = conso_count;
  m_thread_create(idx_final, final_spsc, &n2);

  // Wait for jobs to be done.
  for(int i = 0; i < prod_count; i++) {
    m_thread_join(idx_p[i]);
  }
  for(int i = 0; i < conso_count; i++) {
    m_thread_join(idx_c[i]);
  }
  m_thread_join(idx_final);

  // Clear & quit
  queue_single_ull_clear(g_final_spsc);
  queue_single_uint_clear(g_buff_spsc);
}

/********************************************************************************************/

#define BULK_SIZE 20

static void final_spsc_bulk(void *arg)
{
  size_t *p_n = arg;
  size_t    n = *p_n;
  unsigned long long j, s = 0;
  for(int i = 0; i < n;i++) {
    while (!queue_single_ull_pop(&j, g_final_spsc)) m_thread_yield();
    s += j;
  }
  g_result = s;
}

static void conso_spsc_bulk(void *arg)
{
  size_t *p_n = arg;
  size_t n = *p_n;
  unsigned long long s = 0;
  unsigned tab[BULK_SIZE];
  for(int i = 0; i < n;i+= BULK_SIZE) {
    unsigned k = queue_single_uint_pop_bulk(BULK_SIZE, tab, g_buff_spsc);
    while (k != BULK_SIZE) {
      m_thread_yield();
      k += queue_single_uint_pop_bulk(BULK_SIZE-k, tab+k, g_buff_spsc);
    }
    for(k = 0; k < BULK_SIZE;k++)
      s += tab[k];
  }
  while (!queue_single_ull_push(g_final_spsc, s));
}

static void prod_spsc_bulk(void *arg)
{
  size_t *p_n = arg;
  size_t n = *p_n;
  if ((n % BULK_SIZE) != 0) abort();
  size_t r = n;
  unsigned tab[BULK_SIZE];
  for(unsigned int i = 0; i < n;i+= BULK_SIZE) {
    for(unsigned k = 0; k < BULK_SIZE; k++) {
      tab[k] = r;
      r = r * 31421U + 6927U;
    }
    unsigned k = queue_single_uint_push_bulk(g_buff_spsc, BULK_SIZE, tab);
    while (k != BULK_SIZE) {
      m_thread_yield();
      k += queue_single_uint_push_bulk(g_buff_spsc, BULK_SIZE-k, tab+k);
    }
  }
}

static void test_queue_single_bulk(size_t n)
{
  const int cpu_count   = 2;
  const int prod_count  = cpu_count/2;
  const int conso_count = cpu_count - prod_count;
  // Init
  queue_single_uint_init(g_buff_spsc, 64*cpu_count);
  queue_single_ull_init (g_final_spsc, 64*cpu_count);

  // Create thread
  m_thread_t idx_p[prod_count];
  m_thread_t idx_c[conso_count];
  m_thread_t idx_final;
  for(int i = 0; i < prod_count; i++) {
    m_thread_create (idx_p[i], prod_spsc_bulk, &n);
  }
  for(int i = 0; i < conso_count; i++) {
    m_thread_create (idx_c[i], conso_spsc_bulk, &n);
  }
  size_t n2 = conso_count;
  m_thread_create(idx_final, final_spsc_bulk, &n2);

  // Wait for jobs to be done.
  for(int i = 0; i < prod_count; i++) {
    m_thread_join(idx_p[i]);
  }
  for(int i = 0; i < conso_count; i++) {
    m_thread_join(idx_c[i]);
  }
  m_thread_join(idx_final);

  // Clear & quit
  queue_single_ull_clear(g_final_spsc);
  queue_single_uint_clear(g_buff_spsc);
}

/********************************************************************************************/

DEQUE_DEF(deque_uint, unsigned int)
CONCURRENT_DEF(cdeque_uint, deque_uint_t, M_OPEXTEND(DEQUE_OPLIST(deque_uint, M_DEFAULT_OPLIST), PUSH(deque_uint_push_front)))

DEQUE_DEF(deque_ull, unsigned long long)
CONCURRENT_DEF(cdeque_ull, deque_ull_t, M_OPEXTEND(DEQUE_OPLIST(deque_ull, M_DEFAULT_OPLIST), PUSH(deque_ull_push_front)))

cdeque_uint_t g_buff_conc;
cdeque_ull_t g_final_conc;

static void final_conc(void *arg)
{
  size_t *p_n = arg;
  size_t    n = *p_n;
  unsigned long long j, s = 0;
  for(int i = 0; i < n;i++) {
    cdeque_ull_pop_blocking(&j, g_final_conc, true);
    s += j;
  }
  g_result = s;
}

static void conso_conc(void *arg)
{
  unsigned int j;
  size_t *p_n = arg;
  size_t n = *p_n;
  unsigned long long s = 0;
  for(int i = 0; i < n;i++) {
    cdeque_uint_pop_blocking(&j, g_buff_conc, true);
    s += j;
  }
  cdeque_ull_push(g_final_conc, s);
}

static void prod_conc(void *arg)
{
  size_t *p_n = arg;
  size_t n = *p_n;
  size_t r = n;
  for(unsigned int i = 0; i < n;i++) {
    cdeque_uint_push(g_buff_conc, r );
    r = r * 31421U + 6927U;
  }
}

static void test_queue_concurrent(size_t n)
{
  const int cpu_count   = n > SIZE_LIMIT ? 2 : get_cpu_count();
  const int prod_count  = cpu_count/2;
  const int conso_count = cpu_count - prod_count;
  if (cpu_count < 2) {
    fprintf(stderr, "WARNING: Can not measure CONCURRENT performance.\n");
    return;
  }
  n = n > SIZE_LIMIT ? n - SIZE_LIMIT : n;
  // Init
  cdeque_uint_init(g_buff_conc);
  cdeque_ull_init (g_final_conc);

  // Create thread
  m_thread_t idx_p[prod_count];
  m_thread_t idx_c[conso_count];
  m_thread_t idx_final;
  for(int i = 0; i < prod_count; i++) {
    m_thread_create (idx_p[i], prod_conc, &n);
  }
  for(int i = 0; i < conso_count; i++) {
    m_thread_create (idx_c[i], conso_conc, &n);
  }
  size_t n2 = conso_count;
  m_thread_create(idx_final, final_conc, &n2);

  // Wait for jobs to be done.
  for(int i = 0; i < prod_count; i++) {
    m_thread_join(idx_p[i]);
  }
  for(int i = 0; i < conso_count; i++) {
    m_thread_join(idx_c[i]);
  }
  m_thread_join(idx_final);

  // Clear & quit
  cdeque_ull_clear(g_final_conc);
  cdeque_uint_clear(g_buff_conc);
}

/********************************************************************************************/

static unsigned long *g_p;

static void test_hash_prepare(size_t n)
{
  g_p = malloc (n * sizeof(unsigned long));
  if (g_p == NULL) abort();
  for(size_t i = 0; i < n ; i++)
    g_p[i] = ((rand_get()*RAND_MAX + rand_get())*RAND_MAX+rand_get())*RAND_MAX + rand_get();
}


static void test_hash_final(void)
{
  free(g_p);
}

static void test_hash(size_t n)
{
  M_HASH_DECL(hash);
  for(size_t i = 0; i < n; i++)
    M_HASH_UP(hash, g_p[i]);
  g_result = M_HASH_FINAL(hash);
}

static void test_core_hash(size_t n)
{
  g_result = m_core_hash(g_p, n*sizeof(unsigned long));
}


/********************************************************************************************/


int main(int argc, const char *argv[])
{
  int n = (argc > 1) ? atoi(argv[1]) : 0;
  if (n == 10)
    test_function("List   time",10000000, test_list);
  if (n == 11)
    test_function("DPList time",10000000, test_dlist);
  if (n == 20)
    test_function("Array  time", 100000000, test_array);
  if (n == 30)
    test_function("Rbtree time", 1000000, test_rbtree);
  if (n == 31)
    test_function("B+tree time", 1000000, test_bptree);
  if (n == 40)
    test_function("Dict   time", 1000000, test_dict);
  if (n == 42)
    test_function("DictOA time", 1000000, test_dict_oa);
  if (n == 41)
    test_function("DictB  time", 1000000, test_dict_big);
  if (n == 43)
    test_function("DictS  time", 1000000, test_dict_str);
  if (n == 50)
    test_function("Sort   time", 10000000, test_sort);
  if (n == 51)
    test_function("Stable Sort time", 10000000, test_stable_sort);
  if (n == 60)
    test_function("Buffer time", 1000000, test_buffer);
  if (n == 61)
    test_function("Queue MPMC time", 1000000, test_queue);
  if (n == 62)
    test_function("Buffer time (P2)", SIZE_LIMIT+1000000, test_buffer);
  if (n == 63)
    test_function("Queue MPMC time (P2)", SIZE_LIMIT+1000000, test_queue);
  if (n == 64)
    test_function("Queue SPSC time", 1000000, test_queue_single);
  if (n == 65)
    test_function("Queue CONCURRENT time", 1000000, test_queue_concurrent);
  if (n == 66)
    test_function("Queue SPSC BULK time", 1000000, test_queue_single_bulk);
  if (n == 70) {
    n = (argc > 2) ? atoi(argv[2]) : 100000000;
    test_hash_prepare(n);
    test_function("M_HASH time", n, test_hash);
    test_hash_final();
  }
  if (n == 71) {
    n = (argc > 2) ? atoi(argv[2]) : 100000000;
    test_hash_prepare(n);
    test_function("CORE_HASH time", n, test_core_hash);
    test_hash_final();
  }
  
  exit(0);
}

