#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
#include <avr/eeprom.h>
#include <EEPROM.h>

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

typedef enum state_e {SYNCHRONISATION, AFTER_SYNCHRONISATION, MAIN} state_t;

long isPressing = 0;
bool isShown = false;
bool isLongDetected = false;
int top = 0;
int bottom = 0;
long timeTracker = 0;
int x = 0;
int y = 1;
int count = 0;

#define PURPLE 5
#define WHITE 7
#define GREEN 2
#define TEAL 6
#define BLUE 4
#define RED 1
#define YELLOW 3

//custom down arrow
byte DOWN[8] = {
  B00000,
  B00000,
  B00100,
  B00100,
  B00100,
  B11111,
  B01110,
  B00100
};

//custom up arrow
byte UP[8] = {
  B00100,
  B01110,
  B11111,
  B00100,
  B00100,
  B00100,
  B00000,
  B00000
};

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else // __ARM__
extern char *__brkval;
#endif // __arm__
int freeMemory() {
char top;
#ifdef __arm__
return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
return &top - __brkval;
#else // __arm__
return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif // __arm__
}

struct Channel {
  char description[15];
  char ID;
  byte value;
  byte minimum;
  byte maximum;
  byte valueCount;
  byte average;
  byte channelChange;
};

const int ALPHA = 26;
const char ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";


void alphabeticalOrder(struct Channel *channelArray) {
  //count number of channels made
  int count = 0;
  for(int i = 0; i <ALPHA;i++){
    if(channelArray[i].ID != ' '){
      count++;
    }else{
      break;
    }
  }

  
  bool swap = true;
  while(swap) {
    swap = false;
    for(byte i = 0; i < count-1; i++) {
      if(channelArray[i].ID > channelArray[i+1].ID) {
        struct Channel temporary = channelArray[i+1];
        channelArray[i+1] = channelArray[i];
        channelArray[i] = temporary;
        swap = true;
      }
    } 
  } 
}

void information(struct Channel channelArray[]) {
  Serial.println("Printing...");

  for (byte i = 0; i < ALPHA; i++) {
    Serial.print(F("Channel: "));
    Serial.print(channelArray[i].ID);
    Serial.print(F(" Description: "));
    Serial.print(channelArray[i].description);
    Serial.print(F(" Value: "));
    Serial.print(channelArray[i].value);
    Serial.print(F(" Max: "));
    Serial.print(channelArray[i].maximum);
    Serial.print(F(" Min: "));
    Serial.print(channelArray[i].minimum);
    Serial.print(F(" Average: "));
    Serial.println(channelArray[i].average);
  }
  return;
}

//returns the index in channelArray of the channel with given character id
//returning -1 indicates that channel has not been made yet
int get_index(struct Channel channelArray[], char id) {
  for (byte i = 0; i < ALPHA; i++) {
    if (channelArray[i].ID == id) {
      return i;
    }
  }
  return -1;
}

//gets index of the next location of empty channel
//returning -1 indicates there are no more free channels
int next_free_channel(struct Channel channelArray[]) {
  for (byte i = 0; i < ALPHA; i++) {
    if (channelArray[i].ID == ' ') {
      return i;
    }
  }
  return -1;
}


void creation(struct Channel channelArray[], char declarations, char descriptions[15]) {

  //get index in channelArray of the channel with given letter (-1 indicates channel not found in array)
  int i = get_index(channelArray, declarations);

  //if channel has not been created yet then create it
  if (i == -1) {
    i = next_free_channel(channelArray);
    //if next_free_channel returns -1 then it means there are no more channels available
    if (i == -1) {
      Serial.println(F("ERROR: no more free channels"));
      return;
    }
    //change character id of new channel
    channelArray[i].ID = declarations;
  }

  //change channel description
  strcpy(channelArray[i].description, descriptions);
  //end of creation function
}

