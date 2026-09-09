#include "wifi_ap.h"

#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

// ---- Change these if you want a different network name/password ----
#define AP_SSID       "MotorDriver-GUI"
#define AP_PASSWORD   "motor1234"   // must be 8+ chars for WPA2
#define AP_CHANNEL    1
#define AP_MAX_CONN   4
// ----------------------------------------------------------------------

static const char *TAG = "wifi_ap";

void wifi_ap_start(void)
{
    // NVS is required by the WiFi driver to store calibration data.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config{};
    strncpy((char *)wifi_config.ap.ssid, AP_SSID, sizeof(wifi_config.ap.ssid));
    wifi_config.ap.ssid_len = strlen(AP_SSID);
    wifi_config.ap.channel = AP_CHANNEL;
    strncpy((char *)wifi_config.ap.password, AP_PASSWORD, sizeof(wifi_config.ap.password));
    wifi_config.ap.max_connection = AP_MAX_CONN;
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.ap.pmf_cfg.required = false;

    if (strlen(AP_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "SoftAP started. SSID:%s password:%s", AP_SSID, AP_PASSWORD);
    ESP_LOGI(TAG, "Connect to it, then browse to http://192.168.4.1");
}
