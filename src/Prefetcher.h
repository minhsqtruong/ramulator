/***************************** PREFETCHER.H ***********************************


This file contains the different prefetcher engines that memory controller can
use in adjunction with a scheduler to enqueue prefetching request.

Current Prefetcher Engines:

1) ASD - Adaptive Stream Detection

2) MAJ - Majority Depth - A simple prefetching mechanism that issue a read request
at the depth of the most occuring normal read stride.

*****************************************************************************/

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

using namespace std;

namespace ramulator
{
/***************************** PREFETCHER.H ************************************
1) NextLine_Prefetcher: Exist on L1, L2 and L3 cache. Fetch the next cache line
of the current request address.

2) Adaptive Stream Detector: Exist on L3 cache. Base on a histogram of stream
length, prefetch a request at a distance that is probabilistically sound.
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
    MAX
  } type = Type::ASD;

  Prefetcher(Cache* cache);
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

} /*namespace ramulator*/

#endif /*__PREFETCHER_H*/
