#include <Arduino.h>
#include <config.h>

#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
//#include <MFRC522DriverI2C.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>

MFRC522DriverPinSimple ss_pin(5); // Configurable, see typical pin layout above.

MFRC522DriverSPI driver{ss_pin}; // Create SPI driver.
//MFRC522DriverI2C driver{}; // Create I2C driver.
MFRC522 mfrc522{driver};  // Create MFRC522 instance.

void setup() {

  Serial.begin(115200); // Initialize serial communications with the PC for debugging.
  while (!Serial);      // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4).
  mfrc522.PCD_Init();   // Init MFRC522 board.
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);	// Show details of PCD - MFRC522 Card Reader details.
	Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
}

void handle_chipcard()
{

}

int read_aircraft()
{
  
}

void loop() {
  if ( !mfrc522.PICC_IsNewCardPresent()) {
		return;
	}

	// Select one of the cards.
	if ( !mfrc522.PICC_ReadCardSerial()) {
		return;
	}
  
  //MFRC522Debug::PICC_DumpToSerial(mfrc522, Serial, &(mfrc522.uid));

  Serial.println(F("**Card Detected:**"));
 
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) 
    key.keyByte[i] = 0xFF;
  
  byte buffer1[18];
  byte block;
  byte len;
  MFRC522::StatusCode status;

  
  // Read aircraft callsign
  block = BLOCK_AIRCRAFT;
  len = 18;

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_Command::PICC_CMD_MF_AUTH_KEY_A, BLOCK_AIRCRAFT, &key, &(mfrc522.uid));
  if (status != MFRC522::StatusCode::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    return;
  }

  status = mfrc522.MIFARE_Read(block, buffer1, &len);
  if (status != MFRC522::StatusCode::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    return;
  }
  
  // Dump String
  Serial.print(F("Aircraft: "));
  for (uint8_t i = 0; i < 6; i++)
  {
    Serial.write(buffer1[i]);
  }
  
  Serial.print(" ");

  Serial.println(F("\n**End Reading**\n"));

  delay(1000); //change value if you want to read cards faster

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}