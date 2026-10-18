// Copyright (C) 2026 Modellbahn-Treff for Kids GmbH
// SPDX-License-Identifier: GPL-3.0-or-later

#include "settings.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"
#include "esp_log.h"
#include "esp_system.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "settings";
static const char *NVS_NS = "settings";

// ---------------------------------------------------------------------------
// Default values (compiled-in fallbacks)
// ---------------------------------------------------------------------------

uint8_t AbschNummer       = 1;
uint8_t VerteilerBaustein = 1;

char ssid[SETTINGS_STR_LEN]        = "SSID";
char password[SETTINGS_STR_LEN]    = "PASSWORD";
char mqtt_server[SETTINGS_STR_LEN] = "10.1.0.5";
char client_name[SETTINGS_STR_LEN] = "test_client";

uint8_t networkByte1 = 10;
uint8_t networkByte2 = 1;
uint8_t gatewayByte3 = 0;
uint8_t gatewayByte4 = 1;

int AusSchaltZeitWeiche = 200;

char MqttWMW[8][SETTINGS_TOPIC_LEN] = {
    "otter/WM/0", "otter/WM/1", "otter/WM/2", "otter/WM/3",
    "otter/WM/4", "otter/WM/5", "otter/WM/6", "otter/WM/7",
};

char MqttSet[SETTINGS_TOPIC_LEN] = "otter/Set";

// ---------------------------------------------------------------------------
// Helpers: serialize/deserialize a topic array to/from a JSON string
// ---------------------------------------------------------------------------

static void str_array_to_json_str(char arr[][SETTINGS_TOPIC_LEN], int n, char *out, size_t out_len) {
    cJSON *a = cJSON_CreateArray();
    for (int i = 0; i < n; i++) {
        cJSON_AddItemToArray(a, cJSON_CreateString(arr[i]));
    }
    char *s = cJSON_PrintUnformatted(a);
    snprintf(out, out_len, "%s", s);
    cJSON_free(s);
    cJSON_Delete(a);
}

static void json_str_to_str_array(const char *json_str, char arr[][SETTINGS_TOPIC_LEN], int n) {
    cJSON *a = cJSON_Parse(json_str);
    if (!a || !cJSON_IsArray(a)) { cJSON_Delete(a); return; }
    int cnt = cJSON_GetArraySize(a);
    for (int i = 0; i < n && i < cnt; i++) {
        cJSON *item = cJSON_GetArrayItem(a, i);
        if (cJSON_IsString(item)) {
            snprintf(arr[i], SETTINGS_TOPIC_LEN, "%s", item->valuestring);
        }
    }
    cJSON_Delete(a);
}

// ---------------------------------------------------------------------------
// NVS load — falls back to compiled defaults for any missing key
// ---------------------------------------------------------------------------

void settings_load_from_nvs(void) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "No saved settings, using defaults");
        return;
    }

    // uint8 scalars
    nvs_get_u8(h, "absch_nr", &AbschNummer);
    nvs_get_u8(h, "vb",       &VerteilerBaustein);
    nvs_get_u8(h, "net_b1",   &networkByte1);
    nvs_get_u8(h, "net_b2",   &networkByte2);
    nvs_get_u8(h, "gw_b3",    &gatewayByte3);
    nvs_get_u8(h, "gw_b4",    &gatewayByte4);

    // int32 scalar
    int32_t weiche = AusSchaltZeitWeiche;
    if (nvs_get_i32(h, "weiche_ms", &weiche) == ESP_OK) {
        AusSchaltZeitWeiche = (int)weiche;
    }

    // strings
    size_t len;
    len = SETTINGS_STR_LEN; nvs_get_str(h, "ssid",      ssid,        &len);
    len = SETTINGS_STR_LEN; nvs_get_str(h, "password",  password,    &len);
    len = SETTINGS_STR_LEN; nvs_get_str(h, "mqtt_srv",  mqtt_server, &len);
    len = SETTINGS_STR_LEN; nvs_get_str(h, "client_nm", client_name, &len);

    // topic array stored as a JSON string
    char big[1600];
    size_t blen;
    blen = sizeof(big); if (nvs_get_str(h, "mqtt_wmw", big, &blen) == ESP_OK) json_str_to_str_array(big, MqttWMW, 8);

    len = SETTINGS_TOPIC_LEN; nvs_get_str(h, "mqtt_set", MqttSet, &len);

    nvs_close(h);
    ESP_LOGI(TAG, "Settings loaded from NVS");
}

// ---------------------------------------------------------------------------
// NVS save — all non-WiFi settings
// ---------------------------------------------------------------------------

