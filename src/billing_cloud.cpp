#include <HttpClient.h>
#include <WiFiClientSecure.h>
#include "main.h"
#include "config.h"
#include <ArduinoJson.h>
#include <Crypto.h>
#include "base64.hpp"
#include "ca.hpp"



extern t_Config Config;

int SendRefueling(t_refueling Refueling)
{   
    WiFiClientSecure *client = new WiFiClientSecure;
    if(client) {
        // set secure client with certificate
        client->setCACert(rootCACertificate);
        //create an HTTPClient instance
        HTTPClient https;
    

        String requestBody;
        StaticJsonDocument<200> doc;
        int retry_count=10;
        int httpResponseCode;

        #define KEY_LENGTH 16

        /* Define our key*/
        byte base64_key[24];
        byte key[KEY_LENGTH];
        Config.billingkey.getBytes(base64_key,24);
        decode_base64(base64_key, key);

        char s_amount[6];
        unsigned char base64[32];
        snprintf(s_amount,6,"%.2f", Refueling.amount);


        // Calculate HMAC for message
        SHA256HMAC hmac(key, KEY_LENGTH);
        /* Update the HMAC with just a plain string (null terminated) */
        hmac.doUpdate(Refueling.aircraft);
        hmac.doUpdate(Refueling.memberid);
        hmac.doUpdate(s_amount);
        hmac.doUpdate(Refueling.article);
        byte authCode[SHA256HMAC_SIZE];
        hmac.doFinal(authCode);

        unsigned int base64_length = encode_base64(authCode, 32, base64);
        log_i("Base64: %s Length: %d", base64, base64_length);

        // Add data
        doc["aircraft"] = Refueling.aircraft;
        doc["amount"] = s_amount;
        doc["article"] = Refueling.article;
        doc["memberid"] = Refueling.memberid;
        doc["auth"] = base64;
        serializeJson(doc, requestBody);
        log_i("Message: %s", requestBody);
        
        while (retry_count)
        {
            log_i("Try to connect billing server %s", Config.billingserver.c_str() + Config.terminal_id);
            // Handle retry counter
            retry_count--;
            // Try http POST
            https.begin(*client, Config.billingserver + "/" + Config.terminal_id);
            https.addHeader("Content-Type", "application/json");
            httpResponseCode = https.POST(requestBody);
            log_i("Server Respone %d", httpResponseCode);
            https.end();

            if (httpResponseCode == 200){
                // Transfer successful    
                return httpResponseCode;
            }

        }
        // Transfer not successful!!
        log_e("Http Transfer not successful!");
        return(httpResponseCode);
    }
}

void checkConnection(t_terminalStatus *ts) 
{
    WiFiClientSecure *client = new WiFiClientSecure;
     if(client) {
        // set secure client with certificate
        client->setCACert(rootCACertificate);
        //create an HTTPClient instance
        HTTPClient https;
    
        https.begin(*client, Config.billingserver + "/" + Config.terminal_id + "/ping");
        // Send HTTP GET request
        int httpResponseCode = https.GET();
        log_i("Server Respone %d", httpResponseCode);
        if (httpResponseCode>0) {
            log_i("Terminal connected");
            ts->connected = 1; 
        }
        else {
            log_i("Terminal NOT connected");
            ts->connected = 0;
        }
        https.end();
    }
}