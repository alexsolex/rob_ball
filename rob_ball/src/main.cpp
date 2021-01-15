#include "Arduino.h"
#include <U8g2lib.h>
#include <Adafruit_PWMServoDriver.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
#include <FastLED.h>//https://github.com/FastLED/FastLED

// called this way, it uses the default address 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define SERVOMIN 80 // This is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX 450
// This is the 'maximum' pulse length count (out of 4096)
#define USMIN 390     // This is the rounded 'minimum' microsecond length based on the minimum pulse of 150
#define USMAX 2197    // This is the rounded 'maximum' microsecond length based on the maximum pulse of 600
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates

//WS2812 SETTINGS
#define NUM_STRIPS 1
#define NUM_LEDS 2
#define BRIGHTNESS 10
#define LED_TYPE WS2812B
#define COLOR_ORDER BRG//RGB
#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_INTERRUPT_RETRY_COUNT 1
#define FRAMES_PER_SECOND 60
#define COOLING 55
#define SPARKING 120
//Parameters
const int stripPin  = 11;
int hue = 0;

//Variables
bool gReverseDirection  = false;

//Objects
CRGB leds[NUM_LEDS];

//Other settings

int homeAngles[3] = {1,1,1};
int currentAngles[3] = {homeAngles[0], homeAngles[1], homeAngles[2]}; //X,Y,Z angle current positions
int positions[3] = {0, 2, 1};                                         //X, Y, and Z pin numbers on PCA9685
int setpointAngles[3] = {0,0,0}; //{homeAngles[0], homeAngles[1], homeAngles[2]};

