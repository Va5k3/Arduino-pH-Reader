//LIBRARY_START------------------------------
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
//LIBRARY_END------------------------------

const byte rows[4] = {7, 8, 9, 10}; //konekcija sa pinovina 
const byte cols[3] = {11, 12, 13}; //konekcija sa pinovina 

char keys[4][3] = 
{
  {'1', '2' ,'3'},
  {'4', '5' ,'6'},
  {'7', '8' ,'9'},  //Kreiranje matrice za tastere
  {'*', '0' ,'#'},
};

Keypad mykeypad = Keypad(makeKeymap(keys), rows, cols, 4, 3); //Pravljenje instance klase Keypad

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int DS_TEMP = 3;
const int NTC_TEMP = A0;
int R1 = 10000;
int B=3950;
float T0 = 25+273.16;

const int RELAY_PERTILIJER = 4;
const int BUZZER = 5;
const int PH_INPUT = A1;

//PID konstante

const double SETPOINT = 25.0;
const double MRTVA_ZONA = 0.5;
const double MAX_IZLAZ = 255.0;
const double MIN_IZLAZ = 0.0;
const unsigned long PID_PERIOD = 1000;

double Kp = 2.0;
double Ki = 0.05;
double Kd = 1.0;

//PID promenljive

double temperatura = 0.0;
double pid_izlaz = 0.0;
double sumaGresaka = 0.0;
double proslaGreska = 0.0;
unsigned long prosloVreme = 0;

// IZ MAIN-FUNKCIJE PROMENLJIVE
float izmereniPhNapon, izmerenaPhVrednost;
float Vph1, Vph2, ph1, ph2; // naponi sa ph senzora 
float pocetnaV1, pocetnaV2; // pocetni naponi v1, v2
float m,b;


void setup() {

  Serial.begin(9600);

  lcd.init(); // inicijalizacija lcd
  lcd.backlight(); // ukljucivanje pozadinskog osvetljenja LCD-a
  
  
  pinMode(DS_TEMP,INPUT);
  pinMode(NTC_TEMP,INPUT);
  pinMode(RELAY_PERTILIJER,OUTPUT);
  pinMode(BUZZER,OUTPUT);
  pinMode(PH_INPUT,INPUT);

  digitalWrite(RELAY_PERTILIJER, LOW);
  prosloVreme = millis();

}
double racunajPID(double izmereno, double zeleno, double dt)
{
  double greska = zeleno - izmereno;

  //P deo
  double P = Kp * greska;

  //I deo
  if(pid_izlaz < MAX_IZLAZ && greska > 0)
  {
    sumaGresaka += greska * dt;
  }
  sumaGresaka = constrain(sumaGresaka, 0, MAX_IZLAZ / Ki);
  double I = Ki * sumaGresaka;

  //D deo
  double promenaGreske = (greska - proslaGreska) / dt;
  proslaGreska = greska;
  double D = Kd * promenaGreske;

  return constrain(P + I + D, MIN_IZLAZ, MAX_IZLAZ);
}

void pertilijerPID()
{
  unsigned long sada = millis();

  if(sada - prosloVreme < PID_PERIOD)
  return;

  double dt = (sada - prosloVreme) / 1000.0;
  prosloVreme = sada;

  temperatura = tempNtc();

  pid_izlaz = racunajPID(temperatura, SETPOINT, dt);

  if(pid_izlaz > MRTVA_ZONA)
  {
    digitalWrite(RELAY_PERTILIJER, HIGH);
  }
  else
  {
    digitalWrite(RELAY_PERTILIJER, LOW);
  }
}

float VphMerenje(){ // merenje napona sa ph senzora
  int br = 0;
  float result = 0;
  while(br<3){          // TREBA DA MERI 300 SEKUNDI, STAVILI SMO 3 SEKUNDE ZA TEST
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Merenje");
  lcd.setCursor(0,1);
  lcd.print("u toku");
  pertilijerPID(); // PROVERAVANJE DA LI JE DOSLO DO 25, AKO NIJE ZAGREJE GA
  result = analogRead(PH_INPUT)*5.0/1023;
  delay(1000);
  br++;
  }
  return result;
}

