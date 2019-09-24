
#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFiMulti.h>
//#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <WebSocketsServer.h>
#define EVENTS 8

ESP8266WiFiMulti WiFiMulti;       // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

ESP8266WebServer server(80);       // create a web server on port 80
WebSocketsServer webSocket(81);    // create a websocket server on port 81

File fsUploadFile;                                    // a File variable to temporarily store the received file

const char *ssid = "IoT Device 14"; // The name of the Wi-Fi network that will be created
const char *password = "iotdevice";   // The password required to connect to it, leave blank for an open network

const char *OTAName = "ESP8266";           // A name and a password for the OTA service
const char *OTAPassword = "esp8266";


const char* mdnsName = "esp8266"; // Domain name for the mDNS responder

bool rainbow = false;             // The rainbow effect is turned off on startup

unsigned long prevMillis = millis();

const char* WiFi_SSID = "";
const char* Password = "";
const char* iotPlatform = "";
const char* EventName = "";
const char* iftttKey = "";
const char* blynkAuthToken = "" ;

bool startBlynk = false;

//////////////////////
// Button Variables //
//////////////////////
int BUTTON_STATE;
int LAST_BUTTON_STATE = HIGH;
long LAST_DEBOUNCE_TIME = 0;
long DEBOUNCE_DELAY = 100;
int BUTTON_COUNTER = 0;
int IDLE_DELAY = 5000;
int lastPressedMillis = 0;


int pushButton = 0;

/*__________________________________________________________JSON FUNCTIONS____________________________________________________________*/

bool loadConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    Serial.println("Failed to parse config file");
    return false;
  }

  WiFi_SSID = json["WiFi_SSID"];
  Password = json["WiFi_Password"];
  iotPlatform = json["platform"];
  EventName = json["Event_Name"];
  iftttKey = json["Ifttt_Key"];
  blynkAuthToken = json["Auth_Token"];


  // Real world application would store these values in some variables for
  // later use.

  Serial.print("Loaded WiFi_SSID: ");
  Serial.println(WiFi_SSID);
  Serial.print("Loaded Password: ");
  Serial.println(Password);
  Serial.print("Loaded iotPlatform: ");
  Serial.println(iotPlatform);
  Serial.print("Loaded EventName: ");
  Serial.println(EventName);
  Serial.print("Loaded iftttKey: ");
  Serial.println(iftttKey);
  Serial.print("Loaded blynkAuthToken: ");
  Serial.println(blynkAuthToken);
  return true;
}

bool saveConfig(uint8_t *  input) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(input);
  /*  ssidName = root["WiFi_SSID"].as<String>();
    passwd = root["Password"].as<String>();
    platform = root["iotPlatform"].as<String>();
    eventname = root["eventName"].as<String>();
    iftttkey = root["iftttKey"].as<String>();
    blynkToken =root["blynkAuthToken"].as<String>();

    /* StaticJsonBuffer<200> jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["WiFi_SSID"] = ssid;
    json["Password"] = passwd;
    json["iotPlatform"] = platform;
    json["eventName"] = eventname;
    json["iftttKey"] = iftttkey;
    json["blynkAuthToken"] = blynkToken;*/

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  root.printTo(configFile);
  return true;
}

/*__________________________________________________________HELPER_FUNCTIONS__________________________________________________________*/

String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
}

String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

/*
  void updateHtml(){
  if (!SPIFFS.begin()) {
      Serial.println("Failed to mount file system");
      //return false;
     }
  if (!loadConfig()) {
      Serial.println("Failed to load config");
      //return false;
     } else {
       Serial.println("Config loaded");}
  Serial.print("Loaded DATA: ");
  Serial.println(WiFi_SSID);
  //Serial.print("Loaded Password: ");
  Serial.println(Password);
  //Serial.print("Loaded iotPlatform: ");
  Serial.println(iotPlatform);
  //Serial.print("Loaded EventName: ");
  Serial.println(EventName);
  //Serial.print("Loaded iftttKey: ");
  Serial.println(iftttKey);
  //Serial.print("Loaded blynkAuthToken: ");
  Serial.println(blynkAuthToken);
  String wifi(WiFi_SSID);
  String passwrd(Password);
  String iot(iotPlatform);
  String event(EventName);
  String key(iftttKey);
  String blynktoken(blynkAuthToken);
  String html = "";

  if(iot == "I"){
    html= "";
   html += "<li>Connected to : " + wifi+ "</li>";
   html += "<li>Password: " + passwrd +"</li>";
   html += "<li>Selected Platform : IFTTT </li>";
   html += "<li>IFTTT Event Name : " + event + "</li>";
   html += "<li>IFTTT Key : " + key+ "</li>";
  }
  if(iot== "B"){
    html = "";
    html += "<li>Connected to : " + wifi + "</li>";
   html += "<li>Password: " + passwrd + "</li>";
   html += "<li>Selected Platform : Blynk </li>";
   html += "<li>IFTTT Event Name : " + blynktoken + "</li>";
  }
  File f = SPIFFS.open("/index.html", "w");
  if (!f) {
    Serial.println("Failed to open config file for writing");
  }
  f.println(html);
  }

*/
/*__________________________________________________________SERVER_HANDLERS__________________________________________________________*/

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  //updateHtml();
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

