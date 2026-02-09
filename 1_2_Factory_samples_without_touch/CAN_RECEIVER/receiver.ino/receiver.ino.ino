/******************** INCLUDES ********************/
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <lvgl.h>
#include <LovyanGFX.hpp>

/******************** ESP-NOW DATA ********************/
typedef struct {
  int apa;
  int ulei_p;
  int ulei_t;
  int incarcare;
} SensorData;

volatile bool dataNoua = false;
SensorData rxData;


/******************** DISPLAY ********************/
#define TFT_W 320
#define TFT_H 240

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_Parallel8 _bus;

public:
  LGFX() {
    {
      auto cfg = _bus.config();
      cfg.freq_write = 25000000;
      cfg.pin_wr = 4;
      cfg.pin_rd = 2;
      cfg.pin_rs = 16;
      cfg.pin_d0 = 15;
      cfg.pin_d1 = 13;
      cfg.pin_d2 = 12;
      cfg.pin_d3 = 14;
      cfg.pin_d4 = 27;
      cfg.pin_d5 = 25;
      cfg.pin_d6 = 33;
      cfg.pin_d7 = 32;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }

    {
      auto cfg = _panel.config();
      cfg.pin_cs = 17;
      cfg.pin_rst = -1;
      cfg.panel_width = 240;
      cfg.panel_height = 320;
      _panel.config(cfg);
    }

    setPanel(&_panel);
  }
};

static LGFX tft;

/******************** LVGL ********************/
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[TFT_W * 40];
static lv_color_t buf2[TFT_W * 40];

static void my_disp_flush(lv_disp_drv_t *disp,
                          const lv_area_t *area,
                          lv_color_t *color_p) {

  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.writePixels((lgfx::rgb565_t *)&color_p->full, w * h);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

/******************** UI TYPES ********************/
typedef struct {
  lv_obj_t* bar;
  lv_obj_t* label;
  int minv;
  int maxv;
  const char* unit;
} Row;

/******************** UI FUNCTIONS ********************/
void row_create(Row* r, lv_obj_t* parent,
                const char* name,
                const char* unit,
                int minv, int maxv,
                int y);

void row_set(Row* r, int v);

/******************** UI OBJECTS ********************/
Row rApa, rUleiP, rUleiT, rInc;

/******************** ESP-NOW CALLBACK ********************/
void OnDataRecv(const esp_now_recv_info *info,
                const uint8_t *incomingData,
                int len) {

  Serial.print("ESP-NOW RX, len=");
  Serial.println(len);

  if (len == sizeof(SensorData)) {
    memcpy(&rxData, incomingData, sizeof(SensorData));
    dataNoua = true;

    Serial.println("Struct OK");
  } else {
    Serial.println("Struct size mismatch!");
  }
}


/******************** UI IMPLEMENTARE ********************/
void row_create(Row* r, lv_obj_t* parent,
                const char* name,
                const char* unit,
                int minv, int maxv,
                int y) {

  r->minv = minv;
  r->maxv = maxv;
  r->unit = unit;

  lv_obj_t* cont = lv_obj_create(parent);
  lv_obj_set_size(cont, TFT_W - 20, 40);
  lv_obj_set_pos(cont, 10, y);
  lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(cont, 0, 0);

  lv_obj_t* lbl = lv_label_create(cont);
  lv_label_set_text(lbl, name);
  lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 0, 0);

  r->bar = lv_bar_create(cont);
  lv_bar_set_range(r->bar, minv, maxv);
  lv_obj_set_size(r->bar, 160, 18);
  lv_obj_align(r->bar, LV_ALIGN_CENTER, 30, 0);

  r->label = lv_label_create(cont);
  lv_obj_align(r->label, LV_ALIGN_RIGHT_MID, 0, 0);
}

void row_set(Row* r, int v) {

  if (v < r->minv) v = r->minv;
  if (v > r->maxv) v = r->maxv;

  lv_bar_set_value(r->bar, v, LV_ANIM_OFF);

  static char buf[16];
  sprintf(buf, "%d%s", v, r->unit);
  lv_label_set_text(r->label, buf);
}

/******************** SETUP ********************/
void setup() {

  Serial.begin(115200);

  // WiFi + ESP-NOW
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP NOW INIT FAIL");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  // LVGL INIT
  lv_init();

  tft.init();
  tft.setRotation(1);

  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, TFT_W * 40);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = TFT_W;
  disp_drv.ver_res = TFT_H;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // UI
  lv_obj_t* scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x00A000), 0);

  row_create(&rApa,   scr, "APA", "C", 40, 120, 30);
  row_create(&rUleiP, scr, "ULEI P", "bar", 0, 10, 80);
  row_create(&rUleiT, scr, "ULEI T", "C", 40, 150, 130);
  row_create(&rInc,   scr, "INCARCARE", "%", 0, 100, 180);
}

/******************** LOOP ********************/
void loop() {

  if (dataNoua) {
    dataNoua = false;

      Serial.print("APA=");
      Serial.print(rxData.apa);
      Serial.print(" ULEI_P=");
      Serial.print(rxData.ulei_p);
      Serial.print(" ULEI_T=");
      Serial.print(rxData.ulei_t);
      Serial.print(" INC=");
      Serial.println(rxData.incarcare);

    row_set(&rApa, rxData.apa);
    row_set(&rUleiP, rxData.ulei_p);
    row_set(&rUleiT, rxData.ulei_t);
    row_set(&rInc, rxData.incarcare);
  }

  lv_timer_handler();   // ASTA e suficient
  delay(5);
}

