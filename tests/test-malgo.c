/*
 * Copyright (c) 2017-2019, Patrick Pelissier
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
#include "m-list.h"
#include "m-i-list.h"
#include "m-array.h"
#include "m-string.h"
#include "m-deque.h"
#include "m-dict.h"
#include "m-algo.h"

#include "test-obj.h"

typedef struct over_s {
  unsigned long data;
  ILIST_INTERFACE(ilist_over, over_s);
} over_t;

// TODO: Test Intrusive List
// HASHMAP, RBTREE, B+TREE: Is this useful?
ARRAY_DEF(array_int, int)
#define M_OPL_array_int_t() ARRAY_OPLIST(array_int)
LIST_DEF(list_int, int)
ILIST_DEF(ilist_over, over_t, M_POD_OPLIST)
LIST_DEF(list_string, string_t, STRING_OPLIST)
DEQUE_DEF(deque_obj, testobj_t, TESTOBJ_CMP_OPLIST)
#define M_OPL_deque_obj_t() DEQUE_OPLIST(deque_obj, TESTOBJ_CMP_OPLIST)
DICT_DEF2(dict_obj, string_t, STRING_OPLIST, testobj_t, TESTOBJ_OPLIST)
LIST_DUAL_PUSH_DEF(dlist_int, int)

#include "coverage.h"
START_COVERAGE
ALGO_DEF(algo_array, array_int_t)
ALGO_DEF(algo_list,  LIST_OPLIST(list_int))
ALGO_DEF(algo_over,  ILIST_OPLIST(ilist_over, M_POD_OPLIST))
ALGO_DEF(algo_string, LIST_OPLIST(list_string, STRING_OPLIST))
ALGO_DEF(algo_deque, deque_obj_t)
ALGO_DEF(algo_dict, DICT_OPLIST(dict_obj, STRING_OPLIST, TESTOBJ_OPLIST))
END_COVERAGE
ALGO_DEF(algo_dlist, LIST_OPLIST(list_int))

/* Helper functions */
int g_min, g_max, g_count;

static void g_f(int n)
{
  assert (g_min <= n && n <= g_max);
  g_count++;
}

static void func_map(int *d, int n)
{
  assert (g_min <= n && n <= g_max);
  g_count++;
  *d = n * n;
}

static void func_reduce(int *d, int n)
{
  *d += n;
}

static bool func_test_42(int d)
{
  return d == 42;
}

static bool func_test_101(int d)
{
  return d == 101;
}

static bool func_test_pos(int d)
{
  return d >= 0;
}

static bool func_test_both_even_or_odd(int a, int b)
{
  return (a&1) == (b&1);
}


