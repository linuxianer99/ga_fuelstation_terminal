#include <Arduino.h>

const String default_terminal_id = "ga-fuel-01";
const String default_device = "Terminal";
const String default_ssid = "somessid";
const String default_wifipassword = "somepassword";
const String default_fuelsort = "MOGAS";
const String default_httpuser = "admin";
const String default_httppassword = "admin";
const String default_billingserverURL = "http://192.168.1.40:5000/terminal";
const String default_billingkey ="UmZValhuMnI1dTh4L0E/RA=="; // RfUjXn2r5u8x/A?D
const String default_syslogserver = "192.168.10.21";
const int default_syslogport = 514;
const String default_ntptimezone = "Europe/London";
const int default_ntpsynctime = 60;
const int default_ntpwaitsynctime = 5;
const String default_ntpserver = "192.168.1.254";
const int default_webpagedelay = 1; // minimum should really be 1 second, if set to 0 the webadmin page often refreshes before action has completed
const int default_buzzerintensity = 50;

 
 