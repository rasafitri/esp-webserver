/**************************************************************************
    Verwendung von ESP32 / ESP8266 als Web Server zum Hochladen von Bildern
    und Texten, die auf der HUB75 LED-Matrixanzeige angezeigt werden.
 **************************************************************************/

// ----------------------------
// Standard Libs
// ----------------------------
#ifdef ESP32
#include <WiFi.h>
// WLAN Verbindung für den Server
#include <WebServer.h>
// ESP32 WebServer
#endif

#ifdef ESP8266
#include <ESP8266WiFi.h>
// WLAN Verbindung für den Server
#include <ESP8266WebServer.h>
// ESP8266 WebServer
#endif

// ----------------------------
// Additional Libs
// ----------------------------
#include <ArduinoJson.h>
// JSON Format
#include <PxMatrix.h>
// Steuerung der Anzeige
#include "webclient.h"
// selbsterstellte WebClient Seite https://github.com/rasafitri/webclient-upload

// ----------------------------------------
// Einstellungen der LED-Matrixanzeigetafel
// ----------------------------------------
#define matrix_width 64   // Breite der Anzeige, anzupassen
#define matrix_height 32  // Höhe der Anzeige, anzupassen

// Pin Belegungen der Anzeige, anzupassen
#ifdef ESP32
#define P_A 19
#define P_B 23
#define P_C 18
#define P_D 5
#define P_E -1  // anzupassen bei Verbindung
#define P_LAT 22
#define P_OE 16
#endif

#ifdef ESP8266
#define P_A 5
#define P_B 4
#define P_C 15
#define P_D 12
#define P_E -1  // anzupassen bei Verbindung
#define P_LAT 16
#define P_OE 2
#endif

PxMATRIX display(matrix_width, matrix_height, P_LAT, P_OE, P_A, P_B, P_C, P_D, P_E);

// die Helligkeit der Anzeige, 30-70, je nach Bedarf anpassen
uint8_t display_draw_time = 60;
unsigned long scroll_speed = 50;  // Geschwindigkeit des Scrollens

// ----------------------------------------
// Timer für Callbacks
// ----------------------------------------
#ifdef ESP32
hw_timer_t* timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
#endif
#ifdef ESP8266
#include <Ticker.h>
Ticker display_ticker;
#endif

// ----------------------------
// WLAN-Einstellung
// ----------------------------
const char* ssid = "your-ssid";        // WLAN ssid, anzupassen
const char* password = "your-password";  // WLAN Password, anzupassen

// ----------------------------
// WebServer-Einstellung
// ----------------------------
const char* www_username = "admin";      // Benutzername für Login, anzupassen
const char* www_password = "esp32";      // Passwort für Login, anzupassen
const char* hostname = "myesp32server";  // Die erreichbare Adresse des Servers
WebServer server(80);                    // Server Port 80

// ----------------------------
// Hilfsvariablen
// ----------------------------
char buffer[50];
const char* http = "http://";

bool startTextLoop = false;  // scroll text als loop?
String scroll_text = "";     // Text für scroll_text in loop()
uint16_t text_color = 0;     // Farbe für scroll_text in loop()

bool isGif = false;           // ist gif Endpunkt?
bool startImageLoop = false;  // zeige Bilder in loop?
JsonArray frames;             // Array der Bilder
JsonArray delays;             // Array der Frame Delays
uint16_t frameDelay;          // Frame Delay von movingimages Endpunkt

// ----------------------------------------
// Funktionen für Anzeige Update
// ----------------------------------------
#ifdef ESP32
void IRAM_ATTR display_updater() {
  portENTER_CRITICAL_ISR(&timerMux);
  display.display(display_draw_time);
  portEXIT_CRITICAL_ISR(&timerMux);
}
#endif
#ifdef ESP8266
void display_updater() {
  display.display(display_draw_time);
}
#endif

void display_update_enable(bool is_enable) {
#ifdef ESP32
  if (is_enable) {
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &display_updater, true);
    timerAlarmWrite(timer, 4000, true);
    timerAlarmEnable(timer);
  } else {
    timerDetachInterrupt(timer);
    timerAlarmDisable(timer);
  }
#endif
#ifdef ESP8266
  if (is_enable)
    display_ticker.attach(0.004, display_updater);
  else
    display_ticker.detach();
