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

/***************************** PREFETCHER.CPP **********************************
1) NextLine_Prefetcher: Exist on L1, L2 and L3 cache. Fetch the next cache line
of the current request address.
*******************************************************************************/

using namespace std;

namespace ramulator
{

/*==============================================================================
NEXTLINE PREFETCHER
==============================================================================*/

  NextLine_Prefetcher::NextLine_Prefetcher(Cache* cache): cache(cache) {};

  void NextLine_Prefetcher::activate(Request req)
  {
    assert(req.type == Request::Type::READ);
    auto next_addr = get_next_addr(req.addr);
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
ADS PREFETCHER
==============================================================================*/
  // ADS_Prefetcher::ADS_Prefetcher(Cache* cache): cache(cache) {};
  // void ADS_Prefetcher::activate(Request req){};
  // bool ADS_Prefetcher::exist(){};
} /*namespace ramulator*/
