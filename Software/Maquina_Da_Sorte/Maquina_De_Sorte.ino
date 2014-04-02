#include <limits.h>
#include <SdFat.h>
#include <LiquidCrystal.h>

#define DEBUG true
#define KEYS A0
#define ANALOG_RANDOM_PIN A5
#define SD_CARD_CS 3
#define LCD_LED 10

#define BRIGHT_STEP 3
#define SEED_STEP 3

#define LCD_LED_TIME (500)

#define DRAW_FILE_NAME "drawfile.csv"
#define DL ';'
#define MAX_LINE_SIZE 500
#define NAME_SIZE 80
#define EMAIL_SIZE 80
#define VOUCHER_SIZE 10
#define NUM_HEADER_LINES 5

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

byte bright = 100;

typedef unsigned char random_t;

random_t seed;

long timeLastSeed = millis();

int timeRandomLimit;

byte numLines = 0; // sempre comecar em zero, vai ser feita a contagem quando o arquivo for aberto;
SdFat sd;

ifstream drawfile;

void newSeed(){
  lcd.setCursor(0,1);
  lcd.print(F("Seed:"));
  lcd.setCursor(6,1);
  lcd.print(F("          "));
  lcd.setCursor(5,1);
  lcd.print(seed);
  randomSeed(seed);
}

void showMainMsg(){
  lcd.setCursor(0,0);
  lcd.print(F("Maquina da Sorte"));
}

void setup() {

  if(DEBUG)Serial.begin(9600);

  pinMode(SD_CARD_CS, OUTPUT);

  pinMode(LCD_LED,OUTPUT);
  analogWrite(LCD_LED, bright);

  randomSeed(analogRead(ANALOG_RANDOM_PIN) * random());
  timeRandomLimit =  abs(random(1000*60, 1000*60*5));

  lcd.begin(16, 2);

  showMainMsg();

  lcd.setCursor(0,1);
  lcd.print(F("Inicializando!!!"));
  delay(500);

  if(DEBUG){
    lcd.setCursor(0,1);
    lcd.print(F("Delay 1:         "));
    lcd.setCursor(9,1);
    lcd.print(timeRandomLimit);
    lcd.print(F("ms"));
    delay(1000);
  }
  // initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
  // breadboards.  use SPI_FULL_SPEED for better performance
  do{
    while (!sd.begin(SD_CARD_CS, SPI_HALF_SPEED)){
      lcd.setCursor(0,1);
      lcd.print(F("SD Falhou!!!!   "));
      delay(700);
      lcd.setCursor(0,1);
      lcd.print(F("Verifique!!!!"));
      delay(100);
    }

    lcd.setCursor(0,1);
    lcd.print(F("SD OK.          "));
    delay(1000);

    // open input file
    drawfile.open(DRAW_FILE_NAME,O_RDONLY);
    if(drawfile.is_open())
      break;

    lcd.setCursor(0,1);
    lcd.print(F("drawfile ausente"));
    delay(700);
    lcd.setCursor(0,1);
    lcd.print(F("Verifique!!!!   "));
    delay(700);
    lcd.setCursor(0,1);
    lcd.print(F("Remova SD!!!!   "));
    delay(700);
    lcd.setCursor(0,1);
    lcd.print(F("Crie Drawfile!!!"));
    delay(700);
    lcd.setCursor(0,1);
    lcd.print(F("Reinicie Maquina!!!"));
    delay(700);

  }
  while(!drawfile.is_open());

  lcd.setCursor(0,1);
  lcd.print(F("Drawfile OK.    "));
  delay(1000);

  lcd.setCursor(0,1);
  lcd.print(F("Linhas:         "));

  //  char *name = (char*)malloc(sizeof(char)*MAX_LINE_SIZE);
  char name[MAX_LINE_SIZE];
  while (drawfile.getline(name,MAX_LINE_SIZE)){
    numLines++;
    if(numLines>NUM_HEADER_LINES){
      lcd.setCursor(8,1);
      lcd.print(numLines-NUM_HEADER_LINES);
    }
  }
  //free(name);

  delay(1000);
  if (!drawfile.eof()) {
    lcd.setCursor(0,1);
    lcd.print(F("**Erro Contagem**"));
  }
  drawfile.close();

  seed = analogRead(ANALOG_RANDOM_PIN);
  seed = map(seed,0,1023,0,numLines);
  newSeed();

}

