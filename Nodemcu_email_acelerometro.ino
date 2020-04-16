#include <ESP8266WiFi.h>
#include <ArduinoHttpClient.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif

//SSID e senha da rede Wi-Fi
const char* SSID = "GVT-EAB8";
const char* PASS = "0062204722";

//Endereço IP do Servidor
const char* server = "192.168.25.196"; // localhost

MPU6050 mpu;
#define OUTPUT_READABLE_ACCELGYRO
#define OUTPUT_READABLE_YAWPITCHROLL

const int sda_pin = D5; // definição do pino I2C SDA
const int scl_pin = D6; // definição do pino I2C SCL

bool dmpReady = false; // set true if DMP init was successful
uint8_t mpuIntStatus; // holds actual interrupt status byte from MPU
uint8_t devStatus; // return status after each device operation (0 = success, !0 =error)
uint16_t packetSize; // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount; // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

Quaternion q; // [w, x, y, z] quaternion container
VectorFloat gravity; // [x, y, z] gravity vector
float euler[3]; // [psi, theta, phi] Euler angle container
float ypr[3]; // [yaw, pitch, roll] yaw/pitch/roll container and gravityvecto

// ===========================================================================
//  VARIAVEIS DO ALGORITMO DE QUEDA
// ===========================================================================
// Definições de variáveis para o Reconhecimento da Queda
float angulo1;
float angulo2;
int cont = 0;
float delta = 0;

// ===========================================================================
// INTERRUPT DETECTION ROUTINE
// ===========================================================================
const byte interruptPin = D7; // retirado de tutorial sobre interrupções noNodeMCU
volatile byte interruptCounter = 0;
volatile bool mpuInterrupt = false; // indicates whether MPU interrupt pin has gonehigh
void dmpDataReady() {
  mpuInterrupt = true;
  interruptCounter++;
}
// ===========================================================================
// INITIAL SETUP
// ===========================================================================
void setup() {

  pinMode(interruptPin, INPUT_PULLUP);
  Wire.begin(sda_pin, scl_pin);
  Serial.begin(38400);

  //Conecta a rede WiFi utilizando o modo Access Point
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(SSID, PASS);

  //Aguarda conexão WiFi
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(300);
    Serial.print(".");
  }

  //Informa qual é o endereço IP obtido no DHCP
  Serial.print("Endereço IP obtido: ");
  Serial.println(WiFi.localIP());


  Serial.println("Initializing I2C devices...");
  mpu.initialize();
  Serial.println("Testing device connections...");
  Serial.println(mpu.testConnection() ? "MPU6050 connection successful" : "MPU6050connection failed");
  Serial.println(F("\nSend any character to begin DMP programming and demo: "));
  while (Serial.available() && Serial.read()); // empty buffer
  while (!Serial.available()); // wait for data
  while (Serial.available() && Serial.read()); // empty buffer again
  Serial.println(F("Initializing DMP..."));
  devStatus = mpu.dmpInitialize();

  mpu.setXGyroOffset(-33);
  mpu.setYGyroOffset(4);
  mpu.setZGyroOffset(30);
  mpu.setXAccelOffset(-3919);
  mpu.setYAccelOffset(1885);
  mpu.setZAccelOffset(3714);


  if (devStatus == 0) {

    Serial.println(F("Enabling DMP..."));
    mpu.setDMPEnabled(true);
    Serial.println(F("Enabling interrupt detection (Arduino external interrupt0)..."));
    attachInterrupt(digitalPinToInterrupt(interruptPin), dmpDataReady, RISING);
    mpuIntStatus = mpu.getIntStatus();
    Serial.println(F("DMP ready! Waiting for first interrupt..."));
    dmpReady = true;
    packetSize = mpu.dmpGetFIFOPacketSize();
  }
  else {
    Serial.print(F("DMP Initialization failed (code "));
    Serial.print(devStatus);
    Serial.println(F(")"));
  }
}

// ===========================================================================
// INITIAL LOOP
// ===========================================================================

void loop() {

  WiFiClient client;
  if (!client.connect(server, 80)) {
    Serial.println("Erro - Não foi possível conectar ao servidor!");
  }
  //A conexão foi estabelecida com o servidor!
  else {

    if (!dmpReady) return;
    // wait for MPU interrupt or extra packet(s) available
    while (!mpuInterrupt && fifoCount < packetSize) { }
    // reset interrupt flag and get INT_STATUS byte
    mpuInterrupt = false;
    mpuIntStatus = mpu.getIntStatus();
    fifoCount = mpu.getFIFOCount();
    if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
      mpu.resetFIFO();
    } else if (mpuIntStatus & 0x02) {
      while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();
      mpu.getFIFOBytes(fifoBuffer, packetSize);
      fifoCount -= packetSize;
#ifdef OUTPUT_READABLE_YAWPITCHROLL
      mpu.dmpGetQuaternion(&q, fifoBuffer);
      mpu.dmpGetGravity(&gravity, &q);
      mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
      Serial.print("ypr\t");
      Serial.print(ypr[0] * 180 / M_PI);
      Serial.print("\t");
      Serial.print(ypr[1] * 180 / M_PI);
      Serial.print("\t");
      Serial.print(ypr[2] * 180 / M_PI);

      if (cont == 0) {

        angulo1 = ypr[1] * 180 / M_PI;
        cont++;
      }
      else {
        angulo2 = ypr[1] * 180 / M_PI;
        delta = angulo1 - angulo2;
        if (delta > 15) {
          Serial.print("\t");
          Serial.print(angulo1);
          Serial.print("\t");
          Serial.print(angulo2);
          Serial.print("\t");
          Serial.print(delta);
          Serial.print("\t");
          Serial.println("##  POSSÍVEL QUEDA ###");

          //Pega o endereço IP do Nó Sensor
          IPAddress ip = WiFi.localIP();
          String enderecoIP = String(ip[0]) + "." +
                              String(ip[1]) + "." +
                              String(ip[2]) + "." +
                              String(ip[3]);

          //Cria a carga útil (POST)
          String postData = "enderecoIP=";
          postData += enderecoIP;

          //Cria o pacote HTTP
          client.println("POST /EnviarEmail.php HTTP/1.1");
          client.println("Host: 192.168.25.196");
          client.println("Cache-Control: no-cache");
          client.println("Content-Type: application/x-www-form-urlencoded");
          client.print("Content-Length: ");
          client.println(postData.length());
          client.println();
          client.println(postData);
        }
        cont = 0;
      }
      Serial.print("\n");
      mpu.resetFIFO();
      detachInterrupt(interruptPin);
      delay (500);
      attachInterrupt(digitalPinToInterrupt(interruptPin), dmpDataReady, RISING);
#endif
    }
  } // fim do else
}
