#ifndef WIFI_AP_H
#define WIFI_AP_H

// Starts the ESP32 WiFi in Access Point (SoftAP) mode so a phone/laptop
// can connect directly to it (no home/lab router required).
//
// SSID / password / IP are defined in wifi_ap.cpp.
void wifi_ap_start(void);

#endif