float VphKalibracija(){ // merenje napona sa ph senzora
  int br = 0;
  float result = 0;
  while(br<3){          // TREBA DA MERI 300 SEKUNDI, STAVILI SMO 3 SEKUNDE ZA TEST
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Kalibracija");
  lcd.setCursor(0,1);
  lcd.print("u toku");
  pertilijerPID();
   // PROVERAVANJE DA LI JE DOSLO DO 25, AKO NIJE ZAGREJE GA
  result = analogRead(PH_INPUT)*5.0/1023;
  delay(1000);
  br++;
  }
  return result;
}

float pocetnaNapon(){ // merenje pocetnog napona sa ph senzora
  int br = 0;
  float result;

  while(br<10){
  result =  analogRead(PH_INPUT)*5.0/1023;
  lcd.clear();   // ISPIS ZA DEBUGOVANJE ----------------------------------------------------
              lcd.setCursor(0, 0);
              lcd.print("MERENJE POCETNOG");
              lcd.setCursor(0, 1);
              lcd.print("NAPONA");
              delay(1000); // -----------------------------------------------------------------------
  delay(1000);
  br++;
  
  }
  return result;
}

float racunanjeM(float poznataPh1, float poznataPh2, float V1, float V2){ // konstanta M koja sluzi za sledecu funkciju, poznataPH su poznate PH vrednosti koje unosimo sa tastature
  return (poznataPh2 - poznataPh1) / (V2 - V1);
}
float racunanjeB(float poznataPh1, float Vph, float izracunatoM){ // konstana B 
  return poznataPh1 - izracunatoM * Vph;
}

float merenjePh(float M, float Vph, float B){ //GLAVNA FUNKCIJA - merenje PH
  return M * Vph + B;
}

