# GA Fuel Station Terminal

## 1. Zweck

Das Projekt implementiert die Firmware eines Tankstellen-Terminals auf einem
ESP32-S3. Das Terminal liest eine MIFARE-RFID-Chipkarte, schaltet die Pumpe,
zaehlt die Impulse des Durchflussmessers und uebertraegt den Tankvorgang an
einen Billing-Server. Bei fehlender Serververbindung werden Tankvorgaenge im
LittleFS-Dateisystem zwischengespeichert und spaeter automatisch nachgesendet.

Die Bedienoberflaeche besteht aus:

- einem 20x4-I2C-LCD,
- einer Status-LED und einem Buzzer,
- einem lokalen HTTP-Administrationsbereich,
- einer OTA-Update-Seite von ElegantOTA.

## 2. Systemuebersicht

```text
MIFARE-Karte
     |
     v
ESP32-S3 -- SPI --> MFRC522 RFID-Leser
    |
    +-- GPIO --> Pumpenrelais
    +-- GPIO <-- Durchflussmesser (Pulse Counter)
    +-- I2C --> LCD
    +-- GPIO --> Status-LED / Buzzer
    |
    +-- Wi-Fi --> Billing-Server (HTTPS)
    +-- HTTP --> lokale Administration / OTA
    +-- LittleFS --> Konfiguration, Logs, Offline-Cache
```

### 2.1 Hardware und Plattform

- Boarddefinition: `boards/ga_fuel_station.json`
- Controller: ESP32-S3, 240 MHz
- Flash: 16 MB, Partitionierung aus `ga_fuelstation_16MB.csv`
- Framework: Arduino fuer ESP32
- Dateisystem: LittleFS
- Uploadgeschwindigkeit: 460800 Baud
- Serielle Debug-Ausgabe: 115200 Baud
- Standard-Webserver-Port: TCP 80

## 3. Repository-Struktur

| Pfad | Inhalt |
| --- | --- |
| `src/` | Implementierung der Firmware |
| `include/` | Gemeinsame Header, Konfiguration, Datentypen und Zertifikat |
| `data/` | Dateien fuer das LittleFS-Image, insbesondere Weboberflaeche |
| `boards/` | Benutzerdefinierte PlatformIO-Boarddefinition |
| `doc/` | Projekt-, Software- und Hardwaredokumentation |
| `ca/` | Zertifikatsbezogene Projektdateien |
| `test/` | Testverzeichnis; derzeit ohne automatisierte Tests |
| `platformio.ini` | Buildumgebungen und Bibliotheksabhaengigkeiten |
| `auto_firmware_version.py` | Erzeugt die Firmware-Version aus Git |
| `generate_payload.py` | Erzeugt ein Beispiel fuer die Billing-Nutzlast |
| `chipcard.dmp.sh` | Beispiel-Dump einer MIFARE-1K-Karte |
| `.gitlab-ci.yml` | Tag-basierter Release-Build |

## 4. Entwicklungsumgebung

Installiert werden benoetigt:

1. Visual Studio Code mit PlatformIO IDE oder eine PlatformIO-Core-Installation.
2. Git, weil der Build die Version mit `git describe --dirty=-DEV` ermittelt.
3. Ein ESP32-S3-Board mit der projektspezifischen Boarddefinition.
4. Ein USB-Kabel fuer Upload, serielle Ausgabe und optionales Debugging.

Abhaengigkeiten werden aus `platformio.ini` geladen:

- `computer991/Arduino_MFRC522v2`
- `Wire`
- `marcoschwartz/LiquidCrystal_I2C`
- `intrbiz/Crypto`
- `bblanchon/ArduinoJson`
- `densaugeo/base64`
- `ayushsharma82/ElegantOTA`

## 5. Build, Upload und Monitoring

### 5.1 Debug-Build

```text
pio run -e FuelStationHardware_DEBUG
pio run -e FuelStationHardware_DEBUG -t upload
pio device monitor -b 115200
```

Der Debug-Build aktiviert `TESTMODE` und `DEBUGMODE`. Im `TESTMODE` wird die
Menge waehrend des Zapfvorgangs kuenstlich in 0,01er-Schritten erhoeht; die
echte Durchflussmessung ist damit fuer Hardwaretests umgangen.

### 5.2 Release-Build

```text
pio run -e FuelStationHardware_RELEASE
pio run -e FuelStationHardware_RELEASE -t upload
pio device monitor -b 115200
```