U8G2_SSD1306_128X64_NONAME_1_HW_I2C rightScreen(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
U8G2_SSD1306_128X64_NONAME_1_HW_I2C leftScreen(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

unsigned long blink_time = 0; //remember last time eyes blinked
int blink_alea_period = 500;  //keep next random period to blink eyes

unsigned long lookaround_time = 0; //remember last time eyes looked around
int lookaround_alea_period = 1000; // keep next random period to look around

boolean auto_move = false; // enable/disable auto movements

boolean face_tracking_mode = true; //is in mode face tracking (deactivate random eyes moves to keep most flawless moves)

const byte numChars = 32;
char receivedChars[numChars];
boolean newData = false;

/**
 * Draw Kawaii eyes
 */
void drawKawaii(U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2, int horizontalShift = 0)
{
  u8g2.setDrawColor(1);
  u8g2.drawFilledEllipse(64, 32, 25, 25, U8G2_DRAW_ALL); //external circle
  u8g2.setDrawColor(0);
  u8g2.drawFilledEllipse(64 + horizontalShift, 32, 17, 17, U8G2_DRAW_ALL); //internal off circle
  u8g2.setDrawColor(1);
  u8g2.drawFilledEllipse(58 + horizontalShift, 22, 4, 4, U8G2_DRAW_ALL); //inner shiny circle big
  u8g2.drawFilledEllipse(64 + horizontalShift, 17, 2, 2, U8G2_DRAW_ALL); //inner shiny circle small
}

/**
 * Render simple kawaii eyes
 */
void renderKawaii()
{
  rightScreen.firstPage();
  do
  {
    drawKawaii(rightScreen);
  } while (rightScreen.nextPage());
  leftScreen.firstPage();
  do
  {
    drawKawaii(leftScreen);
  } while (leftScreen.nextPage());
}

/**
 *  Animate eyes by making them blink
 * */
void blinkEyes()
{
  for (int y = 7; y < 57; y += 25)
  {
    rightScreen.firstPage();
    do
    {
      drawKawaii(rightScreen);
      rightScreen.drawBox(39, 7, 50, y);
      rightScreen.drawTriangle(39, y, 50, y, 50, y + 10);
    } while (rightScreen.nextPage());
    leftScreen.firstPage();
    do
    {
      drawKawaii(leftScreen);
      leftScreen.drawBox(39, 7, 50, y);
      leftScreen.drawTriangle(39, y, 50, y, 50, y + 10);
    } while (leftScreen.nextPage());
  }
  for (int y = 57; y >= 7; y -= 25)
  {
    rightScreen.firstPage();
    do
    {
      drawKawaii(rightScreen);
      rightScreen.drawBox(39, 7, 50, y);
      rightScreen.drawTriangle(39, y, 50, y, 50, y + 10);
    } while (rightScreen.nextPage());
    leftScreen.firstPage();
    do
    {
      drawKawaii(leftScreen);
      leftScreen.drawBox(39, 7, 50, y);
      leftScreen.drawTriangle(39, y, 50, y, 50, y + 10);
    } while (leftScreen.nextPage());
  }
  //rightScreen.clearDisplay();
  //leftScreen.clearDisplay();
}

void timedBlinkEyes()
{
  if (millis() >= blink_time + blink_alea_period)
  {
    blinkEyes();
    blink_alea_period = random(1000, 5000);
    Serial.println("next blinking in " + String(blink_alea_period));
    blink_time = millis();
    renderKawaii();
  }
}

/**
 * Display sad eyes
 */
void sad()
{
  rightScreen.firstPage();
  do
  {
    drawKawaii(rightScreen);
    rightScreen.drawTriangle(39, 7, 89, 7, 89, 40);
  } while (rightScreen.nextPage());
  leftScreen.firstPage();
  do
  {
    drawKawaii(leftScreen);
    leftScreen.drawTriangle(39, 7, 89, 7, 39, 40);
  } while (leftScreen.nextPage());
}

void angry()
{
  rightScreen.firstPage();
  do
  {
    drawKawaii(rightScreen);
    rightScreen.drawTriangle(39, 7, 89, 7, 39, 40);
  } while (rightScreen.nextPage());
  leftScreen.firstPage();
  do
  {
    drawKawaii(leftScreen);
    leftScreen.drawTriangle(39, 7, 89, 7, 89, 40);
  } while (leftScreen.nextPage());
}

/**
 * look on right for <duration> ms time
 */
void lookRight(int duration = 1000)
{
  for (int y = 0; y >= -8; y -= 4)
  {
    rightScreen.firstPage();
    do
    {
      drawKawaii(rightScreen, y);
    } while (rightScreen.nextPage());

    leftScreen.firstPage();
    do
    {
      drawKawaii(leftScreen, y);
    } while (leftScreen.nextPage());
  }
  delay(duration);
  for (int y = -8; y <= 0; y += 4)
  {
    rightScreen.firstPage();
    do
    {
      drawKawaii(rightScreen, y);
    } while (rightScreen.nextPage());

    leftScreen.firstPage();
    do
    {
      drawKawaii(leftScreen, y);
    } while (leftScreen.nextPage());
  }
}

/**
 * look on left for <duration> ms time
 */
void lookLeft(int duration = 1000)
{
  for (int y = 0; y <= 8; y += 4)
  {
    rightScreen.firstPage();
    do
    {
      drawKawaii(rightScreen, y);
    } while (rightScreen.nextPage());

    leftScreen.firstPage();
    do
    {
      drawKawaii(leftScreen, y);
    } while (leftScreen.nextPage());
  }
  delay(duration);
  for (int y = 8; y >= 0; y -= 4)
  {
    rightScreen.firstPage();
    do
    {
      drawKawaii(rightScreen, y);
    } while (rightScreen.nextPage());

    leftScreen.firstPage();
    do
    {
      drawKawaii(leftScreen, y);
    } while (leftScreen.nextPage());
  }
}

/**
 *  look probably on left and/or right when it is time to do so
 */
void timedLookAround()
{
  if (millis() >= lookaround_time + lookaround_alea_period)
  {
    if (random(0, 2) == 1)
      lookRight(200);
    if (random(0, 2) == 1)
      lookLeft(200);
    lookaround_alea_period = random(5000, 10000);
    Serial.println("next look around in " + String(lookaround_alea_period));
    lookaround_time = millis();
    renderKawaii();
  }
}

void wink(U8G2_SSD1306_128X64_NONAME_1_HW_I2C screen)
{
  for (int y = 0; y < 63; y += 31)
  {
    screen.firstPage();
    do
    {
      drawKawaii(screen);
      screen.drawBox(0, 0, 128, y);
      screen.drawTriangle(0, y, 128, y, 128, y + 20);
    } while (screen.nextPage());
  }
  for (int y = 63; y >= 0; y -= 31)
  {
    screen.firstPage();
    do
    {
      drawKawaii(screen);
      screen.drawBox(0, 0, 128, y);
      screen.drawTriangle(0, y, 128, y, 128, y + 20);
    } while (screen.nextPage());
  }
}

/*
* SERVO METHODS
*/
/**
 * Get pwm according to angle
 */
int getpwm(int degrees)
{
  return map(degrees, -90, 90, SERVOMIN, SERVOMAX);
}

void syncMove(int x, int y, int z)
{
  int angle[3] = {x, y, z};
  //int amplitude = abs(currentAngles[0] - angle);

  int tempo = 30;
  while (true)
  {
    for (int i = 0; i < 3; i++)
    {
      if (abs(currentAngles[i] - angle[i]) > 0)
      {
        currentAngles[i] += currentAngles[i] > angle[i] ? -1 : 1;
      }
      //if (currentAngles[i] > angle[i]) currentAngles[i] = angle[i];
      pwm.setPWM(positions[i], 0, getpwm(currentAngles[i] + homeAngles[i]));
    }
    if (currentAngles[0] == angle[0] && currentAngles[1] == angle[1] && currentAngles[2] == angle[2])
    {
      break;
    }
    delay(tempo);
  }
}

void servo(int n, int angle)
{
  int amplitude = abs(currentAngles[n] - angle);
  int sens = currentAngles[n] > angle ? -1 : 1;
  int tempo = 30;
  for (int i = 0; i <= amplitude; i += 1)
  {
    if (i < 0.5 * amplitude)
    {
      tempo = tempo - 1;
    }
    if (i > 0.5 * amplitude)
    {
      tempo = tempo + 1;
    }
    if (tempo < 10)
      tempo = 10;
    if (tempo > 30)
      tempo = 30;
    if (currentAngles[n] != angle) currentAngles[n] += sens;
    
    //pwm.setPWM(1, 0, getpwm(currentAngles[2]));
    pwm.setPWM(positions[n], 0, getpwm(currentAngles[n] + homeAngles[n]));
    delay(tempo);
  }
}

void servoX(int angle)
{
  if (angle > 20)
    angle = 20;
  if (angle < -20)
    angle = -20;
  servo(0, angle);
}
void servoY(int angle)
{
  if (angle > 20)
    angle = 20;
  if (angle < -20)
    angle = -20;
  //Serial.println("move Y to "+String(currentAngles[1]));
  servo(1, angle);
}
void servoZ(int angle)
{
  if (angle > 30)
    angle = 30;
  if (angle < -30)
    angle = -30;
  //Serial.println("move Z to "+String(currentAngles[2]));
  servo(2, angle);
}

/**
 *  Move servos to setpoint step by step.
 *    setPointAngles may vary time to time, position should adapt
 */
void syncMoveSetPoint(){
  int tempo = 30;
  
  for (int i = 0; i < 3; i++)
  {
    if (abs(currentAngles[i] - setpointAngles[i]) > 0)
    {
      currentAngles[i] += currentAngles[i] > setpointAngles[i] ? -1 : 1;
    }
    //if (currentAngles[i] > angle[i]) currentAngles[i] = angle[i];
    
    pwm.setPWM(positions[i], 0, getpwm(currentAngles[i]));
    // switch (i)
    // {
    // case 0:
    //   servoX(homeAngles[i]);
    //   break;
    // case 1:
    //   servoY(currentAngles[i] );
    //   break;
    // case 2:
    //   servoZ(currentAngles[i] );
    // default:
    //   break;
    // }
    // Serial.println("moving to :");
    // Serial.println(currentAngles[1]);
    // Serial.println(currentAngles[2]);
  }
  // if (currentAngles[0] == setpointAngles[0] && currentAngles[1] == setpointAngles[1] && currentAngles[2] == setpointAngles[2])
  // {
  //   break;
  // }
  delay(tempo);
  
}

void receivedMotion()
{
  if (!auto_move)
  {
    return;
  }
  int rnd = random(10000);
  if (rnd == 0)
  {
    servoZ(random(-30, 30));
  }
  if (rnd == 1)
  {
    servoX(random(-20, 20));
  }
  if (rnd == 2)
  {
    servoY(random(-20, 20));
  }
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
/*
*   CHOREGRAPHIES
*/
void choreRound()
{
  Serial.println("Got to turn Around !");
  for (double a = 0; a < (4 * PI); a += 0.2)
  {
    syncMove(0, round(10 * sin(a)), round(20 * cos(a)));
  }
}

void sayNo()
{
  Serial.println("Say Nooo !");
  syncMove(0, 0, 0);
  for (int i = 0; i < 3; i++)
  {
    servoZ(-20);
    servoZ(20);
  }
  servoZ(0);
}
/*
*    COMMANDS
*/

void commandMove(int x, int y, int z)
{
  servoX(x);
  servoY(y);
  servoZ(z);
}


/*
*   L E D
*/


void ledScenario(void ) { /* function ledScenario */
  ////LEDS Strip scenario
  if (hue>254) hue=0;
  leds[0].setHSV( hue, 255, 255);
  leds[1].setHSV( hue, 255, 255);
  //leds[1] = -leds[0]; // invert color 
  //leds[1] = CRGB::White;
  //leds[1].nscale8_video(200);
  
  //leds[1] = CRGB::Red;
  // for (int i=250;i>100;i--){
  //   leds[1] = CRGB::Red;
  //   leds[1].fadeLightBy(i);  
  //   FastLED.show();
  //   delay(8);
  // }
  // for (int i=100;i<250;i++){
  //   leds[1] = CRGB::Red;
  //   leds[1].fadeLightBy(i);  
  //   FastLED.show();
  //   delay(8);
  // }
  
  //leds[1].fadeLightBy(100);
  FastLED.show();
  
  //delay(40);
  
  hue++;
}

void handleCommands(String data)
{
  if (sizeof(data)>0){
    if (data.startsWith("T"))
      // "T" Enable /disable face tracking mode
      // "T1" Enable face tracking mode
      // "T0" Disable face tracking mode
      {
        if (sizeof(data) == 1){
          face_tracking_mode = !face_tracking_mode; 
        }
        else {
          face_tracking_mode = data.substring(1) == "1";
        }
        auto_move = !face_tracking_mode;   
          

        //TODO : commande T1 T0 pour activer ou désactiver le face tracking
      }
    if (data.startsWith("M"))
    {
      data = data.substring(1);
      Serial.println("Move command : " + data);
      commandMove(getValue(data, ',', 0).toInt(), getValue(data, ',', 1).toInt(), getValue(data, ',', 2).toInt());
    }
    if (data.startsWith("X"))
    {
      data = data.substring(1);
      servoX(data.toInt());
    }
    if (data.startsWith("Y"))
    {
      data = data.substring(1);
      servoY(data.toInt());
    }
    if (data.startsWith("Z"))
    {
      data = data.substring(1);
      servoZ(data.toInt());
    }
    if (data.startsWith("S"))
    {
      data = data.substring(1);
      Serial.println("Synchro move command : " + data);
      syncMove(getValue(data, ',', 0).toInt(), getValue(data, ',', 1).toInt(), getValue(data, ',', 2).toInt());
    }
    if (data.startsWith("D"))
    {
      choreRound();
    }
    if (data.startsWith("A"))
    {
      if (sizeof(data) == 1)
      {
        auto_move = !auto_move;
      }
      else
      {
        auto_move = data.substring(1) == "1";
      }
      Serial.println("Automove is " + auto_move ? "'on'" : "'off'");
    }
    if (data.startsWith("F")){
      // coordinates to follow for face tracking F<Zvalue>,<Yvalue>
      data = data.substring(1);
      Serial.println("COMMAND F parameters :");
      Serial.println(getValue(data, ',', 0).toInt());
      Serial.println(getValue(data, ',', 1).toInt());
      Serial.println("current angles");
      Serial.println(currentAngles[1]);
      Serial.println(currentAngles[2]);
      Serial.println("setpointAngles before");
      Serial.println(setpointAngles[1]);
      Serial.println(setpointAngles[2]);
      //just set the setpoint, servos will move in the loop trying to go at positions
      setpointAngles[0] = 0;
      setpointAngles[1] = currentAngles[1] + (getValue(data, ',', 1).toInt());
      setpointAngles[2] = currentAngles[2] + (getValue(data, ',', 0).toInt());
      // servoZ(getValue(data, ',', 0).toInt()+currentAngles[2]);
      // servoY(getValue(data, ',', 1).toInt()+currentAngles[1]);
      Serial.println("setpointAngles after");
      Serial.println(setpointAngles[1]);
      Serial.println(setpointAngles[2]);
    }
    if (data.startsWith("NO")){
      sayNo();
    }
    if (data.startsWith("?"))
      {
        Serial.println("<?> : this help");
        Serial.println("<X15> : move X axis to 15°");
        Serial.println("<Y-20> : move Y axis to -20°");
        Serial.println("<Z0> : move Z axis to 0°");
        Serial.println("<S10,20,30> : move synchronously X, Y et Z respectiveley to 10, 20 and 30°");
        Serial.println("<D> : perform a round with head");
        Serial.println("<A> : enable/disable auto move");
        Serial.println("<A1> : enable auto move");
        Serial.println("<A0> : disable auto move");
        Serial.println("TODO : continue this helps ...");
      }
      receivedChars[0] = (char)0;//clear received char buffer 
      newData = false; //ready to receive next command
      
  }
  

}

/**
 *  reading serial datas
 */

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;
 
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();
        //Serial.println(rc);

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

void setup(void)
{
  Serial.begin(9600);

  rightScreen.setI2CAddress(0x3D * 2);
  leftScreen.setI2CAddress(0x3C * 2);
  rightScreen.begin();
  leftScreen.begin();

  pwm.begin();
  /*
   * In theory the internal oscillator (clock) is 25MHz but it really isn't
   * that precise. You can 'calibrate' this by tweaking this number until
   * you get the PWM update frequency you're expecting!
   * The int.osc. for the PCA9685 chip is a range between about 23-27MHz and
   * is used for calculating things like writeMicroseconds()
   * Analog servos run at ~50 Hz updates, It is importaint to use an
   * oscilloscope in setting the int.osc frequency for the I2C PCA9685 chip.
   * 1) Attach the oscilloscope to one of the PWM signal pins and ground on
   *    the I2C PCA9685 chip you are setting the value for.
   * 2) Adjust setOscillatorFrequency() until the PWM update frequency is the
   *    expected value (50Hz for most ESCs)
   * Setting the value here is specific to each individual I2C PCA9685 chip and
   * affects the calculations for the PWM update frequency. 
   * Failure to correctly set the int.osc value will cause unexpected PWM results
   */
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ); // Analog servos run at ~50 Hz updates

  blink_time = millis();
  blink_alea_period = random(1000, 5000);
  lookaround_alea_period = random(5000, 10000);
  lookaround_time = blink_time;

  Serial.println(F("Go !"));
  delay(500);
  for (int i = 0; i < 3; i++)
  {
    servo(i,homeAngles[i]);
    //pwm.setPWM(positions[i], 0, getpwm(homeAngles[i]));
  }
  Serial.setTimeout(100); 

  //Init led strips
  FastLED.addLeds<LED_TYPE, stripPin, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(  BRIGHTNESS );

}

void loop(void)
{
  // Serial.println("*");
  // delay(200);
  //ledScenario();
  recvWithStartEndMarkers(); // serial datas are stored in receivedChars var
  // if (newData){
  //   Serial.println(receivedChars);
  //   newData = false;
  // }
  handleCommands(receivedChars);
  // if (Serial.available() > 0)
  // {
  //   long t = millis();
  //   Serial.print("New Serial message");
  //   String data = Serial.readStringUntil('\n');
  //   Serial.println(millis()-t);
  //   handleCommands(data);
  //   Serial.println(millis()-t);
    
  // }

  if (face_tracking_mode)
  {
    //facetracking mode 
    syncMoveSetPoint();
    
  }
  else{
    // blink eyes if it is time to do so
    timedBlinkEyes();
    // clin d'oeil
    //wink(leftScreen);
    timedLookAround();
    int alea = random(100000);
    if (alea == 1)
    {
      sad();
    }
    if (alea == 2)
    {
      angry();
    }
  }

  //servos
  //receivedMotion();
  //choreRound();
}
