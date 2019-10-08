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
#include "Controller.h"
#include "Scheduler.h"
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

template <typename T>
class Controller;

template <typename T>
class Scheduler;

template <typename T>
class Prefetcher
{
public:
  Controller<T>* ctrl;

  enum class Type {
    FIX, // Use fixed prefetcher specified by other files
    ADS, // Minh: Minimalist Open Page
    MAJ,
    MAX
  } type = Type::MAJ;

  // Constructor
  Prefetcher(Controller<T>* ctrl) : ctrl(ctrl) {}

  void issue(Request& req,list<Request> q)
  {
    // Trivial cases
    if (type == Type::FIX) return;
    if (!q.size()) return;

    // Generate prefetch request based on prefetch engine option. NULL means no prefetch
    auto pft_req = generate[int(type)](req, q);
    if (pft_req->addr != req.addr) {
      pft_req->prefetch = true;

      // Minh: Minimalist Open Page Priority Assignment based on Prefetch Distance
      if (ctrl->scheduler->type == Scheduler<T>::Type::HPFRFCFS) {
        if ((pft_req->distance) >= 0 && (pft_req->distance) < 4) pft_req->priority = 4;
        else if ((pft_req->distance) >= 4 && (pft_req->distance) < 8) pft_req->priority = 3;
        else if ((pft_req->distance) >= 8 && (pft_req->distance) < 12) pft_req->priority = 2;
        else if ((pft_req->distance) >= 12 && (pft_req->distance) < 16) pft_req->priority = 1;
        else pft_req->priority = 0;
      }

      // Insert the prefetch request into the readq
      pft_req->arrive = req.arrive;
      q.push_back(*pft_req);
    }
  }

private:
  function<Request*(Request, list<Request>)> generate[int(Type::MAX)] = {

    // FIX
    [this] (Request req, list<Request> q) {
      return &req;
    },

    // ADS
    [this] (Request req, list<Request> q) {
      return &req; // TODO implement ADS here
    },

    // MAJ
    [this] (Request req, list<Request> q) {
      const int max_stride = 20;
      long freq[max_stride];
      for (auto itr0 = next(q.begin(), 1); itr0 != q.end(); itr0++) {
        for (auto itr1 = next(q.begin(), 1); itr1 != q.end(); itr1++) {
          if (!itr0->prefetch && !itr1->prefetch && itr0 != itr1) {
            long stride = abs(itr0->addr - itr1->addr);
            // We ignore stride that are too long
            if (stride <= max_stride) freq[stride] += 1;
          }
        }
      }
      int max = 0;
      for (int i = 0; i < max_stride; i++) {
        if (freq[i] > max) max = freq[i];
      }

      Request * pft_req = new Request(req.addr + max, Request::Type::READ, req.coreid);
      return pft_req;
    }
  };
};

} /*namespace ramulator*/

#endif /*__PREFETCHER_H*/
