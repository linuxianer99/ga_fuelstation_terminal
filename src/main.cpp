#include <Arduino.h>
#include <config.h>
#include <main.h>
#include <string.h>
#include <Wire.h>

#include <lcd.h>
#include <LiquidCrystal_I2C.h>

//#include <EthernetENC.h>
#include <WiFi.h>

#include <HTTPClient.h>

#include "filesystem.h"
#include <LittleFS.h>
#include "webserver.h"
#include "buzzer.h"
#include "led.h"

#include <driver/pcnt.h>
#include "soc/pcnt_struct.h"

#include "billing_cloud.h"

#include "esp32s3/rom/rtc.h"

#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
//#include <MFRC522DriverI2C.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
//byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Set the static IP address to use if the DHCP fails to assign
//IPAddress MYIPADDR(192, 168, 1, 28);
//IPAddress MYIPMASK(255,255,255,0);
//IPAddress MYDNS(192, 168, 1, 254);
//IPAddress MYGW(192, 168, 1, 254);


MFRC522DriverPinSimple ss_pin(10); // Configurable, see typical pin layout above.

MFRC522DriverSPI driver{ss_pin}; // Create SPI driver.
MFRC522 mfrc522{driver};  // Create MFRC522 instance.

t_terminalStates TerminalState = OFFLINE_ENTRY;
t_terminalStatus TerminalStatus;
t_refueling Refueling;
t_chipcard chipcard;

bool cardRemoved = false;
int counter = 0;
bool current, previous;

unsigned long multPulses  = 0;
unsigned long pcnt_value = 0;
pcnt_isr_handle_t user_isr_handle = NULL; //user's ISR service handle

unsigned long heartbeat_suspended = 0;

bool shouldReboot = false;
unsigned int rebootReason = 0;

// Setup Webserver
// initialise webserver
AsyncWebServer server(80);

// used for loading and saving configuration data
const char *filename = "/config.json";
t_Config Config;


void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(Config.ssid.c_str(), Config.wifipassword.c_str());
  ESP_LOGD("WIFI", "Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    ESP_LOGD("WIFI", ".");
    delay(1000);
  }
  TerminalStatus.wifi=1;
  ESP_LOGD("WIFI", "Local IP:", WiFi.localIP().toString());
}

void checkWifi(t_terminalStatus *ts) {
  if (WiFi.status() == WL_CONNECTED)
  {
    ts->wifi=1;
    ts->wifi_rssi=WiFi.RSSI();
    ESP_LOGD("WIFI", "RSSI: %d", WiFi.RSSI());
  }
  else
  {
    ts->wifi=0;
  }
}


static void IRAM_ATTR pcnt_example_intr_handler(void *arg)
{
    log_i("PCNT INT");

    if (PCNT.int_st.val & BIT(PCNT_UNIT_0))
    {
      multPulses++;
      PCNT.int_clr.val = BIT(PCNT_UNIT_0);
      log_i("PCNT INT IF");
    }
}

void initIO() {
  
  init_buzzer();
  init_status_led();

  unsigned long *ptr;
  
  // Setup IO
  pinMode(PUMP_RELAIS, OUTPUT);
  //pinMode(PUMP_LED, OUTPUT);

  // Setup Pulse Counter
  pcnt_config_t pcntFreqConfig = {                         // Instancia do Contador de Pulsos
    .pulse_gpio_num = PUMP_PULSE, 
    .ctrl_gpio_num = PCNT_PIN_NOT_USED,
    .lctrl_mode = PCNT_MODE_KEEP,
    .hctrl_mode = PCNT_MODE_KEEP,
    .pos_mode = PCNT_COUNT_DIS,
    .neg_mode = PCNT_COUNT_INC,
    .counter_h_lim = PCNT_H_LIM_VAL,
    .counter_l_lim = -32768,
    .unit = PCNT_UNIT_0,
    .channel = PCNT_CHANNEL_0,
  };
  int result;
  result=pcnt_unit_config(&pcntFreqConfig);
  ESP_LOGD("PCNT", "%d", result);                         // configura os registradores do Contador de Pulsos

  /* Enable events on zero, maximum and minimum limit values */
  //pcnt_event_enable(PCNT_UNIT_0, PCNT_EVT_ZERO);
  pcnt_event_enable(PCNT_UNIT_0, PCNT_EVT_H_LIM);

  pcnt_counter_pause(PCNT_UNIT_0);
  pcnt_counter_clear(PCNT_UNIT_0);                        // Zera e reseta o Contador de Pulsos

  /* Register ISR handler and enable interrupts for PCNT unit */
  pcnt_isr_register(pcnt_example_intr_handler, NULL, 0, NULL);
  pcnt_intr_enable(PCNT_UNIT_0);
  //pcnt_counter_resume(PCNT_UNIT_0);                       // reinicia o Contador de Pulsos

  pcnt_set_filter_value(PCNT_UNIT_0, 1023);
  pcnt_filter_enable(PCNT_UNIT_0);

  // Deactivate Pull UP/DOWN on PUMP GPIO 
  gpio_set_pull_mode((gpio_num_t) PUMP_PULSE, GPIO_FLOATING);

}

