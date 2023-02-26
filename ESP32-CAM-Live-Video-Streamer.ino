#include "OV2640.h"
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// -------------------------------------- variables --------------------------------------
// Select camera model
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE
#define CAMERA_MODEL_AI_THINKER

// AP ssid and password
const char* ssid = "ESP32-CAM-AP";
const char* password = "0123456789";

// baudios
#define BAUD_RATE 115200

//camera resolution
//#define CAMERA_RESOLUTION FRAMESIZE_96X96   // 96x96
//#define CAMERA_RESOLUTION FRAMESIZE_QQVGA   // 160x120
//#define CAMERA_RESOLUTION FRAMESIZE_QCIF    // 176x144
//#define CAMERA_RESOLUTION FRAMESIZE_HQVGA   // 240x176
//#define CAMERA_RESOLUTION FRAMESIZE_240X240 // 240x240
//#define CAMERA_RESOLUTION FRAMESIZE_QVGA    // 320x240
//#define CAMERA_RESOLUTION FRAMESIZE_CIF     // 400x296
//#define CAMERA_RESOLUTION FRAMESIZE_HVGA    // 480x320
//#define CAMERA_RESOLUTION FRAMESIZE_VGA     // 640x480
//#define CAMERA_RESOLUTION FRAMESIZE_SVGA    // 800x600
//#define CAMERA_RESOLUTION FRAMESIZE_XGA     // 1024x768
//#define CAMERA_RESOLUTION FRAMESIZE_HD      // 1280x720
#define CAMERA_RESOLUTION FRAMESIZE_SXGA    // 1280x1024
//#define CAMERA_RESOLUTION FRAMESIZE_UXGA    // 1600x1200

//JPEG Quality
#define JPEG_CAMERA_QUALITY 10  //from 0 to 63. 0 Best quality

//fb count
#define FB_COUNT 2  //number of video frame buffers used in the ESP32's RAM to store the video frames captured by the camera.
// -------------------------------------- end variables ----------------------------------


#include "camera_pins.h"

OV2640 cam;

WebServer server(80);

const char HEADER[] = "HTTP/1.1 200 OK\r\n" \
                      "Access-Control-Allow-Origin: *\r\n" \
                      "Content-Type: multipart/x-mixed-replace; boundary=123456789000000000000987654321\r\n";
const char BOUNDARY[] = "\r\n--123456789000000000000987654321\r\n";
const char CTNTTYPE[] = "Content-Type: image/jpeg\r\nContent-Length: ";
const int hdrLen = strlen(HEADER);
const int bdrLen = strlen(BOUNDARY);
const int cntLen = strlen(CTNTTYPE);

void handle_jpg_stream(void)
{
  char buf[32];
  int s;
  WiFiClient client = server.client();
  client.write(HEADER, hdrLen);
  client.write(BOUNDARY, bdrLen);
  while (true)
  {
    if (!client.connected()) break;
    cam.run();
    s = cam.getSize();
    client.write(CTNTTYPE, cntLen);
    sprintf( buf, "%d\r\n\r\n", s );
    client.write(buf, strlen(buf));
    client.write((char *)cam.getfb(), s);
    client.write(BOUNDARY, bdrLen);
  }
}
const char JHEADER[] = "HTTP/1.1 200 OK\r\n" \
                       "Content-disposition: inline; filename=capture.jpg\r\n" \
                       "Content-type: image/jpeg\r\n\r\n";
const int jhdLen = strlen(JHEADER);

void handleNotFound()
{
  String message = "Server is running!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  server.send(200, "text / plain", message);
}

void setup()
{
  Serial.begin(BAUD_RATE);
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = CAMERA_RESOLUTION;
  config.jpeg_quality = JPEG_CAMERA_QUALITY;
  config.fb_count = FB_COUNT;

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  cam.init(config);

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Configurar el ESP32-CAM en modo AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  IPAddress ip = WiFi.softAPIP();
  Serial.println(F("WiFi connected in AP mode"));
  Serial.print("AP IP address: ");
  Serial.println(ip);

  server.on("/video", HTTP_GET, handle_jpg_stream);
  server.onNotFound(handleNotFound);
  server.begin();
}

void loop()
{
  server.handleClient();
}
