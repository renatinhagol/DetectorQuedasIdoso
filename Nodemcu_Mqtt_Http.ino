#include <ESP8266WiFi.h>  //essa biblioteca já vem com a IDE. Portanto, não é preciso baixar nenhuma biblioteca adicional
#include <PubSubClient.h> // Importa a Biblioteca PubSubClient
#include <Arduino.h>
#include <ArduinoHttpClient.h>

#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif

//defines
#define SSID_REDE     "GVT-EAB8"  //coloque aqui o nome da rede que se deseja conectar
#define SENHA_REDE    "0062204722"  //coloque aqui a senha da rede que se deseja conectar
#define INTERVALO_ENVIO_MQTT        10000  //intervalo entre envios de dados via MQTT (em ms)
//defines de id mqtt e tópicos para publicação e subscribe
#define TOPICO_SUBSCRIBE "teste"     //tópico MQTT de escuta
#define TOPICO_PUBLISH   "teste"    //tópico MQTT de envio de informações para Broker
//IMPORTANTE: recomendamos fortemente alterar os nomes
//            desses tópicos. Caso contrário, há grandes
//            chances de você controlar e monitorar o NodeMCU
//            de outra pessoa.
#define ID_MQTT  "192.168.25.196"     //id mqtt (para identificação de sessão)
//IMPORTANTE: este deve ser único no broker (ou seja,
//            se um client MQTT tentar entrar com o mesmo
//            id de outro já conectado ao broker, o broker
//            irá fechar a conexão de um deles).

MPU6050 mpu;
#define OUTPUT_READABLE_ACCELGYRO
#define OUTPUT_READABLE_YAWPITCHROLL

//constantes e variáveis globais
const char* BROKER_MQTT = "192.168.25.196"; //URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1515;                      // Porta do Broker MQTT
long lastConnectionTime;
long lastMQTTSendTime;
WiFiClient client;
WiFiClient clientMQTT;
PubSubClient MQTT(clientMQTT); // Instancia o Cliente MQTT passando o objeto clientMQTT
const char* server = "192.168.25.196"; // Endereço IP do Servidor

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


//prototypes
void initWiFi(void);
void initMQTT(void);
void reconectWiFi(void);
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void VerificaConexoesWiFIEMQTT(void);

/*
   Implementações
*/

//Função: inicializa e conecta-se na rede WI-FI desejada
//Parâmetros: nenhum
//Retorno: nenhum
void initWiFi()
{
  delay(10);
  Serial.println("------Conexao WI-FI------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(SSID_REDE);
  Serial.println("Aguarde");

  reconectWiFi();
}

//Função: inicializa parâmetros de conexão MQTT(endereço do
//        broker, porta e seta função de callback)
//Parâmetros: nenhum
//Retorno: nenhum
void initMQTT()
{
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);   //informa qual broker e porta deve ser conectado
  MQTT.setCallback(mqtt_callback);            //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}

//Função: função de callback
//        esta função é chamada toda vez que uma informação de
//        um dos tópicos subescritos chega)
//Parâmetros: nenhum
//Retorno: nenhum
void mqtt_callback(char* topic, byte* payload, unsigned int length)
{

}

//Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
//        em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
//Parâmetros: nenhum
//Retorno: nenhum
void reconnectMQTT()
{
  while (!MQTT.connected())
  {
    Serial.print("* Tentando se conectar ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(ID_MQTT))
    {
      Serial.println("Conectado com sucesso ao broker MQTT!");
      MQTT.subscribe(TOPICO_SUBSCRIBE);
    }
    else
    {
      Serial.println("Falha ao reconectar no broker.");
      Serial.println("Havera nova tentatica de conexao em 2s");
      delay(2000);
    }
  }
}

//Função: reconecta-se ao WiFi
//Parâmetros: nenhum
//Retorno: nenhum
void reconectWiFi()
{
  //se já está conectado a rede WI-FI, nada é feito.
  //Caso contrário, são efetuadas tentativas de conexão
  if (WiFi.status() == WL_CONNECTED)
    return;

  WiFi.begin(SSID_REDE, SENHA_REDE); // Conecta na rede WI-FI

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Conectado com sucesso na rede ");
  Serial.print(SSID_REDE);
  Serial.println("IP obtido: ");
  Serial.println(WiFi.localIP());
}

//Função: verifica o estado das conexões WiFI e ao broker MQTT.
//        Em caso de desconexão (qualquer uma das duas), a conexão
//        é refeita.
//Parâmetros: nenhum
//Retorno: nenhum
void VerificaConexoesWiFIEMQTT(void)
{
  if (!MQTT.connected())
    reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita

  reconectWiFi(); //se não há conexão com o WiFI, a conexão é refeita
}

void setup()
{

  pinMode(interruptPin, INPUT_PULLUP);
  Wire.begin(sda_pin, scl_pin);
  Serial.begin(38400);
  lastConnectionTime = 0;
  lastMQTTSendTime = 0;
  initWiFi();
  initMQTT();
  Serial.println("teste de queda"); Serial.println("Initializing I2C devices...");
  mpu.initialize();
  Serial.println("Testing device connections...");
  Serial.println(mpu.testConnection() ? "MPU6050 connection successful" : "MPU6050connection failed");
  Serial.println(F("\nSend any character to begin DMP programming and demo: "));
  while (Serial.available() && Serial.read()); // empty buffer
  while (!Serial.available()); // wait for data
  while (Serial.available() && Serial.read()); // empty buffer again
  Serial.println(F("Initializing DMP..."));
  devStatus = mpu.dmpInitialize();

  mpu.setXGyroOffset(33);
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

//loop principal
void loop()
{
  char MsgUmidadeMQTT[50];
  VerificaConexoesWiFIEMQTT();

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
          Serial.println("POSSÍVEL QUEDA !!!");
          sprintf(MsgUmidadeMQTT, "- POSSÍVEL QUEDA !!!");
          MQTT.publish(TOPICO_PUBLISH, MsgUmidadeMQTT);

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
          client.println("POST /EnviarEmailBanco.php HTTP/1.1");
          client.println("Host: 192.168.25.5");
          client.println("Cache-Control: no-cache");
          client.println("Content-Type: application/x-www-form-urlencoded");
          client.print("Content-Length: ");
          client.println(postData.length());
          client.println();
          client.println(postData);
        }
        cont = 0;
        reconnectMQTT();
      }
      Serial.print("\n");
      mpu.resetFIFO();
      detachInterrupt(interruptPin);
      delay (500);
      attachInterrupt(digitalPinToInterrupt(interruptPin), dmpDataReady, RISING);
#endif
    }
  }
} // fim do else

