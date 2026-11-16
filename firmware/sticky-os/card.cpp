#include "card.h"

#include <Arduino.h>

#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#include "pins.h"

static sdmmc_card_t *card;
static unsigned generation;

bool cardIn() { return digitalRead(PIN_SD_DETECT) == LOW; }
unsigned cardGeneration() { return generation; }

bool cardMount() {
  pinMode(PIN_SD_DETECT, INPUT_PULLUP);
  if (!cardIn()) {
    if (card) {  // taken out: let go of it, so the next one mounts cleanly
      esp_vfs_fat_sdcard_unmount(CARD_ROOT, card);
      card = nullptr;
      Serial.println("card: taken out");
    }
    return false;
  }
  if (card) return true;
  // The screen already set up SPI2; the card is one more device on it.
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = SPI2_HOST;
  sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot.gpio_cs = (gpio_num_t)PIN_SD_CS;
  slot.host_id = SPI2_HOST;
  esp_vfs_fat_sdmmc_mount_config_t mount = {};
  mount.max_files = 8;  // Books: the book, its text, contents and cover while it's first opened, and the library's own
  esp_err_t err = esp_vfs_fat_sdspi_mount(CARD_ROOT, &host, &slot, &mount, &card);
  if (err != ESP_OK) {
    card = nullptr;
    Serial.printf("card: mount failed: %s\n", esp_err_to_name(err));
    return false;
  }
  generation++;
  Serial.printf("card: mounted, %llu MB\n", (unsigned long long)card->csd.capacity * card->csd.sector_size / (1024 * 1024));
  return true;
}