void io_UpdateStatus(t_terminalStatus tstatus, t_terminalStates tstates)
{
  if (tstates == WAIT_CARD)
    status_led_fade();

  else if ((tstatus.pump == 1) && (tstates == COUNT_FUEL))
  {
    status_led_on();
    digitalWrite(PUMP_RELAIS, 1);
  }
  else
  {
    status_led_off();
    digitalWrite(PUMP_RELAIS, 0);
  }

  // Check   
}

void updateStatus(void * paramter)
{
  for(;;)
  {
    if (TerminalState == SEND_DATA_ENTRY || TerminalState == SEND_DATA)
    {
      log_i("Update Status - SUSPENDED");
      heartbeat_suspended++;  
    }    
    else
    {
      checkConnection(&TerminalStatus);
      checkWifi(&TerminalStatus);

    //TerminalStatus.wifi = 1;
    //TerminalStatus.connecsted = 1;

    if (!TerminalStatus.wifi || !TerminalStatus.connected)
      TerminalState=OFFLINE_ENTRY;

      log_i("Update Status");
      unsigned int temp = uxTaskGetStackHighWaterMark(nullptr);
      log_i("Suspended cntr: %d", heartbeat_suspended);
    }
    vTaskDelay(Config.heartbeat / portTICK_PERIOD_MS);
  }
}

void print_reset_reason(int reason)
{
  switch ( reason)
  {
    case 1 : Serial.println ("POWERON_RESET");break;          /**<1,  Vbat power on reset*/
    case 3 : Serial.println ("SW_RESET");break;               /**<3,  Software reset digital core*/
    case 4 : Serial.println ("OWDT_RESET");break;             /**<4,  Legacy watch dog reset digital core*/
    case 5 : Serial.println ("DEEPSLEEP_RESET");break;        /**<5,  Deep Sleep reset digital core*/
    case 6 : Serial.println ("SDIO_RESET");break;             /**<6,  Reset by SLC module, reset digital core*/
    case 7 : Serial.println ("TG0WDT_SYS_RESET");break;       /**<7,  Timer Group0 Watch dog reset digital core*/
    case 8 : Serial.println ("TG1WDT_SYS_RESET");break;       /**<8,  Timer Group1 Watch dog reset digital core*/
    case 9 : Serial.println ("RTCWDT_SYS_RESET");break;       /**<9,  RTC Watch dog Reset digital core*/
    case 10 : Serial.println ("INTRUSION_RESET");break;       /**<10, Instrusion tested to reset CPU*/
    case 11 : Serial.println ("TGWDT_CPU_RESET");break;       /**<11, Time Group reset CPU*/
    case 12 : Serial.println ("SW_CPU_RESET");break;          /**<12, Software reset CPU*/
    case 13 : Serial.println ("RTCWDT_CPU_RESET");break;      /**<13, RTC Watch dog Reset CPU*/
    case 14 : Serial.println ("EXT_CPU_RESET");break;         /**<14, for APP CPU, reseted by PRO CPU*/
    case 15 : Serial.println ("RTCWDT_BROWN_OUT_RESET");break;/**<15, Reset when the vdd voltage is not stable*/
    case 16 : Serial.println ("RTCWDT_RTC_RESET");break;      /**<16, RTC Watch dog reset digital core and rtc module*/
    default : Serial.println ("NO_MEAN");
  }
}

