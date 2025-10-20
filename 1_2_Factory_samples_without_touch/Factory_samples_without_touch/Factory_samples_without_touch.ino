#include <Arduino.h>
#include <lvgl.h>
#include "CST820.h"

// ===================== ECRAN/PLATFORMA =====================
#define TFT_W 320
#define TFT_H 240

// #define TOUCH
#define I2C_SDA 21
#define I2C_SCL 22

#ifdef TOUCH
CST820 touch(I2C_SDA, I2C_SCL, -1, -1);
#endif

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789   _panel;
  lgfx::Bus_Parallel8  _bus;
public:
  LGFX(void) {
    { // BUS 8080 8-bit
      auto cfg = _bus.config();
      cfg.freq_write = 25000000;
      cfg.pin_wr = 4;
      cfg.pin_rd = 2;
      cfg.pin_rs = 16;
      cfg.pin_d0 = 15; cfg.pin_d1 = 13; cfg.pin_d2 = 12; cfg.pin_d3 = 14;
      cfg.pin_d4 = 27; cfg.pin_d5 = 25; cfg.pin_d6 = 33; cfg.pin_d7 = 32;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    { // Panou ST7789 (240x320)
      auto cfg = _panel.config();
      cfg.pin_cs = 17;
      cfg.pin_rst = -1;
      cfg.pin_busy = -1;
      cfg.panel_width = 240;
      cfg.panel_height = 320;
      cfg.offset_x = 0; cfg.offset_y = 0; cfg.offset_rotation = 0;
      cfg.readable = false; cfg.invert = false; cfg.rgb_order = false;
      cfg.dlen_16bit = false; cfg.bus_shared = true;
      _panel.config(cfg);
    }
    setPanel(&_panel);
  }
};
static LGFX tft;

// ===================== LVGL: buffer & drivere =====================
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1, *buf2;

#if LV_USE_LOG
static void my_print(const char * buf) { Serial.print(buf); }
#endif

static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  int w = (area->x2 - area->x1 + 1);
  int h = (area->y2 - area->y1 + 1);
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.writePixels(&color_p->full, w * h, false);
  tft.endWrite();
  lv_disp_flush_ready(disp);
}

#ifdef TOUCH
static void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  bool touched; uint8_t g; uint16_t x, y;
  touched = touch.getTouch(&x, &y, &g);
  data->state = touched ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
  if (touched) { data->point.x = x; data->point.y = y; }
}
#endif

// ===================== UI: bare orizontale =====================
// (Fix pt. Arduino: prototipuri înainte de struct)
typedef struct RowBar RowBar;
static void rowbar_create(RowBar* r, lv_obj_t* parent, const char* name, const char* unit,
                          int minv, int maxv, int x, int y, int w, int h, lv_color_t color);
static void rowbar_set(RowBar* r, int v);

struct RowBar {
  lv_obj_t* cont;        // container de rând
  lv_obj_t* bar;         // bara propriu-zisă
  lv_obj_t* lbl_left;    // nume + icon (text)
  lv_obj_t* lbl_right;   // valoare + unitate
  int minv, maxv;
  const char* unit;
  lv_color_t color;
};

// 4 rânduri: apă, ulei presiune, ulei temperatură, încărcare
static RowBar rb_apa, rb_ulei_p, rb_ulei_t, rb_incarcare;

