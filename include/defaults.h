#include <Arduino.h>

const String default_terminal_id = "ga-fuel-01";
const String default_device = "Terminal_default";
const String default_ssid = "somessid";
const String default_wifipassword = "somepassword";
const String default_fuelsort = "WATER";
const String default_httpuser = "admin_user";
const String default_httppassword = "admin";
const String default_billingserver = "http://192.168.1.40:5000/terminal";
const String default_billingkey ="changeme"; // RfUjXn2r5u8x/A?D
const int default_heartbeat = 10000;  // 10 seconds
const int default_webpagedelay = 1; // minimum should really be 1 second, if set to 0 the webadmin page often refreshes before action has completed
const int default_buzzerintensity = 50;
const float default_calibration = 90;

 
 