#include <ff.h>
#include <fpioa.h>
#include <gpio.h>
#include <lcd.h>
#include <sdcard.h>
#include <sleep.h>
#include <stdio.h>
#include <sysctl.h>
#include "A-Eye.h"
#include "spi.h"

FIL file;
FRESULT ret = FR_OK;
uint32_t v_ret_len = 0;
char filename_buffer[28];
uint8_t frame_data[64800] __attribute__((aligned(8)));

static int sdcard_init(void) {
  uint8_t status;

  status = sd_init();
  printf("sd init %d\r\n", status);
  if (status != 0) {
    return status;
  }

  printf("card info status %d\r\n", status);
  printf("CardCapacity:%ld\r\n", cardinfo.CardCapacity);
  printf("CardBlockSize:%d\r\n", cardinfo.CardBlockSize);
  return 0;
}

static int fs_init(void) {
  static FATFS sdcard_fs;
  FRESULT status;
  DIR dj;
  FILINFO fno;

  status = f_mount(&sdcard_fs, _T("0:"), 1);
  printf("mount sdcard:%d\r\n", status);
  if (status != FR_OK) return status;

  // printf("printf filename\r\n");
  // status = f_findfirst(&dj, &fno, _T("0:"), _T("*"));
  // while (status == FR_OK && fno.fname[0]) {
  //   if (fno.fattrib & AM_DIR)
  //     printf("dir:%s\r\n", fno.fname);
  //   else
  //     printf("file:%s\r\n", fno.fname);
  //   status = f_findnext(&dj, &fno);
  // }
  // f_closedir(&dj);
  return 0;
}

extern int main3d(const char *model, const char *motion);

int core1_function(void *ctx) {
  uint64_t core = current_coreid();
  printf("Core %ld Hello world\r\n", core);
  while (1)
    ;
}

int main(void) {
  // sysctl_pll_set_freq(SYSCTL_PLL0, 800000000UL);
  // sysctl_pll_set_freq(SYSCTL_PLL1, 800000000UL);

  usleep(100000);
  sysctl_cpu_set_freq(600000000);
  usleep(100000);

  uint64_t core = current_coreid();
  printf("Core %ld Hello world\r\n", core);

  printf("core 1 registering...\r\n");
  register_core1(core1_function, 0);
  for (volatile int i = 0; i < 10000; i++)
    ;

  plic_init();
  sysctl_enable_irq();
  spi_set_clk_rate(SPI_DEVICE_1, 30000000);
  sleep(1);

  if (sdcard_init()) {
    printf("SD card err\r\n");
    return -1;
  }
  if (fs_init()) {
    printf("FAT32 err\r\n");
    return -1;
  }

  lcd_init();
  lcd_set_backlight(0.2);
  lcd_clear(WHITE);
  lcd_draw_string(85, 65, "Bad Apple", BLACK);
  sleep(1);

  main3d("model.pmd", "motion.vmd");

  while (1)
    ;
}
