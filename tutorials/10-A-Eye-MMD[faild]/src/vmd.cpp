/**
Copyright (c) 2018 Gombe.

This software is released under the MIT License.
http://opensource.org/licenses/mit-license.php
*/
#include "vmd.hpp"
#include "ff.h"
#include "mmdstructure.h"
#include "pmd.hpp"

// extern int filopen(const char *pathname, FIL *f);
// extern int filread(FIL *fp, void *buff, size_t byte);

int filread(FIL *fp, void *buff, size_t byte) {
  unsigned int ra;
  FRESULT r;
  r = f_read(fp, buff, byte, &ra);
  if (r) return 0 /*fail*/;
  return ra;
}

void vmd::load(const char *path, pmd *model) {
  FIL f;
  unsigned int ra;

  printf("path: %s\r\n", path);
  // if (filopen(path, &f) != 0) {
  if (f_open(&f, path, FA_READ)) {
    printf("file cannot open(%s)\r\n", path);
    // fail();
  }

  {
    vmdheader h;
    filread(&f, &h, sizeof(vmdheader));
    // f_read(&f, &h, sizeof(vmdheader), &ra);
  }

  {  // load and convert bone name to bone index
#define N 16
    vmdmotion m[N];
    int idx = 0;
    filread(&f, &motioncount, sizeof(uint32_t));
    // f_read(&f, &motioncount, sizeof(uint32_t), &ra);
    motionlist = (motion_t *)malloc(sizeof(motion_t) * motioncount);
    if (motionlist == NULL) {
      // fail();
      printf("motionlist == null\r\n");
    }
    for (unsigned int i = 0; i < motioncount; i += N) {
      int l = motioncount - i;
      if (l > N) l = N;
      filread(&f, m, sizeof(vmdmotion) * l);
      // f_read(&f, m, sizeof(vmdmotion) * l, &ra);
      for (int j = 0; j < l; j++) {
        m[j].bonename[14] = '\0';
        int k;
        for (k = 0; k < model->bonecount; k++) {
          if (strcmp(m[j].bonename, model->bonenamelist[k]) == 0) break;
        }
        if (k == model->bonecount) {
          // not found same name => bone not found
          // skip remaining tasks.
          continue;
        }
        motionlist[idx].frame = m[j].frame;
        motionlist[idx].pos.x = m[j].location[0];
        motionlist[idx].pos.y = m[j].location[1];
        motionlist[idx].pos.z = m[j].location[2];
        motionlist[idx].rotation.qx = m[j].rotation[0];
        motionlist[idx].rotation.qy = m[j].rotation[1];
        motionlist[idx].rotation.qz = m[j].rotation[2];
        motionlist[idx].rotation.qw = m[j].rotation[3];
        motionlist[idx].boneid = k;
        idx++;
      }
    }
    // update motioncount, removed some unused motion.
    motioncount = idx;
    printf("count:%d\r\n", motioncount);
    motionlist =
        (motion_t *)realloc(motionlist, sizeof(motion_t) * motioncount);
    if (motionlist == NULL) {
      // fail();
      printf("motionlist == null\r\n");
    }
  }
#undef N
#define TIME_MAX 5000
  {  // sort motionlist into need order
    int *endframe;
    int *motionid;
    int donemotioncount = 0;
    static int pdc;
    int time = 0;
    endframe = (int *)calloc(model->bonecount, sizeof(int));
    motionid = (int *)calloc(model->bonecount, sizeof(int));
    if (endframe == NULL) {
      // fail();
    }
    for (int i = 0; i < model->bonecount; i++) {
      endframe[i] = -1;
    }
    for (int f = 0;; f++) {
      for (int i = 0; i < model->bonecount; i++) {
        motionid[i] = -1;
      }
      for (unsigned int j = donemotioncount; j < motioncount; j++) {
        if ((motionid[motionlist[j].boneid] == -1 ||
             motionlist[motionid[motionlist[j].boneid]].frame >
                 motionlist[j].frame) &&
            f >= endframe[motionlist[j].boneid]) {
          motionid[motionlist[j].boneid] = j;
        }
      }

      // collision detection and workaround
      for (int i = 0; i < model->bonecount; i++) {
        for (int j = i; j < model->bonecount; j++) {
          if (i != j && motionid[j] == donemotioncount) {
            motionid[j] = motionid[i];
          }
        }
        if (motionid[i] != -1) {
          endframe[motionlist[motionid[i]].boneid] =
              motionlist[motionid[i]].frame;
          {  // swap donemotioncount, motionid[i]
            motion_t tmp;
            tmp = motionlist[donemotioncount];
            motionlist[donemotioncount] = motionlist[motionid[i]];
            motionlist[motionid[i]] = tmp;
            donemotioncount++;
          }
        }
      }
      if (donemotioncount == (int)motioncount - 1) break;
      if (pdc == donemotioncount) {
        if (time > 100) break;
        time++;
      } else {
        time = 0;
        pdc = donemotioncount;
      }
      // {
      // 	int tm=endframe[0];
      // 	for(int i=1;i<model->bonecount;i++){
      // 	  if(endframe[i]>0&&tm>endframe[i])tm=endframe[i];
      // 	}
      // 	printf("f=%d,tm=%d\n",f,tm);
      // 	f=tm+1;
      // }
    }
    free(endframe);
  }
}