void handleNotFound() { // if the requested file or page doesn't exist, return a 404 not found error
  if (!handleFileRead(server.uri())) {        // check if the file exists in the flash memory (SPIFFS), if so, send it
    server.send(404, "text/plain", "404: File Not Found");
  }
}


void handleFileUpload() { // upload a new file to the SPIFFS
  HTTPUpload& upload = server.upload();
  String path;
  if (upload.status == UPLOAD_FILE_START) {
    path = upload.filename;
    if (!path.startsWith("/")) path = "/" + path;
    if (!path.endsWith(".gz")) {                         // The file server always prefers a compressed version of a file
      String pathWithGz = path + ".gz";                  // So if an uploaded file is not compressed, the existing compressed
      if (SPIFFS.exists(pathWithGz))                     // version of that file must be deleted (if it exists)
        SPIFFS.remove(pathWithGz);
    }
    Serial.print("handleFileUpload Name: "); Serial.println(path);
    fsUploadFile = SPIFFS.open(path, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    path = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {                                   // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location", "/success.html");     // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

boolean connectToHotspot()
{
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return false;
  }

  if (!loadConfig()) {
    Serial.println("Failed to load config");
    return false;
  } else {
    Serial.println("Config loaded");
    // WiFi.mode(WIFI_STA);
    // We start by connecting to a WiFi network
    WiFiMulti.addAP(WiFi_SSID, Password);

    Serial.println();
    Serial.println();
    Serial.print("Wait for WiFi... ");
    int count = 0;
    while (WiFiMulti.run() != WL_CONNECTED && count < 50) {
      Serial.print(".");
      count++;
      delay(200);
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.macAddress());

    return true;
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        rainbow = false;                  // Turn rainbow off when a new connection is established
      }
      break;
    case WStype_TEXT:                     // if new text data is received
      Serial.printf("[%u] get Text: %s\n", num, payload);
      //String jsonData(payload);
      if (payload[0] == '{') {            // we get JSON data
        Serial.printf("got json data\n");
        Serial.println("Mounting FS...");

        if (!SPIFFS.begin()) {
          Serial.println("Failed to mount file system");
          return;
        }

        if (!saveConfig(payload)) {
          Serial.println("Failed to save config");
        }
        else {
          Serial.println("Config saved");
          if (!SPIFFS.begin()) {
            Serial.println("Failed to mount file system");
            //return false;
          }

          if (!loadConfig()) {
            Serial.println("Failed to load config");
            //return false;
          } else {
            Serial.println("Config loaded");
          }
          Serial.print("Chosen Platform: ");
          Serial.println(iotPlatform);
          String platform(iotPlatform);
          //
          if (platform == "I")
          {

            startBlynk = false;
          }
          if (platform == "B")
          {

            connectToHotspot();
            if (!SPIFFS.begin()) {
              Serial.println("Failed to mount file system");
              //return false;
            }

            if (!loadConfig()) {
              Serial.println("Failed to load config");
              //return false;
            } else {
              Serial.println("Config loaded");
            }
            Serial.print("auth: ");
            Serial.println(blynkAuthToken);
            String token(blynkAuthToken);
            int len = token.length() + 1;
            char auth[len];
            token.toCharArray(auth, len);
            startBlynk = true;
            if ( WiFi.status() == WL_CONNECTED) {
              Serial.println("Connect to blynk");
              Blynk.config(auth);
              while (Blynk.connect() == false) {}
              //Blynk.begin(blynkAuthToken, WiFi_SSID, Password);
            }
          }
        }
      }/* else if (payload[0] == 'R') {                      // the browser sends an R when the rainbow effect is enabled
        rainbow = true;
      } else if (payload[0] == 'N') {                      // the browser sends an N when the rainbow effect is disabled
        rainbow = false;
      }*/
      break;
  }
}



/*__________________________________________________________SETUP_FUNCTIONS__________________________________________________________*/

void startWiFi() { // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  WiFi.softAP(ssid, password);             // Start the access point
  Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.println("\" started\r\n");
  /*
    WiFiMulti.addAP(WiFi_SSID, Password);   // add Wi-Fi networks you want to connect to
    // wifiMulti.addAP("ssid_from_AP_2", "your_password_for_AP_2");
    // wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

    Serial.println("Connecting");
    while (WiFiMulti.run() != WL_CONNECTED && WiFi.softAPgetStationNum() < 1) {  // Wait for the Wi-Fi to connect
      delay(250);
      Serial.print('.');
    }
    Serial.println("\r\n");
    if(WiFi.softAPgetStationNum() == 0) {      // If the ESP is connected to an AP
      Serial.print("Connected to ");
      Serial.println(WiFi.SSID());             // Tell us what network we're connected to
      Serial.print("IP address:\t");
      Serial.print(WiFi.localIP());            // Send the IP address of the ESP8266 to the computer
    } else {                                   // If a station is connected to the ESP SoftAP
      Serial.print("Station connected to ESP8266 AP");
    }*/
  Serial.println("\r\n");
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
}
/*
  void startOTA() { // Start the OTA service
  ArduinoOTA.setHostname(OTAName);
  ArduinoOTA.setPassword(OTAPassword);

  ArduinoOTA.onStart([]() {
    Serial.println("Start");

  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\r\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready\r\n");
  }
*/
void startSPIFFS() { // Start the SPIFFS and list all contents
  SPIFFS.begin();                             // Start the SPI Flash File System (SPIFFS)
  Serial.println("SPIFFS started. Contents:");
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {                      // List the file system contents
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }
}

void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
}

void startMDNS() { // Start the mDNS responder
  MDNS.begin(mdnsName);                        // start the multicast domain name server
  Serial.print("mDNS responder started: http://");
  Serial.print(mdnsName);
  Serial.println(".local");
}

void startServer() { // Start a HTTP server with a file read handler and an upload handler
  server.on("/edit.html",  HTTP_POST, []() {  // If a POST request is sent to the /edit.html address,
    server.send(200, "text/plain", "");
  }, handleFileUpload);                       // go to 'handleFileUpload'

  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
  // and check if the file exists

  server.begin();                             // start the HTTP server
  Serial.println("HTTP server started.");
}

// Debounce Button Presses
boolean debounce() {
  boolean retVal = false;
  int reading = digitalRead(pushButton);
  if (reading != LAST_BUTTON_STATE) {
    LAST_DEBOUNCE_TIME = millis();
  }
  if ((millis() - LAST_DEBOUNCE_TIME) > DEBOUNCE_DELAY) {
    if (reading != BUTTON_STATE) {
      BUTTON_STATE = reading;
      if (BUTTON_STATE == LOW) {
        retVal = true;
      }
    }
  }
  LAST_BUTTON_STATE = reading;
  return retVal;
}

/*__________________________________________________________SETUP__________________________________________________________*/

void setup() {

  Serial.begin(115200);        // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println("\r\n");
  // make the pushbutton's pin an input:
  pinMode(pushButton, INPUT);
  //updateHtml();
  startWiFi();                 // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection

  //startOTA();                  // Start the OTA service

  startSPIFFS();               // Start the SPIFFS and list all contents

  startWebSocket();            // Start a WebSocket server

  startMDNS();                 // Start the mDNS responder

  startServer();               // Start a HTTP server with a file read handler and an upload handler

  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    //return false;
  }

  if (!loadConfig()) {
    Serial.println("Failed to load config");
    //return false;
  } else {
    Serial.println("Config loaded");
  }
  Serial.print("Loaded WiFi_SSID: ");
  Serial.println(WiFi_SSID);
  Serial.print("Loaded Password: ");
  Serial.println(Password);
  Serial.print("Loaded iotPlatform: ");
  Serial.println(iotPlatform);
  Serial.print("Loaded EventName: ");
  Serial.println(EventName);
  Serial.print("Loaded iftttKey: ");
  Serial.println(iftttKey);
  Serial.print("Loaded blynkAuthToken: ");
  Serial.println(blynkAuthToken);
  String iotplatform(iotPlatform);
  if (iotplatform == "I")
  {

    startBlynk = false;
  }
  if (iotplatform == "B")
  {

    connectToHotspot();
    if (!SPIFFS.begin()) {
      Serial.println("Failed to mount file system");
      //return false;
    }

    if (!loadConfig()) {
      Serial.println("Failed to load config");
      //return false;
    } else {
      Serial.println("Config loaded");
    }
    Serial.print("auth: ");
    Serial.println(blynkAuthToken);
    String atoken(blynkAuthToken);
    int tokenlen = atoken.length() + 1;
    char authToken[tokenlen];
    atoken.toCharArray(authToken, tokenlen);
    startBlynk = true;
    if ( WiFi.status() == WL_CONNECTED) {
      Serial.println("Connect to blynk");
      Blynk.config(authToken);
      while (Blynk.connect() == false) {}
      //Blynk.begin(blynkAuthToken, WiFi_SSID, Password);
    }
  }
}


