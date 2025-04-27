// call libraries
#include <WiFi.h>
#include <UbidotsESPMQTT.h>
#include <Firebase.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <MFRC522.h>

// define macros 
// - oled 
#define width 128
#define hight 64
Adafruit_SSD1306 o_led(width , hight , & Wire , -1);

// - wifi 
#define ssid "Nada Safwat"
#define password "nada2123"

//- rfid 
#define ss 2 
#define rst 5
MFRC522 rfid(ss,rst);

//- firebase 
#define url "https://smart-attendance-system-2bfbd-default-rtdb.firebaseio.com/"
#define token "cSIwxDGfhnnXDSwoPw4aFfWrmc6tpb91aJDQrSp5"
Firebase fb(url,token);

//ubidots
#define device_token "BBUS-0Y1EJRNg3AqYwI1RRKB2FqziYdp6Om"
Ubidots client(device_token);

// - outputs 
#define blue_led 12
#define red_led 14 
#define buzzer 27 

String SerialInput = "" ; 

//- define call back fnc
void callback(char* topic , byte* payload , unsigned int length ){
  String msg = "";
  Serial.print("topic: ");
  Serial.println(topic);
  for(int i = 0 ; i < length ; i++)
  {
    msg += (char)payload[i];
  }
  Serial.println(msg);
}

void AccessGranted(String UID){
  digitalWrite(blue_led,HIGH);
  digitalWrite(red_led,LOW);
  digitalWrite(buzzer,HIGH);
  delay(200);
  digitalWrite(buzzer,LOW);
  Serial.println("Granted ID: "+ UID);
  String path = "names/" + UID ;
  String name = fb.getString(path);
  show_message("welcome"+ name);
  fb.pushString("logs", "Access Granted for "+ UID );
  delay(2000);
  digitalWrite(blue_led,LOW);
  digitalWrite(red_led,LOW);
  digitalWrite(buzzer,LOW);
}

void AccessDenied(String UID){
  digitalWrite(blue_led,LOW);
  digitalWrite(red_led,HIGH);
  digitalWrite(buzzer,HIGH);
  delay(500);
  digitalWrite(buzzer,LOW);
  Serial.println("Denied ID: "+ UID);
  show_message("NON AUTHORIED");
  fb.pushString("logs", "Access Denied for "+ UID );
  delay(2000);
  digitalWrite(blue_led,LOW);
  digitalWrite(red_led,LOW);
  digitalWrite(buzzer,LOW);
}

void show_message(String msg){
  o_led.clearDisplay();
  o_led.setCursor(10,20);
  o_led.setTextColor(WHITE);
  o_led.setTextSize(1);
  o_led.println(msg);
  o_led.display();
}

String Get_UId(){
  String U_id = "";
  for(byte i = 0 ; i < rfid.uid.size;i++){
    U_id += String(rfid.uid.uidByte[i]);
  }
  return U_id;
}

void setup() {

  //1- serial 
  Serial.begin(115200);
  //2-rfid
  SPI.begin();
  rfid.PCD_Init();
  //3-wifi
  WiFi.begin(ssid,password);
  //4- set pinModes
  pinMode(blue_led, OUTPUT);
  pinMode(red_led, OUTPUT);
  pinMode(buzzer, OUTPUT);
  //5- RSET VALUES
  digitalWrite(blue_led,LOW);
  digitalWrite(red_led,LOW);
  digitalWrite(buzzer,LOW);
  //6- oled
  o_led.begin(SSD1306_SWITCHCAPVCC,0X3C);
  o_led.clearDisplay();
  o_led.display();
  // 7- CHECK OLED COnNECTIVITY 
  if(!o_led.begin(SSD1306_SWITCHCAPVCC,0X3C)){
    Serial.println("can't connect to oled");
    while(true);
  }
  // 8- check wifi connectivity :
  while(WiFi.status()!=WL_CONNECTED)
  {
    Serial.println("Try to connect...");
    delay(500);
  }
  Serial.println("Connected! ");
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());

  // 9- ubidots : 
  client.setDebug(true);
  client.wifiConnection(ssid,password);
  client.begin(callback);

 Serial.println("Choose which action you want to take add or check");
}

void loop() 
{
   if (!client.connected()) {
    client.reconnect();
  }
  client.loop();  // Maintain connection
  if (Serial.available())
  {
    String UID = Get_UId();
    SerialInput = Serial.readStringUntil('\n');
    SerialInput.trim();
    handel_cmd(SerialInput,UID);
  }
  if(rfid.PICC_IsNewCardPresent()&&rfid.PICC_ReadCardSerial())
  {
    String UID = Get_UId();
    delay(2000);
    rfid.PICC_HaltA();
  }
}

 void handel_cmd(String cmd , String UID){
    if(SerialInput =="add")
    {
      String path = "authorized/"+ UID ;
      int result = fb.getInt(path);
      if(result == 1){
        show_message("Already Exist!");
        Serial.println("this id already exist");
        client.add( "attendancecount", 0);
        client.ubidotsPublish("iot-project");
      }
      else{
        int id = UID.toInt() ;
        fb.setInt("authorized/"+ UID , 1);
        Serial.println("Added!");
        show_message("Added!");
        client.add( "attendancecount", 1);
        client.add( "attendanceTable", id );
        client.ubidotsPublish("iot-project");
        digitalWrite(blue_led,HIGH);
        delay(2000);
        digitalWrite(blue_led,LOW);
        o_led.clearDisplay();
        o_led.display();
      }
     
    }
    else if(SerialInput=="check")
    {
      int result = fb.getInt("authorized/"+ UID);
      if(result == 1)
      {
        AccessGranted(UID);
      }
      else
      {
        AccessDenied(UID);
      }
    }
    else{
      Serial.println("wrong cmd choose from  add or check ");
      show_message("error!");
    }
 }