// stil comun pentru bară
static void style_bar(lv_obj_t* bar, lv_color_t indicator) {
  // fundalul barei (track)
  lv_obj_set_style_bg_color(bar, lv_color_hex(0x006600), LV_PART_MAIN);      // verde închis
  lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_radius(bar, 6, LV_PART_MAIN);

  // indicatorul (umplerea)
  lv_obj_set_style_bg_color(bar, indicator, LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_radius(bar, 6, LV_PART_INDICATOR);
}

static void rowbar_create(RowBar* r, lv_obj_t* parent, const char* name, const char* unit,
                          int minv, int maxv, int x, int y, int w, int h, lv_color_t color) {
  r->minv = minv; r->maxv = maxv; r->unit = unit; r->color = color;

  // container de rând (transparent)
  r->cont = lv_obj_create(parent);
  lv_obj_set_size(r->cont, w, h);
  lv_obj_set_pos(r->cont, x, y);
  lv_obj_set_style_bg_opa(r->cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(r->cont, 0, 0);
  lv_obj_set_style_pad_all(r->cont, 0, 0);

  // eticheta stânga (nume)
  r->lbl_left = lv_label_create(r->cont);
  lv_label_set_text(r->lbl_left, name);
  lv_obj_set_style_text_color(r->lbl_left, lv_color_black(), 0);
  lv_obj_align(r->lbl_left, LV_ALIGN_LEFT_MID, 0, 0);

  // bară
  r->bar = lv_bar_create(r->cont);
  lv_bar_set_range(r->bar, minv, maxv);
  lv_obj_set_size(r->bar, w - 120, h - 10);
  lv_obj_align(r->bar, LV_ALIGN_LEFT_MID, 85, 0);
  style_bar(r->bar, color);
  lv_bar_set_value(r->bar, minv, LV_ANIM_OFF);

  // eticheta dreapta (valoare)
  r->lbl_right = lv_label_create(r->cont);
  lv_label_set_text(r->lbl_right, "-");
  lv_obj_set_style_text_color(r->lbl_right, lv_color_black(), 0);
  lv_obj_align(r->lbl_right, LV_ALIGN_RIGHT_MID, 0, 0);
}

static void rowbar_set(RowBar* r, int v) {
  if (v < r->minv) v = r->minv;
  if (v > r->maxv) v = r->maxv;
  lv_bar_set_value(r->bar, v, LV_ANIM_ON);
  static char buf[32];
  snprintf(buf, sizeof(buf), "%d%s", v, r->unit ? r->unit : "");
  lv_label_set_text(r->lbl_right, buf);
}

// Funcții simple ca să actualizezi din codul tău
void set_apa_temp(int celsius)         { rowbar_set(&rb_apa, celsius); }
void set_ulei_presiune(int bar10)      { rowbar_set(&rb_ulei_p, bar10); }   // ex.: 0..10 bar
void set_ulei_temp(int celsius)        { rowbar_set(&rb_ulei_t, celsius); }
void set_incarcare(int procente)       { rowbar_set(&rb_incarcare, procente); }

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  pinMode(0, OUTPUT); digitalWrite(0, HIGH);

#ifdef TOUCH
  touch.begin();
#endif

  lv_init();
#if LV_USE_LOG
  lv_log_register_print_cb(my_print);
#endif

  tft.init();
  tft.setRotation(1);                 // landscape
  tft.fillScreen(TFT_BLACK);

  buf1 = (lv_color_t*)heap_caps_malloc(TFT_W * 80 * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  buf2 = (lv_color_t*)heap_caps_malloc(TFT_W * 80 * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, TFT_W * 80);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = TFT_W;
  disp_drv.ver_res = TFT_H;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

#ifdef TOUCH
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);
#endif

  // Fundal verde + titlu negru
  lv_obj_t* scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x00A000), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

  lv_obj_t* title = lv_label_create(scr);
  lv_label_set_text(title, "SENZORI MOTOR");
  lv_obj_set_style_text_color(title, lv_color_black(), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

  // Coordonate pentru 4 rânduri
  const int X = 10, W = TFT_W - 20;
  const int H = 40, GAP = 6;
  int y = 26;

  // Creează rândurile (culori indicator: negru/gri închis sau ce vrei)
  rowbar_create(&rb_apa,        scr, "APA",          "°C",  40, 120, X, y, W, H, lv_color_black());   y += H + GAP;
  rowbar_create(&rb_ulei_p,     scr, "ULEI PRES",   " bar", 0,  10,  X, y, W, H, lv_color_black());   y += H + GAP;
  rowbar_create(&rb_ulei_t,     scr, "ULEI TEMP",    "°C",  40, 150, X, y, W, H, lv_color_black());   y += H + GAP;
  rowbar_create(&rb_incarcare,  scr, "INCARCARE",     "%",   0, 100, X, y, W, H, lv_color_black());

  // Valori inițiale (doar demo)
  set_apa_temp(88);
  set_ulei_presiune(3);
  set_ulei_temp(96);
  set_incarcare(72);
}

// ===================== LOOP =====================
void loop() {
  // DEMO: animație ușoară (poți șterge)
  static uint32_t t0 = millis();
  uint32_t t = millis() - t0;

  set_apa_temp( 85 + (int)(10 * sin(t / 900.0)) );
  set_ulei_presiune( 2 + (int)(2 * (sin(t / 700.0) + 1)) );
  set_ulei_temp( 90 + (int)(12 * sin(t / 800.0)) );
  set_incarcare( 50 + (int)(40 * (0.5 + 0.5 * sin(t / 1200.0))) );

  lv_timer_handler();
  delay(5);
}