Der Release-Build verwendet keine Testmodus-Mengenberechnung. Fuer ein
vollstaendiges Flash-Image inklusive Webdateien:

```text
pio run -e FuelStationHardware_RELEASE -t buildfs
```

Das Dateisystem kann anschliessend mit dem PlatformIO-Target `uploadfs`
separat aufgespielt werden.

Die erzeugte Firmware-Datei heisst nach dem Buildschema
`GA_FUEL_TERMINAL-<git-version>`. Ist das Arbeitsverzeichnis veraendert, wird
die Version mit dem Suffix `-DEV` markiert.

## 6. Erstinbetriebnahme

1. RFID-Leser, Durchflussmesser, Relais, LCD, LED und Buzzer gemaess Pinout
   anschliessen.
2. Firmware und LittleFS-Image auf das Board laden.
3. `data/config.json` vor dem Dateisystem-Upload an die Installation anpassen.
4. Board starten und die serielle Ausgabe mit 115200 Baud beobachten.
5. Das Terminal verbindet sich beim Start mit dem konfigurierten Wi-Fi.
   Der Startvorgang wartet, bis eine Verbindung besteht.
6. Die angezeigte IP-Adresse im Browser oeffnen und mit den konfigurierten
   HTTP-Zugangsdaten anmelden.
7. Einen RFID-Test mit einer korrekt beschriebenen MIFARE-1K-Karte und einen
   Durchfluss-/Relais-Test durchfuehren.
8. Einen Testtankvorgang ausfuehren und im Billing-Server, im Webstatus und in
   `/log.txt` pruefen.

Wenn `/config.json` fehlt, legt die Firmware eine Datei mit den Standardwerten
aus `include/defaults.h` an. Diese Standardwerte sind nicht fuer den
Produktivbetrieb gedacht.

## 7. Konfiguration

Die aktive Konfiguration liegt auf dem Board unter `/config.json`. Sie wird
beim Start gelesen und bei fehlenden Werten mit Defaults ergaenzt. Die
vollstaendige Struktur entspricht `t_Config` in `include/config.h`:

| Feld | Typ | Bedeutung |
| --- | --- | --- |
| `terminal_id` | String | Eindeutige Terminal-ID; wird auch als Host-/URL-Bestandteil verwendet |
| `device` | String | Geraetebezeichnung |
| `ssid` | String | Wi-Fi-SSID |
| `wifipassword` | String | Wi-Fi-Passwort |
| `fuelsort` | String | Artikelpraefix, z. B. `AVGAS` oder `MOGAS` |
| `httpuser` | String | Benutzername fuer die lokale Administration |
| `httppassword` | String | Passwort fuer die lokale Administration und OTA |
| `httpapitoken` | String | Derzeit gespeichert, aber im Webserver nicht ausgewertet |
| `billingserver` | String | Basis-URL des Billing-Servers |
| `billingkey` | String | Base64-kodierter HMAC-Schluessel |
| `heartbeat` | Integer | Intervall des Status-/Cache-Tasks in Millisekunden |
| `webpagedelay` | Integer | Verzoegerung der Weboberflaeche in Sekunden |
| `buzzerintensity` | Integer | PWM-Aussteuerung des Buzzers |
| `calibration` | Float | Pulse pro Mengeneinheit; Menge = Pulse / Kalibrierwert |
| `pump_timeout` | Integer | Timeout-Zaehler fuer die Zapfphase; Schleifendauer ist nicht in Millisekunden garantiert |

Beispiel einer syntaktisch gueltigen Konfiguration:

```json
{
  "terminal_id": "ga-fuel-01",
  "device": "FuelStation",
  "ssid": "<SSID>",
  "wifipassword": "<WIFI-PASSWORT>",
  "fuelsort": "AVGAS",
  "billingserver": "https://billing.example/terminal",
  "billingkey": "<BASE64-HMAC-SCHLUESSEL>",
  "httpuser": "admin",
  "httppassword": "<ADMIN-PASSWORT>",
  "httpapitoken": "",
  "heartbeat": 10000,
  "webpagedelay": 1,
  "buzzerintensity": 20,
  "calibration": 91,
  "pump_timeout": 5000
}
```

Die vorhandene Datei `data/config.json.example` sollte vor der Verwendung an
diese Struktur angeglichen werden; sie enthaelt im aktuellen Stand fehlende
Kommas und ist kein gueltiges JSON.

