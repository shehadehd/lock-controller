#ifndef COMMUNICATION_H
#define COMMUNICATION_H
 
#include <inttypes.h>
#include "common.h"
 
/* ESPNOW can work in both station and softap mode. It is configured in menuconfig. */
#if CONFIG_ESPNOW_WIFI_MODE_STATION
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_AP
#endif
 
#define ESPNOW_QUEUE_SIZE           6
 
#define IS_BROADCAST_ADDR(ref, addr) (memcmp(addr, ref, ESP_NOW_ETH_ALEN) == 0)
 
 
 
void network_init(void);
void network_send(uint8_t *pBuffer, uint8_t uLength, eMessageId MessageId);
 
#endif