//Ino Board: esp32 -> ESP32-WROOM_DA-Module
#include <WiFi.h>
#include <NetworkClient.h>
#include <WiFiAP.h>
#include <HardwareSerial.h>

//LED
#define LED_PIN 2 

//Server
const char *ssid = "KAmodESP32";
const char *password = "12345678";
NetworkServer server(80);

IPAddress AP_LOCAL_IP(192, 168, 1, 1);
IPAddress AP_GATEWAY_IP(192, 168, 1, 254);
IPAddress AP_NETWORK_MASK(255, 255, 255, 0);

//RS485
#define RS485_RXD 27 //RO PIN
#define RS485_TXD 25 //DI PIN
#define RS485_RE_DE 26 //RE & DE PIN
#define RX_BUFF_SIZE 64
HardwareSerial RS485Port(2);

//----------------------------------
void rs485send(String to_send){
  RS485Port.flush();
  digitalWrite(RS485_RE_DE, HIGH);
  RS485Port.println(to_send);
  RS485Port.flush();
  digitalWrite(RS485_RE_DE, LOW);
}

//----------------------------------
void setup() {
  //Serial
  Serial.begin(115200);
  Serial.println("Hello. Configuring access point...");

  //Server
  WiFi.softAPConfig(AP_LOCAL_IP, AP_GATEWAY_IP, AP_NETWORK_MASK);
  if (!WiFi.softAP(ssid, password)) {
    log_e("Soft AP creation failed.");
    while (1);
  }
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  Serial.print("AP Mac Address: ");
  Serial.println(WiFi.softAPmacAddress());
  server.begin();

  //RS485
  pinMode(RS485_RE_DE, OUTPUT);
  digitalWrite(RS485_RE_DE, LOW);
  //RS485Port.setPins(RS485_RXD, RS485_TXD, RS485_RE_DE);
  //RS485Port.setHwFlowCtrlMode(UART_HW_FLOWCTRL_CTS);
  RS485Port.setMode(UART_MODE_RS485_HALF_DUPLEX);
  RS485Port.begin(115200, SERIAL_8N1, RS485_RXD, RS485_TXD);
  //RS485Port.println("RS485 Hello");
  rs485send("RS485 Hello");

  //LED
  pinMode(LED_PIN, OUTPUT);
  for(int i=0; i<10; i++){
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
}

//----------------------------------
void loop() {
  String ser_rec = "";
  String rs_rec = "";

  NetworkClient client = server.accept();

  if (client) {
    Serial.println("********New Client********");

    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        if (c == '\n') {

          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.print("<H1>Click <a href=\"/H\">here</a> to turn ON the LED.<br><br>");
            client.print("Click <a href=\"/L\">here</a> to turn OFF the LED.<br><br>");
            client.print("Click <a href=\"/RS485\">here</a> to write to RS485.<br><br></H1>");
            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }

        //if (currentLine.endsWith("GET /H")) {
        if (currentLine.indexOf("GET /H") >= 0) {
          digitalWrite(LED_PIN, HIGH);
        }
        //if (currentLine.endsWith("GET /L")) {
        if (currentLine.indexOf("GET /L") >= 0) {
          digitalWrite(LED_PIN, LOW);
        }
        if (currentLine.endsWith("GET /RS485")) {
          RS485Port.println("You send message via RS485");
        }
      }
    }
    client.stop();
    Serial.println("********Client Disconnected********");
  } else {
    //RS485Port.println("Tick...");
    rs485send("Tick...");
    delay(1000);
  }

  while (Serial.available()){
    char m;
    m = Serial.read();
    ser_rec += m;
    if (m == '\r'){
      delay(10);
      Serial.println("Sent via RS485");
      //RS485Port.println(ser_rec);
      rs485send(ser_rec);
      ser_rec = "";
    }
  }

  while (RS485Port.available()){
    char m;
    m = RS485Port.read();
    rs_rec += m;
    if (m == '\r'){
      delay(10);
      rs485send(rs_rec);
      Serial.println("Read via RS485");
      Serial.println(rs_rec);
      rs_rec = "";
    }
  }  
}
