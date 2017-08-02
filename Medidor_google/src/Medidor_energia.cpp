#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <HTTPSRedirect.h>
//#include "DebugMacros.h"

//https://github.com/tzapu/WiFiManager
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <Ticker.h>

extern "C"{
#include "user_interface.h"
}

#define wifi_ssid "PAPAYA"
#define wifi_password "papaya2014"

#define LED_AZUL 2 // led azul na placa lolin nodemcu v3

os_timer_t mTimer;
WiFiClient  client;

int contador, leituras, cont_pulso;
bool led_medidor, led_medidor_ant=1, pisca;
int pulso, pulso_max; //valor da medicao do led na piscada
String envio;
bool flag_pulso;  //verifica q houve pulso
int now_interrupt;  //tempo da interrupcao do led

//Nunca execute nada na interrupcao, apenas setar flags!
void tCallback(void *tCall){
    //now_interrupt = millis();  //calcula o tempo da interrupcao
    pulso = analogRead(A0);

    if(pulso<pulso_max-20) led_medidor = 0; //nao houve pulso do led do medidor, valor de leitura base do opto
    else led_medidor = 1;

    if(!led_medidor_ant & led_medidor) { //se mudou de 0 pra 1, incrementa
      cont_pulso++;
      flag_pulso=1;
      //pisca o led azul da placa 10ms em alto, depois desliga
      //digitalWrite(LED_AZUL, LOW);
    }

    led_medidor_ant = led_medidor;

    if(pulso>pulso_max)  pulso_max = pulso; //pega o valor maximo do pulso do led

    contador++;   //pra calcular quantas vezes essa funcao eh executada por segundo
}

//FUNCAO DE EXECUCAO COM ESTOURO DE TIMER
void usrInit(void){
    os_timer_setfn(&mTimer, tCallback, NULL);  //Chama a funcao tCallback a cada 10ms
    os_timer_arm(&mTimer, 10, true);  //10ms
}

void setup_wifi();

const char* host = "script.google.com";
const char* googleRedirHost = "script.googleusercontent.com";
// Replace with your own script id to make server side changes
const char *GScriptId = "AKfycby6pTof46p1U0oUxtmjEKF5sBDhzzjrdHPBoYb_u_PpHBJ0rWbo";
const int httpsPort = 443;
// echo | openssl s_client -connect script.google.com:443 |& openssl x509 -fingerprint -noout
const char* fingerprint = ""; //############# PREENCHER

// Write to Google Spreadsheet
String url = String("/macros/s/") + GScriptId + "/exec?value=Conected_ESP8266";
// Fetch Google Calendar events for 1 week ahead
//String url2 = String("/macros/s/") + GScriptId + "/exec?cal";
// Read from Google Spreadsheet
//String url3 = String("/macros/s/") + GScriptId + "/exec?read";

//for LED status
Ticker ticker;
void tick()
{
  //toggle state
  int state = digitalRead(LED_AZUL);  // get the current state of GPIO1 pin
  digitalWrite(LED_AZUL, !state);     // set pin to the opposite state
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.1, tick);
}

// Use HTTPSRedirect class to create TLS connection
void Conecta_google(void) {
  bool flag_fingerprint = 0;
  HTTPSRedirect client(httpsPort);
  Serial.print("Connecting to ");
  Serial.println(host);

  bool flag = false;
  for (int i=0; i<5; i++){
  int retval = client.connect(host, httpsPort);
  if (retval == 1) {
     flag = true;
     break;
  }
  else
    Serial.println("Connection failed. Retrying...");
  }

  Serial.flush();
  if (!flag){
  Serial.print("Could not connect to server: ");
  Serial.println(host);
  Serial.println("Exiting...");
  return;
  }

  Serial.flush();
  while(!flag_fingerprint) {  //se o fingerprint nao foi aceito repete indefinidamente
    if (client.verify(fingerprint, host)) {
      Serial.println("Certificate match.");
      flag_fingerprint = 1; //obteve sucesso
    }
    else {
      Serial.println("Certificate mis-match");
      ticker.attach(0.5, tick);
      delay(1000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_AZUL, OUTPUT);  //define o led da placa como saida
  WiFiManager wifiManager;
  //wifiManager.resetSettings(); //força entrada no portal de configuracao, só pra testes
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.autoConnect("ESP8266", "smolder"); //nome e senha para acessar o portal
  Serial.println("ESP conectado no WIFI !");
  ticker.detach();
  Serial.flush();
  //setup_wifi();
  digitalWrite(LED_AZUL, HIGH); //desliga o led azul
  usrInit();  //ativa a interrupção
  Conecta_google(); //tenta se conectar no google



  // Note: setup() must finish within approx. 1s, or the the watchdog timer
  // will reset the chip. Hence don't put too many requests in setup()
  // ref: https://github.com/esp8266/Arduino/issues/34


  Serial.println("==============================================================================");
  //url = String("/macros/s/") + GScriptId + "/exec?now=ESP8266 conectado!";
  //client.printRedir(url, host, googleRedirHost);
  //client.printRedir(url2, host, googleRedirHost);
  //Serial.println("==============================================================================");
}

// void setup_wifi() {
//   delay(10);
//   // We start by connecting to a WiFi network
//   Serial.println();
//   Serial.print("Connecting to ");
//   Serial.println(wifi_ssid);
//   WiFi.begin(wifi_ssid, wifi_password);
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }
// }

long lastMsg = 0;
int cont = 0, valor_lum;

void loop()
{
  // while(millis() < now_interrupt + 10);
  // digitalWrite(LED_AZUL, HIGH);
  Serial.print("pulsos: ");
  Serial.println(cont_pulso);
  Serial.print("leituras por loop: ");
  Serial.println(contador);
  Serial.print("opto_min: ");
  Serial.println(pulso);
  Serial.print("opto_max: ");
  Serial.println(pulso_max);
  pulso_max = 0; //reinicia o pulso_max para se recalibrar
  contador = 0;  //zera a contagem de interrupcoes por segundo q esta ocorrendo
  yield();
  delay(1000);
  // Serial.print("tempo da interrupcao (ms): ");
  // Serial.println(millis()-now_interrupt);

  if(flag_pulso) {  //so envia dados se houver pulso no led do medidor
    Serial.println("========================================================== INICIO");
    long now = millis();
    flag_pulso=0; //so executa de novo se houver nova piscada no led do medidor
    HTTPSRedirect client(httpsPort);
    if (!client.connected()) client.connect(host, httpsPort);
    url = String("/macros/s/") + GScriptId + "/exec?value=" + cont_pulso; //imprime a contagem de pulsos
    client.printRedir(url, host, googleRedirHost);
    Serial.print("tempo de post (ms): ");
    Serial.println(millis()-now);
    Serial.println("============================================================ FIM");
  }
}
