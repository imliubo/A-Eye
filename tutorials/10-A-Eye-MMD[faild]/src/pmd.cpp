/**
Copyright (c) 2018 Gombe.

This software is released under the MIT License.
http://opensource.org/licenses/mit-license.php
*/
#include "pmd.hpp"
#include <stdio.h>  //for poor C++ implementation at k210:(
#include <string.h>
#include <unistd.h>
#include "3dconfig.hpp"
#include "images.hpp"
#include "lcd.h"
#include "skeletal-animation.hpp"
#include "texturepoly.hpp"

#define ZNEAR 0.1f

// extern int filopen(const char *pathname, FIL *f);
// extern int filread(FIL *fp, void *buff, size_t byte);

static bonelist bl;

volatile enum vtaskstate {
  VTASK_WAIT = 0,
  VTASK_VERTEX_CALCULATION,
  VTASK_DRAW,
  VTASK_QUIT
} vtstate;

imgs::images il;
void pmd::init(const char *pathname, const char *motionpath) {
  il.init(16);

  FIL f;
  unsigned int ra;
  // if (filopen(pathname, &f) != 0) {
  if (f_open(&f, pathname, FA_READ)) {
    printf("%s cannot open\r\n", pathname);
  }

  {  // read headder
    pmdheader d;
    // filread(&f, &d, sizeof(d));
    f_read(&f, &d, sizeof(d), &ra);
    d.name[19] = '\0';  // handle force 19 charactor limit :) fix me.
    name = (char *)malloc(strlen(d.name) + 1);
    if (name == NULL) {
      // fail();
    }

    strcpy(name, d.name);
  }

  // read vertexes
  // filread(&f, &vertexcount, 4);
  f_read(&f, &vertexcount, 4, &ra);
  vertexlist = (fvector3_t *)malloc(sizeof(fvector3_t) * vertexcount);
  if (vertexlist == NULL) {
    // fail();
  }
  tvertexlist = (fvector4 *)malloc(sizeof(fvector4) * vertexcount);
  if (vertexlist == NULL) {
    // fail();
  }
  uvlist = (fvector2_t *)malloc(sizeof(fvector2_t) * vertexcount);
  if (uvlist == NULL) {
    // fail();
  }
  matrixnolist = (uint32_t *)malloc(sizeof(uint32_t) * vertexcount);
  if (matrixnolist == NULL) {
    // fail();
  }

  {
#define READNUM 32
    pmdvertex b[READNUM];
    int ln;
    for (uint32_t i = 0; i < vertexcount; i += READNUM) {
      if (vertexcount - i < READNUM) {
        ln = vertexcount - i;
      } else {
        ln = READNUM;
      }
      // filread(&f, &b, ln * sizeof(pmdvertex));
      f_read(&f, &b, ln * sizeof(pmdvertex), &ra);
      for (int j = 0; j < ln; j++) {
        int idx;
        idx = i + j;
        vertexlist[idx].x = b[j].pos[0];
        vertexlist[idx].y = b[j].pos[1];
        vertexlist[idx].z = b[j].pos[2];
        uvlist[idx].x = b[j].uv[0];
        uvlist[idx].y = b[j].uv[1];
        if (b[j].boneweight <= 5) {
          matrixnolist[idx] = b[j].bonenum[1];
        } else if (b[j].boneweight >= 95) {
          matrixnolist[idx] = b[j].bonenum[0];
          //	  printf("%d,//%d\n",boneidx[i.bone_num[0]],i.bone_num[0]);
        } else {
          matrixnolist[idx] = b[j].bonenum[1] | (b[j].bonenum[0] << 8) |
                              (b[j].boneweight << 16);
        }
      }
    }
  }

  // load faces
  // filread(&f, &facecount, 4);
  f_read(&f, &facecount, 4, &ra);
  facecount /= 3;  // triangle only
  facelist = (uint16_t *)malloc(3 * sizeof(uint16_t) * facecount);
  // filread(&f, facelist, 3 * sizeof(uint16_t) * facecount);
  f_read(&f, facelist, 3 * sizeof(uint16_t) * facecount, &ra);
  // load material
  // filread(&f, &materialcount, 4);
  f_read(&f, &materialcount, 4, &ra);
  materiallist = (imgs::image *)malloc(sizeof(imgs::image) * materialcount);
  materialfacecountlist = (uint16_t *)malloc(sizeof(uint16_t) * materialcount);
  {
    for (uint32_t i = 0; i < materialcount; i++) {
      pmdmaterial m;
      // filread(&f, &m, sizeof(pmdmaterial));
      f_read(&f, &m, sizeof(pmdmaterial), &ra);
      materiallist[i] = il.get_or_add(m.texture_filename);
      materialfacecountlist[i] = m.facevert_count / 3;
    }
  }

  // filread(&f, &bonecount, 2);
  f_read(&f, &bonecount, 2, &ra);
  bonelist = (bone_t *)malloc(sizeof(bone_t) * bonecount);
  bonenamelist = (char **)malloc(sizeof(char *) * bonecount);
  ikknee[0] = 0;
  {  // load bone and animation
    // bone name list is not needed after loading materials.
    pmdbone b;
    for (int i = 0; i < bonecount; i++) {
      // filread(&f, &b, sizeof(pmdbone));
      f_read(&f, &b, sizeof(pmdbone), &ra);
      b.name[19] = 0;  // fix me, support up to 20 charactors.(19 right now.)
      bonenamelist[i] = (char *)malloc(sizeof(char) * (strlen(b.name) + 1));
      strcpy(bonenamelist[i], b.name);
      bonelist[i].parent = b.parent;
      bonelist[i].pos.x = b.headpos[0];
      bonelist[i].pos.y = b.headpos[1];
      bonelist[i].pos.z = b.headpos[2];
      if (bonenamelist[i][0] != '\0' &&                                    //?
          bonenamelist[i][1] != '\0' &&                                    //?
          bonenamelist[i][2] == '\x82' &&                                  //ひ
          bonenamelist[i][3] == '\xd0' && bonenamelist[i][4] == '\x82' &&  //ざ
          bonenamelist[i][5] == '\xb4') {  //"ひざ" means knee
        if (ikknee[0] == 0) {
          ikknee[0] = i;
        } else {
          ikknee[1] = i;
        }
      }
    }
  }

  v->load(motionpath, this);

  // filread(&f, &ikcount, sizeof(uint16_t));
  f_read(&f, &ikcount, sizeof(uint16_t), &ra);
  printf("ik=%d\r\n", ikcount);
  iklist = (ik_t **)malloc(sizeof(ik_t *) * ikcount);
  {
    pmdik ik;
    for (int i = 0; i < ikcount; i++) {
      // filread(&f, &ik, sizeof(pmdik));
      f_read(&f, &ik, sizeof(pmdik), &ra);
      iklist[i] = (ik_t *)malloc(
          sizeof(ik_t) + (ik.len - 1 /*default length*/) * sizeof(uint16_t));
      iklist[i]->rootid = ik.root;
      iklist[i]->targetid = ik.target;
      iklist[i]->len = ik.len;
      iklist[i]->looptimes = ik.iterations;
      iklist[i]->anglelimit = ik.ctrlweight;
      // filread(&f, iklist[i]->data, sizeof(uint16_t) * ik.len);
      f_read(&f, iklist[i]->data, sizeof(uint16_t) * ik.len, &ra);
    }
  }
}

