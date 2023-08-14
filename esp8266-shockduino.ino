// ESP8266 to provide a webserver to control a shock collar
//
// The core code to send to the collar is based on 
//  https://github.com/smouldery/shock-collar-control/blob/master/Arduino%20Modules/transmitter_vars.ino
//
// I've taken this and just kludged it, and then put an ESP8266 web interface
// in front.
//
// The NOTES file explains how the data needs to be sent.

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include "html.h"
#include "version.h"

#define default_pin D0

// Values to store in EEPROM
//  UI username
//  UI password
//  WiFi SSD
//  WiFi password
//  Collar name (for mDNS)
//  Transmitter key
//  What pin the transmitter is connected to

#define EEPROM_SIZE 1024
#define maxlen 100
#define eeprom_magic "SHOCK:"
#define eeprom_magic_len 6

#define ui_username_offset   0
#define ui_pswd_offset       128
#define ui_wifi_ssid_offset  256
#define ui_wifi_pswd_offset  384
#define transmitter_offset   512
#define collarname_offset    640
#define pin_offset           768

// These can be read at startup time from EEPROM
String ui_username;
String ui_pswd;
String wifi_ssid;
String wifi_pswd;
String transmitter_key; // We store as decimal but convert to binary when used
String collarname;
String pinstr;
int    pin;

// Do we have WiFi?
boolean wifi_connected;

// Last time data was sent to the collar
time_t transmit_last = 0;

// Create the webserver structure for port 80
ESP8266WebServer server(80);

/////////////////////////////////////////

void do_log(String s)
{
  Serial.print(time(NULL));
  Serial.print(": ");
  Serial.println(s);
}

/////////////////////////////////////////

// Read/write EEPROM values

String get_eeprom(int offset)
{ 
  char d[maxlen];
  String val;

  for (int i=0; i<maxlen; i++)
  { 
    d[i]=EEPROM.read(offset+i);
  }

  val=String(d);
  
  if (val.startsWith(eeprom_magic))
  { 
    val=val.substring(eeprom_magic_len);
  }
  else
  {
    val="";
  }

  return val;
}

void set_eeprom(String s, int offset, bool commit=true)
{ 
  String val=eeprom_magic + s;

  for(int i=0; i < val.length(); i++)
  {
    EEPROM.write(offset+i,val[i]);
  }
  EEPROM.write(offset+val.length(),0);
  if (commit)
    EEPROM.commit();
}

/////////////////////////////////////////

void send_text(String s)
{
  server.send(200,"text/html", s);
}

void set_ap()
{
  if (server.hasArg("setwifi"))
  {
    do_log("Setting WiFi client");
    collarname=server.arg("collarname");
    collarname.replace(".local","");
    if (collarname != "")
    {
      do_log("  Setting mDNS name to "+collarname);
      set_eeprom(collarname,collarname_offset);
    }

    pinstr=server.arg("pin");
    if (pinstr != "")
    {
      do_log("  Setting active pin to "+pinstr);
      set_eeprom(pinstr,pin_offset);
    }

    String key=server.arg("key");
    if (key != transmitter_key)
    {
      do_log("Setting transmitter key");
      set_eeprom(key,transmitter_offset);
    }

    if (server.arg("ssid") != "" && server.arg("password") != "")
    {
      do_log("Setting WiFi client:");
      set_eeprom(server.arg("ssid"),ui_wifi_ssid_offset,false);
      set_eeprom(server.arg("password"),ui_wifi_pswd_offset);
    }

    send_text("Restarting in 5 seconds");
    delay(5000);
    ESP.restart();
  }

  String page = change_ap_html;
         page.replace("##collarname##", collarname);
         page.replace("##pin##", String(pin));
         page.replace("##key##", transmitter_key);
         page.replace("##VERSION##", VERSION);
  send_text(page);
}

void set_auth()
{
  do_log("Setting Auth details");
  ui_username=server.arg("username");
  ui_pswd=server.arg("password");
  set_eeprom(ui_username,ui_username_offset);
  set_eeprom(ui_pswd,ui_pswd_offset);
  send_text("Password reset");
}