void setup() {

  Serial.begin(115200); // Initialize serial communications with the PC for debugging.

  rebootReason = rtc_get_reset_reason(0);
  TerminalStatus.rebootReason = rebootReason;
  TerminalStatus.rebooted = true;
  print_reset_reason(rebootReason);
  init_FS();

  log_i("Loading Configuration ...");
  loadConfiguration(filename, Config);
  printConfig(Config);

  // GPIOs
  initIO();

  lcd_init();
  lcd_WelcomeMessage();
  beep(1000);

  delay(1000);

  // Try to connect terminal to network
  lcd_ConnectingWIFI();
  initWiFi();
  //initLAN();
  String localIP = WiFi.localIP().toString();
  localIP.toCharArray(&TerminalStatus.s_IP[0], 16);
  lcd_ShowIP(&TerminalStatus.s_IP[0]);
  delay(2000);

  configureWebServer(&server);

  // Init Card reader
  mfrc522.PCD_Init();   // Init MFRC522 board.
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);	// Show details of PCD - MFRC522 Card Reader details.

  // Create Monitor Task
  xTaskCreatePinnedToCore(
    updateStatus,    // Function that should be called
    "Update Status",   // Name of the task (for debugging)
    8000,            // Stack size (bytes)
    NULL,            // Parameter to pass
    1,               // Task priority
    NULL,             // Task handle
    0  // Core to run on
  );
}

bool handle_chipcard(t_chipcard *cc)
{

  // Check if card is present ...
  if ( !mfrc522.PICC_IsNewCardPresent()) {
	  return false;
	}
  
  ESP_LOGD("SC", "Card Detected");

	// Select one of the cards.
	if ( !mfrc522.PICC_ReadCardSerial()) {
	 	return false;
	}

  
  // Access the card and read aircraft and suffix
  MFRC522::MIFARE_Key key;
  // generate key ... TODO: This has to be coded new
  for (byte i = 0; i < 6; i++) 
     key.keyByte[i] = 0xFF;
  
  MFRC522::StatusCode status;
  byte buffer[18];
  byte hash[32];
  byte block;
  byte len;

  // Dump debug info about the card; PICC_HaltA() is automatically called.
  //MFRC522Debug::PICC_DumpToSerial(mfrc522, Serial, &(mfrc522.uid));

  // Authenticate to PICC for Aircraft and Article
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_Command::PICC_CMD_MF_AUTH_KEY_A, BLOCK_AIRCRAFT, &key, &(mfrc522.uid));
  if (status != MFRC522::StatusCode::STATUS_OK) {
    ESP_LOGD("SC", "Authentication failed: block AIRCRAFT ");
    return false;
  }

  // Read aircraft callsign
  block=BLOCK_AIRCRAFT;
  len=BLOCK_LEN_AIRCRAFT;

  status = mfrc522.MIFARE_Read(block, buffer, &len);
  if (status != MFRC522::StatusCode::STATUS_OK) {
    ESP_LOGD("SC", "Reading falied: aircraft");
    return false;
  }

  // Write data to Refueling struct
  strcpy(cc->aircraft, (char*)buffer);

  // Read suffix
  block=BLOCK_SUFFIX;
  len=BLOCK_LEN_SUFFIX;

  status = mfrc522.MIFARE_Read(block, buffer, &len);
  if (status != MFRC522::StatusCode::STATUS_OK) {
    ESP_LOGD("SC", "Reading failed: suffix");
    return false;
  }

  // Write data to Refueling struct
  strcpy(cc->suffix, (char*)buffer);


  // Authenticate to PICC for memberid
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_Command::PICC_CMD_MF_AUTH_KEY_A, BLOCK_MEMBERID, &key, &(mfrc522.uid));
  if (status != MFRC522::StatusCode::STATUS_OK) {
    ESP_LOGD("SC", "Authentication failed: block AIRCRAFT ");
    return false;
  }

  // Read aircraft callsign
  block=BLOCK_MEMBERID;
  len=BLOCK_LEN_MEMBERID;

  status = mfrc522.MIFARE_Read(block, buffer, &len);
  if (status != MFRC522::StatusCode::STATUS_OK) {
    ESP_LOGD("SC", "Reading falied: aircraft");
    return false;
  }

  // Write data to Refueling struct
  strcpy(cc->memberid, (char*)buffer);


  status = mfrc522.PCD_Authenticate(MFRC522::PICC_Command::PICC_CMD_MF_AUTH_KEY_A, BLOCK_HASH, &key, &(mfrc522.uid));
  if (status != MFRC522::StatusCode::STATUS_OK) {
    ESP_LOGD("SC", "Authentication failed: block HASH");
    return false;
  }

  // Read Hash
  block=BLOCK_HASH;
  len=BLOCK_LEN_HASH;

  status = mfrc522.MIFARE_Read(block, buffer, &len);
  if (status != MFRC522::StatusCode::STATUS_OK) {
    ESP_LOGD("SC", "Reading failed: hash");
    return false;
  }

  // Save first half of the hash
  int i;
  for (i=0; i<16;i++)
  {
    hash[i] = buffer[i];
  }

  status = mfrc522.MIFARE_Read(block+1, buffer, &len);
  if (status != MFRC522::StatusCode::STATUS_OK) {
    ESP_LOGD("SC", "Reading failed: hash");
    return false;
  }
 
  // Save second half of the hash
  for (i=0; i<16;i++)
  {
    hash[i+16] = buffer[i];
  }

  // Close PICC
  mfrc522.PCD_StopCrypto1();
  mfrc522.PICC_HaltA();

  // Print info for debug
  ESP_LOGI("SC", "Got Transponder: Aircraft: %s, Suffix: %s, MemberID: %s", cc->aircraft, cc->suffix, cc->memberid);
  //snprintf((char*)hash, 32, "%x", (char*)buffer);
  ESP_LOGI("SC", "Hash: %x", buffer);
  
  // Give the user a beep as acknowledge
  beep(500);

  return true;

}

