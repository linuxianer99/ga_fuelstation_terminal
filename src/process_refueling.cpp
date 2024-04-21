#include <Arduino.h>
#include "main.h"
#include "config.h"
#include <ArduinoJson.h>
#include <Crypto.h>
#include "base64.hpp"

extern t_Config Config;

int ProcessRefueling(t_refueling Refueling, JsonDocument& doc){
    
    int retry_count=10;
    int httpResponseCode;

    #define KEY_LENGTH 16

    /* Define our key*/
    byte base64_key[24];
    byte key[KEY_LENGTH];
    Config.billingkey.getBytes(base64_key, 24);
    decode_base64(base64_key, key);

    char s_amount[6];
    unsigned char base64[32];
    snprintf(s_amount,6,"%.1f", Refueling.amount);
    char s_date[11];


    // Calculate HMAC for message
    SHA256HMAC hmac(key, KEY_LENGTH);
    /* Update the HMAC with just a plain string (null terminated) */
    hmac.doUpdate(Refueling.aircraft);
    hmac.doUpdate(Refueling.memberid);
    hmac.doUpdate(s_amount);
    hmac.doUpdate(Refueling.article);
    hmac.doUpdate(Refueling.date);
    byte authCode[SHA256HMAC_SIZE];
    hmac.doFinal(authCode);

    unsigned int base64_length = encode_base64(authCode, 32, base64);
    ESP_LOGD("Billing", "Base64: %s Length: %d", base64, base64_length);

    // Add data
    doc["aircraft"] = Refueling.aircraft;
    doc["article"] = Refueling.article;
    doc["amount"] = s_amount;
    doc["date"] = Refueling.date;
    doc["memberid"] = Refueling.memberid;
    doc["auth"] = base64;
#ifdef DEBUG
    String output;
    serializeJson(doc, output);
    ESP_LOGD("Billing", "Message: %s", output.c_str());
#endif
    return 1;
}