/////////////////////////////////////////

String dec_to_bin(String value, int digits)
{
  char res[digits+1];
  int val=value.toInt();
  int i;
  for (i=digits-1;i>=0;i--)
  {
    res[i]= (val & 1)?'1':'0';
    val = val>>1;
  }
  res[digits]='\0';
  return String(res);
}

void tx(String bitstring,int durn)
{
  send_text(bitstring);
  time(&transmit_last);

  // Ensure the pin state is set
  pinMode(pin,OUTPUT);
  digitalWrite(pin,LOW);

  // Turn on the LED
  digitalWrite(LED_BUILTIN,LOW);

  time_t cmd_start=time(NULL);
  while (time(NULL)-cmd_start < durn)
  {
    // start bit
    digitalWrite(pin, HIGH);
    delayMicroseconds(1500); // wait 1500 uS
    digitalWrite(pin, LOW);
    delayMicroseconds(741);// wait 741 uS

    for (int n = 0; n < 41 ; n++)
    {
      if (bitstring.charAt(n) == '1') // Transmit a one
      {
        digitalWrite(pin, HIGH);
        delayMicroseconds(741);
        digitalWrite(pin, LOW);
        delayMicroseconds(247);
      }
      else // Transmit a zero
      {
        digitalWrite(pin, HIGH);
        delayMicroseconds(247);
        digitalWrite(pin, LOW);
        delayMicroseconds(741);
      }
    }
    yield();
    delayMicroseconds(4500);
    yield();
  }
  digitalWrite(LED_BUILTIN,HIGH);
}

void build_and_send(String cmd,String cmdrev,String pow,String durn)
{
  String start="1",
         channel="111", channelrev="000",
         key=dec_to_bin(transmitter_key,17),
         power=dec_to_bin(pow,7),
         end="00";

  String to_send=start+channel+cmd+key+power+cmdrev+channelrev+end;
  
  do_log("Sending start=" + start + ", channel=" + channel + ", cmd=" +cmd + ", key=" + key + ", power=" + power + ", cmdrev=" + cmdrev + ", channelrev=" + channelrev + ", end=" + end);
  
  tx(to_send,durn.toInt());
}

void send()
{
  if (server.arg("vibrate") != "")
    build_and_send("0010","1011",server.arg("v_str"),server.arg("v_dur"));
  else if (server.arg("shock") != "")
    build_and_send("0001","0111",server.arg("s_str"),server.arg("s_dur"));
  else if (server.arg("beep") != "")
    build_and_send("0100","1101","100","1");
  else
    send_text("Unknown to send!");
}

/////////////////////////////////////////

boolean handleRequest()
{ 
  String path=server.uri();
  if (!wifi_connected)
  {
    // If we're in AP mode then all requests must go to change_ap
    // and there's no authn required
    ui_username = "";
    ui_pswd = "";
    path="/change_ap.html";
  }

  do_log("New client for >>>"+path+"<<<");

  for(int i=0;i<server.args();i++)
  {
    do_log("Arg " + String(i) + ": " + server.argName(i) + " --- " + server.arg(i));
  }

  // Ensure username/password have been passed
  if (ui_username != "" && !server.authenticate(ui_username.c_str(), ui_pswd.c_str()))
  {
    do_log("Bad authentication; login needed");
    server.requestAuthentication();
    return true;
  }

       if (path == "/")                  { send_text(index_html); }
  else if (path == "/top_frame.html")    { send_text(top_frame_html); }
  else if (path == "/bottom_frame.html") { send_text(bottom_frame_html); }
  else if (path == "/change_auth.html")  { send_text(change_auth_html); }
  else if (path == "/change_ap.html")    { set_ap(); }
  else if (path == "/setauth/")          { set_auth(); }
  else if (path == "/send/")             { send(); }
  else
  {
    do_log("File not found: " + path);
    return false;
  }
  return true;
}

/////////////////////////////////////////

