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
#include <vector>

using namespace std;

namespace ramulator
{

/*==============================================================================
BASE PREFETCHER
==============================================================================*/
  Prefetcher::Prefetcher(Cache* cache, Type _type): cache(cache)
  {
    type = _type;
    switch(type) {
      case Type::Nextline:
        engine = new NextLine_Prefetcher(cache);
        break;
      case Type::ASD:
        engine = new ASD_Prefetcher(cache);
        break;
      case Type::Markov:
        engine = new Markov_Prefetcher(cache);
        break;
      default:
        engine = new ASD_Prefetcher(cache);
    }
  };

  void Prefetcher::insert_prefetch(long next_addr, Request req)
  {
    assert(req.type == Request::Type::READ);
    // If prefetch engin predicts depth of 0
    if (next_addr == req.addr)
      return;

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

    // create new prefetch request;
    map<int, int> latencies;
    auto read_complete = [&latencies](Request& r){latencies[r.depart - r.arrive]++;};
    Request prefetch_req(next_addr, req.type, read_complete);
    prefetch_req.prefetch = true;

    // Set priority of request
    long distance = prefetch_req.addr - req.addr;
    if (distance >= 0 && distance < 4) prefetch_req.priority = 4;
    if (distance >= 4 && distance < 8) prefetch_req.priority = 3;
    if (distance >= 8 && distance <12) prefetch_req.priority = 2;
    if (distance >=12 && distance <16) prefetch_req.priority = 1;
    else prefetch_req.priority = 0;

    // Send the request to next level;
    if (!cache->is_last_level) {
      if(!cache->lower_cache->send(prefetch_req)) {
        cache->retry_list.push_back(prefetch_req);
      }
    } else {
      cache->cachesys->wait_list.push_back(
          make_pair(cache->cachesys->clk + cache->latency[int(cache->level)], prefetch_req));
    }
  };

  void Prefetcher::activate(Request req) {engine->activate(req);};
  bool Prefetcher::exist() {return engine->exist();};

/*==============================================================================
NEXTLINE PREFETCHER
==============================================================================*/

  NextLine_Prefetcher::NextLine_Prefetcher(Cache* cache): Prefetcher(cache, type) {};

  void NextLine_Prefetcher::activate(Request req)
  {
    auto next_addr = get_next_addr(req.addr);
    insert_prefetch(next_addr, req);
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
  ASD_Prefetcher::ASD_Prefetcher(Cache* cache): Prefetcher(cache, type)
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
    auto next_addr = req.addr + stream_filter(req.addr);
    insert_prefetch(next_addr, req);
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

    int bin = addr - prev_addr;
    if (bin == prev_bin) {
      new_SLH[bin]++;
    }
    else {
      prev_bin = bin;
    }
    total_readrq++;
    prev_addr = addr;
  };

  long ASD_Prefetcher::stream_filter(long addr) {
    float Pii; // P(i,i)
    float P_right_tail; // P(i+1, fs)
    for (int i = 0; i < fs; i++) {
      Pii = ((float) SLH[i]) / ((float) total_readrq);
      P_right_tail = 0.0;
      for (int j = i+1; j < fs; j++) {
        P_right_tail += ((float) SLH[j]) / ((float) total_readrq);
      }
      if (Pii < P_right_tail)
        return (long) i;
    }
    return (long) 0;
  };

/*==============================================================================
GLOBAL HISTORY PREFETCHER
==============================================================================*/
Markov_Prefetcher::Markov_Prefetcher(Cache* cache): Prefetcher(cache, type){};

void Markov_Prefetcher::activate(Request req)
{
  struct BUFFER_ENTRIES* header;
  vector <long> prefetch_addrs;
  if (index_hit(header, req.addr)) {
    prefetch_addrs = traverse_buffer(header, prefetch_addrs);
    add_to_buffer(header, req.addr);
  }
  else {
    add_to_index(req.addr);
  }

  // issue prefetch requests
  for (auto i = prefetch_addrs.begin(); i != prefetch_addrs.end(); i+=1)
    insert_prefetch(*i, req);
};

bool Markov_Prefetcher::exist()
{
  if (cache->level == Cache::Level::L2
   || cache->level == Cache::Level::L3) {
    return true;
  }
  return false;
};

bool Markov_Prefetcher::index_hit(struct BUFFER_ENTRIES* header ,long addr) {
  for (auto i = index_table.begin(); i != index_table.end(); i+=1) {
    if ((*i).addr == addr) {
      header = (*i).header;
      return true;
    }
  }
  return false;
};

vector <long> Markov_Prefetcher::traverse_buffer(struct BUFFER_ENTRIES* header, vector <long> prefetch_addrs)
{
  prefetch_addrs.push_back(header->prefetch_addr);
  if (header->next_instance == NULL)
    return prefetch_addrs;
  return traverse_buffer(header->next_instance, prefetch_addrs);
};

void Markov_Prefetcher::add_to_buffer(struct BUFFER_ENTRIES* header, long addr)
{
  struct BUFFER_ENTRIES new_entry;
  new_entry.next_instance = header;
  new_entry.prefetch_addr = global_history_buffer.back().addr;
  new_entry.addr = addr;
  if (global_history_buffer.size() > max_buffer_size)
    global_history_buffer.erase(global_history_buffer.begin());
  global_history_buffer.push_back(new_entry);

  // update the index table to the new request;
  for (auto i = index_table.begin(); i != index_table.end(); i+=1) {
    if ((*i).addr == addr)
      (*i).header = &new_entry;
  }
 };

 void Markov_Prefetcher::add_to_index(long addr)
 {
   struct INDEX_ENTRIES  new_idx_entry;
   struct BUFFER_ENTRIES new_buf_entry;
   new_idx_entry.addr = addr;
   new_idx_entry.header = &new_buf_entry;
   new_buf_entry.addr = addr;

   // add new entry to buffer header
   if (global_history_buffer.size() > max_buffer_size)
     global_history_buffer.erase(global_history_buffer.begin());
   global_history_buffer.push_back(new_buf_entry);

   // add new entry to index table
   if (index_table.size() > max_index_size)
    index_table.erase(index_table.begin());
  index_table.push_back(new_idx_entry);
 };
} /*namespace ramulator*/