/* Tests starts */
static void test_list(void)
{
  list_int_t l;
  list_int_init(l);
  for(int i = 0; i < 100; i++)
    list_int_push_back (l, i);
  assert( algo_list_contain_p(l, 62) == true);
  assert( algo_list_contain_p(l, -1) == false);
  
  assert( algo_list_count(l, 1) == 1);
  list_int_push_back (l, 17);
  assert( algo_list_count(l, 17) == 2);
  assert( algo_list_count(l, -1) == 0);

  assert (algo_list_all_of_p(l, func_test_42) == false);
  assert (algo_list_any_of_p(l, func_test_42) == true);
  assert (algo_list_none_of_p(l, func_test_42) == false);
  assert (algo_list_all_of_p(l, func_test_101) == false);
  assert (algo_list_any_of_p(l, func_test_101) == false);
  assert (algo_list_none_of_p(l, func_test_101) == true);
  assert (algo_list_all_of_p(l, func_test_pos) == true);
  assert (algo_list_any_of_p(l, func_test_pos) == true);
  assert (algo_list_none_of_p(l, func_test_pos) == false);

#define f(x) assert((x) >= 0 && (x) < 100);
  ALGO_FOR_EACH(l, LIST_OPLIST(list_int), f);
#define g(y, x) assert((x) >= 0 && (x) < y);
  ALGO_FOR_EACH(l, LIST_OPLIST(list_int), g, 100);

  assert( algo_list_count_if(l, func_test_42) == 1);
  assert( algo_list_count_if(l, func_test_101) == 0);

  int *p = algo_list_min(l);
  assert(p != NULL && *p == 0);
  p = algo_list_max(l);
  assert(p != NULL && *p == 99);
  int *p2;
  algo_list_minmax(&p, &p2, l);
  assert(p != NULL && *p == 0);
  assert(p2 != NULL && *p2 == 99);
  
  list_int_push_back (l, 3);
  list_int_it_t it1, it2;
  algo_list_find (it1, l, 3);
  assert (!list_int_end_p (it1));
  algo_list_find_last (it2, l, 3);
  assert (!list_int_end_p (it2));
  assert (!list_int_it_equal_p(it1, it2));

  algo_list_find_if(it1, l, func_test_101);
  assert (list_int_end_p (it1));
  algo_list_find_if(it1, l, func_test_42);
  assert (!list_int_end_p (it1));
  assert (*list_int_cref(it1) == 42);

  algo_list_mismatch(it1, it2, l, l);
  assert (list_int_end_p (it1));
  assert (list_int_end_p (it2));
  
  for(int i = -100; i < 100; i+=2)
    list_int_push_back (l, i);

  assert (!algo_list_sort_p(l));
  algo_list_sort(l);
  assert (algo_list_sort_p(l));

  list_int_clear(l);

  ALGO_INIT_VA(l, LIST_OPLIST(list_int), 1, 2, 3, 4, 5);
  assert (list_int_size(l) == 5);

  algo_list_remove_val(l, 3);
  assert (list_int_size(l) == 4);
  algo_list_find (it1, l, 3);
  assert (list_int_end_p (it1));
  algo_list_remove_val(l, 3);
  assert (list_int_size(l) == 4);

  algo_list_remove_if(l, func_test_42);
  assert (list_int_size(l) == 4);
  list_int_push_back(l, 42);
  list_int_push_back(l, 43);
  list_int_push_back(l, 42);
  algo_list_remove_if(l, func_test_42);
  assert (list_int_size(l) == 5);
  algo_list_find (it1, l, 42);
  assert (list_int_end_p (it1));
  
  list_int_clear(l);
}

