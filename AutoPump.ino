#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <Wire.h>
#include <string.h>

//Define Pin numbers and other globals
#define HEIGHT 32
#define WIDTH 128
#define PUMP_PIN D4
#define MENU_SIZE 3
#define SUB_MENU_SIZE 6
#define OLED_RESET -1
#define encoderCLK D5
#define encoderData D6
#define SELECT_PIN D0
#define disk1 0x50


long unsigned prevTime = 0;
long unsigned presentTime;
//Menu related variables
volatile int selected_item = 0;
volatile int sub_selected_item = 0;
boolean showingSubMenu = 0;

//Encoder and push button variables
volatile int currentStateCLK, lastStateCLK;
int currentBtnState, prevBtnState;

//Pump variables
boolean pumpState = LOW;
boolean autoStart = 0;
boolean turnOnPump = 0;

//Time variables
int timings[4] = { 0, 0, 0, 0 };
unsigned long toEpochTime;
unsigned long fromEpochTime;
unsigned long timeLeft;
String Data;

//Menu and subMenu Items
String menuItems[MENU_SIZE] = { "Set Duration", autoStart ? "Enabled" : "Disabled", "Reset" };
String subMenuItems[SUB_MENU_SIZE] = { "From Hour: ", "From Min: ", "To Hour: ", "To Min: ", "Done", "Cancel" };

//Create OLED, RTC object
Adafruit_SSD1306 display(WIDTH, HEIGHT, &Wire, OLED_RESET);
RTC_DS3231 rtc;
DateTime now;


void setup() {
  Serial.begin(115200);
  Wire.begin();

  PCAChannel(1);
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }
  now = rtc.now();
// rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  //Setup pinModes
  pinMode(encoderCLK, INPUT);
  pinMode(encoderData, INPUT);
  pinMode(SELECT_PIN, INPUT_PULLUP);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, !pumpState);
  prevBtnState = digitalRead(SELECT_PIN);


  //Initialize RTC
  Serial.println("DATA READ");
  readEEPROM(disk1, 0x00);
  readEEPROM(disk1, 0x10);
  Serial.println(toEpochTime);


  //Initialize oled
  PCAChannel(0);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Error initializing OLED display");
  }
  display.display();
  delay(1000);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.setCursor(0, 0);
  mainMenu();
}


void loop() {

  // Show current time
  PCAChannel(1);
  now = rtc.now();
  showCurrentTime();

  //Handle encoder changes if any
  encoderHandler();

  //Update display with latest info

  PCAChannel(0);
  display.display();

  //Handle button clicks if any

  prevBtnState = digitalRead(SELECT_PIN);
  selectHandler();

  //Set the pump on automatically if autoStart is enabled

  if (!showingSubMenu) {
    if (autoStart) {
      if ((now.unixtime() > fromEpochTime) && (now.unixtime() < toEpochTime)) {
        turnOnPump = 1;
      } else {
        turnOnPump = 0;
      }
    } else {
      turnOnPump = 0;
    }
  }

  if (pumpState != turnOnPump) {
    pumpState = !pumpState;
    if (pumpState == 0) {
      writeEEPROM(disk1, 16, ("0"));
    }
    digitalWrite(PUMP_PIN, !pumpState);
  }

  if (pumpState) {
    presentTime = millis();
    if (presentTime - prevTime > 60000) {
      unsigned long dif = toEpochTime - now.unixtime();
      String str = String((dif)) + '\0';
      Serial.println(dif);
      writeEEPROM(disk1, 16, str);
      prevTime = presentTime;
    }
  }
}

void encoderHandler() {
  currentStateCLK = digitalRead(encoderCLK);

  if (currentStateCLK != lastStateCLK && currentStateCLK == 1) {
    if (digitalRead(encoderData) != currentStateCLK) {
      if (showingSubMenu) {
        sub_selected_item = (sub_selected_item == 0) ? SUB_MENU_SIZE - 1 : abs((sub_selected_item - 1) % SUB_MENU_SIZE);
        showDuration();
      } else {
        selected_item = (selected_item == 0) ? MENU_SIZE - 1 : abs((selected_item - 1) % MENU_SIZE);
        mainMenu();
      }
    } else {
      // Encoder is rotating CW so increment
      if (showingSubMenu) {
        sub_selected_item = ((sub_selected_item + 1) % SUB_MENU_SIZE);
        showDuration();
      } else {
        selected_item = ((selected_item + 1) % MENU_SIZE);
        mainMenu();
      }
    }
  }
  lastStateCLK = currentStateCLK;
}

void selectHandler() {
  currentBtnState = digitalRead(SELECT_PIN);
  String str;
  if (currentBtnState == LOW) {
    switch (selected_item) {
      case 0:
        if (currentBtnState == prevBtnState) {
          Serial.println("delayed");
          delay(400);
        }
        showingSubMenu ? setDuration() : showDuration();
        break;

      case 1:
        if (currentBtnState == prevBtnState) {
          delay(400);
        }
        setAutoState();
        str = String(timings[0]) + "-" + String(timings[1]) + "-" + String(timings[2]) + "-" + String(timings[3]) + "-" + String(autoStart) + '\0';
        writeEEPROM(disk1, 0, str);
        break;
      case 2:
        ESP.restart();
        break;
    }
  }
}

void showCurrentTime() {
  PCAChannel(0);
  display.setCursor(0, 0);
  display.setTextColor(WHITE, BLACK);
  display.print("Time: ");
  display.print(now.hour());
  display.print(":");
  display.print(now.minute());
  display.print(":");
  display.print(now.second());
  display.println("  ");
}