#endif
}

// ----------------------------
// Event-Handler WebServer
// ----------------------------
// root endpoint, zeigt WebClient
void handleRoot() {
  // sende die html Seite zum Client beim Aufruf des Servers
  server.send(200, "text/html", htmlPage);
  Serial.println("HTML Seite geöffnet");
}

// /text endpoint für POST Text, Text Behandlung & Anzeige
void handleText() {
  if (server.method() != HTTP_POST) {
    // keine gültige Methode, sende HTTP Response 405
    server.send(405, "text/plain", "Method Not Allowed");
    Serial.println("Method not allowed");
  } else {
    // ist eine POST Request, Text-Einstellung in JSON Format entgegennehmen
    Serial.println("Text-Einstellung entgegengenommen");

#ifdef ESP32
    DynamicJsonDocument jsonDoc(ESP.getMaxAllocHeap());
#endif
#ifdef ESP8266
    DynamicJsonDocument jsonDoc(ESP.getMaxFreeBlockSize() - 512);
#endif
    // Deserialization des Requests
    DeserializationError error = deserializeJson(jsonDoc, server.arg("plain"));

    switch (error.code()) {
      case DeserializationError::Ok:
        Serial.println("Deserialization erfolgreich");
        break;
      case DeserializationError::EmptyInput:
        server.send(400, "text/plain", "Empty Input");
        Serial.println("Empty Input");
        return;
      case DeserializationError::IncompleteInput:
        server.send(400, "text/plain", "Incomplete Input");
        Serial.println("Incomplete Input");
        return;
      case DeserializationError::InvalidInput:
        server.send(400, "text/plain", "JSON Invalid");
        Serial.println("JSON Invalid");
        return;
      case DeserializationError::NoMemory:
        server.send(413, "text/plain", "No Memory");
        Serial.println("kein Memory");
        return;
      case DeserializationError::TooDeep:
        server.send(413, "text/plain", "Too Deep");
        Serial.println("Too Deep");
        return;
      default:
        server.send(400, "text/plain", "Deserialization failed");
        Serial.println("Deserialization Fehler");
        return;
    }

    // Request Format
    // value: "eingegebener Text",
    // color: [R,G,B],
    // mode: "scroll" ODER mode: "static"

    JsonObject root = jsonDoc.as<JsonObject>();
    String text = root["value"].as<String>();
    JsonArray color = root["color"].as<JsonArray>();
    uint8_t r = static_cast<uint8_t>(color[0]);
    uint8_t g = static_cast<uint8_t>(color[1]);
    uint8_t b = static_cast<uint8_t>(color[2]);
    String mode = root["mode"].as<String>();

    if (mode == "scroll") {
      // Lauftext ausgewählt --> sende Rückmeldung, dass der Lauftext verarbeitet wurde
      server.send(200, "text/plain", "Lauftext \"" + text + "\" erfolgreich verarbeitet!");
      Serial.println("Lauftext verarbeitet");

      startImageLoop = false;
      startTextLoop = true;
      scroll_text = text;
      text_color = display.color565(r, g, b);
    } else {
      // sende Rückmeldung, dass der Text verarbeitet wurde
      server.send(200, "text/plain", "Text \"" + text + "\" erfolgreich verarbeitet!");
      Serial.println("Text verarbeitet");
      // stehender Text in der ausgewählten Farbe
      startImageLoop = false;
      startTextLoop = false;
      drawText(text, display.color565(r, g, b));
    }
  }
}

