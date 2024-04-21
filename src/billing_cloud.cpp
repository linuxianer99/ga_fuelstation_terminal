#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "main.h"
#include "config.h"
#include <ArduinoJson.h>
#include <Crypto.h>
#include "ca.hpp"



extern t_Config Config;

int SendRefueling(JsonDocument &doc)
{   
    WiFiClientSecure *client = new WiFiClientSecure;
    if(client) {
        // set secure client with certificate
        client->setCACert(rootCACertificate);
        //create an HTTPClient instance
        HTTPClient https;
    
        int retry_count=10;
        int httpResponseCode;
        
        while (retry_count)
        {
            ESP_LOGD("Billing", "Try to connect billing server %s", (Config.billingserver + Config.terminal_id).c_str());
            // Handle retry counter
            retry_count--;
            // Try http POST
            https.begin(*client, Config.billingserver + "/" + Config.terminal_id);
            https.addHeader("Content-Type", "application/json");
            String data;
            serializeJson(doc, data);
            ESP_LOGD("Billing", "Message: %s", data.c_str());
            httpResponseCode = https.POST(data);
            ESP_LOGD("Billing", "Server Respone %d", httpResponseCode);
            https.end();
            client->stop();
            delete client;

            if (httpResponseCode == 200){
                // Transfer successful    
                return 1;
            }
            else if (httpResponseCode == 403)
            {
                return 403;
            }
        }
        // Transfer not successful!!
        ESP_LOGE("Billing", "Http Transfer not successful!");
        return(99);
    }
    return(98);
}


void checkConnection(t_terminalStatus *ts) 
{
    int httpResponseCode;
    String requestBody;
    JsonDocument doc;

    WiFiClientSecure *client = new WiFiClientSecure;
    
    if(client) {
        // set secure client with certificate
        client->setCACert(rootCACertificate);
        //create an HTTPClient instance
        HTTPClient https;
    
        // Add data
        doc["freeheap"] = ts->freeHeap;
        if (ts->rebooted == true) {
            doc["reboot_reason"] = ts->rebootReason;
            ts->rebooted = false;
        }
        
        doc["ip"] = ts->s_IP;
        doc["status"] = ts->status;
        doc["rssi"] = ts->wifi_rssi;
        ESP_LOGD("Billing", "Status: %d", ts->status);
        serializeJson(doc, requestBody);
        ESP_LOGD("Billing", "Message: %s", requestBody.c_str());
        https.begin(*client, Config.billingserver + "/" + Config.terminal_id + "/status");
        
        // Send HTTP GET request
        https.addHeader("Content-Type", "application/json");
        httpResponseCode = https.POST(requestBody);
        
        if (httpResponseCode>0) {
            ESP_LOGD("Billing", "Terminal connected");
            ts->connected = 1; 
        }
        else {
            ESP_LOGD("Billing", "Terminal NOT connected");
            ts->connected = 0;
        }
        
        https.end();
        client->stop();
        delete client;
    }
}

// void checkConnection(t_terminalStatus *ts) 
// {
//     WiFiClientSecure *client = new WiFiClientSecure;
//      if(client) {
//         // set secure client with certificate
//         client->setCACert(rootCACertificate);
//         //create an HTTPClient instance
//         HTTPClient https;
    
//         https.begin(*client, Config.billingserver + "/" + Config.terminal_id + "/ping");
//         // Send HTTP GET request
//         int httpResponseCode = https.GET();
//         log_i("Server Respone %d", httpResponseCode);
//         if (httpResponseCode>0) {
//             log_i("Terminal connected");
//             ts->connected = 1; 
//         }
//         else {
//             log_i("Terminal NOT connected");
//             ts->connected = 0;
//         }
//         https.end();
//         client->stop();
//         delete client;
//     }
// }