### 7.1 Sicherheitsrelevante Konfigurationshinweise

- Keine produktiven Wi-Fi-, HTTP- oder Billing-Schluessel in das Repository
  einchecken.
- Das Webinterface wird ueber HTTP auf Port 80 angeboten. Die Anmeldung ist
  Basic Authentication, die Verbindung selbst ist nicht TLS-verschluesselt.
- Die Billing-Kommunikation verwendet `WiFiClientSecure` und das in
  `include/ca.hpp` eingebettete Root-Zertifikat.
- Der RFID-Code verwendet aktuell den MIFARE-Schluessel `FF FF FF FF FF FF`.
  Dieser Schluessel muss vor einem Produktiveinsatz ersetzt und die Karten
  muessen entsprechend personalisiert werden.

## 8. Hardware-Pinout

Die Pinbelegung wird in `include/config.h` und fuer SPI zusaetzlich im
RFID-Treiber festgelegt.

| Signal | ESP32-WROOM-32 laut Entwurf | ESP32-S3 / Implementierung |
| --- | ---: | ---: |
| RFID MOSI | GPIO23 | GPIO11 |
| I2C SCL | GPIO22 | GPIO9 |
| I2C SDA | GPIO21 | GPIO8 |
| RFID MISO | GPIO19 (typischer WROOM-Pin) | GPIO13 |
| RFID SCK | GPIO18 | GPIO12 |
| RFID SS/SDA | GPIO5 (Entwurf) | GPIO10 |
| Durchfluss-Pulse `PUMP_PULSE` | - | GPIO3 |
| Pumpenrelais `PUMP_RELAIS` | - | GPIO2 |
| Status-LED `PUMP_LED` | - | GPIO1 |
| Buzzer | - | GPIO4 |

Weitere in `config.h` definierte, aktuell nicht fuer den Hauptpfad verwendete
Ethernet-Pins sind CS GPIO38, IRQ GPIO47, Reset GPIO46 sowie SPI SCK GPIO36,
MISO GPIO37 und MOSI GPIO35.

## 9. RFID-Kartenformat

Der Code erwartet eine MIFARE-1K-Karte und authentifiziert mit Key A. Die
relevanten Bloecke sind:

| Block | Inhalt | Maximale Nutzlaenge im Code |
| ---: | --- | ---: |
| 1 | Aircraft/Registrierung | 17 Zeichen plus Nullterminierung |
| 2 | Artikel-Suffix | 17 Zeichen plus Nullterminierung |
| 4 | Member-ID | 17 Zeichen im Lesevorgang; Zielpuffer ist kleiner |
| 8 und 9 | Hash-/Zusatzdaten | je 16 Nutzbytes werden uebernommen |

Die finale Artikelbezeichnung wird aus `fuelsort + suffix` gebildet. Der
gespeicherte Hash wird derzeit gelesen und geloggt, aber nicht gegen einen
berechneten Wert validiert. Die Karte muss deshalb mit den erwarteten
Nullterminierungen beschrieben sein, da der Code die gelesenen Daten als
C-Strings behandelt.

## 10. Zustandsautomat und Bedienablauf

Die Zustandsdefinition befindet sich in `include/main.h`, die Verarbeitung in
`src/main.cpp`.

```text
WAIT_CARD_ENTRY -> WAIT_CARD
WAIT_CARD -> WAIT_CARD_REMOVED_ENTRY       (gueltige Karte)
WAIT_CARD_REMOVED_ENTRY -> WAIT_CARD_REMOVED
WAIT_CARD_REMOVED -> COUNT_FUEL_ENTRY      (Karte entfernt)
COUNT_FUEL_ENTRY -> COUNT_FUEL
COUNT_FUEL -> SEND_DATA_ENTRY              (Karte erneut gelesen oder Timeout)
SEND_DATA_ENTRY -> SEND_DATA
SEND_DATA -> SHOW_SUMMARY_ENTRY
SHOW_SUMMARY_ENTRY -> SHOW_SUMMARY
SHOW_SUMMARY -> WAIT_CARD_ENTRY
```

Sonderzustaende:

- `OFFLINE_ENTRY` zeigt einen Offline-/Cache-Fehler an und fuehrt nach
  `OFFLINE`.
