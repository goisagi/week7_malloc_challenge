//
// >>>> malloc challenge! <<<<
// 課題：元のmallocプログラム(simple_malloc.cと同じ)のメモリ使用効率と処理速度を向上させる

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BIN_NUM 14

//
// OSからメモリページを取得するためのインタフェース
//

void *mmap_from_system(size_t size);
void munmap_to_system(void *ptr, size_t size);

//
// Struct definitions
//

// pageの構造体 (pageが空になったらOSに返す処理の実装のため)
typedef struct my_page_t
{
  struct my_page_t *next;
  int allocated_count;
} my_page_t;

typedef struct my_metadata_t
{
  size_t size;
  struct my_metadata_t *next;
  my_page_t *page; // metadataからどのpageに属しているかを辿れるようにする
} my_metadata_t;

typedef struct my_heap_t
{
  my_metadata_t *free_head[BIN_NUM];
  my_metadata_t dummy;
  my_page_t *page_head; // pageの連結リストの先頭を追加
} my_heap_t;

//
// Static variables (追加禁止)
//
my_heap_t my_heap;

//
// Helper functions (自由に追加・削除・編集OK)
//

int get_bin_num(size_t size)
{
  if (size < 1000)
    return size / 100;
  if (size < 1500)
    return 10;
  if (size < 2000)
    return 11;
  if (size < 2500)
    return 12;
  return 13;
}

void my_add_to_free_list(my_metadata_t *metadata)
{
  assert(!metadata->next);
  int bin = get_bin_num(metadata->size);
  metadata->next = my_heap.free_head[bin];
  my_heap.free_head[bin] = metadata;
}

void my_remove_from_free_list(my_metadata_t *metadata, my_metadata_t *prev)
{
  if (prev)
  {
    prev->next = metadata->next;
  }
  else
  {
    int bin = get_bin_num(metadata->size);
    my_heap.free_head[bin] = metadata->next;
  }
  metadata->next = NULL;
}

// my_remove_from_free_listのprevがわかっていない場合用
// prevを探索して求めたら、それをmy_remove_from_free_listに渡す
void my_remove_from_free_list_search(my_metadata_t *metadata)
{
  int bin = get_bin_num(metadata->size);
  my_metadata_t *cur_metadata = my_heap.free_head[bin];
  my_metadata_t *prev_metadata = NULL;

  while (cur_metadata != &my_heap.dummy)
  {

    if (cur_metadata == metadata)
    {
      my_remove_from_free_list(cur_metadata, prev_metadata);
      return;
    }

    prev_metadata = cur_metadata;
    cur_metadata = cur_metadata->next;
  }
}

// page_listに要素を追加する関数
void my_add_to_page_list(my_page_t *page)
{
  assert(!page->next);
  page->next = my_heap.page_head;
  page->allocated_count = 0;
  my_heap.page_head = page;
}

// page_listから要素を取り除く関数
// これを呼ぶ際、pageのprevは不明なので、この関数内で探索する
void my_remove_from_page_list(my_page_t *page)
{
  my_page_t *cur_page = my_heap.page_head;
  my_page_t *prev = NULL;

  while (cur_page && cur_page != page)
  {
    prev = cur_page;
    cur_page = cur_page->next;
  }

  if (!cur_page)
  {
    return;
  }

  if (prev)
  {
    prev->next = page->next;
  }
  else
  {
    my_heap.page_head = page->next;
  }
  page->next = NULL;
}

// 受け取ったメタデータの左隣のメタデータを返す関数
my_metadata_t *my_find_left_metadata(my_metadata_t *metadata)
{
  my_page_t *page = metadata->page;
  my_metadata_t *cur_metadata = (my_metadata_t *)(page + 1);
  my_metadata_t *left_metadata = NULL;

  while ((char *)cur_metadata < (char *)metadata)
  {
    left_metadata = cur_metadata;
    cur_metadata = (my_metadata_t *)((char *)(cur_metadata + 1) + cur_metadata->size);
  }
  return left_metadata;
}

