#include <TinyGPS.h>
#define RX1PIN 35
#define TX1PIN 13
float latitude;
float longitude;
float the_speed;
float flat, flon, fkmph;
float falt; 
unsigned long age;
int calories;
float distparc = 0;
float ti;


TinyGPS gps;
HardwareSerial GPSserial(1);

void setup() {  
  Serial.begin(115200);
  GPSserial.begin(9600,SERIAL_8N1,RX1PIN,TX1PIN);
}


void loop() {
   
   if (GPSserial.available()) {
    char c = GPSserial.read(); //stocke le signal dans une variable c
    //Serial.write(c); 
    if (gps.encode(c)) { // on rentre dedans si les données sont valides
       
        
            gps.f_get_position(&flat, &flon, &age);
            fkmph = gps.f_speed_kmph(); 
            falt = gps.f_altitude();
            latitude = flat;
            longitude = flon;
            the_speed = fkmph;
            calories += 1.25*the_speed;
            
            ti = millis();
            Serial.print('\n');
            Serial.print("LAT=");Serial.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
            Serial.print(" LON=");Serial.print(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);
            Serial.print(" SPEED=");Serial.print(fkmph);
            Serial.print(" ALT=");Serial.print(falt);
            Serial.print("CALORIES=");Serial.print(calories);
            
            
            
        
    }
 }
}