void pmd::calcvertexes(int ps) {
  fvector4 f;
  for (unsigned int j = ps * vertexcount / PROCESSNUM;
       j < (ps + 1) * vertexcount / PROCESSNUM; j++) {
#ifdef DISABLE_ANIMATION
    f = m.mul_fv4(fvector3(vertexlist[j]));
#else
    f = bl.boneworld[matrixnolist[j] & 0xFF].mul_fv4(fvector3(vertexlist[j]));
#endif
    tvertexlist[j].x = f.x * window_width + window_width / 2;
    tvertexlist[j].y = f.y * window_height + window_height / 2;
    tvertexlist[j].z = f.z;
    tvertexlist[j].w = f.w;
  }
}

void pmd::draw(uint16_t *drawbuff, uint16_t *zbuff, int ps) {
  texturetriangle t;
  fvector4 v[3];
  fvector2 uv[3];
  int zovercount = 0;
  int face[3];
  unsigned mati = 0;
  int nextmati = 0;

  nextmati = materialfacecountlist[0];

  for (uint32_t i = ps * facecount / PROCESSNUM;
       i < (ps + 1) * facecount / PROCESSNUM; i++) {
    for (int j = 0; j < 3; j++) {
      face[j] = facelist[i * 3 + j];
    }
    for (int j = 0; j < 3; j++) {
      v[j] = tvertexlist[face[j]];
      if (v[j].w < 0) {
        zovercount++;
      }
    }
    //    if(zovercount==0&&unvisible(v))continue;
    for (int j = 0; j < 3; j++) {
#define SIZE_TEX 256
      uv[j].x = (1.f - uvlist[face[j]].x) * SIZE_TEX;
      uv[j].y = (uvlist[face[j]].y) * SIZE_TEX;
    }
    while ((int)i >= nextmati) {
      mati++;
      nextmati += materialfacecountlist[mati];
    }

    t.triangle_set(v, 1, &(materiallist[mati]), uv);
    t.draw(zbuff, drawbuff, 0);
  }
}

