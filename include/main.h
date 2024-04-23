#ifndef MAIN_H
#define MAIN_H

enum t_terminalStates {
  OFFLINE_ENTRY, // Terminal is OFFLINE
  OFFLINE, // Terminal is OFFLINE
  WAIT_CARD_ENTRY, // Wait for a valid RFID card to be presented ENTRY STATE 
  WAIT_CARD, // Wait for a valid RFID card to be presented
  WAIT_CARD_REMOVED_ENTRY,
  WAIT_CARD_REMOVED, // Wait if the card is removed again
  COUNT_FUEL_ENTRY, // Count the fuel through the flow meter
  COUNT_FUEL, // Count the fuel through the flow meter
  SEND_DATA_ENTRY, // Send the data of the refueling to the cloud
  SEND_DATA, // Send the data of the refueling to the cloud
  SHOW_SUMMARY_ENTRY,
  SHOW_SUMMARY, // Show Summary
  OTA //
};

enum t_status{
  normal,
  connection_error,
  blocked,
  waitcard,
  pumpon,
  cache_error
};

typedef struct {
    char aircraft[20];        // Callsign of the aircraft
    char article[20];  // Article
    char memberid[10];
    char date[11];
    float amount;
} t_refueling;

typedef struct  {
  int pump;
  int connected;
  int wifi;
  int wifi_rssi;
  int wifi_state;
  char s_IP[16];
  unsigned int rebootReason;
  bool rebooted;
  enum t_status status;
  uint32_t freeHeap;
  int cachedRefuelings;
} t_terminalStatus;



typedef struct {
  bool validcard = false; // Was a valid card read ??
  char aircraft[20];        // Callsign of the aircraft
  char suffix[10];        // Suffix for article
  char memberid[10];  // VF Member ID
} t_chipcard;

#endif