- `OFFLINE` bleibt stehen; eine automatische Rueckkehr in den normalen Ablauf
  ist im aktuellen Code auskommentiert.
- `OTA` zeigt waehrend eines OTA-Vorgangs einen Hinweis auf dem LCD.

Ein Tankvorgang laeuft folgendermassen ab:

1. Eine Karte wird erkannt und Aircraft, Suffix und Member-ID werden gelesen.
2. Die Karte muss entfernt werden, bevor die Pumpe freigegeben wird.
3. Das Relais wird aktiviert und der Pulse Counter misst fallende Flanken.
4. Die Menge wird aus den Pulsen und `calibration` berechnet.
5. Eine erneute Kartenlesung oder der konfigurierte Timeout beendet die
   Zapfphase.
6. Die Pumpe wird abgeschaltet, die Nutzlast wird signiert und uebertragen
   oder im Offline-Cache gespeichert.
7. Das LCD zeigt Erfolg, Speicherung oder einen Fehler an.

## 11. Billing-Nutzlast und Uebertragung

Die Nutzlast wird in `ProcessRefueling()` erzeugt. Die Felder sind:

```json
{
  "aircraft": "D-MLIW",
  "article": "MOGAS_MIT",
  "amount": "1.8",
  "date": "22.03.2024",
  "memberid": "858250",
  "auth": "<BASE64-SHA256-HMAC>"
}
```

Der HMAC-SHA256 wird ueber die verkettete Zeichenfolge

```text
aircraft + memberid + amount + article + date
```

berechnet. Als Schluessel dient der aus `billingkey` dekodierte 16-Byte-Key.
Die Menge wird mit einer Nachkommastelle formatiert; das Datum hat das Format
`TT.MM.JJJJ`.

Gesendet wird per HTTPS-POST an:

```text
<billingserver>/<terminal_id>
```

Der Status-Heartbeat wird per HTTPS-POST an folgende Route gesendet:

```text
<billingserver>/<terminal_id>/status
```

Ein HTTP-Status `200` gilt beim Tankvorgang als Erfolg. `403` wird separat als
Authentifizierungs-/Berechtigungsfehler zurueckgegeben. Andere Fehler werden
bis zu zehnmal versucht und danach als Fehler behandelt.

## 12. Offline-Cache und Logs

Beim Start wird LittleFS gemountet und `/rf` angelegt, falls das Verzeichnis
fehlt. Kann ein Tankvorgang nicht direkt gesendet werden, wird die JSON-Datei
unter `/rf/` unter einem zufaelligen Dateinamen gespeichert.

Der Status-Task laeuft im konfigurierten `heartbeat`-Intervall. Sobald der
Billing-Server erreichbar ist, wird jeweils die naechste Cache-Datei gesendet
und nach erfolgreicher Uebertragung geloescht.

Weitere Dateien:

| Datei | Zweck |
| --- | --- |
| `/config.json` | Persistente Terminalkonfiguration |
| `/rf/*` | Noch nicht uebertragene Tankvorgaenge |
| `/log.txt` | Zeitgestempelte Betriebs- und Tankvorgangslogs |
| `/index.html` | Lokale Weboberflaeche |

## 13. Lokale Weboberflaeche und HTTP-Routen

Nach erfolgreicher Wi-Fi-Verbindung ist das Terminal unter seiner IP-Adresse
auf Port 80 erreichbar.

| Route | Methode | Authentifizierung | Funktion |
| --- | --- | --- | --- |
| `/` | GET | Ja | Statusseite und Administration |
| `/health` | GET | Nein | Liefert `OK` |
| `/status` | GET | Nein | JSON mit Backendstatus, Terminalstatus und Cacheanzahl |
| `/reboot` | GET | Ja | Plant einen Neustart |
| `/logout` | GET | Nein | Beendet die Browser-Authentifizierung mit Status 401 |
| `/logged-out` | GET | Nein | Abmeldeseite |
| `/listfiles` | GET | Ja | Listet LittleFS-Dateien |
| `/file?name=...&action=download` | GET | Ja | Datei herunterladen |
| `/file?name=...&action=delete` | GET | Ja | Datei loeschen |
| Upload auf `/` | POST | Ja | Datei in LittleFS hochladen |
| `/update` | ElegantOTA | Ja | Firmware-OTA-Update |

`/status` liefert aktuell kurze Schluessel:

```json
{
  "b": 1,
  "t": 3,
  "c": 0
}
```

