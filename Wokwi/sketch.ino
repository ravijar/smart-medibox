#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>

// oled parameters
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1   // same reset pin as esp for the oled
#define SCREEN_ADDRESS 0x3c

// pin mappings
#define BUZZER 5
#define LED 15
#define PB_CANCEL 34
#define PB_OK 32
#define PB_UP 33
#define PB_DOWN 35
#define DHTPIN 12

// ntp constants
#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET_DST 0

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dht_sensor;

// time parameters
int days = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;
int month = 0;
String months[] = {"JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC"};
int utc_offsets[] = {-43200, -39600, -36000, -34200, -32400, -28800, -25200, -21600, -18000, -16200, -14400, -12600,
                   -10800, -7200, -3600, 0, 3600, 7200, 10800, 12600, 14400, 16200, 18000, 19800, 20700, 21600, 23400,
                    25200, 28800, 31500, 32400, 34200, 36000, 37800, 39600, 43200, 45900, 46800, 50400};
int n_utc_offsets = sizeof(utc_offsets) / sizeof(utc_offsets[0]);
int utc_offset = 19800; // UTC+05:30 is set as the default timezone

// alarm parameters
int n_alarms = 3;
int alarm_hours[] = {0,0,0};
int alarm_minutes[] = {0,0,0};
bool alarm_triggered[] = {true,true,true}; // initially all the alarms are disabled.

// delay times
int btn_debounce_delay = 200;
int btn_after_pressed_delay = 50;
int alert_delay = 3000;

// buzzer parameters
int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int C_H = 523;
int notes[] = {C,D,E,F,G,A,B,C_H};
int n_notes = 8;

// navigation menu parameters
int current_mode = 0;
int n_modes = 5;
String modes[] = {
  "Set Time  Zone",
  "Set Alarm     1",
  "Set Alarm     2",
  "Set Alarm     3",
  "Disable   Alarms"
};

// dht sensor min-max temperature & humidity values
int max_tem = 32;
int min_tem = 26;
int max_hum = 80;
int min_hum = 60;

void setup() {
  // put your setup code here, to run once:
  pinMode(BUZZER, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);

  dht_sensor.setup(DHTPIN, DHTesp::DHT22);

  // intitialize OLED display and Serial Monitor
  Serial.begin(115200);
  if(! display.begin(SSD1306_SWITCHCAPVCC,SCREEN_ADDRESS)){
    Serial.println("SSD1306 allocation failed!");
    for(;;);
  }
  
  // turn on OLED display
  display.display();
  delay(2000);

  // initialize wifi
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("CONNECTING TO WIFI...",0,0,1);
  }

  display.clearDisplay();
  print_line("CONNECTED TO WIFI!",0,0,1);
  delay(1000);

  configTime(utc_offset, UTC_OFFSET_DST, NTP_SERVER);

  display.clearDisplay();
  print_line("Welcome to MediBox!",0,0,2);
  display.clearDisplay();
  
}

void loop() {
  // put your main code here, to run repeatedly:
  update_time_with_check_alarm();
  check_temp();
  if(digitalRead(PB_OK) == LOW){
    delay(btn_debounce_delay);
    go_to_menu();
  }
}

void print_line(String text, int column, int row, int text_Size){
  // display a custom message
  display.setTextSize(text_Size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row);
  display.println(text);

  display.display();
}

String get_two_digit_string(int value){
  // returns the string of the input value in two digit format
  String output = "";
  if(value<10){
    output +='0';
  }
  output += String(value);
  return output;
}

void print_time_now(){
  display.clearDisplay();
  print_line(get_two_digit_string(hours)+":"+get_two_digit_string(minutes)+":"
    +get_two_digit_string(seconds)+"\n\n   "+months[month]+" "+String(days),17,0,2);
}

void update_time(){
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  char timeHour[3];
  strftime(timeHour,3, "%H", &timeinfo);
  hours = atoi(timeHour);

  char timeMinute[3];
  strftime(timeMinute,3, "%M", &timeinfo);
  minutes = atoi(timeMinute);

  char timeSecond[3];
  strftime(timeSecond,3, "%S", &timeinfo);
  seconds = atoi(timeSecond);

  char timeDay[3];
  strftime(timeDay,3, "%d", &timeinfo);
  days = atoi(timeDay);

  month = timeinfo.tm_mon;
}