// 受け取ったメタデータの右隣のメタデータを返す関数
my_metadata_t *my_find_right_metadata(my_metadata_t *metadata)
{

  my_metadata_t *right_metadata = (my_metadata_t *)((char *)(metadata + 1) + metadata->size);
  char *next_page = (char *)metadata->page + 4096;
  if ((char *)right_metadata >= next_page)
    return NULL;
  return right_metadata;
}

// 受け取ったメタデータのオブジェクトがfreeかを返す関数
bool my_is_free(my_metadata_t *metadata)
{
  int bin = get_bin_num(metadata->size);
  my_metadata_t *cur_metadata = my_heap.free_head[bin];

  while (cur_metadata != &my_heap.dummy)
  {
    if (cur_metadata == metadata)
      return true;
    cur_metadata = cur_metadata->next;
  }

  return false;
}

//
// mallocのインタフェース (以下の関数の名前の変更不可)
//

// 各challengeの開始時に呼び出される
void my_initialize()
{
  for (int i = 0; i < BIN_NUM; i++)
  {
    my_heap.free_head[i] = &my_heap.dummy;
  }
  my_heap.dummy.size = 0;
  my_heap.dummy.next = NULL;
  my_heap.page_head = NULL;
}

// オブジェクトが割り当てられるたびにmy_malloc()が呼び出される
// |size| は8バイトの倍数であることが保証されており、 8 <= |size| <= 4000 を満たす
// mmap_from_system() / munmap_to_system()以外のライブラリ関数の使用は不可
void *my_malloc(size_t size)
{
  // Best-fitの実装
  int bin = get_bin_num(size);
  my_metadata_t *best_metadata = NULL;
  my_metadata_t *best_prev = NULL;

  for (int b = bin; b < BIN_NUM; b++)
  {
    my_metadata_t *cur_metadata = my_heap.free_head[b];
    my_metadata_t *cur_prev = NULL;

    while (cur_metadata != &my_heap.dummy)
    {
      if (cur_metadata->size >= size)
      {
        if (!best_metadata || best_metadata->size > cur_metadata->size)
        {
          best_metadata = cur_metadata;
          best_prev = cur_prev;
        }
      }
      cur_prev = cur_metadata;
      cur_metadata = cur_metadata->next;
    }
    if (best_metadata)
      break;
  }

  // この時点で、best_metadataは十分な大きさの空き容量のうち最も小さいもの
  // best_prevは1つ前のエントリを指す

  // 空きスロットがない場合は、mmap_from_system()を呼び出して、システムから
  // 新しいメモリ領域を要求する必要がある
  if (!best_metadata)
  {
    size_t buffer_size = 4096;
    // metadataの前にpageを入れるように変更
    my_page_t *page = (my_page_t *)mmap_from_system(buffer_size);
    my_metadata_t *metadata = (my_metadata_t *)(page + 1);
    my_add_to_page_list(page);

    metadata->size = buffer_size - sizeof(my_metadata_t) - sizeof(my_page_t); // page分も引く
    metadata->next = NULL;
    metadata->page = page;

    // 取得したメモリ領域をfree_listに追加
    my_add_to_free_list(metadata);
    // 空き領域ができたのでmy_mallocを再度実行する
    return my_malloc(size);
  }

  // ... |   metadata   | object | ...
  //     ^              ^
  //     best_metadata  ptr
  //
  // |ptr| を割り当てられたbestなオブジェクトの先頭にする
  void *ptr = best_metadata + 1;
  size_t remaining_size = best_metadata->size - size;
  // 割り当てられたbestなオブジェクトのメタデータを、free_listから消去する
  my_remove_from_free_list(best_metadata, best_prev);

  if (remaining_size > sizeof(my_metadata_t))
  {
    // 割り当てられたbestなオブジェクトのメタデータを、欲しかったsizeの大きさに縮小し、
    // size分と、それ以外の残りに領域を分離する
    // それ以外の残りの領域の先頭からmy_metadata_tの型分のbyteを新しいメタデータ用に確保する
    // 残りの領域がmy_metadata_tの型以下のサイズだった場合は、
    // 領域を分割せず、要求のsizeより大きくはなるがそのまま割り当てる

    best_metadata->size = size;

    // 分割した残り領域用に新しいメタデータを作成する

    // ... |    metadata    |   object   | metadata | free slot | ...
    //     ^                ^            ^
    //     best_metadata    ptr          new_metadata
    //                       <-----------><---------------------->
    //                            size        remaining size

    my_metadata_t *new_metadata = (my_metadata_t *)((char *)ptr + size);
    new_metadata->size = remaining_size - sizeof(my_metadata_t);
    new_metadata->next = NULL;
    new_metadata->page = best_metadata->page;
    // free_listに作成した新しいメタデータを追加する
    my_add_to_free_list(new_metadata);
  }
  best_metadata->page->allocated_count++;
  return ptr;
}

