/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <bsp.h>
#include <ff.h>
#include <stdio.h>
#include <unistd.h>
#include "3dconfig.hpp"
#include "3dmain.hpp"
#include "fpioa.h"
#include "gpiohs.h"
#include "lcd.h"
#include "sdcard.h"
#include "sysctl.h"

// int filopen(const char *pathname, FIL *f) {
//   return f_open(f, pathname, FA_READ);
// }

// int filread(FIL *fp, void *buff, size_t byte) {
//   unsigned int ra;
//   FRESULT r;
//   r = f_read(fp, buff, byte, &ra);
//   if (r) return 0 /*fail*/;
//   return ra;
// }

// overclock and voltageboost suported XD
// use  to configure core voltage.
// #define CORE_VOLTAGE_GPIONUM (5)
// int set_cpu_freq(uint32_t f) {  // MHz
//   if (f < 600) {
//     gpiohs_set_drive_mode(CORE_VOLTAGE_GPIONUM, GPIO_DM_INPUT);
//     gpiohs_set_pin(CORE_VOLTAGE_GPIONUM, GPIO_PV_LOW);
//   } else {
//     gpiohs_set_drive_mode(CORE_VOLTAGE_GPIONUM, GPIO_DM_INPUT);
//   }
//   // Wait for voltage setting done.
//   for (volatile int i = 0; i < 1000; i++)
//     ;
// #define MHz *1000000
//   return sysctl_cpu_set_freq(f MHz) / 1000000;
// #undef MHz
// }

uint64_t get_time(void) {
  uint64_t v_cycle = read_cycle();
  return v_cycle * 1000000 / sysctl_clock_get_freq(SYSCTL_CLOCK_CPU);
}

// int core1_function(void *ctx) {
//   uint64_t core = current_coreid();
//   printf("Core %ld Hello world\n", core);
//   while (1)
//     ;
// }

// int main(void) {
//   FATFS sdfs;
//   uint64_t core = current_coreid();
//   printf("Core %ld Hello world\n", core);
//   // make another thread\n
// #if PROCESSNUM == 2
//   printf("core 1 registering...\n");
//   register_core1(core1_function, 0);
//   for (volatile int i = 0; i < 10000; i++)
//     ;
// #endif

//   /* SD card init */
//   if (sd_init()) {
//     printf("Fail to init SD card\n");
//     return -1;
//   }

//   /* mount file system to SD card */
//   if (f_mount(&sdfs, _T("0:"), 1)) {
//     printf("Fail to mount file system\n");
//     return -1;
//   }

//   /* system start */
//   printf("system start\n");

//   main3d("model.pmd", "motion.vmd");
//   while (1)
//     ;
// }