String get_utc_offset_string(int offset){
  // returns the utc offset in UTC+HH:MM or UTC-HH:MM format
  String output = "UTC";
  if(offset<0){
    offset = -offset;
    output += "-";
  }else{
    output += "+";
  }
  output += get_two_digit_string(offset/3600)+":";
  output += get_two_digit_string((offset%3600)/60);
  return output;
}

void ring_alarm(){
  bool break_happened = false;

  display.clearDisplay();
  print_line("MEDICINE",20,15,2);
  print_line("TIME!",37,40,2);

  int i = 0;
  while(!break_happened && digitalRead(PB_CANCEL) == HIGH){
    if(digitalRead(PB_CANCEL) == LOW){
      delay(btn_debounce_delay);
      break_happened = true;
      break;
    }
    // creates buzzer tone
    tone(BUZZER, notes[i]);
    digitalWrite(LED, HIGH);
    delay(120);
    digitalWrite(LED, LOW);
    delay(120);
    noTone(BUZZER);
    delay(10);
    i+=5;
    i%=n_notes;
  }

  display.clearDisplay();
}

void update_time_with_check_alarm(){
  update_time();
  print_time_now();

  for(int i=0; i<n_alarms; i++){
    if(!alarm_triggered[i] && alarm_hours[i] == hours && alarm_minutes[i] == minutes){
      ring_alarm();
      alarm_triggered[i] = true;
    }
  }
}

int wait_for_button_press(){
  while(true){
    if(digitalRead(PB_UP) == LOW){
      delay(btn_debounce_delay);
      return PB_UP;
    }

    if(digitalRead(PB_DOWN) == LOW){
      delay(btn_debounce_delay);
      return PB_DOWN;
    }

    if(digitalRead(PB_OK) == LOW){
      delay(btn_debounce_delay);
      return PB_OK;
    }

    if(digitalRead(PB_CANCEL) == LOW){
      delay(btn_debounce_delay);
      return PB_CANCEL;
    }

    update_time();
  }
}

void go_to_menu(){
  while(digitalRead(PB_CANCEL) == HIGH){
    display.clearDisplay();
    print_line("["+String(current_mode+1)+"]",57,0,1);  // displays mode number
    print_line(modes[current_mode],0,18,2);
    print_line("|",116,current_mode*12,2);  // displays scroll bar

    int pressed = wait_for_button_press();
    if(pressed == PB_UP){
      delay(btn_after_pressed_delay);
      current_mode -= 1;
      if(current_mode<0){
        current_mode = n_modes - 1;
      }
    }
    else if(pressed == PB_DOWN){
      delay(btn_after_pressed_delay);
      current_mode += 1;
      current_mode %= n_modes;
    }    
    else if(pressed == PB_OK){
      delay(btn_after_pressed_delay);
      run_mode(current_mode);
    }
    else if(pressed == PB_CANCEL){
      delay(btn_after_pressed_delay);
      break;
    }
  }  
}

void set_time_zone(){
  int temp_index = 23;  // UTC+05:30 is set as the default timezone
  bool time_zone_set = false;

  while(true){
    display.clearDisplay();
    print_line("Select Time Zone",16,10,1);
    print_line(get_utc_offset_string(utc_offsets[temp_index]),7,30,2);

    int pressed = wait_for_button_press();
    if(pressed == PB_DOWN){
      delay(btn_after_pressed_delay);
      temp_index += 1;
      temp_index %= n_utc_offsets;
    }
    else if(pressed == PB_UP){
      delay(btn_after_pressed_delay);
      temp_index -= 1;
      if(temp_index<0){
        temp_index = n_utc_offsets-1;
      }
    }
    else if(pressed == PB_OK){
      delay(btn_after_pressed_delay);
      utc_offset = utc_offsets[temp_index];
      configTime(utc_offset, UTC_OFFSET_DST, NTP_SERVER);
      time_zone_set = true;
      break;
    }
    else if(pressed == PB_CANCEL){
      delay(btn_after_pressed_delay);
      break;
    }
  }

  display.clearDisplay();

  if(time_zone_set){
    print_line("Time Zone set to",10,10,1);
    print_line(get_utc_offset_string(utc_offsets[temp_index]),10,30,2);
  }else{
    print_line("Time Zone setup",10,20,1);
    print_line("cancelled!",10,30,1);
  }
  
  delay(alert_delay);
}