// 以下はオブジェクトが解放されるたびに呼び出される。
// mmap_from_system / munmap_to_system 以外のライブラリ関数の使用不可
void my_free(void *ptr)
{
  // メタデータの位置を求める。メタデータはオブジェクトの直前に配置されている

  // ... | metadata | object | ...
  //     ^          ^
  //     metadata   ptr

  my_metadata_t *metadata = (my_metadata_t *)ptr - 1;
  my_page_t *page = metadata->page;

  page->allocated_count--;

  if (page->allocated_count > 0)
  {
    my_metadata_t *left = my_find_left_metadata(metadata);
    my_metadata_t *right = my_find_right_metadata(metadata);
    bool left_is_free = left && my_is_free(left);
    bool right_is_free = right && my_is_free(right);

    if (left_is_free && right_is_free) // 左右の両方がfreeだった場合の処理
    {
      my_remove_from_free_list_search(left);
      my_remove_from_free_list_search(right);

      // ... | left | left_object | metadata | object | right | right_object |
      //            <-------------------------------------------------------->
      //                                   結合後のサイズ

      left->size += 2 * sizeof(my_metadata_t) + metadata->size + right->size;
      my_add_to_free_list(left);
    }
    else if (left_is_free)
    {
      my_remove_from_free_list_search(left);
      left->size += sizeof(my_metadata_t) + metadata->size;
      my_add_to_free_list(left);
    }
    else if (right_is_free)
    {
      my_remove_from_free_list_search(right);
      metadata->size += sizeof(my_metadata_t) + right->size;
      my_add_to_free_list(metadata);
    }
    else
    {
      // 左右がfreeでなければ、何とも結合せずにそのままfree_listに追加
      my_add_to_free_list(metadata);
    }
  }
  else // pageの中が全てfreeになった場合、そのページをOSに返す作業を行う
  {
    my_remove_from_page_list(metadata->page);

    // free_listに入っている、metadata->pageに含まれる領域を、free_listから取り除く
    for (int b = 0; b < BIN_NUM; b++)
    {
      my_metadata_t *cur_metadata = my_heap.free_head[b];
      my_metadata_t *prev = NULL;

      while (cur_metadata != &my_heap.dummy)
      {
        my_metadata_t *next = cur_metadata->next;
        if (cur_metadata->page == page)
        {
          my_remove_from_free_list(cur_metadata, prev);
          // cur_metadataの削除が行われた場合にはprevは変わらないので更新不要
        }
        else
        {
          prev = cur_metadata;
        }
        cur_metadata = next;
      }
    }

    munmap_to_system(page, 4096);
  }
}

// 各challengeの終了時に呼び出される
void my_finalize()
{
  // 終了時にまだ使用中であるpageの解放
  /*
  my_page_t *page = my_heap.page_head;
  my_page_t *next = NULL;
  while (page)
  {
    next = page->next;
    munmap_to_system(page, 4096);
    page = next;
  }
  */
}

void test()
{
  // Implement here!
  assert(1 == 1); /* 1 is 1. That's always true! (You can remove this.) */
}
