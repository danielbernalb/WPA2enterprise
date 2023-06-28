/* WiFi Connection Example using WPA2 Enterprise
 *
 * Original Copyright (C) 2006-2016, ARM Limited, All Rights Reserved, Apache 2.0 License.
 * Additions Copyright (C) Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD, Apache 2.0 License.
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/uart.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_event_loop.h"

/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"

   You can choose EAP method via 'make menuconfig' according to the
   configuration of AP.
*/
#define EXAMPLE_EAP_METHOD EAP_PEAP

//#define EXAMPLE_WIFI_SSID "SED CORPORATIVO"
//#define EXAMPLE_EAP_ID "wiedctabora"
//#define EXAMPLE_EAP_USERNAME "wiedctabora"
//#define EXAMPLE_EAP_PASSWORD "OZ4\"Xbk0"

#define EXAMPLE_WIFI_SSID "TPwpa2"
#define EXAMPLE_EAP_ID "prueba1"
#define EXAMPLE_EAP_USERNAME "prueba1"
#define EXAMPLE_EAP_PASSWORD "daniel2022"

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
#define WIFI_CONNECTED_BIT BIT0

/* Constants that aren't configurable in menuconfig */
#define EAP_PEAP 1
#define EAP_TTLS 2

static const char* TAG = "example";

static void event_handler(void* arg, esp_event_base_t event_base, 
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        system_event_sta_disconnected_t *event = (system_event_sta_disconnected_t *)event_data;

        ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");

        if (event->reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT) {
            /*Switch to 802.11 bgn mode */
            esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
            ESP_LOGI(TAG, "WIFI_PROTOCOL_11_CHANGE");
        }
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
    }
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
        },
    };
    ESP_LOGI(TAG, "SSID: %s", EXAMPLE_WIFI_SSID);
    ESP_LOGI(TAG, "Identity: %s", EXAMPLE_EAP_ID);
    ESP_LOGI(TAG, "Password: %s", EXAMPLE_EAP_PASSWORD);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    esp_wifi_sta_wpa2_ent_clear_identity();
    esp_wifi_sta_wpa2_ent_clear_username();
    esp_wifi_sta_wpa2_ent_clear_password();
    esp_wifi_sta_wpa2_ent_clear_new_password();
    esp_wifi_sta_wpa2_ent_clear_ca_cert();
    esp_wifi_sta_wpa2_ent_clear_cert_key();
    esp_wifi_sta_wpa2_ent_set_disable_time_check(false);

    ESP_ERROR_CHECK(esp_wifi_sta_wpa2_ent_set_identity((uint8_t*)EXAMPLE_EAP_ID, strlen(EXAMPLE_EAP_ID)));

    if (EXAMPLE_EAP_METHOD == EAP_PEAP || EXAMPLE_EAP_METHOD == EAP_TTLS) {
        ESP_ERROR_CHECK(esp_wifi_sta_wpa2_ent_set_username((uint8_t*)EXAMPLE_EAP_USERNAME, strlen(EXAMPLE_EAP_USERNAME)));
        ESP_ERROR_CHECK(esp_wifi_sta_wpa2_ent_set_password((uint8_t*)EXAMPLE_EAP_PASSWORD, strlen(EXAMPLE_EAP_PASSWORD)));
    }
    ESP_ERROR_CHECK(esp_wifi_sta_wpa2_ent_enable());
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Wifi start");
    ESP_LOGI(TAG, "wifi_init_sta finished.");

}

static void wpa2_enterprise_example_task(void* pvParameters)
{
    tcpip_adapter_ip_info_t ip;
    memset(&ip, 0, sizeof(tcpip_adapter_ip_info_t));
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    while (1) {
        vTaskDelay(2000 / portTICK_PERIOD_MS);

        if (tcpip_adapter_get_ip_info(ESP_IF_WIFI_STA, &ip) == 0) {
            ESP_LOGI(TAG, "~~~~~~~~~~~");
            ESP_LOGI(TAG, "IP:"IPSTR, IP2STR(&ip.ip));
            ESP_LOGI(TAG, "MASK:"IPSTR, IP2STR(&ip.netmask));
            ESP_LOGI(TAG, "GW:"IPSTR, IP2STR(&ip.gw));
            ESP_LOGI(TAG, "~~~~~~~~~~~");
		//	Serial.println("task");
        ESP_LOGI(TAG, "task");
        }

        ESP_LOGI(TAG, "free heap:%d\n", esp_get_free_heap_size());
    }
}

void app_main()
{
     // Configure parameters of an UART driver,
    // communication pins and install the driver
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config);

    // Install UART driver, and get the queue.
    uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 0, 0, 0, NULL, 0);
    ESP_ERROR_CHECK(nvs_flash_init());
    initialise_wifi();
    xTaskCreate(&wpa2_enterprise_example_task, "wpa2_enterprise_example_task", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "main");
}
