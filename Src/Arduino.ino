// #define DEBUG
// #define DEBUG_WIFI

#include <WiFiEsp.h>
#include <SoftwareSerial.h>
#include <MsTimer2.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <DHT.h>

#define AP_SSID "iot0"
#define AP_PASS "iot00000"
#define SERVER_NAME "10.10.141.61"
#define SERVER_PORT 5001
#define LOGID "PSK_ARD"
#define PASSWD "PASSWD"

#define DHTPIN 4
#define WIFIRX 6  //6:RX-->ESP8266 TX
#define WIFITX 7  //7:TX -->ESP8266 RX

#define CMD_SIZE 50
#define ARR_CNT 5
#define DHTTYPE DHT11
bool timerIsrFlag = false;
unsigned int secCount;

char sendId[10] = "PSK_ARD";
char sendBuf[CMD_SIZE];
char lcdLine1[17] = "Smart IoT By KSH";
char lcdLine2[17] = "WiFi Connecting!";

char getSensorId[10];
int sensorTime;
float temp = 0.0;
float humi = 0.0;
float dust = 0.0;
bool updatTimeFlag = false;
typedef struct {
  int year;
  int month;
  int day;
  int hour;
  int min;
  int sec;
} DATETIME;
DATETIME dateTime = { 0, 0, 0, 12, 0, 0 };
DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial wifiSerial(WIFIRX, WIFITX);
WiFiEspClient client;
LiquidCrystal_I2C lcd(0x3f, 16, 2);

byte sendDust[] = { 0x11, 0x01, 0x01, 0xED };
byte receive_data[16];

void setup() {
  lcd.init();
  lcd.backlight();
  lcdDisplay(0, 0, lcdLine1);
  lcdDisplay(0, 1, lcdLine2);

  wifi_Setup();

  MsTimer2::set(1000, timerIsr);  // 1000ms period
  MsTimer2::start();

  dht.begin();

  Serial.begin(9600);
}

void loop() {
  if (client.available()) {
    socketEvent();
  }

  if (timerIsrFlag)  //1초에 한번씩 실행
  {
    timerIsrFlag = false;
    if (!(secCount % 10))  //10초에 한번씩 실행
    {
      humi = dht.readHumidity();
      temp = dht.readTemperature();
      dust = getDustSensor();
      sprintf(lcdLine2, "%d", (int)dust);
      lcdDisplay(0, 1, lcdLine2);
      
      if(client.connected()) {
        sprintf(sendBuf, "[SQL]SENSOR@%d@%d@%d\r\n", (int)humi, (int)temp, (int)dust);
        client.write(sendBuf, strlen(sendBuf));
        client.flush();
      }
      else if (!client.connected()) {
        lcdDisplay(0, 1, "Server Down");
        server_Connect();
      }
    }
    sprintf(lcdLine1, "%02d.%02d  %02d:%02d:%02d", dateTime.month, dateTime.day, dateTime.hour, dateTime.min, dateTime.sec);
    lcdDisplay(0, 0, lcdLine1);
    if (updatTimeFlag) {
      updatTimeFlag = false;
    }
  }
}
void socketEvent() {
  int i = 0;
  char *pToken;
  char *pArray[ARR_CNT] = { 0 };
  char recvBuf[CMD_SIZE] = { 0 };
  int len;

  sendBuf[0] = '\0';
  len = client.readBytesUntil('\n', recvBuf, CMD_SIZE);
  client.flush();

  pToken = strtok(recvBuf, "[@]");
  while (pToken != NULL) {
    pArray[i] = pToken;
    if (++i >= ARR_CNT)
      break;
    pToken = strtok(NULL, "[@]");
  }
  //[KSH_ARD]LED@ON : pArray[0] = "KSH_ARD", pArray[1] = "LED", pArray[2] = "ON"
  if ((strlen(pArray[1]) + strlen(pArray[2])) < 16) {
    sprintf(lcdLine2, "%s %s", pArray[1], pArray[2]);
    lcdDisplay(0, 1, lcdLine2);
  }
  if (!strncmp(pArray[1], " New", 4))  // New Connected
  {
    strcpy(lcdLine2, "Server Connected");
    lcdDisplay(0, 1, lcdLine2);
    updatTimeFlag = true;
    return;
  } else if (!strncmp(pArray[1], " Alr", 4))  //Already logged
  {
    client.stop();
    server_Connect();
    return;
  } 
  else
    return;

  client.write(sendBuf, strlen(sendBuf));
  client.flush();
}
void timerIsr() {
  timerIsrFlag = true;
  secCount++;
  clock_calc(&dateTime);
}
void clock_calc(DATETIME *dateTime) {
  int ret = 0;
  dateTime->sec++;  // increment second

  if (dateTime->sec >= 60)  // if second = 60, second = 0
  {
    dateTime->sec = 0;
    dateTime->min++;

    if (dateTime->min >= 60)  // if minute = 60, minute = 0
    {
      dateTime->min = 0;
      dateTime->hour++;  // increment hour
      if (dateTime->hour == 24) {
        dateTime->hour = 0;
        updatTimeFlag = true;
      }
    }
  }
}