static void test_array(void)
{
  array_int_t l;
  array_int_init(l);
  for(int i = 0; i < 100; i++)
    array_int_push_back (l, i);
  assert( algo_array_contain_p(l, 62) == true);
  assert( algo_array_contain_p(l, -1) == false);
  assert( algo_array_sort_p(l) == true);

  assert (algo_array_all_of_p(l, func_test_42) == false);
  assert (algo_array_any_of_p(l, func_test_42) == true);
  assert (algo_array_none_of_p(l, func_test_42) == false);
  assert (algo_array_all_of_p(l, func_test_101) == false);
  assert (algo_array_any_of_p(l, func_test_101) == false);
  assert (algo_array_none_of_p(l, func_test_101) == true);
  assert (algo_array_all_of_p(l, func_test_pos) == true);
  assert (algo_array_any_of_p(l, func_test_pos) == true);
  assert (algo_array_none_of_p(l, func_test_pos) == false);

  assert( algo_array_count(l, 1) == 1);
  array_int_push_back (l, 17);
  assert( algo_array_count(l, 17) == 2);
  assert( algo_array_count(l, -1) == 0);
  assert( algo_array_sort_p(l) == false);

  assert( algo_array_count_if(l, func_test_42) == 1);
  assert( algo_array_count_if(l, func_test_101) == 0);

  array_int_it_t it;
  algo_array_find_last(it, l, 17);
  assert (!array_int_end_p (it));
  assert (array_int_last_p (it));
  algo_array_find_last(it, l, 1742);
  assert (array_int_end_p (it));
  algo_array_find(it, l, 1742);
  assert (array_int_end_p (it));

#define f(x) assert((x) >= 0 && (x) < 100);
  ALGO_FOR_EACH(l, ARRAY_OPLIST(array_int), f);
  ALGO_FOR_EACH(l, array_int_t, f);

  g_min = 0;
  g_max = 99;
  g_count = 0;
  algo_array_for_each(l, g_f);
  assert(g_count == 101);

  int n;
  algo_array_reduce(&n, l, func_reduce);
  assert(n == 100*99/2+17);

  g_min = 0;
  g_max = 99;
  g_count = 0;
  algo_array_map_reduce(&n, l, func_reduce, func_map);
  assert(g_count == 101);
  assert(n == (328350 + 17*17));

  int *min, *max;
  assert (*algo_array_min(l) == 0);
  assert (*algo_array_max(l) == 99);
  algo_array_minmax(&min, &max, l);
  assert (*min == 0);
  assert (*max == 99);
  array_int_push_back (l, 1742);
  array_int_push_back (l, -17);
  assert (*algo_array_min(l) == -17);
  assert (*algo_array_max(l) == 1742);
  algo_array_minmax(&min, &max, l);
  assert (*min == -17);
  assert (*max == 1742);
  assert( algo_array_sort_p(l) == false);

  algo_array_sort(l);
  assert( algo_array_sort_p(l) == true);
  assert (array_int_size(l) == 103);
  algo_array_uniq(l);
  assert (array_int_size(l) == 102);
  assert( algo_array_sort_p(l) == true);

  M_LET(l2, array_int_t) {
    array_int_set(l2, l);
    array_int_it_t it1, it2;
    algo_array_mismatch(it1, it2, l, l2);
    assert (array_int_end_p (it1));
    assert (array_int_end_p (it2));
    array_int_pop_back(NULL, l2);
    array_int_push_back(l2, 159);
    algo_array_mismatch(it1, it2, l, l2);
    assert (!array_int_end_p (it1));
    assert (!array_int_end_p (it2));
    assert (*array_int_cref(it1) == 1742);
    assert (*array_int_cref(it2) == 159);
    algo_array_mismatch_if(it1, it2, l, l2, func_test_both_even_or_odd);
    assert (!array_int_end_p (it1));
    assert (!array_int_end_p (it2));
    assert (*array_int_cref(it1) == 1742);
    assert (*array_int_cref(it2) == 159);
    array_int_pop_back(NULL, l2);
    array_int_push_back(l2, 152);
    algo_array_mismatch_if(it1, it2, l, l2, func_test_both_even_or_odd);
    assert (array_int_end_p (it1));
    assert (array_int_end_p (it2));
  }

  array_int_clean(l);
  assert (algo_array_min(l) == NULL);
  assert (algo_array_max(l) == NULL);
  algo_array_minmax(&min, &max, l);
  assert (min == NULL);
  assert (max == NULL);
  assert (algo_array_sort_p(l) == true);
  algo_array_uniq(l);
  assert (array_int_size(l) == 0);
  assert( algo_array_sort_p(l) == true);

  for(int i = -14025; i < 324035; i+=17)
    array_int_push_back(l, i);
  for(int i = -14025; i < 324035; i+=7)
    array_int_push_back(l, i);
  assert( algo_array_sort_p(l) == false);
  array_int_special_stable_sort(l);
  assert( algo_array_sort_p(l) == true);
  
  array_int_clear(l);

  ALGO_INIT_VA(l, ARRAY_OPLIST(array_int), 1, 2, 3, 4, 5);
  assert (array_int_size(l) == 5);
  assert (algo_array_sort_p(l) == true);
  assert (algo_array_sort_dsc_p(l) == false);
  algo_array_sort_dsc(l);
  assert (algo_array_sort_p(l) == false);
  assert (algo_array_sort_dsc_p(l) == true);

  array_int_clear(l);

  ALGO_LET_INIT_VA(arr, array_int_t, 1, 5, 34) {
    assert (array_int_size(arr) == 3);
    assert (algo_array_sort_p(arr) == true);
    assert (algo_array_sort_dsc_p(arr) == false);
  }
}

