#include <WifiLora.h>
#include <TempInternal.h>
#include <SSD1306.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <MPU6050.h>
#include <TinyGPS.h>
#include <SPIFFS.h>

#define SDA GPIO_NUM_21
#define SCL GPIO_NUM_22
#define RX1PIN 35
#define TX1PIN 13
#define OUTPUT_READABLE_ACCELGYRO

//création
TinyGPS gps;  
MPU6050 accelgyro; 
MAX30105 particleSensor;
SSD1306 display(0x3c, 4, 15);

HardwareSerial GPSserial(1);

String fileTxt;

//Messages envoyés
String str_mpu;
String str_max; //'max' pour MAX30105
String str_gps = "";
char c ;

int j = 0;

float start;
float endl;
float timeloop;
//Variables MPU
int16_t ax, ay, az;
int16_t gx, gy, gz;
float cs = 16384/9.81;
float Ax, Ay, Az;
float tot_x = 0;
float tot_y = 0;
float tot_z = 0;
int loopcounter = 0;
int pas = 0;
String str_pas;
String str_debug;
float avg;
float prev1 = 0;
float prev2 = 0;
float max_diff = 0;



//Variables MAX30105
const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred
float beatsPerMinute;
int beatAvg;

//Variables GPS
float latitude;
float longitude;
float runspeed;
float flat, flon, fkmph;
float falt; 
unsigned long age;
int calories;
float distparc = 0;
float ti;
int i = 0;


//Lora/mqtt message
TxMsg prev; //previous message from the WifiLora lib
unsigned long lastsent=0; //timestamp of the last message sent
unsigned long lastrec = 0;
unsigned long interval=10000; //interval period between messages (in ms)
unsigned long intervalrec=60000;
int startrec = 20000;

void incrementation(float tot){  // fonction qui incremente pas s'il y a vraiment un pas
  avg = tot / 10;
  max_diff = max(prev1 - avg, prev1 - prev2);
  if (prev1 > avg && prev1 > prev2 && max_diff > 1.1){ // le 1.1 c'est à partir de cette valeur que la difference max(prec,)          
    pas++;
  
  }
}


void decalage(float* a , float* b, float* c){  //fonction qui change les valeurs des variables : b devient a et c devient b , a est inchangé
  float temporaire1 = *a;
  float temporaire2 = *b;
  *c = temporaire2;
  *b = temporaire1;
  }


//Update the OLED display
void update_display() {
    display.clear();
    display.drawString(0,0,String("Lora Node ")+String(WifiLora.getAddress()));  
    if (!WifiLora.isEnableWifi())
      display.drawString(0,10,String("Wifi is disabled "));
    else {
      if (WifiLora.isClientWifi()) {
        display.drawString(0,10,String("Wifi client: ")+IP2Str(WiFi.localIP()));
      }
      else {
        display.drawString(0,10,String("Wifi AP: ")+IP2Str(WiFi.softAPIP()));
      }
    }
    if (prev.state>2) {
      display.drawString(0,20,String("Msg: ")+prev.msg.substring(0,40));
      display.drawString(0,30,(prev.loramode?String("Sent by lora ")+(prev.state==4?String(" ack ")+String(prev.ackRSSI):String("not ack")):String("Sent by mqtt ("+String(prev.mqttnb))+String(")")));
      display.drawString(0,40,String("Lora stats: ")+String(prev.loranbok)+"/"+String(prev.loranb));
    }
    display.display();
}

