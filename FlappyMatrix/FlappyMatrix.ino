#include <FastLED.h>
#define LED_PIN  2
#define COLOR_ORDER GRB
#define CHIPSET     WS2812B
#define BRIGHTNESS 35
#define STATUS_LED 5
#define BTN 4
#define GAME_SPEED 80 // delay in ms for each frame
#define SCROLL_SPEED 10 // """ delay """" for scroll
#define ACCEL 0.01
#define JUMP 0.1
#define defAcc 0.1
#define defJmp 0.1

void(* resetFunc) (void) = 0;

int gameState = 0;
float playerPos = 0;
float accn = defAcc;
float jmp = defJmp;
int updateCounter = 0; 
// Params for width and height
const uint8_t kMatrixWidth = 8;
const uint8_t kMatrixHeight = 8;
// Param for different pixel layouts
const bool    kMatrixSerpentineLayout = true;
const bool    kMatrixVertical = false;

uint16_t XY( uint8_t x, uint8_t y){
  uint16_t i;
  if( kMatrixSerpentineLayout == false) {
    if (kMatrixVertical == false) {
      i = (y * kMatrixWidth) + x;
    } else {
      i = kMatrixHeight * (kMatrixWidth - (x+1))+y;
    }
  }
  if( kMatrixSerpentineLayout == true) {
    if (kMatrixVertical == false) {
      if( y & 0x01) {
        // Odd rows run backwards
        uint8_t reverseX = (kMatrixWidth - 1) - x;
        i = (y * kMatrixWidth) + reverseX;
      } else {
        // Even rows run forwards
        i = (y * kMatrixWidth) + x;
      }
    } else { // vertical positioning
      if ( x & 0x01) {
        i = kMatrixHeight * (kMatrixWidth - (x+1))+y;
      } else {
        i = kMatrixHeight * (kMatrixWidth - x) - (y+1);
      }
    }
  }
  return i;
}

uint16_t XYsafe( uint8_t x, uint8_t y){
  if( x >= kMatrixWidth) return -1;
  if( y >= kMatrixHeight) return -1;
  return XY(x,y);
}

#define NUM_LEDS (kMatrixWidth * kMatrixHeight)
CRGB leds_plus_safety_pixel[ NUM_LEDS + 1];
CRGB* const leds( leds_plus_safety_pixel + 1);

int grid[kMatrixWidth][kMatrixHeight]; //frame Buffer
// colours
CHSV skyBlue = CHSV(130, 255, 255);
CHSV red = CHSV(0, 255, 255);
CHSV green = CHSV(95, 255, 255);

// gameObjects
CHSV cloud[3][5] = {{CHSV(130, 130, 255), CHSV(100, 50, 255), CHSV(130, 130, 255)},
                    {CHSV(100, 50, 255), CHSV(100, 50, 255), CHSV(100, 50, 255)},
                    {CHSV(0, 0, 0)     , CHSV(0, 0, 0)     , CHSV(0, 0, 0)     }};
CHSV bird = CHSV(67, 255, 255);

int Pipe[kMatrixWidth] = {0, 0, 0, 0, 0, 0, 0, 0};

void setup(){
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness( BRIGHTNESS );
  pinMode(STATUS_LED, OUTPUT);
  pinMode(BTN, INPUT_PULLUP);
  for(int y=0; y < kMatrixHeight; y++){ // init grid
    for(int x=0; x < kMatrixWidth; x++){
      grid[y][x] = 1;
    }
  }
  delay(500);
}

void loop(){
  int btnStatus = digitalRead(BTN);
  if(btnStatus == 0){
    digitalWrite(STATUS_LED, HIGH);
    accn = defAcc;
    jmp -= JUMP;
  }
  else{
    digitalWrite(STATUS_LED, LOW);
    accn+=ACCEL;
    jmp = defJmp;
  }

  switch (gameState) {
  case 0: // Home
    homeScreen();
    if(btnStatus == 0){
      gameState++;
    }
    break;
  case 1: // Play
    drawWorld(0);
    // updatePipe
    if(updateCounter == SCROLL_SPEED){
      updateCounter = 0;
      shiftPipeMap();
    }
    drawPipe();
    drawPlayer(btnStatus);
    if(int(playerPos) > kMatrixHeight-1 || collision()){
      leds[XY(2, int(playerPos))] = red;
      FastLED.show();
      delay(1000);
      gameState = 2;
    }
    break;
  case 2: // game over
    gameOver();
    if(btnStatus == 0){
      gameState = 0;
      resetFunc();
    }
  }
  updateCounter++;
  FastLED.show();
  delay(GAME_SPEED);
}

