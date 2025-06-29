 ///////// INCLUDES /////////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include "communication.h"
#include "common.h"
#include "nvs_flash.h"
 
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_crc.h"
 
///////// POST-INCLUDE PRE-PROCESSOR DIRECTIVES ////////////////////////////////////////////////////





///////// MNEMONICS ////////////////////////////////////////////////////////////////////////////////


///////// DEFINES //////////////////////////////////////////////////////////////////////////////

#define ESPNOW_MAXDELAY 512
 
///////// ENUMERATIONS /////////////////////////////////////////////////////////////////////////

///////// FORWARD STRUCT DECLARATIONS //////////////////////////////////////////////////////////////

///////// TYPEDEFS /////////////////////////////////////////////////////////////////////////////////

///////// MACROS ///////////////////////////////////////////////////////////////////////////////////

///////// FUNCTION PROTOTYPES //////////////////////////////////////////////////////////////////////

///////// STRUCT DECLARATIONS //////////////////////////////////////////////////////////////////////

///////// LOCAL CONSTANTS //////////////////////////////////////////////////////////////////////////


///////// EXTERNAL CONSTANTS ///////////////////////////////////////////////////////////////////////


///////// LOCAL VARIABLES ////////////////////////////////////////////////////////////////////////// 
 
static QueueHandle_t send_queue = NULL;
static QueueHandle_t receive_queue = NULL;
 
static uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static uint16_t sequenceIndex = 0;
static bool bTransmitBusy = false;
 
///////// EXTERNAL VARIABLES ///////////////////////////////////////////////////////////////////////

extern suSystemData systemData;

///////// LOCAL FUNCTIONS ////////////////////////////////////////////////////////////////////////// 

/* Prepare data to be sent. */
void network_send(eMessageId MessageId, uint8_t *pBuffer, uint8_t uLength)
{
    suNetworkPacket NetworkPacket;
 
    NetworkPacket.Header.MessageId = MessageId;
    NetworkPacket.Header.uLength = uLength;
    if (uLength > 0)
    {
        memcpy(NetworkPacket.payload, pBuffer, uLength);
    }
    NetworkPacket.Header.seq_num = sequenceIndex++;
    NetworkPacket.Header.crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)NetworkPacket.payload, uLength);
 
    xQueueSend(send_queue, &NetworkPacket, ESPNOW_MAXDELAY);
}
 
static void network_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if(status == ESP_NOW_SEND_SUCCESS)
    {
    }
    else
    {
        printf("Send FAILED\n");
    }
 
    bTransmitBusy = false;
}
 
/* Parse received data. */
void data_parse(suNetworkPacket *pNetworkPacket)
{
    uint16_t crc, crc_cal = 0;
 
    crc = pNetworkPacket->Header.crc;
    pNetworkPacket->Header.crc = 0;
    crc_cal = esp_crc16_le(UINT16_MAX, (uint8_t const *)pNetworkPacket->payload, pNetworkPacket->Header.uLength);
 
    if (crc_cal == crc)
    {
        xQueueSend(receive_queue, pNetworkPacket, ESPNOW_MAXDELAY);
    }
    else
    {
        printf("Received BAD data\n");
    }
}
 
static void network_receive_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    suNetworkReceiveData recv_cb;
    uint8_t * mac_addr = recv_info->src_addr;
    uint8_t * des_addr = recv_info->des_addr;
 
    if (mac_addr == NULL || data == NULL || len <= 0) {
        return;
    }
 
    if (IS_BROADCAST_ADDR(broadcast_mac, des_addr)) {
        /* If added a peer with encryption before, the receive packets may be
         * encrypted as peer-to-peer message or unencrypted over the broadcast channel.
         * Users can check the destination address to distinguish it.
         */
    }
 
    recv_cb.uLength = len;
    memcpy(recv_cb.mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    memcpy(&recv_cb.NetworkPacket, data, len);
    data_parse(&recv_cb.NetworkPacket);
}
 
void run_network_tx(void *pvParameter)
{    
    suNetworkPacket SendPacket;
 
    do {
        while(bTransmitBusy)
        {
            vTaskDelay(100);
        }
 
        if (xQueueReceive(send_queue, &SendPacket, portMAX_DELAY) == pdTRUE) {
                if (esp_now_send(broadcast_mac, (uint8_t*)&SendPacket, (sizeof(SendPacket.Header) + SendPacket.Header.uLength)) != ESP_OK) {
                    printf ("Failed to send data\n");
                }
        }
    } while (true);
}
 
void run_network_rx(void *pvParameter)
{
    suNetworkPacket NetworkPacket;
 
    while (xQueueReceive(receive_queue, &NetworkPacket, portMAX_DELAY) == pdTRUE) {
        printf("Received: ");
        switch(NetworkPacket.Header.MessageId)
        {
            case HELLO:
            {
                printf("HELLO\n");
            }
            break;
 
            case SYSTEM_REPORT:
            {
                // printf("SYSTEM_REPORT\n");
                suSystemData SystemData = *(suSystemData*)NetworkPacket.payload;
                printf("bDockingRequested: %s\n", SystemData.bDockingRequested ? "TRUE" : "FALSE");
            }
            break;
 
            case STATE_REPORT:
            {
                // printf("STATE_REPORT\n");
                suStateReport *pStateReport = (suStateReport*)NetworkPacket.payload;
                printf("Endpoint: %s, State: %s\n", pStateReport->sEndpointName, pStateReport->sStateName);
            }
            break;
 
            case ERROR_REPORT:
            {
                 printf("ERROR_REPORT\n");
            }
            break;
 
            case DOCK_COMMAND:
            {
                 printf("DOCK_COMMAND\n");
                 bool bDockingRequested = *(bool*)NetworkPacket.payload;
                 systemData.bDockingRequested = bDockingRequested;
                printf("bDockingRequested: %s\n", bDockingRequested ? "TRUE" : "FALSE");
            }
            break;
 
            default:
                printf("UNKNOWN\n");
            break;
        }
    }
}

///////// EXTERNAL FUNCTIONS ///////////////////////////////////////////////////////////////////////

void network_init(void)
{
        // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
 
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(ESPNOW_WIFI_MODE) );
    ESP_ERROR_CHECK( esp_wifi_start());
    ESP_ERROR_CHECK( esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
    /* Initialize ESPNOW and register sending and receiving callback function. */
 
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(network_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(network_receive_cb) );
 
        /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t peer = {0};
 
    peer.channel = CONFIG_ESPNOW_CHANNEL;
    peer.ifidx = ESPNOW_WIFI_IF;
    peer.encrypt = false;
    memcpy(peer.peer_addr, broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(&peer) );
 
    send_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(suNetworkPacket));
    if (send_queue == NULL) {
        return;
    }
 
    receive_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(suNetworkPacket));
    if (receive_queue == NULL) {
        return;
    }
 
    xTaskCreate(run_network_tx, "run_network_tx", 2048, NULL, 4, NULL);
    xTaskCreate(run_network_rx, "run_network_rx", 2048, NULL, 4, NULL);
}

///////// POST-CODE COMPILER DIRECTIVES ////////////////////////////////////////////////////////////