void newValue(struct Channel channelArray[], char valueDeclaration, byte value) {
  //get index of channel to edit
  int i = get_index(channelArray, valueDeclaration);
  //if i is -1 then the channel does not exist so show error
  if (i == -1) {
    Serial.println("ERROR: channel has not been created yet so can't change value");
    return;
  }
  channelArray[i].value = value;
  channelArray[i].valueCount = channelArray[i].valueCount + value;
  channelArray[i].channelChange++;
  channelArray[i].average = (channelArray[i].valueCount / channelArray[i].channelChange); 
}

void newMax(struct Channel channelArray[], char maxDeclaration, byte maximum) {
  //get index of channel to edit
  int i = get_index(channelArray, maxDeclaration);
  //if i is -1 then the channel does not exist so show error
  if (i == -1) {
    Serial.println("ERROR: channel has not been created yet so can't change maximum value");
    return;
  }
  channelArray[i].maximum = maximum;
}

void ScreenUpdate(struct Channel channelArray[]) {
  bool above = false;
  bool below = false;

  lcd.clear();
  struct Channel temp = channelArray[x];

  if (temp.ID != ' ') {
    lcd.setCursor(0, 0);
    if (x == 0) {
      lcd.print(' ');
    } else {
      lcd.write(byte(1));
    }
    lcd.print(temp.ID);
    if(temp.value < 100) {
      lcd.print(" ");
    }
    if(temp.value < 10) {
      lcd.print(" ");
    }
    if(temp.value < 1) {
      lcd.print(" ");
    }
    else {
      lcd.print(temp.value);
    }
    lcd.print(",");
    if(temp.average < 100) {
      lcd.print(" ");
    }
    if(temp.average < 10) {
      lcd.print(" ");
    }
    if(temp.average < 1) {
      lcd.print(" ");
    }
    else {
      lcd.print(temp.average);
    }
    lcd.print(" ");
    lcd.print(temp.description);
  }

  temp = channelArray[y];
  if (temp.ID != ' ') {
    lcd.setCursor(0, 1);
    if (y != ALPHA && channelArray[y + 1].ID != ' ') {
      lcd.write(byte(2));
    }
    else {
      lcd.print(' ');
    }
    lcd.print(temp.ID);
    if(temp.value < 100) {
      lcd.print(" ");
    }
    if(temp.value < 10) {
      lcd.print(" ");
    }
    if(temp.value < 1) {
      lcd.print(" ");
    }
    else {
      lcd.print(temp.value);
    }
    lcd.print(",");
    if(temp.average < 100) {
      lcd.print(" ");
    }
    if(temp.average < 10) {
      lcd.print(" ");
    }
    if(temp.average < 1) {
      lcd.print(" ");
    }
    else {
      lcd.print(temp.average);
    }
    lcd.print(" ");
    lcd.print(temp.description);
  }

  count = 0;
  for (int i = 0; i < ALPHA; i++) {
    if (channelArray[i].value > channelArray[i].maximum) {
      above = true;
    }
    if (channelArray[i].value < channelArray[i].minimum) {
      below = true;
    }
    if (channelArray[i].ID != ' ') {
      count++;
    }
  }

  if (above & below) {
    lcd.setBacklight(YELLOW);
  }
  else if (below) {
    lcd.setBacklight(GREEN);
  }
  else if (above) {
    lcd.setBacklight(RED);
  }
  else {
    lcd.setBacklight(WHITE);
  }
}


void newMin(struct Channel channelArray[], char minDeclaration, byte minimum) {
  //get index of channel to edit
  int i = get_index(channelArray, minDeclaration);
  //if i is -1 then the channel does not exist so show error
  if (i == -1) {
    Serial.println("ERROR: channel has not been created yet so can't change minimum value");
    return;
  }
  channelArray[i].minimum = minimum;
}

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.setBacklight(PURPLE);//purple
  lcd.createChar(1, UP);
  lcd.createChar(2, DOWN);
}

