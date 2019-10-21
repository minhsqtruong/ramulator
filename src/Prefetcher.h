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
1) NextLine_Prefetcher: Exists on L1, L2 and L3 cache. Fetch the next cache line
of the current request address.
Reference: 18-740 Lecture 18.

2) Adaptive Stream Detector: Exists on L3 cache. Base on a histogram of stream
length, prefetch a request at a distance that is probabilistically sound.
Reference: https://www.cs.utexas.edu/~lin/papers/micro06.pdf

3) Global History Buffer: Exists on L2 and L3. Predict the next line pattern
based on a FIFO buffer that keep track of the access pattern in the past.
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
    GlobalHistory,
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

class GlobalHistory_Prefetcher: public Prefetcher
{
public:
  GlobalHistory_Prefetcher(Cache* cache);
  void activate(Request req);
  bool exist();
private:
  Cache* cache;
};

} /*namespace ramulator*/

#endif /*__PREFETCHER_H*/