void mainMenu() {
  PCAChannel(0);
  display.clearDisplay();
  display.setCursor(0, 8);
  for (int i = 0; i < MENU_SIZE; i++) {
    if (i == selected_item) {
      display.print(">");
      display.println(menuItems[i]);
      continue;
    }
    display.println(menuItems[i]);
  }
}


// void showDate() {
//   display.clearDisplay();
//   display.setCursor(0, 10);
//   display.println("Today:");
//   display.print(now.day());
//   display.print("-");
//   display.print(now.month());
//   display.print("-");
//   display.println(now.year());
// }

void setDuration() {
  // PCAChannel(1);
  String str;
  switch (sub_selected_item) {
    case 0:
      timings[0] = (timings[0] + 1) % 24;
      showDuration();
      break;
    case 1:
      timings[1] = (timings[1] + 5) % 60;
      showDuration();
      break;
    case 2:
      timings[2] = (timings[2] + 1) % 24;
      showDuration();
      break;
    case 3:
      timings[3] = (timings[3] + 2) % 60;
      showDuration();
      break;
    case 4:
      showingSubMenu = 0;
      mainMenu();
      sub_selected_item = 0;
      selected_item = 0;
      str = String(timings[0]) + "-" + String(timings[1]) + "-" + String(timings[2]) + "-" + String(timings[3]) + "-" + autoStart + '\0';
      Serial.println(str);
      // Copy it over
      writeEEPROM(disk1, 0, str);
      setEpochTime('t');
      setEpochTime('f');
      break;

    case 5:
      showingSubMenu = 0;
      mainMenu();
      sub_selected_item = 0;
      selected_item = 0;
      break;
  }
}

void showDuration() {
  showingSubMenu = 1;

  PCAChannel(0);
  display.clearDisplay();
  display.setCursor(0, 8);
  Serial.println(sub_selected_item);
  int subMenuValues[SUB_MENU_SIZE] = { timings[0], timings[1], timings[2], timings[3] };


  if (sub_selected_item <= 1) {
    for (int i = 0; i <= 1; i++) {
      if (i == sub_selected_item) {
        display.print(">");
      }
      display.print(subMenuItems[i]);
      display.println(subMenuValues[i]);
    }
  } else if (sub_selected_item <= 3) {
    for (int i = 2; i <= 3; i++) {
      if (i == sub_selected_item) {
        display.print(">");
      }
      display.print(subMenuItems[i]);
      display.println(subMenuValues[i]);
    }
  } else {
    for (int i = 4; i <= 5; i++) {
      if (i == sub_selected_item) {
        display.print(">");
      }
      display.println(subMenuItems[i]);
    }
  }
}


void setAutoState() {
  autoStart = !autoStart;
  menuItems[1] = autoStart ? "Enabled" : "Disabled";
  mainMenu();
}


void writeEEPROM(int deviceaddress, unsigned int eeaddress, String data) {
  PCAChannel(3);

  // Convert the string to an array of bytes
  byte* byteArr = (byte*)data.c_str();

  // Write the array of bytes to the EEPROM
  Wire.beginTransmission(deviceaddress);
  Wire.write(eeaddress >> 8);    // Send the high byte of the memory address
  Wire.write(eeaddress & 0xFF);  // Send the low byte of the memory address
  for (int i = 0; i < data.length(); i++) {
    Wire.write(byteArr[i]);
  }
  Wire.endTransmission();


  delay(6);  // needs 5ms for page write
  Serial.println("success");
}

void readEEPROM(int deviceaddress, byte eeaddress) {
  PCAChannel(3);

  int numBytes = 13;  // The number of bytes we want to read

  // Read the bytes from the EEPROM
  Wire.beginTransmission(deviceaddress);
  Wire.write(eeaddress >> 8);    // Send the high byte of the memory address
  Wire.write(eeaddress & 0xFF);  // Send the low byte of the memory address
  Wire.endTransmission();
  Wire.requestFrom(deviceaddress, numBytes);
  byte* byteArr = new byte[numBytes];
  for (int i = 0; i < numBytes; i++) {
    byteArr[i] = Wire.read();
  }

  // Convert the array of bytes back to a string
  Data = String((char*)byteArr);

  delete[] byteArr;  // Free the memory allocated for the byte array

  delay(100);

  if (eeaddress == 0x00) {
    char* pch;
    char str[Data.length()];
    Data.toCharArray(str, Data.length() + 1);
    pch = strtok(str, "-");
    int j = 0;
    while (pch != NULL) {
      // printf("%s\n", pch);
      if (j < SUB_MENU_SIZE - 2) {
        timings[j] = String(pch).toInt();
      } else {
        //Set autoState
        Serial.println("AutoState");
        printf("%s\n", pch);
        autoStart = String(pch).toInt();
        menuItems[1] = autoStart ? "Enabled" : "Disabled";
      }
      pch = strtok(NULL, "-");
      j += 1;
    }
    setEpochTime('t');
    setEpochTime('f');
  } else {
    Serial.println(Data);
    timeLeft = Data.toInt();
    toEpochTime += timeLeft;
  }
}

void setEpochTime(char type) {
  struct tm t;
  time_t epoch;
  t.tm_year = now.year() - 1900;
  t.tm_mon = (now.month() - 1) % 12;
  t.tm_mday = now.day();
  t.tm_hour = (type == 't') ? timings[2] : timings[0];
  t.tm_min = (type == 't') ? timings[3] : timings[1];
  t.tm_sec = 0;

  epoch = mktime(&t);
  (type == 't') ? (toEpochTime = ((unsigned long)epoch)) : (fromEpochTime = ((unsigned long)epoch));
}

void PCAChannel(uint8_t i) {
  Wire.beginTransmission(0x70);
  Wire.write(1 << i);
  Wire.endTransmission();
}
