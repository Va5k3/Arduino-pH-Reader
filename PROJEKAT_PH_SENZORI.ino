const int DS_TEMP = 3;
const int NTC_TEMP = A0;
int R1 = 10000;
int B=3950;
float T0 = 25+273.16;

const int RELAY_PERTILIJER = 4;
const int BUZZER = 5;
const int LED_DIODA = 6;
const int PH_INPUT = A1;
const int TASTER = 7;

void setup() {

  Serial.begin(9600);
  
  
  pinMode(TASTER, INTPUT);
  pinMode(DS_TEMP,INPUT);
  pinMode(NTC_TEMP,INPUT);
  pinMode(RELAY_PERTILIJER,OUTPUT);
 // pindMode(BUZZER,OUTPUT);
  //pindMode(LED_DIODA,OUTPUT);
  pinMode(PH_INPUT,INPUT);




}




float Vph(){ // merenje napona sa ph senzora
  int br = 0;
  while(br<300){
  pertelijer(); // PROVERAVANJE DA LI JE DOSLO DO 22, AKO NIJE ZAGREJE GA
  if(tempNtc()>=22)
  float result =  analogRead(PH_INPUT)*5.0/1023;
  delay(1000);
  br++;
  }
  return result;
}


float pocetnaNapon(){ // merenje napona sa ph senzora
  int br = 0;

  while(br<10){
  float result =  analogRead(PH_INPUT)*5.0/1023;
  delay(1000);
  br++;
  
  }
  return result;
}



float racunanjeM(float poznataPh1, float poznataPh2, float V1, float V2){ // konstanta M koja sluzi za sledecu funkciju, poznataPH su poznate PH vrednosti koje unosimo sa tastature
  return (poznataPh2 - poznataPh1)/(V2-V1);
}
float racunanjeB(float poznataPh1, float VpH, float izracunatoM){ // konstana B 
  return poznataPh1-izracunatoM*Vph;
}

float merenjePh(float M, float Vph, float B){ //GLAVNA FUNKCIJA - merenje PH
  return M*V+B;
}

void zvucniSignal(){
  for(int i=0;i<3;++i){
    digitalWrite(BUZZER,HIGH);
    delay(1000);
    digitalWrite(BUZZER,LOW);
  }
}

void zvucniSvetlosni_Signal(){
  for(int i=0;i<3;++i){
    digitalWrite(BUZZER,HIGH);
    digitalWrite(LED_DIODA,HIGH);
    delay(500);
    digitalWrite(LED_DIODA,LOW);
    digitalWrite(BUZZER,LOW);
  }
  
}

void zagrevanjeTecnosti(){ // RADNA TEMPERATURE VODE, PRE SVAKOG MERENJA
  while(tempNtc<22){
    pertelijer();
  }
}

float tempNtc(){
  
  int VanalRaw = analogRead(NTC_TEMP);
  
  float Vanal = VanalRaw*(5.0/1023);
  float Rntc = Vanal*R1/(5-Vanal);
  float A = log(Rntc/R1);
  float T = 1/(A/B+1/T0);
  float Tc = T-273.16;

  return Tc;
  
}

void pertilijer(){
  //NTC TEMPERATURA MERENJE
  if(tempNtc()>=22){
    digitalWrite(RELAY_PERTILIJER,LOW);
  }
  else
    digitalWrite(RELAY_PERTILIJER,HIGH);
    
}


void loop() {

  

  //KALIBRACIJA -------------------------------------------------------------
  
  float ph1,ph2; //  unose se sa tastature
  float Vph1, Vph2; // naponi sa ph senzora 
  float pocetnaV1, pocetnaV2;
  zagrevanjeTecnosti();
  zvucniSignal();
  pocetnaV1 = pocetnaNapon();
  Vph1 = Vph(); // MERENJE NAPONA  TECNOSTI JEDAN
  zvucniSignal();
  delay(10000);
  zagrevanjeTecnosti();
  zvucniSignal();
  pocetnaV2 = pocetnaNapon();
  Vph2 = Vph(); // MERENJA NAPONA TECNOSTI DVA, RADI GLAVNOG MERENJA
  float m,b;
  m = racunanjeM(ph1,ph2,Vph1,Vph2); //nagib krive
  b = racunanjeB(ph1,Vph1,m); // pomeraj, gde sece grafik X osu
  zvucniSignal();
  // GOTOVA KALIBRACIJA OVDE -------------------------------------------------

  // MERENJE PH VREDNOSTI
  if(digitalRead(TASTER)==HIGH){ // DA LI JE TASTER PRETISNUT
  zagrevanjeTecnosti();   // ZAGREVAMO TECNOST NA RADNU TEMPERATURU
  zvucniSvetlosni_Signal();
  float PhNapon = Vph(); // MERENJE NAPONA NEPOZNATE TECNOSTI
  float konacniPh = merenjePh(m,PhNapon,b); // MERENJE PH VREDNOSTI

  }





  delay(1000);

}
