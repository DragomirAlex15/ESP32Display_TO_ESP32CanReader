/******************** INCLUDES ********************/
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <lvgl.h>
#include <LovyanGFX.hpp>
#include <lv_conf.h>

/******************** ESP-NOW DATA ********************/
typedef struct {
    int incarcare;
    int temp_apa;
    int temp_ulei;
    int pres_ulei;
    int lambda;
    int intake;
    int rpm;
    int maf;
    int map;
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

      cfg.rgb_order = true;



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

/******************** UI OBJECTS ********************/
lv_obj_t *incarcare_1;
lv_obj_t *temp_apa_1;
lv_obj_t *temp_ulei_1;
lv_obj_t *pres_ulei_1;
lv_obj_t *lambda_1;
lv_obj_t *intake_1;
lv_obj_t *rpm_1;
lv_obj_t *maf_1;
lv_obj_t *map_1;



/******************** GLOBAL STYLES (FIX CRITICAL) ********************/
lv_style_t style_title;
lv_style_t style_value;

/******************** ESP-NOW CALLBACK ********************/
void OnDataRecv(const esp_now_recv_info *info,
                const uint8_t *incomingData,
                int len) {

  if (len == sizeof(SensorData)) {
    memcpy(&rxData, incomingData, sizeof(SensorData));
    dataNoua = true;
  }
}

/******************** UI CREATE ********************/
void create_dashboard() {

  lv_obj_t* scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);   // fundal negru pur
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

  // --- PORTOCALIU OEM BMW ---
  lv_color_t orange_title = lv_color_hex(0xFF0000);   // titluri
  lv_color_t orange_value = lv_color_hex(0xFF0000);   // valori

  // --- STYLE TITLU ---
lv_style_init(&style_title);
lv_style_set_text_color(&style_title, orange_title);
lv_style_set_text_font(&style_title, &lv_font_montserrat_16);
lv_style_set_text_letter_space(&style_title, 1);
lv_style_set_bg_opa(&style_title, LV_OPA_TRANSP);
lv_style_set_border_width(&style_title, 0);

  

  // --- STYLE VALOARE ---
  lv_style_init(&style_value);
  lv_style_set_text_color(&style_value, orange_value);
  lv_style_set_text_font(&style_value, &lv_font_montserrat_28);
  lv_style_set_text_letter_space(&style_value, 1); 
  lv_style_set_bg_opa(&style_value, LV_OPA_TRANSP); 
  lv_style_set_border_width(&style_value, 0);

  // --- GRID EXACT CA ÎN DISPLAY ---
  int col1 = 10; 
  int col2 = 120;
  int col3 = 235;
  int row1 = 10;
  int row2 = 100;
  int row3 = 180;

  // ------------------ ROW 1 ------------------

  lv_obj_t* t1 = lv_label_create(scr);
  lv_label_set_text(t1, "BATTERY");
  lv_obj_add_style(t1, &style_title, 0);
  lv_obj_set_pos(t1, col1, row1);

  lv_obj_t* t2 = lv_label_create(scr);
  lv_label_set_text(t2, "COOLANT");
  lv_obj_add_style(t2, &style_title, 0);
  lv_obj_set_pos(t2, col2, row1);

  lv_obj_t* t3 = lv_label_create(scr);
  lv_label_set_text(t3, "OIL");
  lv_obj_add_style(t3, &style_title, 0);
  lv_obj_set_pos(t3, col3, row1);

  // ------------------ ROW 2 ------------------

  lv_obj_t* t4 = lv_label_create(scr);
  lv_label_set_text(t4, "OIL PRESS");
  lv_obj_add_style(t4, &style_title, 0);
  lv_obj_set_pos(t4, col1, row2);

  lv_obj_t* t5 = lv_label_create(scr);
  lv_label_set_text(t5, "LAMBDA");
  lv_obj_add_style(t5, &style_title, 0);
  lv_obj_set_pos(t5, col2, row2);

  lv_obj_t* t6 = lv_label_create(scr);
  lv_label_set_text(t6, "INTAKE");
  lv_obj_add_style(t6, &style_title, 0);
  lv_obj_set_pos(t6, col3, row2);

  // ------------------ ROW 3 ------------------

  lv_obj_t* t7 = lv_label_create(scr);
  lv_label_set_text(t7, "RPM");
  lv_obj_add_style(t7, &style_title, 0);
  lv_obj_set_pos(t7, col1, row3);

  lv_obj_t* t8 = lv_label_create(scr);
  lv_label_set_text(t8, "MAF");
  lv_obj_add_style(t8, &style_title, 0);
  lv_obj_set_pos(t8, col2, row3);

  lv_obj_t* t9 = lv_label_create(scr);
  lv_label_set_text(t9, "MAP");
  lv_obj_add_style(t9, &style_title, 0);
  lv_obj_set_pos(t9, col3, row3);

// ------------------ VALORI ------------------

// ROW 1
incarcare_1= lv_label_create(scr);
lv_label_set_text(incarcare_1, "--.--");
lv_obj_add_style(incarcare_1, &style_value, 0);
lv_obj_set_pos(incarcare_1, col1, row1 + 25);

temp_apa_1 = lv_label_create(scr);  // COOLANT
lv_label_set_text(temp_apa_1, "--.--");
lv_obj_add_style(temp_apa_1, &style_value, 0);
lv_obj_set_pos(temp_apa_1, col2, row1 + 25);

temp_ulei_1 = lv_label_create(scr); // RADIATOR
lv_label_set_text(temp_ulei_1, "--.--");
lv_obj_add_style(temp_ulei_1, &style_value, 0);
lv_obj_set_pos(temp_ulei_1, col3, row1 + 25);

// ROW 2
pres_ulei_1 = lv_label_create(scr);
lv_label_set_text(pres_ulei_1, "--.--");
lv_obj_add_style(pres_ulei_1, &style_value, 0);
lv_obj_set_pos(pres_ulei_1, col1, row2 + 25);

lambda_1 = lv_label_create(scr);
lv_label_set_text(lambda_1, "--.--");
lv_obj_add_style(lambda_1, &style_value, 0);
lv_obj_set_pos(lambda_1, col2, row2 + 25);

intake_1 = lv_label_create(scr); // OIL TEMP
lv_label_set_text(intake_1, "--.--");
lv_obj_add_style(intake_1, &style_value, 0);
lv_obj_set_pos(intake_1, col3, row2 + 25);

// ROW 3
rpm_1 = lv_label_create(scr);
lv_label_set_text(rpm_1, "--.--");
lv_obj_add_style(rpm_1, &style_value, 0);
lv_obj_set_pos(rpm_1, col1, row3 + 25);

maf_1 = lv_label_create(scr);
lv_label_set_text(maf_1, "--.--");
lv_obj_add_style(maf_1, &style_value, 0);
lv_obj_set_pos(maf_1, col2, row3 + 25);

map_1 = lv_label_create(scr);
lv_label_set_text(map_1, "--.--");
lv_obj_add_style(map_1, &style_value, 0);
lv_obj_set_pos(map_1, col3, row3 + 25);

}