void zvucniSignal(){
  for(int i=0;i<3;++i){
    digitalWrite(BUZZER,HIGH);
    delay(1000);
    digitalWrite(BUZZER,LOW);
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

char modovi()
{
  int pozicija = 0;
  unsigned long prethodnoVreme = millis();
  int interval = 300;
  String tekst = "Izaberi: 1-Kalibracija  2-Merenje   ";

  while(true)
  {
    char key = mykeypad.getKey(); //provera keypad-a
    if(key == '1' || key == '2')
    {
      lcd.clear();
      return key;
    }

    if(millis() - prethodnoVreme > interval)
    {
      prethodnoVreme = millis();

      lcd.setCursor(0,0);
      lcd.print(tekst.substring(pozicija, pozicija + 16)); //skrolovanje teksta po LCD-u

      pozicija++;
      if(pozicija > tekst.length() -16)
      {
        pozicija = 0;
      }
    }
  }
}
void ispisTeksta(String tekst, int interval, int zadatoVreme)
{
  int pozicija = 0;
  unsigned long prethodnoVreme = millis();
  unsigned long pocetnoVreme = millis();

  while(true)
  {
    if(millis() - prethodnoVreme > interval)
    {
      prethodnoVreme = millis();
      lcd.clear(); //  OVO SMO DODALI -----------------------------------------------
      lcd.setCursor(0,0);
      lcd.print(tekst.substring(pozicija, pozicija + 16));

      pozicija++;
      if(pozicija > tekst.length() - 16)
      {
        pozicija = 0;
      }
    }
    if(millis() - pocetnoVreme >= zadatoVreme)
    break;
  }
}

float unosCelobrojneVrednosti()
{
  unsigned long prethodnoVreme = millis();
  int interval = 300;
  String unos = "";
  String tekst1 = "Uneti celobrojnu vrednost broja";
  int celobrojni = 0;
  int decimalni = 0;
  int decimale = 0;
  char key;

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Uneti celobrojni:");

  lcd.setCursor(0,1);
  unos = "";
  while(true) //unos celobrojnog dela broja
  {
    key = mykeypad.getKey();
    if(key)
      if(key>= '0' && key<='9')
        {
          unos += key;
        }
        else if(key == '*')
        {
          if(unos.length() >0 )
          {
            unos.remove(unos.length()-1);
          }
        }
        else if(key == '#')
        {
          if(unos.length()>0)
          {
            celobrojni = unos.toInt();
            break;
          }
        }
        lcd.setCursor(0,1);
        lcd.print("                ");
        lcd.setCursor(0,1);
        lcd.print(unos);
        delay(150);
  }

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Unesi decimalni");

  lcd.setCursor(0,1);
  unos = "";
  while(true) //unos decimalnog dela broja
  {
    key = mykeypad.getKey();
    if(key)
      if(key>= '0' && key<='9')
        {
          unos += key;
        }
        else if(key == '*')
        {
          if(unos.length() >0 )
          {
            unos.remove(unos.length()-1);
          }
        }
        else if(key == '#')
        {
          if(unos.length()>0)
          {
            decimalni = unos.toInt();
            decimale = unos.length();
            break;
          }
        }
        lcd.setCursor(0,1);
        lcd.print("                ");
        lcd.setCursor(0,1);
        lcd.print(unos);
  }
  float broj = celobrojni + decimalni / pow(10, decimale);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Uneli ste:");
  lcd.setCursor(0,1);
  lcd.print(broj);
  delay(1000);
  return broj;
}
void pertilijerWHILE() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Zagrevanje");
    lcd.setCursor(0, 1);
    lcd.print("u toku!");

    // Cekamo dok temperatura ne udje u opseg 24.5 - 25.5
    float t;
    while(true) {

       lcd.clear();   // ISPIS ZA DEBUGOVANJE ----------------------------------------------------
              lcd.setCursor(0, 0);
              lcd.print("WHILE PTELJA");
              lcd.setCursor(0, 1);
              lcd.print(tempNtc()); // -----------------------------------------------------------------------



        t = tempNtc();  // citamo jednom, koristimo vise puta

        if(t >= 24.5 && t <= 25.5) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Zagrevanje");
            lcd.setCursor(0, 1);
            lcd.print("zavrseno!");
            delay(1500);
            lcd.clear();
            digitalWrite(RELAY_PERTILIJER, LOW);
            return;
        }

        // Direktno upravljamo relejem bez PID-a
        // (dok cekamo da dostignemo 25, nema smisla koristiti PID)
        if(t < 24.5) {
            digitalWrite(RELAY_PERTILIJER, HIGH);
             lcd.clear();   // ISPIS ZA DEBUGOVANJE ----------------------------------------------------
              lcd.setCursor(0, 0);
              lcd.print("Relaj upaljen");
              lcd.setCursor(0, 1);
              lcd.print(""); // -----------------------------------------------------------------------

        } else {
            digitalWrite(RELAY_PERTILIJER, HIGH);
        }

        delay(500);  // Malo pauze da ne bombardujemo senzor
    }
}

void odabirModa(){
  char key;

  while(true)
  {
    key = modovi();

    if(key == '1')  // KALIBRACIJA
    {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Uneti prvu ph:");
    delay(1500);
    ph1 = unosCelobrojneVrednosti();
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Uneti drugu ph:");
    delay(1500);
    ph2 = unosCelobrojneVrednosti();

  pertilijerWHILE();
    
  zvucniSignal();
  pertilijerWHILE();
  pocetnaV1 = pocetnaNapon();
  Vph1 = VphKalibracija(); // MERENJE NAPONA  TECNOSTI JEDAN
  zvucniSignal();
  ispisTeksta("Premesti ph senzor", 300, 10000);
  delay(1500);
  pertilijerWHILE();
  pocetnaV2 = pocetnaNapon();
  Vph2 = VphKalibracija(); // MERENJE NAPONA  TECNOSTI DVA
  zvucniSignal();
  m = racunanjeM(ph1,ph2,Vph1,Vph2); //nagib krive
  b = racunanjeB(ph1,Vph1,m); // pomeraj, gde sece grafik X osu
  ispisTeksta("Kalibracija zavrsena!", 300, 2000);
    }

    if(key == '2')   // MERENJE PH VREDNOSTI
    {
      pertilijerWHILE();
      zvucniSignal();
      izmereniPhNapon = VphMerenje();
      izmerenaPhVrednost = merenjePh(m, izmereniPhNapon, b);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Izmerena");
      lcd.setCursor(0,1);
      lcd.print("pH je: ");
      lcd.print(izmerenaPhVrednost, 2);
      delay(2000);
    }
  }
}


