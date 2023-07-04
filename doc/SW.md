# Software documentation

## States

### WaitCard

This state is the default state. The terminal is waiting for a card.

#### Transistion

* To **CountFuel**: Valid card read

#### Outputs

* PowerToPump: OFF
* SingalLight GREEN: OFF

### CountFuel

This state is active if a valid card has been read. The terminal counts the fuel flow through the meter.

#### Transistion

* To **SendData**: The same card is put again on the reader
* To **SendData**: Another card is put on the reader
* To **SendData**: A Timeout of TBD occurs

#### Outputs

* PowerToPump: ON
* SingalLight GREEN: ON


### SendData

This state sends the data of the refueling to the cloud

#### Transistion

* To **WaitCard**: Sent to cloud was sucessfull

#### Outputs

* PowerToPump: OFF
* SingalLight GREEN: OFF


## Pin description


https://github.com/smford/eeh-esp32-rfid

https://github.com/ayushsharma82/AsyncElegantOTA