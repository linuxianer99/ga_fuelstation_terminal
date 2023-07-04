#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <main.h>

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

void lcd_init(){
     // init LCD
  
  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  lcd.clear();
}

void lcd_WaitForTransponder(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Please present card!");
}

void lcd_SendData(){
  lcd.setCursor(0,2);
  lcd.print("Sending data ...");
}

void lcd_SendDataResult(int result){
  char buffer[16];
  lcd.setCursor(0,2);
  if (result == 200){
    lcd.print("Sending data OK!");
  }
  else{
    snprintf(buffer, 16, "ERROR: %d", result);
    lcd.print(buffer);
  }
}

void lcd_WelcomeMessage()
{   
  lcd.print("GA Fuel Station");
  lcd.setCursor(0,1);
  lcd.print("@MLV Thannhausen");
  lcd.setCursor(0,3);
  lcd.print("Version 1.0");
}

void lcd_ConnectingWIFI()
{
  lcd.clear();
  lcd.print("Connecting WIFI ...");
}



void lcd_ShowIP(const char *address)
{
  char buffer[16]; 
  lcd.setCursor(0,1);
  snprintf(buffer, 16, "IP: %s", &address);
  lcd.print(buffer);
}

void lcd_UpdateFuelCount(t_refueling rf)
{
  char buffer[16];
  lcd.setCursor(9,1);
  snprintf(buffer, 16, "%.2f", rf.amount);
  lcd.print(buffer);

}

void lcd_UpdateStatus(t_refueling rf, t_terminalStatus ts)
{
  char pump[]={0x2d,0x5c,0x7c,0x2f};
  static int index=0;

  // Show pump on status
  lcd.setCursor(0,3);
  lcd.print("P:");
  if (ts.pump)
  {
    lcd.setCursor(2,3);
    lcd.write(pump[index]);
    index++;
    if (index > 3)
      index = 0;
  }
  else
  {
    lcd.setCursor(2,3);
    lcd.write(0x6f);
  }

  // Show LAN connetion status
  lcd.setCursor(18,3);
  if (ts.wifi)
  {
    lcd.write('W');
  }
  else
  {
    lcd.write('x');
  }

  // Show Server connection status
  lcd.setCursor(19,3);
  if (ts.connected)
  {
    lcd.write('C');
  }
  else
  {
    lcd.write('x');
  }

}

void lcd_ShowAircraft(t_refueling rf){
  char buffer[16];
  lcd.clear();
  lcd.setCursor(0,0);
  snprintf(buffer, 16, rf.aircraft);
  lcd.print(buffer);

  lcd.setCursor(10,0);
  snprintf(buffer, 16, rf.article);
  lcd.print(buffer);
}

void lcd_ShowCount()
{
  lcd.setCursor(0,1);
  lcd.print("Amount:");
}