Dabei steht `b` fuer die Backend-Verbindung, `t` fuer den numerischen
Terminalzustand und `c` fuer die Anzahl gepufferter Tankvorgaenge.

## 14. CI/CD

`.gitlab-ci.yml` verwendet ein Python-3.9-Image, installiert PlatformIO und
baut nur bei Git-Tags den Release-Zweig. Der Job erzeugt:

- das Firmware-ELF,
- das Firmware-BIN,
- `littlefs.bin`.

Die Dateien werden als GitLab-Artefakte veroeffentlicht. Ein automatisierter
Testjob ist aktuell nicht aktiviert.

## 15. Fehlersuche

| Symptom | Pruefung |
| --- | --- |
| Keine Wi-Fi-Verbindung | SSID/Passwort in `/config.json`, Reichweite und serielle Logs pruefen. Der Start wartet blockierend auf Wi-Fi. |
| Billing offline | `billingserver`, DNS, Root-Zertifikat, Serverroute und `/log.txt` pruefen. Cache unter `/rf/` kontrollieren. |
| Karte wird nicht erkannt | SPI-Pins, SS/GPIO10, Versorgung, MIFARE-Typ, Key A und Kartenformat pruefen. |
| Pumpe startet nicht | GPIO2, Relaislogik, Kartenentfernung und Status `COUNT_FUEL` pruefen. |
| Menge bleibt falsch | GPIO3, fallende Flanken, Kalibrierwert und PCNT-Filter pruefen. |
| LCD bleibt leer | I2C GPIO8/GPIO9, Adresse `0x27` und Versorgung pruefen. |
| OTA nicht erreichbar | Netzwerkroute, HTTP-Zugangsdaten und laufenden Terminalzustand pruefen. |
| Konfiguration wird verworfen | JSON-Syntax pruefen; bei Fehlern werden Defaults geladen und gespeichert. |

Die serielle Ausgabe ist die wichtigste Diagnosequelle. Im Debug-Build werden
zusatzliche Zustands-, RFID-, Cache- und Billing-Meldungen ausgegeben.

## 16. Bekannte technische Einschraenkungen

- Es existieren derzeit keine automatisierten Unit-, Integrations- oder
  Hardware-in-the-loop-Tests.
- `data/config.json.example` ist im Ausgangsstand kein gueltiges JSON.
- `httpapitoken` ist Teil des Datenmodells, wird aber nicht fuer Requests
  verwendet.
- Der Offline-Zustand kehrt nicht automatisch in `WAIT_CARD` zurueck, obwohl
  eine solche Logik im Code vorbereitet, aber auskommentiert ist.
- Der RFID-Hash wird nicht validiert.
- Der HTTP-Administrationsbereich laeuft ohne TLS und verwendet die
  Standard-Basic-Authentication des Webservers.
- Die Firmware nutzt einige feste Puffer und `strcpy`; Karteninhalte muessen
  deshalb die erwarteten Laengen und Nullterminierungen einhalten.
- Der Pumpen-Timeout wird als Anzahl der `loop()`-Durchlaeufe behandelt, nicht
  als exakt gemessene Zeit in Millisekunden.
- Die Benennung und Dokumentation einzelner Entwurfs-Pins in `doc/Pinout.md`
  weicht von der tatsaechlich verwendeten ESP32-S3-Belegung ab. Fuer die
  Verdrahtung sind `include/config.h` und der RFID-Treiber massgeblich.

## 17. Wartung und Erweiterung

Bei Aenderungen sollten mindestens folgende Bereiche gemeinsam geprueft werden:

1. Neue Konfigurationsfelder in `t_Config`, Defaults, Laden und Speichern.
2. Aenderungen am Zustandsautomaten in `src/main.cpp` und die LCD-Anzeigen.
3. Billing-Payload, HMAC-Eingabereihenfolge und Serververtrag.
4. LittleFS-Partition und Offline-Cache bei groesseren Dateien.
5. Board-Pinout, Relais-Sicherheitsverhalten und Hardwaretests.
6. Debug- und Release-Build sowie ein Test mit abgezogener Netzwerkverbindung.

Vor einem Produktiveinsatz sind insbesondere sichere Schluessel, ein eigener
MIFARE-Key, TLS fuer die Administration, Eingabevalidierung und ein
Wiederanlaufkonzept fuer den Offline-Zustand zu klaeren.