// /image endpoint, Bild Behandlung & Anzeige
void handleImage() {
  if (server.method() != HTTP_POST) {
    // keine gültige Methode, sende HTTP Response 405
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    // ist eine POST Request, Bild in JSON Format entgegennehmen
    Serial.println("Bild entgegengenommen");

#ifdef ESP32
    DynamicJsonDocument jsonDoc(ESP.getMaxAllocHeap());
#endif
#ifdef ESP8266
    DynamicJsonDocument jsonDoc(ESP.getMaxFreeBlockSize() - 512);
#endif
    // Deserialization des Requests
    DeserializationError error = deserializeJson(jsonDoc, server.arg("plain"));

    switch (error.code()) {
      case DeserializationError::Ok:
        Serial.println("Deserialization erfolgreich");
        break;
      case DeserializationError::EmptyInput:
        server.send(400, "text/plain", "Empty Input");
        Serial.println("Empty Input");
        return;
      case DeserializationError::IncompleteInput:
        server.send(400, "text/plain", "Incomplete Input");
        Serial.println("Incomplete Input");
        return;
      case DeserializationError::InvalidInput:
        server.send(400, "text/plain", "JSON Invalid");
        Serial.println("JSON Invalid");
        return;
      case DeserializationError::NoMemory:
        server.send(413, "text/plain", "No Memory");
        Serial.println("kein Memory");
        return;
      case DeserializationError::TooDeep:
        server.send(413, "text/plain", "Too Deep");
        Serial.println("Too Deep");
        return;
      default:
        server.send(400, "text/plain", "Deserialization failed");
        Serial.println("Deserialization Fehler");
        return;
    }

    // Request Format
    // hexValues: [SkaliertesBildAlsCCodeArray],
    // size: [imgWidth, imgHeight]

    JsonObject root = jsonDoc.as<JsonObject>();
    JsonArray imgSize = root["size"].as<JsonArray>();
    JsonArray imgValues = root["hexValues"].as<JsonArray>();
    int valuesCount = imgValues.size();

    // CCodeArray als uint16_t umwandeln
    uint16_t sentImage[valuesCount];
    for (int i = 0; i < valuesCount; i++) {
      sentImage[i] = strtol(imgValues[i].as<String>().c_str(), NULL, 0);
    }

    // sende Rückmeldung, dass das Bild verarbeitet wurde
    server.send(200, "text/plain", "Bild wurde erfolgreich verarbeitet!");
    Serial.println("Bild verarbeitet");
    startTextLoop = false;
    startImageLoop = false;
    // Bild anzeigen in der richtigen Größe
    drawImage(sentImage, strtol(imgSize[0].as<String>().c_str(), NULL, 0), strtol(imgSize[1].as<String>().c_str(), NULL, 0));
  }
}

// /gif endpoint, Bild Behandlung & Anzeige
void handleGif() {
  if (server.method() != HTTP_POST) {
    // keine gültige Methode, sende HTTP Response 405
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    // ist eine POST Request, Bild in JSON Format entgegennehmen
    Serial.println("Gif entgegengenommen");

#ifdef ESP32
    DynamicJsonDocument jsonDoc(ESP.getMaxAllocHeap());
#endif
#ifdef ESP8266
    DynamicJsonDocument jsonDoc(ESP.getMaxFreeBlockSize() - 512);
#endif
    // Deserialization des Requests
    DeserializationError error = deserializeJson(jsonDoc, server.arg("plain"));

    switch (error.code()) {
      case DeserializationError::Ok:
        Serial.println("Deserialization erfolgreich");
        break;
      case DeserializationError::EmptyInput:
        server.send(400, "text/plain", "Empty Input");
        Serial.println("Empty Input");
        return;
      case DeserializationError::IncompleteInput:
        server.send(400, "text/plain", "Incomplete Input");
        Serial.println("Incomplete Input");
        return;
      case DeserializationError::InvalidInput:
        server.send(400, "text/plain", "JSON Invalid");
        Serial.println("JSON Invalid");
        return;
      case DeserializationError::NoMemory:
        server.send(413, "text/plain", "No Memory");
        Serial.println("kein Memory");
        return;
      case DeserializationError::TooDeep:
        server.send(413, "text/plain", "Too Deep");
        Serial.println("Too Deep");
        return;
      default:
        server.send(400, "text/plain", "Deserialization failed");
        Serial.println("Deserialization Fehler");
        return;
    }

    // Request Format
    // delays : [arrayOfDelays]
    // frames : [size, hexValues]

    JsonObject root = jsonDoc.as<JsonObject>();
    delays = root["delays"].as<JsonArray>();
    frames = root["frames"].as<JsonArray>();

    server.send(200, "text/plain", "Bild wurde erfolgreich verarbeitet!");

    isGif = true;
    startTextLoop = false;
    startImageLoop = true;
  }
}