/******************** SETUP ********************/
void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);

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

  

  create_dashboard();
}

/******************** LOOP ********************/
void loop() {

  if (dataNoua) {
    dataNoua = false;

    char buf[16];

    // 1. BATTERY (incarcare) – presupun % 
    snprintf(buf, sizeof(buf), "%d %%", rxData.incarcare);
    lv_label_set_text(incarcare_1, buf); 
    // 2. COOLANT (temp_apa) – °C 
    snprintf(buf, sizeof(buf), "%d C", rxData.temp_apa); 
    lv_label_set_text(temp_apa_1, buf); 
    // 3. OIL TEMP (temp_ulei) – °C 
    snprintf(buf, sizeof(buf), "%d C", rxData.temp_ulei);
    lv_label_set_text(temp_ulei_1, buf);
    // 4. OIL PRESS (pres_ulei) – bar (sau ce unitate ai tu)
    snprintf(buf, sizeof(buf), "%d", rxData.pres_ulei); 
    lv_label_set_text(pres_ulei_1, buf);
    // 5. LAMBDA – de ex. 0.98, 1.02 etc. // dacă o trimiți *1000 pe CAN, o poți împărți aici 
    snprintf(buf, sizeof(buf), "%d", rxData.lambda);
    lv_label_set_text(lambda_1, buf); 
    // 6. INTAKE – de ex. °C sau bar, cum ai definit tu
    snprintf(buf, sizeof(buf), "%d", rxData.intake);
    lv_label_set_text(intake_1, buf); 
    // 7. RPM
     snprintf(buf, sizeof(buf), "%d", rxData.rpm); 
     lv_label_set_text(rpm_1, buf); 
     // 8. MAF 
     snprintf(buf, sizeof(buf), "%d", rxData.maf);
      lv_label_set_text(maf_1, buf); 
      // 9. MAP 
      snprintf(buf, sizeof(buf), "%d", rxData.map); 
      lv_label_set_text(map_1, buf);
  }

  lv_timer_handler();
  delay(5);
}