/**
 * LOOP
 */
void loop() {

  static bool draw = false;
  static bool drawReleased = false;

  int x;
  x = analogRead (KEYS);

  if(DEBUG){
    Serial.print(F("Inicio Loop, key: "));
    Serial.println(x);
  }

  if (x < 100) {
    seed += SEED_STEP; 
    newSeed();
  }
  else if (x < 200) { 
    bright += BRIGHT_STEP;
  }
  else if (x < 400){ 
    bright -= BRIGHT_STEP;
  }
  else if (x < 600){ 
    seed -= SEED_STEP;
    newSeed();
  }
  else if (!draw && x < 800){
    if(DEBUG)Serial.println(F("Sortear"));

    draw = true; 

    digitalWrite(LCD_LED,LOW);

    char** voucher = drawVoucher();
    if(DEBUG){
      Serial.println(voucher[0]);
      Serial.println(voucher[1]);
      Serial.println(voucher[2]);
    }
    lcd.clear();
    digitalWrite(LCD_LED,HIGH);
    lcd.setCursor(0,0);
    lcd.print(voucher[0]);
    lcd.setCursor(0,1);
    lcd.print(voucher[1]);

//    delay(500);

    if(DEBUG)Serial.println(F("Sorteado"));

  }
  else if (draw && drawReleased && x < 800){

    if(DEBUG)Serial.println(F("Iniciar novo processo de sorteio"));

    lcd.clear();
    showMainMsg();
    newSeed();

    draw = false;

  }
  else if(!draw){ 
    if(DEBUG)Serial.println(F("Sorteio Terminado!"));

    analogWrite(LCD_LED, bright);
    drawReleased = true;

  }

 
  delay(200);
}

char** drawVoucher() {
  //char c, name[40], email[40], status[8], voucher[8];//, type, date, value, idbuyer, community, phone, project, projdesc[100];
  byte line = 1; // conta as linhas
      
 
  if(drawfile.is_open()) {
    drawfile.close();
  }

  drawfile.open(DRAW_FILE_NAME,O_RDONLY);
 
  if(drawfile.is_open()){
 
    seed = random(NUM_HEADER_LINES,numLines);
    newSeed();

    char name[NAME_SIZE], email[EMAIL_SIZE], voucher[VOUCHER_SIZE], tmp[MAX_LINE_SIZE];
    do{
      drawfile.getline(tmp,MAX_LINE_SIZE);
      line++;
    }
    while(line < NUM_HEADER_LINES);

 
    if(!drawfile.eof()){
      while (!drawfile.eof() && drawfile.getline(name,NAME_SIZE, DL).getline(email,EMAIL_SIZE,DL).getline(tmp,MAX_LINE_SIZE,DL).getline(voucher,VOUCHER_SIZE,DL).getline(tmp,MAX_LINE_SIZE)) {

        digitalWrite(LCD_LED,!digitalRead(LCD_LED));
        
 
        if(seed == line) {
          drawfile.close();

          char *vouchername[] = {
            voucher,name,email      
          };
          return vouchername;
        }
        line++;
      }
    }
  } 
  else{
    // informar que o arquivo parece vazio
  }

  if (!drawfile.eof()) {
    // informar que ouve alguma falha no arquivo como por exemplo estar conrompido
  }


  drawfile.close();
  char *vouchername[] = {
    "","",""      
  };
  return vouchername;
}