// /movingimages endpoint, Bild Behandlung & Anzeige
void handleMovingImg() {
  if (server.method() != HTTP_POST) {
    // keine gültige Methode, sende HTTP Response 405
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    // ist eine POST Request, Bild in JSON Format entgegennehmen
    Serial.println("Gif entgegengenommen");

#ifdef ESP32
    DynamicJsonDocument jsonDoc(ESP.getMaxAllocHeap());
#endif
#ifdef ESP8266
    DynamicJsonDocument jsonDoc(ESP.getMaxFreeBlockSize() - 512);
#endif
    // Deserialization des Requests
    DeserializationError error = deserializeJson(jsonDoc, server.arg("plain"));

    switch (error.code()) {
      case DeserializationError::Ok:
        Serial.println("Deserialization erfolgreich");
        break;
      case DeserializationError::EmptyInput:
        server.send(400, "text/plain", "Empty Input");
        Serial.println("Empty Input");
        return;
      case DeserializationError::IncompleteInput:
        server.send(400, "text/plain", "Incomplete Input");
        Serial.println("Incomplete Input");
        return;
      case DeserializationError::InvalidInput:
        server.send(400, "text/plain", "JSON Invalid");
        Serial.println("JSON Invalid");
        return;
      case DeserializationError::NoMemory:
        server.send(413, "text/plain", "No Memory");
        Serial.println("kein Memory");
        return;
      case DeserializationError::TooDeep:
        server.send(413, "text/plain", "Too Deep");
        Serial.println("Too Deep");
        return;
      default:
        server.send(400, "text/plain", "Deserialization failed");
        Serial.println("Deserialization Fehler");
        return;
    }

    // Request Format
    // delay : delay value
    // frames : [size, hexValues]    

    JsonObject root = jsonDoc.as<JsonObject>();
    frameDelay = root["delay"].as<uint16_t>();
    frames = root["images"].as<JsonArray>();

    server.send(200, "text/plain", "Bild wurde erfolgreich verarbeitet!");

    isGif = false;
    startTextLoop = false;
    startImageLoop = true;
  }
}

// /size endpoint für GET Displaygröße
void handleSize() {
  if (server.method() != HTTP_GET) {
    // keine gültige Methode, sende HTTP Response 405
    server.send(405, "text/plain", "Method Not Allowed");
    Serial.println("Method not allowed");
  } else {
    // ist eine GET Request, sende die einprogrammierte Displaygröße an Client
    // als JSON Array in der Format size: [width, height]
    StaticJsonDocument<200> doc;
    JsonArray sizeArray = doc.createNestedArray("size");
    sizeArray.add(matrix_width);
    sizeArray.add(matrix_height);
    String jsonString;
    serializeJson(doc, jsonString);
    server.send(200, "application/json", jsonString);
    Serial.println(jsonString + " an Client gesendet");
  }
}

// keine gültige / bekannte endpoints
void handleNotFound() {
  // sende 404 File Not Found
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) { message += " " + server.argName(i) + ": " + server.arg(i) + "\n"; }
  server.send(404, "text/plain", message);
  Serial.println("404 File Not Found");
}

// ----------------------------
// LED-Matrixanzeige Funktionen
// ----------------------------
// zeige einmalig einen Lauftext in bestimmter Farbe
void scrollText(String text, uint16_t colorRGB) {
  uint16_t text_length = text.length();  // Länge des Texts für die Berechnung

  // Einstellung der Display
  display.setTextWrap(false);      // Lauftext --> kein TextWrap nötig
  display.setTextSize(1);          // Standardgröße, 8px
  display.setRotation(0);          // keine Rotation
  display.setTextColor(colorRGB);  // Farbe des Texts wie angegeben

  // Schritt für Schritt Text Anzeigen
  for (int xpos = matrix_width; xpos > -(matrix_width + text_length * 5); xpos--) {
    display.clearDisplay();      // immer Anzeige zurücksetzen, bevor etwas Neues angezeigt wird
    display.setCursor(xpos, 0);  // Setze den Text ganz oben (ypos=0), aber bei variablen xpos
    display.println(text);
    delay(scroll_speed);
    yield();

    delay(scroll_speed / 5);
    yield();
  }
}