void setup()
{
  Serial.begin(115200);
  delay(500);

  do_log("Starting...");

  // Get the EEPROM contents into RAM
  EEPROM.begin(EEPROM_SIZE);

  do_log("Getting values from EEPROM");

  // Try reading the values from the EEPROM
  ui_username = get_eeprom(ui_username_offset);
  ui_pswd     = get_eeprom(ui_pswd_offset);
  wifi_ssid   = get_eeprom(ui_wifi_ssid_offset);
  wifi_pswd   = get_eeprom(ui_wifi_pswd_offset);
  transmitter_key = get_eeprom(transmitter_offset);
  collarname  = get_eeprom(collarname_offset);
  pinstr      = get_eeprom(pin_offset);

  if (collarname=="")
    collarname="shockcollar";

  if (pinstr != "")
    pin=pinstr.toInt();
  else
    pin=default_pin;

  if (transmitter_key == "")
    transmitter_key = "22858";

  // Debugging lines
  do_log("Found in EEPROM:");
  do_log("  UI Username     >>>"+ ui_username + "<<<");
  do_log("  UI Password     >>>"+ ui_pswd + "<<<");
  do_log("  Wifi SSID       >>>"+ wifi_ssid + "<<<");
  do_log("  Wifi Pswd       >>>"+ wifi_pswd + "<<<");
  do_log("  Collar name     >>>"+ collarname + "<<<");
  do_log("  Transmitter key >>>"+ transmitter_key + "<<<");
  do_log("  Transmitter Pin >>>"+ String(pin) + "<<<");

  // Set the pin state
  pinMode(pin,OUTPUT);
  digitalWrite(pin,LOW);

  // Ensure LED is off
  pinMode(LED_BUILTIN,OUTPUT);
  digitalWrite(LED_BUILTIN,HIGH);

  // Connect to the network
  Serial.println();
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  wifi_connected=false;
  if (wifi_ssid != "")
  {
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_pswd);
    Serial.print("Connecting to ");
    Serial.print(wifi_ssid); Serial.println(" ...");

    // Wait for the Wi-Fi to connect.  Give up after 60 seconds and
    // let us fall into AP mode
    int i = 0;
    while (WiFi.status() != WL_CONNECTED && i < 60)
    {
      Serial.print(++i); Serial.print(' ');
      delay(1000);
    }
    Serial.println('\n');
    wifi_connected = (WiFi.status() == WL_CONNECTED);
  }

  if (wifi_connected)
  {
    do_log("Connection established!");  
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());
    Serial.print("Hostname:\t");
    Serial.println(WiFi.hostname());
  }
  else
  {
    // Create an Access Point that mobile devices can connect to
    unsigned char mac[6];
    char macstr[7];
    WiFi.softAPmacAddress(mac);
    sprintf(macstr, "%02X%02X%02X", mac[3], mac[4], mac[5]);
    String AP_name="Collar-"+String(macstr);
    do_log("No connection; creating access point: "+AP_name);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_name);
  }

  //initialize mDNS service.
  MDNS.begin(collarname);
  MDNS.addService("http", "tcp", 80);
  do_log("mDNS responder started");

  // This structure just lets us send all requests to the handler
  // If we can't handle it then send a 404 response
  server.onNotFound([]()
  {
    if (!handleRequest())
      server.send(404,"text/html","Not found");
  });

  // Start TCP (HTTP) server
  server.begin();
  
  do_log("TCP server started");

  // Configure the OTA update service
  ArduinoOTA.setHostname(collarname.c_str());

  if (ui_pswd != "")
    ArduinoOTA.setPassword(ui_pswd.c_str());

  ArduinoOTA.onStart([]()
  {
    do_log("Starting update");
  });

  ArduinoOTA.onEnd([]() {
    do_log("\nEnd update");
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

  do_log("OTA service configured");
}

void loop()
{
  MDNS.update();
  server.handleClient();
  ArduinoOTA.handle();

  // If no data sent in 2 minutes, send a "keepalive" (flash the light)
  if (time(NULL)-transmit_last >= 120)
    build_and_send("1000","1110","100","1");
}