bool settings_save_to_nvs(void) {
    nvs_handle_t h;
    ESP_ERROR_CHECK(nvs_open(NVS_NS, NVS_READWRITE, &h));

    nvs_set_u8(h, "absch_nr", AbschNummer);
    nvs_set_u8(h, "vb",       VerteilerBaustein);
    nvs_set_u8(h, "net_b1",   networkByte1);
    nvs_set_u8(h, "net_b2",   networkByte2);
    nvs_set_u8(h, "gw_b3",    gatewayByte3);
    nvs_set_u8(h, "gw_b4",    gatewayByte4);
    nvs_set_i32(h, "weiche_ms", (int32_t)AusSchaltZeitWeiche);
    nvs_set_str(h, "mqtt_srv",  mqtt_server);
    nvs_set_str(h, "client_nm", client_name);

    char big[1600];
    str_array_to_json_str(MqttWMW, 8, big, sizeof(big)); nvs_set_str(h, "mqtt_wmw", big);
    nvs_set_str(h, "mqtt_set", MqttSet);

    esp_err_t err = nvs_commit(h);
    nvs_close(h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS commit failed: %s", esp_err_to_name(err));
        return false;
    }
    ESP_LOGI(TAG, "Settings saved to NVS");
    return true;
}

// ---------------------------------------------------------------------------
// Apply JSON blob to settings and persist — used by serial 'set' and MQTT
// ---------------------------------------------------------------------------

settings_result_t settings_apply_json(const char *json_str, char *tx_out, size_t tx_out_len) {
    if (tx_out && tx_out_len > 0) tx_out[0] = '\0';

    cJSON *root = cJSON_Parse(json_str);
    if (!root) return SETTINGS_ERR_JSON;

    cJSON *item;
    cJSON *arr;

    if (tx_out && (item = cJSON_GetObjectItem(root, "tx")) && cJSON_IsString(item))
        snprintf(tx_out, tx_out_len, "%s", item->valuestring);

    if ((item = cJSON_GetObjectItem(root, "AbschNummer"))         && cJSON_IsNumber(item)) AbschNummer         = (uint8_t)item->valueint;
    if ((item = cJSON_GetObjectItem(root, "VerteilerBaustein"))   && cJSON_IsNumber(item)) VerteilerBaustein   = (uint8_t)item->valueint;
    if ((item = cJSON_GetObjectItem(root, "networkByte1"))        && cJSON_IsNumber(item)) networkByte1        = (uint8_t)item->valueint;
    if ((item = cJSON_GetObjectItem(root, "networkByte2"))        && cJSON_IsNumber(item)) networkByte2        = (uint8_t)item->valueint;
    if ((item = cJSON_GetObjectItem(root, "gatewayByte3"))        && cJSON_IsNumber(item)) gatewayByte3        = (uint8_t)item->valueint;
    if ((item = cJSON_GetObjectItem(root, "gatewayByte4"))        && cJSON_IsNumber(item)) gatewayByte4        = (uint8_t)item->valueint;
    if ((item = cJSON_GetObjectItem(root, "AusSchaltZeitWeiche")) && cJSON_IsNumber(item)) AusSchaltZeitWeiche = item->valueint;
    if ((item = cJSON_GetObjectItem(root, "mqtt_server"))         && cJSON_IsString(item)) snprintf(mqtt_server, SETTINGS_STR_LEN,  "%s", item->valuestring);
    if ((item = cJSON_GetObjectItem(root, "client_name"))         && cJSON_IsString(item)) snprintf(client_name, SETTINGS_STR_LEN,  "%s", item->valuestring);
    if ((item = cJSON_GetObjectItem(root, "MqttSet"))             && cJSON_IsString(item)) snprintf(MqttSet,     SETTINGS_TOPIC_LEN, "%s", item->valuestring);

    if ((arr = cJSON_GetObjectItem(root, "MqttWMW")) && cJSON_IsArray(arr)) {
        int n = cJSON_GetArraySize(arr);
        for (int i = 0; i < 8 && i < n; i++) {
            cJSON *el = cJSON_GetArrayItem(arr, i);
            if (cJSON_IsString(el)) snprintf(MqttWMW[i], SETTINGS_TOPIC_LEN, "%s", el->valuestring);
        }
    }

    bool do_reboot = (item = cJSON_GetObjectItem(root, "reboot")) && cJSON_IsTrue(item);

    cJSON_Delete(root);

    if (!settings_save_to_nvs()) return SETTINGS_ERR_NVS;

    return do_reboot ? SETTINGS_OK_REBOOT : SETTINGS_OK;
}

// ---------------------------------------------------------------------------
// NVS save — WiFi credentials only
// ---------------------------------------------------------------------------

void settings_save_wifi_to_nvs(void) {
    nvs_handle_t h;
    ESP_ERROR_CHECK(nvs_open(NVS_NS, NVS_READWRITE, &h));
    nvs_set_str(h, "ssid",     ssid);
    nvs_set_str(h, "password", password);
    nvs_commit(h);
    nvs_close(h);
    ESP_LOGI(TAG, "WiFi credentials saved to NVS");
}
