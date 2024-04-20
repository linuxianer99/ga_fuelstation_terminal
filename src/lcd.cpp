#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <main.h>
#include "version.h"

#include "config.h"

extern t_Config Config;

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

uint8_t one[8] = {0b00000, 0b10001,	0b01010, 0b00100,	0b01010, 0b10001,	0b00000, 0b00000};
uint8_t two[8] = {0b00000, 0b00100, 0b00100, 0b11111, 0b00100, 0b00100,	0b00000, 0b00000};
 

void lcd_init(){
  // init LCD
    
  lcd.init();                      // initialize the lcd
  lcd.backlight();
  lcd.createChar(0, one);
  lcd.createChar(1, two);
  lcd.clear();
}

void lcd_WaitForTransponder(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Please present card!");
}

void lcd_SendData(){
  lcd.setCursor(0,2);
  lcd.print("Processing data ... ");
}

void lcd_SendDataResult(int result){
  char buffer[16];
  lcd.setCursor(0,2);
  if (result == 1){
    lcd.print("Sending data OK!    ");
  }
  else if (result == 2)
  {
    lcd.print("Stored data OK!     ");
  }
  
  else{
    lcd.print("                ");
    lcd.setCursor(0,2);
    snprintf(buffer, 16, "ERROR: %d", result);
    lcd.print(buffer);
  }
}

void lcd_OfflineMessage(t_terminalStatus s)
{
  lcd.clear();
  lcd.print("Terminal OFFLINE!");
  lcd.setCursor(0,1);
  switch (s.status)
  {
    case cache_error:
      lcd.print("Cache ERROR");
      break;
    default:
      lcd.print("UNKNOWN ERROR");
      break;
  }
  lcd.setCursor(0,2);
  lcd.print("Contact Admin!");

}

void lcd_WelcomeMessage()
{   
  lcd.print("GA Fuel Station");
  lcd.setCursor(0,2);
  lcd.print(Config.terminal_id.c_str());
  lcd.setCursor(0,3);
  lcd.print(VERSION);
}

void lcd_ConnectingWIFI()
{
  lcd.clear();
  lcd.print("Connecting WIFI ...");
}



void lcd_ShowIP(char *address)
{
  char buffer[20]; 
  lcd.setCursor(0,1);
  snprintf(buffer, 20, "IP: %s", address);
  lcd.print(buffer);
}

void lcd_NTP(void)
{
  char buffer[20]; 
  lcd.setCursor(0,2);
  lcd.print("Getting time ...");
}

void lcd_UpdateFuelCount(t_refueling rf)
{
  char buffer[16];
  lcd.setCursor(9,1);
  snprintf(buffer, 16, "%.1f", rf.amount);
  lcd.print(buffer);

}

void lcd_Backlight(int status)
{
  if (status == 1)
    lcd.backlight();
  else
    lcd.noBacklight();
}

void lcd_UpdateStatus(t_refueling rf, t_terminalStatus ts)
{
  char pump[]={0x2d,0x5c,0x7c,0x2f};
  char buffer[6];
  static int index=0;

  // Show pump on status
  lcd.setCursor(0,3);
  lcd.print("P:");
  if (ts.pump)
  {
    lcd.setCursor(2,3);
    lcd.write(index);
    index++;
    if (index > 1)
      index = 0;
  }
  else
  {
    lcd.setCursor(2,3);
    lcd.write(0x6f);
  }

  // Show Wifi RSSI
  lcd.setCursor(6,3);
  if (ts.wifi)
  {
    snprintf(buffer, 6, "R:%d", ts.wifi_rssi);
    lcd.print(buffer);
  }
  else
  {
    lcd.print("R:N/A");
  }

  // Show number of cached refuelings
  lcd.setCursor(14,3);
  snprintf(buffer, 2, "%d", ts.cachedRefuelings);
  lcd.print(buffer);

  // Show network connetion status
  lcd.setCursor(18,3);
  if (ts.wifi)
  {
    lcd.write('N');
  }
  else
  {
    lcd.write('n');
  }

  // Show Server connection status
  lcd.setCursor(19,3);
  if (ts.connected)
  {
    lcd.write('B');
  }
  else
  {
    lcd.write('b');
  }

}

void lcd_ShowAircraft(t_refueling rf){
  char buffer[20];
  lcd.clear();
  lcd.setCursor(0,0);
  snprintf(buffer, 16, rf.aircraft);
  lcd.print(buffer);

  lcd.setCursor(0,2);
  snprintf(buffer, 20, rf.article);
  lcd.print(buffer);
}

void lcd_ShowCount()
{
  lcd.setCursor(0,1);
  lcd.print("Amount:");
}