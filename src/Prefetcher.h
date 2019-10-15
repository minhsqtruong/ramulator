/***************************** PREFETCHER.H ***********************************


This file contains the different prefetcher engines that memory controller can
use in adjunction with a scheduler to enqueue prefetching request.

Current Prefetcher Engines:

1) ADS - Adaptive Stream Detection

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

class Prefetcher
{
public:
  Prefetcher(Cache* cache): cache(cache)
  {
    // Enable one line to choose prefetching engines
    //engine = new BasePrefetcher(cache);
    //engine = new BasePrefetcher(cache);
    //engine = new BasePrefetcher(cache);
    engine = new NextLine_Prefetcher(cache);
  };
  void activate(Req req) {engine->activate(req);};
  bool exist() {return engine->exist();}
private:
  Cache* cache;

  // Enable one line to choose prefetching engines
  //BasePrefetcher* engine;
  //BasePrefetcher* engine;
  //BasePrefetcher* engine;
  NextLine_Prefetcher* engine;
};

} /*namespace ramulator*/

#endif /*__PREFETCHER_H*/