void loop() {
  static state_t state = SYNCHRONISATION;
  
  //create array of channels
  static struct Channel channelArray[ALPHA];

  //set the starting data for the channels
  static bool finish = false;
  if(!finish) {
    for (int i = 0; i < ALPHA; i++) {
      struct Channel x;
      x.description[0] = '\0';
      x.value = 0;
      x.ID = ' ';
      x.minimum = 0;
      x.maximum = 255;
      x.valueCount = 0;
      x.average = 0;
      x.channelChange = 0;
      channelArray[i] = x;
    }
  }
  finish = true;

  static int lastButtons = 0;

  switch (state) {
    //synchronisation phase
    case SYNCHRONISATION: {
      bool loopit = true;
      while(loopit){
        Serial.print('Q');
        delay(1000);
        char letter = Serial.read();
        if (letter == 'X' || letter == 'x') {
         
          loopit = false;
        }
      }
      state = AFTER_SYNCHRONISATION;
      break;
        
      }

    case AFTER_SYNCHRONISATION: {
         Serial.println("RECENT, FREERAM, SCROLL, UDCHARS");
          lcd.setBacklight(WHITE);//white
          state = MAIN;
          break;
      }

    //main phase loop
    case MAIN: {

if(!isShown) {
  if(500 + timeTracker < millis()) {
            timeTracker = millis();
            top++;
            bottom++;
            if(top > 9 or top > strlen(channelArray[x].description)-6) {
              top = 0;
            }
            if(bottom > 9 or bottom > strlen(channelArray[y].description)-6) {
              bottom = 0;
            }
          }
          
          if(strlen(channelArray[x].description) > 6) {
            lcd.setCursor(10,0);
            for(int i = 0; i < 7; i++) {
              lcd.print(channelArray[x].description[top+i]);
            }
          }
          if(strlen(channelArray[y].description) > 6) {          
            lcd.setCursor(10,1);
            for(int i = 0; i < 7; i++) {
              lcd.print(channelArray[y].description[bottom+i]);
            }
          } 
}

      
        if (Serial.available() > 0) {
          String userInput = Serial.readString();
          char character = userInput[0];
          char channelLetter = userInput[1];
          char data[15] = "              ";//create new blank char array
          //copy the userInput to data starting from userInput[2]
          for (int i = 0; i < strlen(data); i++) {
            data[i] = userInput[i + 2];
            if (data[i] == '\0') break; //if line end character reached then stop
          }

          
          

          switch (character) {
            case 'C':
              creation(channelArray, channelLetter, data);
              alphabeticalOrder(channelArray);
              information(channelArray);
              ScreenUpdate(channelArray);
              break;

            case 'V':
              newValue(channelArray, channelLetter, atoi(data));
              information(channelArray);
              ScreenUpdate(channelArray);
              break;
            case 'X':
              newMax(channelArray, channelLetter, atoi(data));
              information(channelArray);
              ScreenUpdate(channelArray);
              break;
            case 'N':
              newMin(channelArray, channelLetter, atoi(data));
              information(channelArray);
              ScreenUpdate(channelArray);
              break;
            default:
              Serial.println("ERROR: invalid input");
              break;
          } //switch end

        }//end of input

        int button_state = lcd.readButtons();

        if (lastButtons != button_state) {
          if (button_state & BUTTON_UP) {
            //Serial.print("up");
            if (x != 0) {
              x = x - 1;
              y = y - 1;
              ScreenUpdate(channelArray);
            }
          }
          else if (button_state & BUTTON_DOWN) {
            //Serial.print("down");
            if (y != (count - 1)) {
              x = x + 1;
              y = y + 1;
              ScreenUpdate(channelArray);
            }
            if (count < 3) {
              x = 0;
              y = 1;
              ScreenUpdate(channelArray);
            }
          }
          if (isLongDetected & !(button_state & BUTTON_SELECT)) {
            isShown = false;
            isLongDetected = false;
            ScreenUpdate(channelArray);
          }
        }
        lastButtons = button_state;
        if (button_state & BUTTON_SELECT) {
          if (!isLongDetected) {
            isLongDetected = true;
            isPressing = millis();
          }
          else if (millis() - isPressing > 1000 & !(isShown)) {
            isShown = true;
            lcd.clear();
            lcd.setBacklight(PURPLE);
            lcd.print("F135306");
            lcd.setCursor(0, 1);
            lcd.print("Free SRAM: ");
            lcd.print(freeMemory());
          }

        } //SELECT end
        lastButtons = button_state;

      }//main switch case end
  }
} //setup end