// zeige den stehenden Text in bestimmter Farbe
void drawText(String text, uint16_t colorRGB) {
  display.clearDisplay();          // immer Anzeige zurücksetzen, bevor etwas Neues angezeigt wird
  display.setTextWrap(true);       // stehender Text --> TextWrap nötig für längere Texte
  display.setTextSize(1);          // Standardgröße, 8px
  display.setRotation(0);          // keine Rotation
  display.setCursor(0, 0);         // Text fängt bei der Position oben links
  display.setTextColor(colorRGB);  // Farbe des Texts wie angegeben
  display.println(text);
}

// zeige das skalierte Bild
void drawImage(uint16_t image[], int imageWidth, int imageHeight) {
  display.clearDisplay();  // immer Anzeige zurücksetzen, bevor etwas Neues angezeigt wird
  int counter = 0;
  for (int y = 0; y < imageHeight; y++) {
    for (int x = 0; x < imageWidth; x++) {
      display.drawPixel(x, y, image[counter]);
      counter++;
    }
  }
}

// ----------------------------
// ESP Setup
// ----------------------------
void setup() {
  Serial.begin(115200);  // Serial baudrate 115200

  // WLAN Verbindung einstellen
  WiFi.setHostname(hostname);  // Setze den Namen des Servers
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);  // WLAN Verbindung aufbauen
  Serial.print("Verbinden mit ");
  Serial.println(ssid);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    // WLAN nicht verbunden
    Serial.println("Verbindung fehlgeschlagen! Reboot...");
    delay(1000);
    ESP.restart();
  }
  Serial.println("WLAN Verbunden");

  // LED-Matrixanzeige einstellen
  display.begin(16);  // display rows-scan pattern 1/16
  display.setFastUpdate(true);
  display.clearDisplay();  // immer Anzeige zurücksetzen, bevor etwas Neues angezeigt wird
  display_update_enable(true);

  // handle alle Endpunkte
  // server handle root endpoint
  server.on("/", []() {
    // mit Authentifizierung für Login mit Benutzername und Password
    if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
    // authentifizierte Nutzer dürfen die HTML Seite sehen
    handleRoot();
  });

  server.on("/text", []() {
    // mit Authentifizierung für Login mit Benutzername und Password
    if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
    // server handle text endpoint
    handleText();
  });

  server.on("/image", []() {
    // mit Authentifizierung für Login mit Benutzername und Password
    if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
    // server handle image endpoint
    handleImage();
  });

  server.on("/size", []() {
    // mit Authentifizierung für Login mit Benutzername und Password
    if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
    // server handle size endpoint
    handleSize();
  });

  server.on("/gif", []() {
    // mit Authentifizierung für Login mit Benutzername und Password
    if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
    // server handle gif endpoint
    handleGif();
  });

  server.on("/movingimages", []() {
    // mit Authentifizierung für Login mit Benutzername und Password
    if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
    // server handle movingimages endpoint
    handleMovingImg();
  });

  server.onNotFound(handleNotFound);  // server handle not found endpoint
  server.begin();                     // Serverstart
  Serial.println("Server started!");

  // zeige die Adresse des Servers auf der Anzeige in blau
  strcpy(buffer, http);
  strcat(buffer, WiFi.getHostname());
  display.setTextWrap(true);
  display.setCursor(0, 0);
  display.setTextColor(display.color565(0, 0, 255));
  display.println(buffer);
}

// ---------------------------------------
// Server handle Client in einer Schleife
// ---------------------------------------
void loop() {
  server.handleClient();

  if (startTextLoop) {
    // Lauftext in der ausgewählten Farbe
    scrollText(scroll_text, text_color);
  }

  if (startImageLoop) {
    for (int j = 0; j < frames.size(); j++) {
      JsonArray sizeArray = frames[j]["size"];
      int width = strtol(sizeArray[0].as<String>().c_str(), NULL, 0);
      int height = strtol(sizeArray[1].as<String>().c_str(), NULL, 0);

      JsonArray image = frames[j]["hexValues"];
      int valuesCount = image.size();
      // CCodeArray als uint16_t umwandeln
      uint16_t sentImage[valuesCount];
      for (int i = 0; i < valuesCount; i++) {
        sentImage[i] = strtol(image[i].as<String>().c_str(), NULL, 0);
      }

      drawImage(sentImage, width, height);

      if (isGif) {
        delay(delays[j]);
      } else {
        delay(frameDelay);
      }
    }
  }
}