static void test_string(void)
{
  list_string_t l;
  list_string_init(l);
  string_t s;
  string_init_set_str(s, "Hello, World, John");
  algo_string_split(l, s, ',');
  list_string_pop_back (&s, l);
  assert (string_equal_str_p(s, "Hello"));
  list_string_pop_back (&s, l);
  assert (string_equal_str_p(s, " World"));
  list_string_pop_back (&s, l);
  assert (string_equal_str_p(s, " John"));
  assert (list_string_empty_p(l));

  string_set_str(s, "Hello,,John");
  algo_string_split(l, s, ',');
  list_string_pop_back (&s, l);
  assert (string_equal_str_p(s, "Hello"));
  list_string_pop_back (&s, l);
  assert (string_equal_str_p(s, ""));
  list_string_pop_back (&s, l);
  assert (string_equal_str_p(s, "John"));
  assert (list_string_empty_p(l));

  list_string_push_back (l, STRING_CTE("John"));
  list_string_push_back (l, STRING_CTE("Who"));
  list_string_push_back (l, STRING_CTE("Is"));
  algo_string_join(s, l, STRING_CTE("-"));
  // NOTE: List is reverse... Is-it really ok?
  assert (string_equal_str_p(s, "Is-Who-John"));

  string_clear(s);
  
  list_string_clear(l);
}

static void test_extract(void)
{
  list_int_t l;
  list_int_init (l);
  for(int i = -100; i < 100; i++)
    list_int_push_back (l, i);
  array_int_t a;
  array_int_init(a);
#define cond(d) ((d) > 0)
  ALGO_EXTRACT(a, ARRAY_OPLIST(array_int), l, LIST_OPLIST(list_int), cond);
  assert(array_int_size(a) == 99);
#define cond2(c, d) ((d) > (c))
  ALGO_EXTRACT(a, ARRAY_OPLIST(array_int), l, LIST_OPLIST(list_int), cond2, 10);
  assert(array_int_size(a) == 89);

  int dst = 0;
#define inc(d, c) (d) += (c)
  ALGO_REDUCE(dst, a, ARRAY_OPLIST(array_int), inc);
  assert (dst == 100*99/2-10*11/2);
  ALGO_REDUCE(dst, a, array_int_t, inc);
  assert (dst == 100*99/2-10*11/2);
#define sqr(d, c) (d) = (c)*(c)
  ALGO_REDUCE(dst, a, ARRAY_OPLIST(array_int), inc, sqr);
  assert (dst == 327965);
#define sqr2(d, f, c) (d) = (f) * (c)
  ALGO_REDUCE(dst, a, ARRAY_OPLIST(array_int), inc, sqr2, 4);
  assert (dst == (100*99/2-10*11/2) *4 );

  ALGO_REDUCE(dst, a, ARRAY_OPLIST(array_int), sum);
  assert (dst == 100*99/2-10*11/2);
  ALGO_REDUCE(dst, a, ARRAY_OPLIST(array_int), add);
  assert (dst == 100*99/2-10*11/2);
  ALGO_REDUCE(dst, a, ARRAY_OPLIST(array_int), and);
  assert (dst == 0);
  ALGO_REDUCE(dst, a, ARRAY_OPLIST(array_int), or);
  assert (dst == 127);

  array_int_clear(a);
  list_int_clear(l);
}

ARRAY_DEF(aint, int)
LIST_DEF(lint, int)
#define M_OPL_aint_t() ARRAY_OPLIST(aint)
#define M_OPL_lint_t() LIST_OPLIST(lint)

static void test_insert(void)
{
  ALGO_LET_INIT_VA(a, lint_t, 1, 2, 3, 4)
    ALGO_LET_INIT_VA(b, aint_t, -1, -2, -3) {
    lint_it_t i;
    lint_it(i, a);
    // Insert it after first element
    ALGO_INSERT_AT(a, lint_t, i, b, aint_t);
    // TBC: order ok for the list (the reverse is confusing...)?
    ALGO_LET_INIT_VA(c, lint_t, 1, 2, 3, -3, -2, -1, 4) {
      assert (lint_equal_p (c, a));
    }
  }
}

int main(void)
{
  test_list();
  test_array();
  test_string();
  test_extract();
  test_insert();
  exit(0);
}