void loop() {
  odabirModa();
//INICIJALNA KALIBRACIJA
/*    lcd.clear();  -- OVO SE SVE NALAZI U FUNKCIJI odabirMod() - OVO JE ZA MOD 1. NEKE PROMENLJIVE SMO VEC OBRISALI ODAVDE
    lcd.setCursor(0,0);
    lcd.print("Inicijalna");
    delay(1000);
    lcd.setCursor(0,1);
    lcd.print("Kalibracija");
    delay(1500);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Uneti prvu ph:");
    delay(1500);
    float ph1 = unosCelobrojneVrednosti();
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Uneti drugu ph:");
    delay(1500);
    float ph2 = unosCelobrojneVrednosti();

    
    pertilijerWHILE();

  lcd.clear();   // ISPIS ZA DEBUGOVANJE ----------------------------------------------------
              lcd.setCursor(0, 0);
              lcd.print("IZASLI SMO WHILE");
              lcd.setCursor(0, 1);
              lcd.print("");
              delay(1000); // -----------------------------------------------------------------------
    
  zvucniSignal();
  //pertilijerWHILE();                            OVO NISMO PROVALILI ZA STA JE
  pocetnaV1 = pocetnaNapon();
  Vph1 = VphKalibracija(); // MERENJE NAPONA  TECNOSTI JEDAN
  //zvucniSignal();

  ispisTeksta("Premesti ph senzor", 300, 10000);
  pertilijerWHILE();
  pocetnaV2 = pocetnaNapon();
  Vph2 = VphKalibracija(); // MERENJE NAPONA  TECNOSTI DVA
  zvucniSignal();
  float m,b;
  m = racunanjeM(ph1,ph2,Vph1,Vph2); //nagib krive
  b = racunanjeB(ph1,Vph1,m); // pomeraj, gde sece grafik X osu
  ispisTeksta("Kalibracija zavrsena!", 300, 2000);*/

/*   --- NAPRAVILI FUNKCIJU ZA MOD odabirModa()
  char key;

  while(true)
  {
    key = modovi();

    if(key == '1')  // KALIBRACIJA
    {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Uneti prvu ph:");
    delay(1500);
    ph1 = unosCelobrojneVrednosti();
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Uneti drugu ph:");
    delay(1500);
    ph2 = unosCelobrojneVrednosti();

  pertilijerWHILE();
    
  zvucniSignal();
  pertilijerWHILE();
  pocetnaV1 = pocetnaNapon();
  Vph1 = VphKalibracija(); // MERENJE NAPONA  TECNOSTI JEDAN
  zvucniSignal();
  ispisTeksta("Premesti ph senzor", 300, 10000);
  delay(1500);
  pertilijerWHILE();
  pocetnaV2 = pocetnaNapon();
  Vph2 = VphKalibracija(); // MERENJE NAPONA  TECNOSTI DVA
  zvucniSignal();
  m = racunanjeM(ph1,ph2,Vph1,Vph2); //nagib krive
  b = racunanjeB(ph1,Vph1,m); // pomeraj, gde sece grafik X osu
  ispisTeksta("Kalibracija zavrsena!", 300, 2000);
    }

    if(key == '2')   // MERENJE PH VREDNOSTI
    {
      pertilijerWHILE();
      zvucniSignal();
      izmereniPhNapon = VphMerenje();
      izmerenaPhVrednost = merenjePh(m, izmereniPhNapon, b);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Izmerena");
      lcd.setCursor(0,1);
      lcd.print("pH je: ");
      lcd.print(izmerenaPhVrednost, 2);
      delay(2000);
    }
  }*/
}