/*_________________________________________________________Blynk Code_____________________________________________________*/


// This function sends Arduino's up time every second to Virtual Pin (5).
// In the app, Widget's reading frequency should be set to PUSH. This means
// that you define how often to send data to Blynk App.


/*__________________________________________________________LOOP__________________________________________________________*/


void loop() {
  int buttonState = digitalRead(pushButton);
  webSocket.loop();                           // constantly check for websocket events
  server.handleClient();                      // run the server
  //ArduinoOTA.handle();                        // listen for OTA events

  if (startBlynk == true)
  {
    //Serial.println("Blynk code running");
    Blynk.run();
  }
  if (startBlynk == false )
  {
    // Define the WiFi Client
    WiFiClient client;

    // Wait for button Presses
    boolean pressed = debounce();
    if (pressed == true)
    {
      if (BUTTON_COUNTER == EVENTS) // This will make the Button Counter roll back to 1
      {
        BUTTON_COUNTER = 0;
      }
      BUTTON_COUNTER++;
      Serial.print(BUTTON_COUNTER);

      lastPressedMillis = millis();

    }
    if ((millis() - lastPressedMillis) > IDLE_DELAY && lastPressedMillis > 0) //Wait for " IDLE_DELAY" Time to ensure person has clicked switch the right number of times
    {
      lastPressedMillis = 0;

      connectToHotspot();
      if (!SPIFFS.begin()) {
        Serial.println("Failed to mount file system");
        //return false;
      }

      if (!loadConfig()) {
        Serial.println("Failed to load config");
        //return false;
      } else {
        Serial.println("Config loaded");
      }
      Serial.print("event name: ");
      Serial.println(EventName);
      Serial.print("Key: ");
      Serial.println(iftttKey);
      String event_name(EventName);
      String count = String(BUTTON_COUNTER);
      event_name += "_";
      event_name += count;
      String key(iftttKey);
      Serial.println("IFTTT code running");
      Serial.print("Trigger" + String(event_name) + " Event Pressed ");
      Serial.print(BUTTON_COUNTER);
      Serial.println(" times");
      // Set the http Port
      const int httpPort = 80;
      const char* IFTTT_URL = "maker.ifttt.com";
      // Make sure we can connect
      if (!client.connect(IFTTT_URL, httpPort))
      {
        return;
      }

      // We now create a URI for the request
      // String url = "/trigger/" + String(eventName) + "-"+String(eventCount)+ "/with/key/" + String(IFTTT_KEY);
      String url = "/trigger/" + String(event_name) + "/with/key/" + String(key);

      // Set some values for the JSON data depending on which event has been triggered
      IPAddress ip = WiFi.localIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      String value_1 = String(BUTTON_COUNTER);
      //String value_2 = String(h);
      //String value_3 = "";



      // Build JSON data string
      String data = "";
      data = data + "\n" + "{\"value1\":\"" + value_1 + "\"}";
      Serial.print("Send request");
      // Post the button press to IFTTT
      if (client.connect(IFTTT_URL, httpPort))
      {

        // Sent HTTP POST Request with JSON data
        client.println("POST " + url + " HTTP/1.1");
        Serial.println("POST " + url + " HTTP/1.1");
        client.println("Host: " + String(IFTTT_URL));
        Serial.println("Host: " + String(IFTTT_URL));
        client.println("User-Agent: Arduino/1.0");
        Serial.println("User-Agent: Arduino/1.0");
        client.print("Accept: *");
        Serial.print("Accept: *");
        client.print("/");
        Serial.print("/");
        client.println("*");
        Serial.println("*");
        client.print("Content-Length: ");
        Serial.print("Content-Length: ");
        client.println(data.length());
        Serial.println(data.length());
        client.println("Content-Type: application/json");
        Serial.println("Content-Type: application/json");
        client.println("Connection: close");
        Serial.println("Connection: close");
        client.println();
        Serial.println();
        client.println(data);
        Serial.println(data);


      }
      BUTTON_COUNTER = 0;

    }
  }
}