void shiftPipeMap(){
  for(int i=0; i<kMatrixWidth; i++){
    int tmp = Pipe[i+1];
    Pipe[i] = tmp;
  }
  if(Pipe[kMatrixWidth-2] == 0 && Pipe[kMatrixWidth-3] == 0 ){
    Pipe[kMatrixWidth-1] = random(0, 5);
  }else{
    Pipe[kMatrixWidth-1] = 0;
  }
}

void drawPipe(){
  for(int x=0; x<kMatrixWidth; x++){
    if(Pipe[x] != 0){
      for(int y=0; y<Pipe[x]; y++){
        leds[XY(x, y)] = green;
      }

      for(int y=Pipe[x]+3; y<kMatrixHeight; y++){
        leds[XY(x, y)] = green;
      }

    }
  }
}

int collision(){
  int pos = int(playerPos);
  if(pos < Pipe[2] || ( Pipe[2] != 0 &&   pos > (Pipe[2]+2)  ) ){
    return 1;
  }
  return 0;
}

void drawPlayer(int btnStatus){
  leds[XY(2, int(playerPos))] = bird;
  btnStatus == 0 ? playerPos-- : playerPos+=accn;
  if(btnStatus == 0){
    playerPos -= jmp;
  }else{
    playerPos += accn;
  }
  if(playerPos<0)
    playerPos = 0;
}

void drawWorld(uint8_t pos){
  // sky
  sky();

  //clouds
  // int CloudPos = random(0, 2);
  // for(int y=0; y < 3; y++){
  //   for(int x=0; x < 3; x++){
  //     if(cloud[y][x].value == 0){
  //       continue;
  //     }else{
  //       leds[XY(x, CloudPos + y)] = cloud[y][x];
  //     }
  //   }
  // }

  // Bush
}

void sky(){
  for(int y=0; y < kMatrixHeight; y++){
    for(int x=0; x < kMatrixWidth; x++){
      skyBlue.saturation = random(170, 255);
      leds[XY(x, y)] = skyBlue;
    }
  }
}

void homeScreen(){
  int startText[kMatrixHeight][kMatrixWidth] = {{0, 0, 0, 0, 0, 0, 0, 0},
                                                {0, 0, 0, 0, 0, 0, 0, 0},
                                                {1, 1, 1, 0, 1, 1, 0, 0},
                                                {1, 0, 0, 0, 1, 0, 1, 0},
                                                {1, 1, 0, 0, 1, 1, 0, 0},
                                                {1, 0, 0, 0, 1, 0, 1, 0},
                                                {1, 0, 0, 0, 1, 1, 0, 0},
                                                {0, 0, 0, 0, 0, 0, 0, 0},
                                                };
  sky();
  for(int y=0; y < kMatrixHeight; y++){
    for(int x=0; x < kMatrixWidth; x++){
      if(startText[y][x] == 0){
        continue;
      }else{
        leds[XY(x, y)] = green;
      }
    }
  }
}

void gameOver(){
  int GO[kMatrixHeight][kMatrixWidth] =        {{0, 0, 0, 0, 0, 0, 0, 0},
                                                {0, 0, 0, 0, 0, 0, 0, 0},
                                                {0, 1, 1, 0, 0, 0, 0, 0},
                                                {1, 0, 0, 1, 0, 0, 1, 0},
                                                {1, 0, 0, 0, 0, 1, 0, 1},
                                                {1, 0, 1, 1, 0, 1, 0, 1},
                                                {1, 0, 0, 1, 0, 1, 0, 1},
                                                {0, 1, 1, 0, 0, 0, 1, 0},
                                                };
  sky();
  for(int y=0; y < kMatrixHeight; y++){
    for(int x=0; x < kMatrixWidth; x++){
      if(GO[y][x] == 0){
        continue;
      }else{
        leds[XY(x, y)] = red;
      }
    }
  }
}
