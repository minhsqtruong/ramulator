#ifndef __PREFETCHER_H
#define __PREFETCHER_H

#include "DRAM.h"
#include "Request.h"
#include "Cache.h"
#include <vector>
#include <map>
#include <list>
#include <functional>
#include <cassert>
#include <cstdlib>
#include <algorithm>
#include <vector>

using namespace std;

namespace ramulator
{
/***************************** PREFETCHER.H ************************************
1) NextLine Prefetcher: Exists on L1, L2 and L3 cache. Fetch the next cache line
of the current request address.
Reference: 18-740 Lecture 18.

2) Adaptive Stream Detector: Exists on L3 cache. Base on a histogram of stream
length, prefetch a request at a distance that is probabilistically sound.
Reference: https://www.cs.utexas.edu/~lin/papers/micro06.pdf

3) Markov Prefetcher: Based on the Global Buffer FIFO and an index key table.
Reference: http://www.eecg.toronto.edu/~steffan/carg/readings/ghb.pdf
*******************************************************************************/

/*==============================================================================
PREFETCHER BASE CLASS DECLARATION
==============================================================================*/
class Cache;

class Prefetcher
{
public:
  enum class Type {
    ASD,
    Nextline,
    Markov,
    MAX
  } type = Type::ASD;

  Prefetcher(Cache* cache, Type _type);
  void insert_prefetch(long next_addr, Request req);
  virtual void activate(Request req);
  virtual bool exist();
private:
  Prefetcher* engine;
  Cache* cache;
};

/*==============================================================================
PREFETCHER ENGINES DECLARATION
==============================================================================*/

class NextLine_Prefetcher: public Prefetcher
{
public:
  NextLine_Prefetcher(Cache* cache);
  void activate(Request req);
  bool exist();
private:
  long get_next_addr(long next_addr);
  Cache* cache;
};

class ASD_Prefetcher: public Prefetcher
{
public:
  ASD_Prefetcher(Cache* cache);
  void activate(Request req);
  bool exist();
private:
  Cache* cache;
  int counter = 0;
  int epoch_size = 100;
  int fs = 16; // maximum stream length that the engine track
  int* SLH;
  int* new_SLH;
  long prev_addr;
  int prev_bin;
  int total_readrq = 0;

  void build_new_SLH(long addr); // construct new_SLH
  long stream_filter(long addr); // determine fetch depth
};

class Markov_Prefetcher: public Prefetcher
{
public:
  Markov_Prefetcher(Cache* cache);
  void activate(Request req);
  bool exist();
private:
  struct BUFFER_ENTRIES {
    long addr;
    struct BUFFER_ENTRIES *next_instance = NULL;
    long prefetch_addr;
  };
  struct INDEX_ENTRIES {
    long addr;
    struct BUFFER_ENTRIES *header;
  };

  Cache* cache;
  unsigned int max_index_size = 8;
  unsigned int max_buffer_size = 64;
  vector <struct INDEX_ENTRIES> index_table;
  vector <struct BUFFER_ENTRIES> global_history_buffer;

  bool index_hit(struct BUFFER_ENTRIES* header, long addr);
  vector <long> traverse_buffer(struct BUFFER_ENTRIES* header, vector<long> prefetch_addrs);
  void add_to_buffer(struct BUFFER_ENTRIES* header, long req);
  void add_to_index(long addr);
};

} /*namespace ramulator*/

#endif /*__PREFETCHER_H*/