void loop() {

  static int sm_counter=0;
  static int state_delay=0;
  static int pump_timeout=0;
  static int httpResult;

  switch(TerminalState) {
    case OFFLINE_ENTRY:
      TerminalState = OFFLINE;
      lcd_OfflineMessage();
      lcd_Backlight(1);
    break;

    case OFFLINE:
      if (TerminalStatus.connected && TerminalStatus.wifi)
        TerminalState = WAIT_CARD_ENTRY;
      else
      {
        TerminalState = OFFLINE;
        TerminalStatus.status = connection_error;
      }
        
    break;

    case WAIT_CARD_ENTRY:
      ESP_LOGD("SM", "Entered state WAITCARD");
      lcd_WaitForTransponder();
      TerminalState=WAIT_CARD;
      TerminalStatus.status = waitcard;
#ifdef BACKLIGHT_DIM
      lcd_Backlight(0);
#endif
    break;

    case WAIT_CARD:
    
      // Fade LED
      status_led_fade();

      if (handle_chipcard(&chipcard))
      {
        // a valid chipcard was read
        ESP_LOGD("SM", "Chip card read");

        // turn on Backlight
        lcd_Backlight(1);
        
        // Create current refueling object
        strcpy(Refueling.aircraft, chipcard.aircraft);
        strcpy(Refueling.memberid, chipcard.memberid);
        strcpy(Refueling.article, Config.fuelsort.c_str());
        strcat(Refueling.article, chipcard.suffix);

        Refueling.amount = 0.0;

        // Print info for debug
        ESP_LOGI("SM", "Create Refueling: Aircraft: %s, MemberID: %s, Article: %s, Amount: %f", \
          Refueling.aircraft, Refueling.memberid, Refueling.article, Refueling.amount);

        // Jump to next state
        TerminalState = WAIT_CARD_REMOVED_ENTRY;
        ESP_LOGD("SM", "Transition: -> WAIT_CARD_REMOVED_ENTRY");
      }
      break;

    case WAIT_CARD_REMOVED_ENTRY:
        lcd_ShowAircraft(Refueling);
        lcd_ShowCount();
        previous = !mfrc522.PICC_IsNewCardPresent();
        cardRemoved = false;
        counter=0;
        // Jump to next state
        TerminalState = WAIT_CARD_REMOVED;
        ESP_LOGD("SM", "Transition: -> WAIT_CARD_REMOVED");
      break;

    case WAIT_CARD_REMOVED:
      
        if (!cardRemoved)
        {
          ESP_LOGD("SM", "State: Card still present");
          current = ! mfrc522.PICC_IsNewCardPresent();
          if (current && previous) counter++;

          previous = current;
          cardRemoved = (counter > 2);
          //delay(10);

        }
        else
        {
          // Jump to next state
          TerminalState = COUNT_FUEL_ENTRY;
          ESP_LOGD("SM", "Transition: -> COUNTFUEL");
        
        }
    break;
    
    case COUNT_FUEL_ENTRY:
      ESP_LOGD("SM", "Entered state COUNTFUEL");
      
      if (mfrc522.PICC_ReadCardSerial())
      {
        break;
      }

      // clear and restart the counter
      multPulses = 0;
      pcnt_counter_pause(PCNT_UNIT_0);
      pcnt_counter_clear(PCNT_UNIT_0);
      pcnt_counter_resume(PCNT_UNIT_0);

      // reset pump timeout
      pump_timeout = 0;

      TerminalState=COUNT_FUEL;

      TerminalStatus.status = pumpon;
    break;

    case COUNT_FUEL:
      // Turn on fuel pump
      TerminalStatus.pump=1;

      // Count fuel 
      // insert code here
      int16_t amount;
      pcnt_get_counter_value(PCNT_UNIT_0, &amount);
      ESP_LOGD("SM", "Counter: %d, MultPulses: %d, Total Pulses: %d", amount,multPulses, (multPulses * PCNT_H_LIM_VAL + amount));
      Refueling.amount = (float)(multPulses * PCNT_H_LIM_VAL + amount) / (float)Config.calibration;
      ESP_LOGD("SM", "Fuel %f", Refueling.amount);
      
      lcd_UpdateFuelCount(Refueling);

      // increas timeout counter
      pump_timeout++;
      ESP_LOGD("SM", "Pump Timeout %d", pump_timeout);

      // check for chipcard
      //if (state_delay > 0){
        // handle state delay
      //  state_delay--;
      //}
      //else {
      if (handle_chipcard(&chipcard))
        {
          // a valid chipcard was read
          ESP_LOGD("SM", "Chip card read");

          // Change State
          TerminalState=SEND_DATA_ENTRY;
          // Stop counter
          pcnt_counter_pause(PCNT_UNIT_0);
        }
      
      if (pump_timeout > Config.pump_timeout)
      {
        // Timeout reached Change State
        TerminalState=SEND_DATA_ENTRY;
        ESP_LOGD("SM", "Pump Timeout reached!");
        beep(500);
      }
      //}
      
    break;

    case SEND_DATA_ENTRY:

      // Turn off fuel pump
      TerminalStatus.pump=0;
      //pc0.pause();
      ESP_LOGD("SM", "Entered state SENDDATA");
      
      // Update LCD
      lcd_SendData();
      // Change State
      TerminalState=SEND_DATA;
    break;

    case SEND_DATA:

      // Send Data to Cloud
      httpResult = SendRefueling(Refueling);
      log_d("HTTP Response %d", httpResult);
      ESP_LOGD("SM", "Data sent to Cloud");
      // Change State
      TerminalState=SHOW_SUMMARY_ENTRY;
    break;

    case SHOW_SUMMARY_ENTRY:
    ESP_LOGD("SM", "SHOW_SUMMARY_ENTRY");
    // Show LCD Message
    if (httpResult == 200){
      lcd_SendDataResult(httpResult);
    }
    else{
      lcd_SendDataResult(httpResult);
      TerminalStatus.status=blocked;
      //log_i("Set Terminal Status to: %d", TerminalStatus.status);
      while(1);
    }
      state_delay=50;
    
      // Change State
      TerminalState=SHOW_SUMMARY;
    break;
      

    case SHOW_SUMMARY:
      if (state_delay > 0){
        state_delay--;
      }
      else{
        // Change State
        TerminalState=WAIT_CARD_ENTRY;
      }
    break;
  }

  //log_i("Loop running on core: %d", xPortGetCoreID());
  TerminalStatus.freeHeap = ESP.getFreeHeap();
  io_UpdateStatus(TerminalStatus, TerminalState);
  lcd_UpdateStatus(Refueling, TerminalStatus);
  //delay(500); //change value if you want to read cards faster
  
  if (shouldReboot)
  {
    ESP.restart();
  }

  if(user_isr_handle) {
        //Free the ISR service handle.
        esp_intr_free(user_isr_handle);
        user_isr_handle = NULL;
  }

  delay(100);
  
}



// void initLAN() {
  
//   ESP_LOGD("LAN", "Init Ethernet ...");

//   // Reset LAN Chip
//   pinMode(4, OUTPUT);
//   digitalWrite(4, 0);
//   delay(10);
//   digitalWrite(4,1);

//   Ethernet.init(7);  // Most Arduino shields

//   // try to configure using IP address instead of DHCP:
//   //Ethernet.begin(mac, ip, myDns);
//   Ethernet.begin(mac, MYIPADDR, MYDNS, MYGW, MYIPMASK);

//   // Check for Ethernet Hardware
//   if (Ethernet.hardwareStatus() == EthernetNoHardware)
//   {
//     ESP_LOGD("LAN", "Check Ethernet HARDWARE");
//   }

//   if (Ethernet.linkStatus() == LinkOFF)
//   {
//     ESP_LOGD("LAN", "No Link! Check network cable");
//   }
  
//   // give the Ethernet shield a second to initialize:
//   delay(1000);

//   ESP_LOGD("LAN", "IP: ", Ethernet.localIP());

// }