/**
Copyright (c) 2018 Gombe.

This software is released under the MIT License.
http://opensource.org/licenses/mit-license.php
*/
#ifndef _VMD_H
#define _VMD_H

#include "fvector3.hpp"
#include "pmd.hpp"
#include "quaternion.hpp"

struct motion_t {
  int frame;
  fvector3_t pos;
  quaternion_t rotation;
  int boneid;
  /*todo implement non linear interporation*/
};

class pmd;

class vmd {
 public:
  uint32_t motioncount;
  motion_t *motionlist;

  vmd() { motionlist = NULL; }

  void load(const char *path, pmd *model); /*model specified loader*/

  ~vmd() {
    if (motionlist != NULL) free(motionlist);
    motionlist = NULL;
  }
};

#endif  //_VMD_H
