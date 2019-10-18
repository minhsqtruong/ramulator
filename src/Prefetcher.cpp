#include "DRAM.h"
#include "Request.h"
#include "Prefetcher.h"
#include "Cache.h"
#include <vector>
#include <map>
#include <list>
#include <functional>
#include <cassert>
#include <cstdlib>
#include <algorithm>
#include <string.h>
#include <stdio.h>

using namespace std;

namespace ramulator
{

/*==============================================================================
BASE PREFETCHER
==============================================================================*/
  Prefetcher::Prefetcher(Cache* cache): cache(cache)
  {
    switch(type) {
      case Type::Nextline:
        engine = new NextLine_Prefetcher(cache);
        break;
      case Type::ASD:
        engine = new ASD_Prefetcher(cache);
      default:
        engine = new ASD_Prefetcher(cache);
    }
  };

  void Prefetcher::insert_prefetch(long next_addr)
  {
    auto mshr = cache->hit_mshr(next_addr);

    // if the prefetch miss is already registered
    if (mshr != cache->mshr_entries.end()) {
      cache->cache_mshr_hit++;
      return;
    }

    // if the mshr is full
    if (cache->mshr_entries.size() == cache->mshr_entry_num) {
      cache->cache_mshr_unavailable++;
      return;
    }

    // create new entry to add to mshr
    auto& lines = cache->get_lines(next_addr);
    auto newline = cache->allocate_line(lines, next_addr);
    if (newline == lines.end()) {
      return;
    }

    // add new miss entry to mshr
    newline->dirty = false; // only READ request
    cache->mshr_entries.push_back(make_pair(next_addr, newline));
  };

  void Prefetcher::activate(Request req) {engine->activate(req);};
  bool Prefetcher::exist() {return engine->exist();};

/*==============================================================================
NEXTLINE PREFETCHER
==============================================================================*/

  NextLine_Prefetcher::NextLine_Prefetcher(Cache* cache): Prefetcher(cache) {};

  void NextLine_Prefetcher::activate(Request req)
  {
    assert(req.type == Request::Type::READ);
    auto next_addr = get_next_addr(req.addr);
    insert_prefetch(next_addr);
  };

  bool NextLine_Prefetcher::exist()
  {
    if (cache->level == Cache::Level::L1
     || cache->level == Cache::Level::L2
     || cache->level == Cache::Level::L3) {
      return true;
    }
    return false;
  };

  long NextLine_Prefetcher::get_next_addr(long addr) {
    long next_addr = addr + cache->block_size;
    return next_addr;
  };


/*==============================================================================
ASD PREFETCHER
==============================================================================*/
  ASD_Prefetcher::ASD_Prefetcher(Cache* cache): Prefetcher(cache)
  {
    SLH = new int[fs];
    float start_prob = 1.0 / (float) fs;
    // Starting probability is the same for all stream length
    for (int i = 0; i < fs; i++)
      SLH[i] = start_prob;
    new_SLH = new int[fs];
  };

  void ASD_Prefetcher::activate(Request req)
  {
    if (counter == epoch_size) {
      counter = 0;
      memcpy(new_SLH, SLH, sizeof(int) * fs);
    }
    counter++;
    build_new_SLH(req.addr);

    assert(req.type == Request::Type::READ);
    auto next_addr = stream_filter(req.addr);
    insert_prefetch(next_addr);
  };

  bool ASD_Prefetcher::exist()
  {
    if (cache->level == Cache::Level::L3) {
      return true;
    }
    return false;
  };

  void ASD_Prefetcher::build_new_SLH(long addr) {
    if (addr < prev_addr) return;

    bin = addr - prev_addr;
    if (bin == prev_bin) {
      new_SLH[bin]++
    }
    else {
      prev_bin = bin;
    }
    prev_addr = addr;
  };

  long ASD_Prefetcher::stream_filter(long addr) {
    for (int i = 0; i < fs; i++) {
      
    }
  }
} /*namespace ramulator*/
