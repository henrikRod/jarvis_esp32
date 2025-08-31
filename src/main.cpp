#include <Arduino.h>
#include <WiFi.h>
#include "driver/i2s.h"
#include "secrets.h"
#include <math.h>

// Pinagem I2S (como combinamos)
#define PIN_I2S_BCLK   26   // BCLK para mic e amp
#define PIN_I2S_LRCLK  25   // WS/LRCLK para mic e amp
#define PIN_I2S_DOUT   27   // DIN -> MAX98357A (TX)
#define PIN_I2S_DIN    22   // SD  -> INMP441 (RX)

// ====== UTIL ======
void waitSeconds(float s) { vTaskDelay((TickType_t)(s * 1000) / portTICK_PERIOD_MS); }

// ====== REDE ======
void wifiConnect() {
  Serial.println("\n[WiFi] Conectando...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    Serial.print(".");
    delay(300);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] Conectado! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[WiFi] Falha ao conectar (ok para o teste de áudio, só avisei).");
  }
}

// ====== I2S - PLAY (AMP) ======
bool i2sBeginTX(uint32_t sampleRateHz = 44100) {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = (int)sampleRateHz,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // mono
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };
  if (i2s_driver_install(I2S_NUM_0, &cfg, 0, nullptr) != ESP_OK) return false;

  i2s_pin_config_t pins = {
    .bck_io_num = PIN_I2S_BCLK,
    .ws_io_num = PIN_I2S_LRCLK,
    .data_out_num = PIN_I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  if (i2s_set_pin(I2S_NUM_0, &pins) != ESP_OK) return false;
  i2s_zero_dma_buffer(I2S_NUM_0);
  return true;
}

void playSine(float freq = 1000.0f, float seconds = 2.0f, uint32_t sr = 44100) {
  const float twoPiF = 2.0f * PI * freq;
  const size_t chunk = 256;
  int16_t buf[chunk];
  size_t written = 0;
  uint32_t total = (uint32_t)(seconds * sr);
  for (uint32_t n = 0; n < total; n += chunk) {
    for (size_t i = 0; i < chunk; i++) {
      float t = (float)(n + i) / (float)sr;
      // 0.5 para evitar clip; ajuste se ficar baixo/alto
      float s = 0.5f * sinf(twoPiF * t);
      buf[i] = (int16_t)(s * 32767.0f);
    }
    i2s_write(I2S_NUM_0, buf, chunk * sizeof(int16_t), &written, portMAX_DELAY);
  }
  i2s_driver_uninstall(I2S_NUM_0);
}

// ====== I2S - CAPTURE (MIC) ======
bool i2sBeginRX(uint32_t sampleRateHz = 16000) {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = (int)sampleRateHz,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // lê os dois canais
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  if (i2s_driver_install(I2S_NUM_0, &cfg, 0, nullptr) != ESP_OK) return false;

  i2s_pin_config_t pins = {
    .bck_io_num = PIN_I2S_BCLK,
    .ws_io_num = PIN_I2S_LRCLK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = PIN_I2S_DIN
  };
  if (i2s_set_pin(I2S_NUM_0, &pins) != ESP_OK) return false;
  i2s_zero_dma_buffer(I2S_NUM_0);
  return true;
}

void micLevelLoop(uint32_t sr = 16000) {
  const size_t chunk = 256;
  int32_t buf[chunk];
  size_t readBytes = 0;

  if (i2s_read(I2S_NUM_0, buf, chunk * sizeof(int32_t), &readBytes, portMAX_DELAY) != ESP_OK) {
    Serial.println("[MIC] Falha ao ler I2S");
    return;
  }
  size_t n = readBytes / sizeof(int32_t);
  if (n < 2) return;

  long long accL = 0, accR = 0;
  size_t cntL = 0, cntR = 0;

  for (size_t i = 0; i + 1 < n; i += 2) {
    int32_t r32 = buf[i]   >> 14; // RIGHT
    int32_t l32 = buf[i+1] >> 14; // LEFT
    int16_t r16 = (int16_t)max(-32768, min(32767, r32));
    int16_t l16 = (int16_t)max(-32768, min(32767, l32));
    accR += (long long)r16 * r16; cntR++;
    accL += (long long)l16 * l16; cntL++;
  }

  float rmsL = (cntL > 0) ? sqrt((double)accL / (double)cntL) / 32768.0f : 0.0f;
  float rmsR = (cntR > 0) ? sqrt((double)accR / (double)cntR) / 32768.0f : 0.0f;

  float dBL = (rmsL > 0.000001f) ? 20.0f * log10f(rmsL) : -120.0f;
  float dBR = (rmsR > 0.000001f) ? 20.0f * log10f(rmsR) : -120.0f;

  Serial.printf("[MIC] L: RMS=%.3f (%.1f dBFS) | R: RMS=%.3f (%.1f dBFS)\n", rmsL, dBL, rmsR, dBR);
}

// ====== FLUXO PRINCIPAL ======
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Autoteste Jarvis (WiFi / AMP / MIC-contínuo) ===");

  // 1) Wi-Fi
  wifiConnect();

  // 2) AMP: toca seno 1 kHz
  Serial.println("\n[AMP] Inicializando I2S TX e tocando 1 kHz por 2 s...");
  if (!i2sBeginTX(44100)) {
    Serial.println("[AMP] ERRO ao iniciar I2S TX. Cheque pinos VIN/GND/DIN/BCLK/LRC.");
  } else {
    playSine(1000.0f, 2.0f, 44100);
    Serial.println("[AMP] OK (som deve ter sido ouvido).");
  }

  // 3) MIC: inicializa RX (vai rodar no loop)
  Serial.println("\n[MIC] Inicializando I2S RX (RIGHT_LEFT)...");
  if (!i2sBeginRX(16000)) {
    Serial.println("[MIC] ERRO ao iniciar I2S RX. Cheque VDD(3V3)/GND/BCLK/LRCL/SD.");
  }
}

void loop() {
  micLevelLoop(16000); // fica rodando continuamente
  delay(200);          // ajuste a taxa de print
}