void wifi_Setup() {
  wifiSerial.begin(19200);
  wifi_Init();
  server_Connect();
}
void wifi_Init() {
  do {
    WiFi.init(&wifiSerial);
    if (WiFi.status() == WL_NO_SHIELD) {
#ifdef DEBUG_WIFI
      Serial.println("WiFi shield not present");
#endif
    } else
      break;
  } while (1);

#ifdef DEBUG_WIFI
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(AP_SSID);
#endif
  while (WiFi.begin(AP_SSID, AP_PASS) != WL_CONNECTED) {
#ifdef DEBUG_WIFI
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(AP_SSID);
#endif
  }
  sprintf(lcdLine1, "ID:%s", LOGID);
  lcdDisplay(0, 0, lcdLine1);
  sprintf(lcdLine2, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  lcdDisplay(0, 1, lcdLine2);

#ifdef DEBUG_WIFI
  Serial.println("You're connected to the network");
  printWifiStatus();
#endif
}
int server_Connect() {
#ifdef DEBUG_WIFI
  Serial.println("Starting connection to server...");
#endif

  if (client.connect(SERVER_NAME, SERVER_PORT)) {
#ifdef DEBUG_WIFI
    Serial.println("Connect to server");
#endif
    client.print("[" LOGID ":" PASSWD "]");
  } else {
#ifdef DEBUG_WIFI
    Serial.println("server connection failure");
#endif
  }
}
void printWifiStatus() {
  // print the SSID of the network you're attached to
#ifdef DEBUG_WIFI
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
#endif
  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
#ifdef DEBUG_WIFI
  Serial.print("IP Address: ");
  Serial.println(ip);
#endif
  // print the received signal strength
  long rssi = WiFi.RSSI();
#ifdef DEBUG_WIFI
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
#endif
}
void lcdDisplay(int x, int y, char *str) {
  int len = 16 - strlen(str);
  lcd.setCursor(x, y);
  lcd.print(str);
  for (int i = len; i > 0; i--)
    lcd.write(' ');
}

float getDustSensor() {
  for (int i = 0; i < 4; i++) {
    // send data
    Serial.write(sendDust[i]);
  }
  // delay(1000);
  if (Serial.available()) {
    for (int i = 0; i < 16; i++) {
      receive_data[i] = Serial.read();
    }
  }

  //아두이노 소스 계산 시작//
  float ugm, pcsl;
  ugm = receive_data[3] * 255.0 * 255.0 * 255.0
        + receive_data[4] * 255.0 * 255.0
        + receive_data[5] * 255.0
        + receive_data[6];
  //ugm = (receive_data[3]<<24) + (receive_data[4]<<16) + (receive_data[5]<<8) + receive_data[6];
  pcsl = ugm * 3528.0 / 1000000.0;

  //아두이노 소스 계산 끝/
  return pcsl;
}