void set_alarm(int alarm){
  bool alarm_hour_set = false;
  bool alarm_minute_set = false;
  int temp_hour = alarm_hours[alarm];

  while(true){
    display.clearDisplay();
    print_line("Enter hour",15,10,1);
    print_line("> "+get_two_digit_string(temp_hour),15,30,2);

    int pressed = wait_for_button_press();
    if(pressed == PB_UP){
      delay(btn_after_pressed_delay);
      temp_hour += 1;
      temp_hour %= 24;
    }
    else if(pressed == PB_DOWN){
      delay(btn_after_pressed_delay);
      temp_hour -= 1;
      if(temp_hour<0){
        temp_hour = 23;
      }
    }
    else if(pressed == PB_OK){
      delay(btn_after_pressed_delay);
      alarm_hours[alarm] = temp_hour;
      alarm_hour_set = true;
      break;
    }
    else if(pressed == PB_CANCEL){
      delay(btn_after_pressed_delay);
      break;
    }
  }

  int temp_minute = alarm_minutes[alarm];

  while(true){
    display.clearDisplay();
    print_line("Enter minute",15,10,1);
    print_line("> "+get_two_digit_string(temp_minute),15,30,2);

    int pressed = wait_for_button_press();
    if(pressed == PB_UP){
      delay(btn_after_pressed_delay);
      temp_minute += 1;
      temp_minute %= 60;
    }
    else if(pressed == PB_DOWN){
      delay(btn_after_pressed_delay);
      temp_minute -= 1;
      if(temp_minute<0){
        temp_minute = 59;
      }
    }
    else if(pressed == PB_OK){
      delay(btn_after_pressed_delay);
      alarm_minutes[alarm] = temp_minute;
      alarm_minute_set = true;
      break;
    }
    else if(pressed == PB_CANCEL){
      delay(btn_after_pressed_delay);
      break;
    }
  }

  display.clearDisplay();
  
  if(alarm_hour_set && alarm_minute_set){
    alarm_triggered[alarm] = false;   // enabling the relevant alarm.
    print_line("Alarm " + String(alarm+1) + " set to",10,10,1);
    print_line(get_two_digit_string(temp_hour) + ":" + get_two_digit_string(temp_minute),10,30,2);  
  }else{
    print_line("Alarm "+String(alarm+1)+" setup",10,20,1);
    print_line("cancelled!",10,30,1);
  }
  delay(alert_delay);
  
}

void disable_alarms(){
  // disabling all the alarms.
  for(int i=0;i<n_alarms;i++){
    alarm_triggered[i] = true;
  }
  display.clearDisplay();
  print_line("All Alarms Disabled!",5,10,1);
  delay(alert_delay);
}

void run_mode(int mode){
  if(mode == 0){
    set_time_zone();
  }
  else if(mode == 1 || mode == 2 || mode == 3){
    set_alarm(mode - 1);
  }
  else if(mode == 4){
    disable_alarms();
  }
}

void check_temp(){
  TempAndHumidity data = dht_sensor.getTempAndHumidity();
  float tem = data.temperature;
  float hum = data.humidity;
  bool danger = true;
  String output = "";

  if(tem>max_tem && hum>max_hum){
    output = "HIGH TEM | HIGH HUM";
  }else if(tem>max_tem && hum<min_hum){
    output = "HIGH TEM | LOW HUM";
  }else if(tem<min_tem && hum<min_hum){
    output = "LOW TEM | LOW HUM";
  }else if(tem<min_tem && hum>max_hum){
    output = "LOW TEM | HIGH HUM";
  }else if(tem>max_tem){
    output = "HIGH TEM";
  }else if(tem<min_tem){
    output = "LOW TEM";
  }else if(hum>max_hum){
    output = "HIGH HUM";
  }else if(hum<min_hum){
    output = "LOW HUM";
  }else{
    danger = false;
  }

  if(danger){
    print_line(output,8,55,1);
    digitalWrite(LED, HIGH);
  }else{
    digitalWrite(LED, LOW);
  }
}