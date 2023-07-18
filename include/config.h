// Chipcard config

#define MIFARE_KEY FFFFFFFFFFFF

#define SECTOR_AIRCRAFT 0
#define BLOCK_AIRCRAFT 1
#define BLOCK_LEN_AIRCRAFT 18
#define BLOCK_SUFFIX 2
#define BLOCK_LEN_SUFFIX 18
#define BLOCK_MEMBERID 4
#define BLOCK_LEN_MEMBERID 18
#define BLOCK_HASH 8
#define BLOCK_LEN_HASH 18

#define FUELSORT "AVGAS"

#define PUMP_PULSE 3
#define PUMP_RELAIS 2
#define PUMP_LED 1
#define BUZZER 4

#define BUZZER_FREQ 2000
#define BUZZER_LEN 500

#define PCNT_H_LIM_VAL      1000

// configuration structure
typedef struct {
  String terminal_id;           // hostname of device
  String device;             // device name
  String ssid;               // wifi ssid
  String wifipassword;       // wifi password
  String fuelsort;           // Current pump the terminal is connected too
  String httpuser;           // username to access web admin
  String httppassword;       // password to access web admin
  String httpapitoken;       // api token used to authenticate against the device
  String billingserver;      // Server to report the refueling to
  String billingkey;         // Base64 encoded key
  int heartbeat;             // Heartbeat interval
  String ntptimezone;        // ntp time zone to use, use the TZ database name from https://en.wikipedia.org/wiki/List_of_tz_database_time_zones
  int ntpsynctime;           // how frequently to schedule the regular syncing of ntp time
  int ntpwaitsynctime;       // upon boot, wait these many seconds to get ntp time from server
  String ntpserver;          // hostname or ip of the ntpserver
  int webpagedelay;
  int buzzerintensity;
  float calibration;
} t_Config;
