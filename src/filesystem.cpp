#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

#include <ArduinoJson.h>
#include "defaults.h"
#include "config.h"


// Make size of files human readable
// source: https://github.com/CelliesProjects/minimalUploadAuthESP32
String humanReadableSize(const size_t bytes) {
  if (bytes < 1024) return String(bytes) + " B";
  else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " KB";
  else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
  else return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}


void init_FS()
{
    
    // Try to mount FS
    if(!LittleFS.begin(true))
    {
        ESP_LOGE("FS", "An Error has occurred while mounting SPIFFS");
        //rebootESP("ERROR: Cannot mount SPIFFS, Rebooting");
        return;
    }

    ESP_LOGI("FS", "FS Free: %s", humanReadableSize(LittleFS.totalBytes() - LittleFS.usedBytes()));
    ESP_LOGI("FS", "FS Used: %s", humanReadableSize(LittleFS.usedBytes()));
    ESP_LOGI("FS", "FS Total: %s", humanReadableSize(LittleFS.totalBytes()));

}


void saveConfiguration(const char *filename, const t_Config &config) {
  
  ESP_LOGI("FS", "Save configfile %s", filename);

  // Delete existing file, otherwise the configuration is appended to the file
  LittleFS.remove(filename);

  // Open file for writing
  File file = LittleFS.open(filename, FILE_WRITE);
  if (!file) {
    ESP_LOGE("FS", "Failed to create file");
    return;
  }

  StaticJsonDocument<2000> doc;

  // Set the values in the document
  doc["terminal_id"] = config.terminal_id;
  doc["device"] = config.device;
  doc["ssid"] = config.ssid;
  doc["wifipassword"] = config.wifipassword;
  doc["httpuser"] = config.httpuser;
  doc["httppassword"] = config.httppassword;
  doc["httpapitoken"] = config.httpapitoken;
  doc["fuelsort"] = config.fuelsort;
  doc["billingserver"] = config.billingserver;
  doc["billingkey"] = config.billingkey;
  doc["heartbeat"] = config.heartbeat;
  doc["webpagedelay"] = config.webpagedelay;
  doc["buzzerintensity"] = config.buzzerintensity;
  doc["calibration"] = config.calibration;
  doc["pump_timeout"] = config.pump_timeout;
  

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    ESP_LOGE("FS", "Failed to write to file");
  }

  // need to print out the deserialisation to discern size

  // Close the file
  file.close();
}

// list all of the files, if ishtml=true, return html rather than simple text
String listFiles(bool ishtml) {
    String returnText = "";
    ESP_LOGD("FS", "Listing files stored on SPIFFS");
    File root = LittleFS.open("/");
    File foundfile = root.openNextFile();
    if (ishtml) {
        returnText += "<table><tr><th align='left'>Name</th><th align='left'>Size</th><th></th><th></th></tr>";
    }
    while (foundfile) {
        if (ishtml) {
        returnText += "<tr align='left'><td>" + String(foundfile.name()) + "</td><td>" + humanReadableSize(foundfile.size()) + "</td>";
        //"<td><a href='/file?name=" + String(foundfile.name()) + "&action=download'>Download</a></td><td><a href='/file?name=" + String(foundfile.name()) + "&action=delete'>Delete</a></td>";
        returnText += "<td><button onclick=\"downloadDeleteButton(\'/" + String(foundfile.name()) + "\', \'download\')\">Download</button>";
        returnText += "<td><button onclick=\"downloadDeleteButton(\'/" + String(foundfile.name()) + "\', \'delete\')\">Delete</button></tr>";
        } else {
            returnText += "File: " + String(foundfile.name()) + "\n";
        }
            foundfile = root.openNextFile();
    }
    if (ishtml) {
        returnText += "</table>";
    }
    root.close();
    foundfile.close();
    return returnText;
}

