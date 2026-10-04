#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>
#include <esp_camera.h>
#include <esp_sleep.h>

namespace {

// Canvieu 5 per 10 per fer una fotografia cada deu minuts.
constexpr uint64_t CAPTURE_INTERVAL_MINUTES = 5;
constexpr uint64_t US_PER_SECOND = 1000000ULL;
constexpr uint64_t SECONDS_PER_MINUTE = 60ULL;

constexpr char PHOTO_DIRECTORY[] = "/timelapse";
constexpr uint8_t WARMUP_FRAMES = 3;

// GOOUUU ESP32-S3-CAM: mateix pinout de camera que ESP32-S3-EYE.
constexpr int CAM_PIN_PWDN = -1;
constexpr int CAM_PIN_RESET = -1;
constexpr int CAM_PIN_XCLK = 15;
constexpr int CAM_PIN_SIOD = 4;
constexpr int CAM_PIN_SIOC = 5;
constexpr int CAM_PIN_Y9 = 16;
constexpr int CAM_PIN_Y8 = 17;
constexpr int CAM_PIN_Y7 = 18;
constexpr int CAM_PIN_Y6 = 12;
constexpr int CAM_PIN_Y5 = 10;
constexpr int CAM_PIN_Y4 = 8;
constexpr int CAM_PIN_Y3 = 9;
constexpr int CAM_PIN_Y2 = 11;
constexpr int CAM_PIN_VSYNC = 6;
constexpr int CAM_PIN_HREF = 7;
constexpr int CAM_PIN_PCLK = 13;

// Lector microSD integrat, bus SDMMC d'un bit.
constexpr int SD_PIN_CLK = 39;
constexpr int SD_PIN_CMD = 38;
constexpr int SD_PIN_D0 = 40;

RTC_DATA_ATTR uint32_t nextPhotoNumber = 1;

void enterDeepSleep() {
  const uint64_t sleepTimeUs =
      CAPTURE_INTERVAL_MINUTES * SECONDS_PER_MINUTE * US_PER_SECOND;

  Serial.printf("Entrant en deep sleep durant %llu minuts.\n",
                CAPTURE_INTERVAL_MINUTES);
  Serial.flush();
  esp_sleep_enable_timer_wakeup(sleepTimeUs);
  esp_deep_sleep_start();
}

bool initialiseCamera() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = CAM_PIN_Y2;
  config.pin_d1 = CAM_PIN_Y3;
  config.pin_d2 = CAM_PIN_Y4;
  config.pin_d3 = CAM_PIN_Y5;
  config.pin_d4 = CAM_PIN_Y6;
  config.pin_d5 = CAM_PIN_Y7;
  config.pin_d6 = CAM_PIN_Y8;
  config.pin_d7 = CAM_PIN_Y9;
  config.pin_xclk = CAM_PIN_XCLK;
  config.pin_pclk = CAM_PIN_PCLK;
  config.pin_vsync = CAM_PIN_VSYNC;
  config.pin_href = CAM_PIN_HREF;
  config.pin_sccb_sda = CAM_PIN_SIOD;
  config.pin_sccb_scl = CAM_PIN_SIOC;
  config.pin_pwdn = CAM_PIN_PWDN;
  config.pin_reset = CAM_PIN_RESET;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_UXGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  const esp_err_t result = esp_camera_init(&config);
  if (result != ESP_OK) {
    Serial.printf("ERROR: no s'ha pogut iniciar la camera (0x%x).\n", result);
    return false;
  }

  return true;
}

bool mountSdCard() {
  if (!SD_MMC.setPins(SD_PIN_CLK, SD_PIN_CMD, SD_PIN_D0)) {
    Serial.println("ERROR: no s'han pogut assignar els pins de la microSD.");
    return false;
  }

  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("ERROR: no s'ha pogut muntar la microSD.");
    return false;
  }

  if (SD_MMC.cardType() == CARD_NONE) {
    Serial.println("ERROR: no s'ha detectat cap targeta microSD.");
    SD_MMC.end();
    return false;
  }

  if (!SD_MMC.exists(PHOTO_DIRECTORY) && !SD_MMC.mkdir(PHOTO_DIRECTORY)) {
    Serial.println("ERROR: no s'ha pogut crear /timelapse.");
    SD_MMC.end();
    return false;
  }

  return true;
}

camera_fb_t *capturePhoto() {
  // Els primers fotogrames permeten estabilitzar exposicio i balanc de blancs.
  for (uint8_t frame = 0; frame < WARMUP_FRAMES; ++frame) {
    camera_fb_t *warmup = esp_camera_fb_get();
    if (warmup == nullptr) {
      Serial.println("ERROR: ha fallat un fotograma d'estabilitzacio.");
      return nullptr;
    }
    esp_camera_fb_return(warmup);
    delay(100);
  }

  return esp_camera_fb_get();
}

bool savePhoto(const camera_fb_t *photo) {
  char path[40];

  // Evita sobreescriure fotos si es perd la memoria RTC per un reset complet.
  do {
    snprintf(path, sizeof(path), "%s/img_%08lu.jpg", PHOTO_DIRECTORY,
             static_cast<unsigned long>(nextPhotoNumber++));
  } while (SD_MMC.exists(path));

  File file = SD_MMC.open(path, FILE_WRITE);
  if (!file) {
    Serial.printf("ERROR: no s'ha pogut obrir %s.\n", path);
    return false;
  }

  const size_t bytesWritten = file.write(photo->buf, photo->len);
  file.close();

  if (bytesWritten != photo->len) {
    Serial.printf("ERROR: escriptura incompleta de %s (%u de %u bytes).\n",
                  path, static_cast<unsigned>(bytesWritten),
                  static_cast<unsigned>(photo->len));
    return false;
  }

  Serial.printf("Foto desada: %s (%u bytes).\n", path,
                static_cast<unsigned>(photo->len));
  return true;
}

}  // namespace

void setup() {
  const uint32_t startedAtMs = millis();
  Serial.begin(115200);
  delay(300);

  Serial.printf("\nDespertar numero %lu. Causa: %d.\n",
                static_cast<unsigned long>(nextPhotoNumber),
                static_cast<int>(esp_sleep_get_wakeup_cause()));

  if (!initialiseCamera()) {
    enterDeepSleep();
  }

  if (!mountSdCard()) {
    esp_camera_deinit();
    enterDeepSleep();
  }

  camera_fb_t *photo = capturePhoto();
  if (photo == nullptr) {
    Serial.println("ERROR: no s'ha pogut capturar la fotografia final.");
  } else {
    savePhoto(photo);
    esp_camera_fb_return(photo);
  }

  SD_MMC.end();
  esp_camera_deinit();
  Serial.printf("Temps despert: %lu ms.\n",
                static_cast<unsigned long>(millis() - startedAtMs));
  enterDeepSleep();
}

void loop() {
  // setup() sempre acaba entrant en deep sleep.
}

