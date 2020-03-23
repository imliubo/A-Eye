#include <camera.h>
#include <dvp.h>
#include <image_process.h>
#include <lcd.h>
#include <plic.h>
#include <region_layer.h>
#include <stdio.h>
#include <sysctl.h>
#include <uarths.h>
#include <w25qxx.h>
#include "flash-manager.h"
#include "kpu.h"
#include "pred.h"
#include "prior.h"
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#define INCBIN_PREFIX
#include "incbin.h"
// #include "iomem.h"
#include "utils.h"

#define PLL0_OUTPUT_FREQ 800000000UL
#define PLL1_OUTPUT_FREQ 400000000UL

volatile uint32_t g_ai_done_flag;
volatile uint8_t g_dvp_finish_flag;

static image_t kpu_image, display_image;
static region_layer_t rl;
static box_info_t boxes;

static float variances[2] = {0.1, 0.2};
float *pred_box, *pred_land, *pred_clses;
size_t pred_box_size, pred_landm_size, pred_clses_size;

uint8_t model_data[KMODEL_SIZE];
// INCBIN(model, "./../ulffd_landmark.kmodel");

kpu_model_context_t face_detect_task;

static int ai_done(void *ctx) {
  g_ai_done_flag = 1;
  return 0;
}

static int dvp_irq(void *ctx) {
  if (dvp_get_interrupt(DVP_STS_FRAME_FINISH)) {
    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE,
                         0);
    dvp_clear_interrupt(DVP_STS_FRAME_FINISH);
    g_dvp_finish_flag = 1;
  } else {
    dvp_start_convert();
    dvp_clear_interrupt(DVP_STS_FRAME_START);
  }
  return 0;
}

static void drawboxes(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2,
                      uint32_t class, float prob, uint32_t *landmark,
                      uint32_t landm_num) {
  if (x1 >= 320) x1 = 319;
  if (x2 >= 320) x2 = 319;
  if (y1 >= 224) y1 = 223;
  if (y2 >= 224) y2 = 223;

  lcd_draw_rectangle(x1, y1, x2, y2, 2, RED);
  for (uint32_t i = 0; i < landm_num; i++) {
    lcd_draw_point(landmark[2 * i], landmark[1 + 2 * i], GREEN);
  }
}

void main(void) {
  sysctl_pll_set_freq(SYSCTL_PLL0, PLL0_OUTPUT_FREQ);
  sysctl_pll_set_freq(SYSCTL_PLL1, PLL1_OUTPUT_FREQ);
  sysctl_clock_enable(SYSCTL_CLOCK_AI);
  sysctl_set_spi0_dvp_data(1);

  sysctl_clock_enable(SYSCTL_CLOCK_AI);

  uarths_init();
  plic_init();

  printf("flash init\n");
  w25qxx_init(3, 0, 40000000);
  w25qxx_read_data(KMODEL_START, model_data, KMODEL_END);

  printf("LCD init\n");
  lcd_init();
  lcd_clear(WHITE);

  printf("DVP init\n");
  camera_init();

  kpu_image.pixel = 3;
  kpu_image.width = 320;
  kpu_image.height = 240;
  image_init(&kpu_image);
  display_image.pixel = 2;
  display_image.width = 320;
  display_image.height = 240;
  image_init(&display_image);
  dvp_set_ai_addr((uint32_t)kpu_image.addr,
                  (uint32_t)(kpu_image.addr + 320 * 240),
                  (uint32_t)(kpu_image.addr + 320 * 240 * 2));
  dvp_set_display_addr((uint32_t)display_image.addr);
  dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
  dvp_disable_auto();
  /* DVP interrupt config */
  printf("DVP interrupt config\n");
  plic_set_priority(IRQN_DVP_INTERRUPT, 1);
  plic_irq_register(IRQN_DVP_INTERRUPT, dvp_irq, NULL);
  plic_irq_enable(IRQN_DVP_INTERRUPT);
  /* init face detect model */
  if (kpu_load_kmodel(&face_detect_task, model_data) != 0) {
    printf("\nmodel init error\n");
    while (1)
      ;
  }
  region_layer_init(&rl, anchor, 3160, 4, 5, 1, 320, 240, 0.7, 0.4, variances);
  boxes_info_init(&rl, &boxes, 200);

  /* enable global interrupt */
  sysctl_enable_irq();
  /* system start */
  printf("System start\n");
  while (1) {
    g_dvp_finish_flag = 0;
    dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE,
                         1);
    printf("config \n");
    while (g_dvp_finish_flag == 0) {
    };
    /* run face detect */
    g_ai_done_flag = 0;
    kpu_run_kmodel(&face_detect_task, kpu_image.addr, DMAC_CHANNEL5, ai_done,
                   NULL);
    printf("kpu run model\n");
    while (!g_ai_done_flag) {
      printf("ai not done\r\n");
      sleep(1);
    };

    kpu_get_output(&face_detect_task, 0, (uint8_t **)&pred_box, &pred_box_size);
    kpu_get_output(&face_detect_task, 1, (uint8_t **)&pred_land,
                   &pred_landm_size);
    kpu_get_output(&face_detect_task, 2, (uint8_t **)&pred_clses,
                   &pred_clses_size);
    printf("kpu get\n");
    rl.bbox_input = pred_box;
    rl.landm_input = pred_land;
    rl.clses_input = pred_clses;
    region_layer_run(&rl, &boxes);

    printf("layer run\n");
    lcd_draw_picture_resized(0, 0, 160, 120, (uint32_t *)display_image.addr);
    printf("draw pic\n");

    /* run key point detect */
    region_layer_draw_boxes(&boxes, drawboxes);

    boxes_info_reset(&boxes);
  }
}