void loadConfiguration(const char *filename, t_Config &config) {
    ESP_LOGD("FS", "Loading configuration from %s", String(filename));

    // flag used to detect if a default value is loaded, if default value loaded initiate a save after load
    bool initiatesave = false;

    if (!LittleFS.exists(filename)) {
        ESP_LOGD("FS", "%s not found", String(filename));
        initiatesave = true;
    } else {
        ESP_LOGD("FS", "%s found", String(filename));
    }

    // Open file for reading
    ESP_LOGD("FS", "Opening %s", String(filename));
    File file = LittleFS.open(filename);

    if (!file) {
        ESP_LOGE("FS", "ERROR: Failed to open file %s", String(filename));
        return;
    }

    StaticJsonDocument<2000> doc;

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        ESP_LOGE("FS", "Failed to process configuration file, will load default configuration");
        ESP_LOGE("FS", "===ERROR===");
        ESP_LOGE("FS", "%s", error.c_str());
        ESP_LOGE("FS", "===========");
    }

    // Copy values from the JsonDocument to the Config
    config.terminal_id = doc["terminal_id"].as<String>();
    if (config.terminal_id == "null") {
        initiatesave = true;
        config.terminal_id = default_terminal_id;
    }

    config.device = doc["device"].as<String>();
    if (config.device == "null") {
        initiatesave = true;
        config.device = default_device;
    }

    config.ssid = doc["ssid"].as<String>();
    if (config.ssid == "null") {
        initiatesave = true;
        config.ssid = default_ssid;
    }

    config.wifipassword = doc["wifipassword"].as<String>();
    if (config.wifipassword == "null") {
        initiatesave = true;
        config.wifipassword = default_wifipassword;
    }

    config.fuelsort = doc["fuelsort"].as<String>();
    if (config.fuelsort == "null") {
        initiatesave = true;
        config.fuelsort = default_fuelsort;
    }

    config.httpuser = doc["httpuser"].as<String>();
    if (config.httpuser == "null") {
        initiatesave = true;
        config.httpuser = default_httpuser;
    }

    config.httppassword = doc["httppassword"].as<String>();
    if (config.httppassword == "null") {
        initiatesave = true;
        config.httppassword = default_httppassword;
    }

    config.billingserver = doc["billingserver"].as<String>();
    if (config.billingserver == "null") {
        initiatesave = true;
        config.billingserver = default_billingserver;
    }

    config.billingkey = doc["billingkey"].as<String>();
    if (config.billingkey == "null") {
        initiatesave = true;
        config.billingkey = default_billingkey;
    }

    config.heartbeat = doc["heartbeat"];
    if (config.heartbeat == 0) {
        initiatesave = true;
        config.heartbeat = default_heartbeat;
    }

    if (doc.containsKey("webpagedelay")) {
        config.webpagedelay = doc["webpagedelay"].as<int>();
    } else {
        initiatesave = true;
        config.webpagedelay = default_webpagedelay;
    }

    if (doc.containsKey("buzzerintensity")) {
        config.buzzerintensity = doc["buzzerintensity"].as<int>();
    } else {
        initiatesave = true;
        config.buzzerintensity = default_buzzerintensity;
    }

    if (doc.containsKey("calibration")) {
        config.calibration = doc["calibration"].as<float>();
    } else {
        initiatesave = true;
        config.calibration = default_calibration;
    }

    config.pump_timeout = doc["pump_timeout"];
    if (config.pump_timeout == 0) {
        initiatesave = true;
        config.pump_timeout = default_pump_timeout;
    }

    file.close();

    if (initiatesave) {
        ESP_LOGI("FS", "Default configuration values loaded, saving configuration to %s", String(filename));
        saveConfiguration(filename, config);
    }
}


void printConfig(t_Config &config) {
  ESP_LOGI("FS", "         terminal id: %s", config.terminal_id);
  ESP_LOGI("FS", "              device: %s", config.device);
  ESP_LOGI("FS", "                ssid: %s", config.ssid);
  ESP_LOGI("FS", "        wifipassword: %s", config.wifipassword);
  ESP_LOGI("FS", "            fuelsort: %s", config.fuelsort);
  ESP_LOGI("FS", "         calibration: %f", config.calibration);
  ESP_LOGI("FS", "        pump timeout: %d", config.pump_timeout);

}

// Prints the content of a file to the Serial
void printFile(const char *filename) {
  // Open file for reading
  File file = LittleFS.open(filename);
  if (!file) {
    ESP_LOGE("FS", "Failed to read file");
    return;
  }
  // Extract each characters by one by one
  while (file.available()) {
    ESP_LOGD("FS", "%c", (char)file.read());
  }

  // Close the file
  file.close();
}
