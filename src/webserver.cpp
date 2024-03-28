#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"
#include "filesystem.h"
#include "main.h"
#include "version.h"

extern t_Config Config;
extern bool shouldReboot;
extern t_terminalStates TerminalState;
extern t_terminalStatus TerminalStatus;

void notFound(AsyncWebServerRequest *request) {
  ESP_LOGE("Web", "Page not found");
  request->send(404, "text/plain", "Not found");
}

// used by server.on functions to discern whether a user has the correct httpapitoken OR is authenticated by username and password
bool checkUserWebAuth(AsyncWebServerRequest * request) {
    bool isAuthenticated = false;

    if (request->authenticate(Config.httpuser.c_str(), Config.httppassword.c_str())) {
        ESP_LOGD("Web", "is authenticated via username and password");
        isAuthenticated = true;
    }
    return isAuthenticated;
}


// handles uploads to the filserver
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    // make sure authenticated before allowing upload
    if (!request->authenticate(Config.httpuser.c_str(), Config.httppassword.c_str())) {
        return request->requestAuthentication();
    }

    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    ESP_LOGD("Web", "%s", logmessage.c_str());

    if (!index) {
        logmessage = "Upload Start: " + String(filename);
        // open the file on first call and store the file handle in the request object
        request->_tempFile = LittleFS.open("/" + filename, "w");
        ESP_LOGD("Web", "%s", logmessage.c_str());
    }

    if (len) {
        // stream the incoming chunk to the opened file
        request->_tempFile.write(data, len);
        logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
        ESP_LOGD("Web", "%s", logmessage.c_str());
    }

    if (final) {
        logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
        // close the file handle as the upload is now done
        request->_tempFile.close();
        ESP_LOGD("Web", "%s", logmessage.c_str());
        request->redirect("/");
    }
}


// parses and processes index.html
String processor(const String& var) {

    if (var == "EEH_HOSTNAME") {
        return Config.terminal_id;
    }

    if (var == "WEBPAGEDELAY") {
        return String(Config.webpagedelay * 1000);
    }

    if (var == "TERMINALSTATUS") {
        return String(TerminalState);
    }

   if (var == "FIRMWARE") {
     return VERSION;
   }

//   if (var == "FREESPIFFS") {
//     return humanReadableSize((SPIFFS.totalBytes() - SPIFFS.usedBytes()));
//   }

//   if (var == "USEDSPIFFS") {
//     return humanReadableSize(SPIFFS.usedBytes());
//   }

//   if (var == "TOTALSPIFFS") {
//     return humanReadableSize(SPIFFS.totalBytes());
//   }

  return String();
}


void configureWebServer(AsyncWebServer *server) {
    // configure web server
    ESP_LOGD("Web", "Setup Webserver");
    // if url isn't found
    server->onNotFound(notFound);

    server->on("/health", HTTP_GET, [](AsyncWebServerRequest * request) {
        String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
        ESP_LOGD("Web", "%s", logmessage);
        request->send(200, "text/plain", "OK");
    });

    // https://randomnerdtutorials.com/esp32-esp8266-web-server-http-authentication/
    // Route for root / web page
    server->on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
        if (!request->authenticate(Config.httpuser.c_str(), Config.httppassword.c_str())) {
            return request->requestAuthentication();
        }

    /*
      int headers = request->headers();
      int i;
      for(i=0;i<headers;i++){
      AsyncWebHeader* h = request->getHeader(i);
      Serial.printf("HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
      }
    */
        request->send(LittleFS, "/index.html", String(), false, processor);

    });

    server->on("/reboot", HTTP_GET, [](AsyncWebServerRequest * request) {
        String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();

        if (checkUserWebAuth(request)) {
            request->send(LittleFS, "/reboot.html");
            logmessage += " Auth: Success";
            ESP_LOGD("Web", "%s", logmessage);
            shouldReboot = true;
        }
        else {
            logmessage += " Auth: Failed";
            ESP_LOGD("Web", "%s", logmessage);
            return request->requestAuthentication();
        }

    });

    server->on("/logout", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(401);
    });

    server->on("/logged-out", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(LittleFS, "/logout.html", String(), false, processor);
    });

    server->on("/file", HTTP_GET, [](AsyncWebServerRequest * request) {
        String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
        if (checkUserWebAuth(request)) {
            logmessage += " Auth: Success";
            ESP_LOGD("Web", "%s", logmessage.c_str());

            //printWebAdminArgs(request);

            if (request->hasParam("name") && request->hasParam("action")) {
                const char *fileName = request->getParam("name")->value().c_str();
                const char *fileAction = request->getParam("action")->value().c_str();

                logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url() + "?name=" + String(fileName) + "&action=" + String(fileAction);

                if (!LittleFS.exists(fileName)) {
                    ESP_LOGE("Web", "%s ERROR: file does not exist", logmessage.c_str());
                    request->send(400, "text/plain", "ERROR: file does not exist");
                }
                else {
                    ESP_LOGD("Web", "%s file exists", logmessage.c_str());
                    if (strcmp(fileAction, "download") == 0) {
                        logmessage += " downloaded";
                        request->send(LittleFS, fileName, "application/octet-stream");
                    }
                    else if (strcmp(fileAction, "delete") == 0) {
                        logmessage += " deleted";
                        LittleFS.remove(fileName);
                        request->send(200, "text/plain", "Deleted File: " + String(fileName));
                    }
                    else {
                        logmessage += " ERROR: invalid action param supplied";
                        request->send(400, "text/plain", "ERROR: invalid action param supplied");
                    }
                    ESP_LOGD("Web", "%s", logmessage.c_str());
                }
            }
            else {
                request->send(400, "text/plain", "ERROR: name and action params required");
            }
        }
        else {
            logmessage += " Auth: Failed";
            ESP_LOGE("Web", "%s", logmessage.c_str());
            return request->requestAuthentication();
        }
    });

    // run handleUpload function when any file is uploaded
    server->onFileUpload(handleUpload);

    server->on("/listfiles", HTTP_GET, [](AsyncWebServerRequest * request)
    {
        String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
        if (checkUserWebAuth(request)) {
            logmessage += " Auth: Success";
            ESP_LOGD("Web", "%s", logmessage.c_str());
            request->send(200, "text/plain", listFiles(true));
        }
        else {
            logmessage += " Auth: Failed";
            ESP_LOGD("Web", "%s", logmessage.c_str());
            return request->requestAuthentication();
        }
    });

    server->on("/status", HTTP_GET, [](AsyncWebServerRequest * request)
    {
        StaticJsonDocument<200> doc;
        String requestBody;

         // Add data
        doc["b"] = TerminalStatus.connected;
        doc["t"] = TerminalStatus.status;
        doc["c"] = TerminalStatus.cachedRefuelings;
        serializeJson(doc, requestBody);
        request->send(200, "application/json", requestBody);
    });

    ESP_LOGI("Web", "Configuring OTA Webserver ...");
    AsyncElegantOTA.begin(server, Config.httpuser.c_str(), Config.httppassword.c_str());
    ESP_LOGI("Web", "Starting Webserver ...");
    server->begin();
}