//setup the different components
void setup() {
  Serial.begin(115200); //Port série
  Wire.begin(21,22);  //Initialisation I2C
  GPSserial.begin(9600,SERIAL_8N1,RX1PIN,TX1PIN); //Initialisation GPSserial

  //initialsisation SPIFFS
  SPIFFS.begin();
  Serial.println("Demarrage file System");
 
  File f = SPIFFS.open("/nouveau.txt", "r");   //Ouverture fichier pour le lire
  Serial.println("Lecture du fichier en cours:");
  
  //Affichage des données du fichier
  fileTxt = f.readString(); // on recupere le contenu entier du ficher
  Serial.println(fileTxt);
  f.close();
  
  Serial.println("Ficher fermé");

  //MPU initialisation
  accelgyro.initialize();
  

  // MAX30105 initialisation
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Utilise le port I2C par défaut, vitesse de 400kHz
  {
    Serial.println("Le MAX30105 n'a pas été trouvé. Branchements? ");
    while (1);
  }
  Serial.println("Placez votre doigt sur la LED.");

  particleSensor.setup(); //Configure le capteur avec les paramètres par défaut
  particleSensor.setPulseAmplitudeRed(0x0A); //Allume la LED rouge pour indiquer que le capteur est allumé
  particleSensor.setPulseAmplitudeGreen(0); //Eteint la LED verte

  
  
  
  WifiLora.start(); 
  //OLED     
  pinMode(16,OUTPUT);
  digitalWrite(16, LOW);    // met le GPIO16 en low pour reset l'écran OLED
  delay(50); 
  digitalWrite(16, HIGH); // tant que l'écran OLED est en marche, il faut mettre le GPIO16 en high、 
  display.init();
  display.clear();
  display.flipScreenVertically(); 
  display.setTextAlignment(TEXT_ALIGN_LEFT); 
  display.setFont(ArialMT_Plain_10);  
  update_display();     
  
  c = 'a';
}

//main loop
void loop() {
  loopcounter++; 
  
  //GPS
  String str;
  bool newData = false;
  
  
   while (GPSserial.available()) {
  char c=GPSserial.read();
  //Serial.write(c); 
    if (gps.encode(c)) {
      newData = true;
      if (newData) {  
        unsigned long age;  
        gps.f_get_position(&flat, &flon, &age);
        fkmph = gps.f_speed_kmph(); //en km/h
        latitude = flat;
        longitude = flon;
        runspeed = fkmph;
      }
    }
  }
  

  //MAX30105
  
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }
    
  //MPU
  
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  Ax = ax/cs; Ay = ay/cs; Az = az/cs;
  tot_x += fabs(Ax);
  tot_y += fabs(Ay);
  tot_z += fabs(Az);
  if(loopcounter >= 10) {
    float maximum = max(tot_x, tot_y);
    maximum = max(maximum, tot_z);
    loopcounter = 0;
    if (maximum == tot_x){
        incrementation(tot_x);
        str_pas = "X," + String(pas);
    }
    else if (maximum == tot_y){
        incrementation(tot_x);
        str_pas = "Y," + String(pas);
      }
    else if (maximum == tot_z){
        incrementation(tot_x);
        str_pas = "Z," + String(pas);
      }
      else {
      Serial.println("ERROR 404");
      }  
  tot_x = 0;
  tot_y = 0;
  tot_z = 0;
  decalage(&avg, &prev1, &prev2);
  }
  
  
  str = str_gps + "/" + String(beatsPerMinute) + "," + String(beatAvg) + "/" + str_pas;
  if (irValue < 50000){
    str = str_gps + "/" + String(" finger??") +"/" +str_pas;
  }
  
  Serial.println(str);
  
  //Message to send LORA
  if (millis()-lastsent>interval) {
    //read temperature every 10sec and send it 
    lastsent=millis();
    prev=WifiLora.getTx(); //get informations about the previous message sent
    update_display();                    
    if (WifiLora.send(str)) Serial.println(String("Send message : ")+str);      
  }

  /*//Message to send WIFI
  prev=WifiLora.getTx(); //get informations about the previous message sent
    if (millis()-lastsent>interval) { //read temperature every 10sec and send it 
    lastsent=millis();
    update_display();}
  if (WifiLora.send(str)) Serial.println(String("sent"));*/

  //ENREGISTREMENT DE DONNEES
  if (millis() > startrec && millis() < intervalrec + startrec) {
    lastrec = millis();
    
  File f = SPIFFS.open("/nouveau.txt", "w");
  f.println(fileTxt);  // on réécrit l'ancien contenu sauvegardé au préalable
  f.print(String(millis()) + " "); // puis les nouvelles infos.
  f.println(str);
  f.close();
  // on ferme le fichier une fois les enregistrements terminés

  // on ouvre de nouveau pour lecture
  f = SPIFFS.open("/nouveau.txt", "r");
  fileTxt = f.readString();
  //Affichage des données du fichier
  Serial.println("nouveau string :");
  Serial.println(fileTxt);
  f.close();
  //pause de 80 secondes , afin d'éviter de remplir la mémoire trop vite.Pensez à débrancher votre ESP ou d'uploader un nouveau sketch avant de saturer votre mémoire.
  }
  else if (millis() > interval + startrec) {
    Serial.println("FINI");}
   str = "";
   str_gps = "";
   delay(10);
}