#ifdef __cplusplus
extern "C" {
#endif
void *vTask(void *prm);
int main3d(const char *model, const char *motion);
void send_line(int ypos, uint8_t *line);
#ifdef __cplusplus
};
#endif

uint16_t g_drawbuff[2][window_width * DRAW_NLINES];

uint16_t g_zbuff[window_width * DRAW_NLINES];

static pmd p;

static int lastbuff = 0;

void *vTask(void *prm) {
  int ps = 1;
  int quit = 0;

  while (!quit) {
    switch (vtstate) {
      case VTASK_WAIT:
        break;
      case VTASK_VERTEX_CALCULATION:
        p.calcvertexes(1);
        if (vtstate == VTASK_VERTEX_CALCULATION) vtstate = VTASK_WAIT;
        break;
      case VTASK_DRAW:
        p.draw(g_drawbuff[lastbuff], g_zbuff, 1);
        if (vtstate == VTASK_DRAW) vtstate = VTASK_WAIT;
        break;
      case VTASK_QUIT:
        quit = 1;
        break;
    }
  }
}

// extern "C" {
// void send_line(int y, uint16_t *ptr);
// }

void send_line(int ypos, uint8_t *line) {
  uint32_t *b = (uint32_t *)line;
  for (int i = 0; i < window_width * DRAW_NLINES / 2; i++) {
    b[i] = line[0] << 16 | line[1];
    line += 2;
  }
  lcd_draw_picture(0, ypos, window_width, DRAW_NLINES,
                   (uint32_t *)((((uint32_t)*b) & 0x0FFFFFFF) | 0x80000000));
}

int main3d(const char *model, const char *motion) {
  Matrix4 projection;
  Matrix4 obj;

  // printf("%lu\n", sizeof(float));
  p.init(model, motion);

  //透視投影行列
  projection = loadPerspective(0.25f, float(window_height) / window_width,
                               ZNEAR, 40.f, 0, 0) *
               projection;

  fvector3 viewdir, veye;
  float dist = 3.f;
  fvector2 np = fvector2(150.f, 0);
  // float fps = 0.f;

  fvector3 n;

  float disttarget = 25.f;
  fvector3 transtarget = fvector3(0, -12, -1.2);
  fvector3 trans = fvector3(0, -12, -1.2);

  veye = fvector3(0, 0, -15.5f);

#ifndef DISABLE_ANIMATION
  bl.init(p.bonelist, p.bonecount, p.v->motionlist, &p);
#endif

  while (1) {
#ifdef VISUALDEBUG
    vdb_begin();
    vdb_frame();
#endif

    bl.calcall(p.m);

    //視点計算
    dist = dist * 0.7 +
           disttarget * 0.3;  // + 1.4f*cosf(np.x/150.f*3.14159265358979324f);
    trans = trans * 0.7 + transtarget * 0.3;

    veye = -fvector3(
        cosf(np.x / 300.f * 3.14159265f) * cosf(np.y / 300.f * 3.14159265f),
        sinf(np.y / 300.f * 3.14159265f),
        sinf(np.x / 300.f * 3.14159265f) * cosf(np.y / 300.f * 3.14159265f));
    //透視投影行列とカメラ行列の合成
    Matrix4 m = projection * lookat(fvector3(0, 0, 0), veye * dist) * obj *
                translation(trans);
#ifndef DISABLE_ANIMATION
    bl.calcall(m);
#endif

    for (int i = 0; i < window_width * DRAW_NLINES; i++) {
      g_zbuff[i] = 0xFFFF;
      g_drawbuff[lastbuff][i] = 0x8410; /*RGB*/
    }

    // initialize_g_draw_buffer();

    //描画ステージ
    p.m = m;
    vtstate = VTASK_VERTEX_CALCULATION;
    p.calcvertexes(0);
    while (vtstate != VTASK_WAIT)
      ;
    vtstate = VTASK_DRAW;
    p.draw(g_drawbuff[lastbuff], g_zbuff, 0);
    while (vtstate != VTASK_WAIT)
      ;

#ifndef DISABLE_OUTPUT
    {
      static int ptime;
      float fps;
      char fpsstr[20];
      fps = 1000000.f / (get_time() - ptime);
      sprintf(fpsstr, "%5.1ffps", fps);
      // lcd_boxfill(0xFFFF,0,240-12,64,12);
      // lcd_ram_draw_string_width(fpsstr, (uint32_t *)g_drawbuff[lastbuff],
      //                           0xF800, 0xFFFF, window_width / 2);
      printf("hello world\r\n");
      ptime = get_time();
    }
    send_line(0, (uint8_t *)(g_drawbuff[lastbuff]));
#endif  // disable output flag
    lastbuff = 1 - lastbuff;
  }
  return 0;
}
