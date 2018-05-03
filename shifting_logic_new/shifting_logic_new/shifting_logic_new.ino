

#include <LiquidCrystal.h>

#define wheel_diameter 0.68

unsigned int gear;
int counter = 0;
int g_counter = 0;
float upshift_speed[7] = {9.86, 11.46, 12.51, 13.77, 15.32, 17.25, 19.71};
float downshift_speed[7] = {4.23, 4.91, 5.36, 5.9, 6.56, 7.39, 8.43};
int upshift_timing[7] = {400, 400, 400, 400, 400, 400, 400};
int compensate_timing[7] = {150, 150, 100, 100, 100, 100, 100};
int downshift_timing[7] = {700, 600, 550, 550, 550, 550, 550};
int upshift_cadence=50;
int downshift_cadence=40;

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 7, en = 8, d4 = 9, d5 = 10, d6 = 11, d7 = 12;
LiquidCrystal lcd(4,5,6,7,8,9);//(rs, en, d4, d5, d6, d7);

//buttons 
const int up_buttonPin = A2;
const int down_buttonPin = A3;
const int manual_buttonPin = 2;
int up_buttonState = 0;
int pre_up = 0;
int down_buttonState = 0;
int pre_down = 0;
int manual_buttonState = 0;
int pre_manual = 0;
int control_mode = 0; //0 auto, 1 manual

//Motor 
const int motor_driver1 = A5;
const int motor_driver2 = A4;

//Sensors
#define hall_speed A0
#define hall_cadence A1

//storage variables
float radius = 13.35;// tire radius (in inches)- CHANGE THIS FOR YOUR OWN BIKE

int hall_speed_val;
long speed_timer = 0;// time between one full rotation (in ms)
float bike_speed = 0.00;
float circumference;

int hall_cadence_val;
long cadence_timer = 0;
float cadence = 0.00;

int hall_speed_prev;
int hall_cadence_prev;

int maxcounter = 100;//min time (in ms) of one rotation (for debouncing)
int cadence_maxcounter = 100;//min time (in ms) of one rotation (for debouncing)
int hall_speed_counter;
int hall_cadence_counter;
bool cadence_flag = false;
bool speed_flag = false; 

void setup()
{
    
  hall_speed_counter = maxcounter;
  hall_cadence_counter = cadence_maxcounter;
  circumference = 2*3.14*radius;
  // pinMode(1,OUTPUT);//tx
  // pinMode(2,OUTPUT);//backlight switch
  pinMode(hall_speed, INPUT); //digital
  pinMode(hall_cadence, INPUT); //digital

  //motor control pins
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);

  //button pins
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(3, INPUT);
  
  // checkBacklight();
  
  // Serial.write(12);//clear
  
  // TIMER SETUP- the timer interrupt allows preceise timed measurements of the reed switch
  //for mor info about configuration of arduino timers see http://arduino.cc/playground/Code/Timer1
  cli();//stop interrupts

  //set timer1 interrupt at 1kHz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;
  // set timer count for 1khz increments
  OCR1A = 1999;// = (1/1000) / ((1/(16*10^6))*8) - 1
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS11 bit for 8 prescaler
  TCCR1B |= (1 << CS11);   
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  
  sei();//allow interrupts
  //END TIMER SETUP
  
  Serial.begin(9600);

  //initialize gear to 0
  gear = 0;

  //set up the LCD screen
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  lcd.setCursor(13, 1);
  lcd.print("AT");
  lcd.setCursor(13, 0);
  lcd.print(gear+1);
}


ISR(TIMER1_COMPA_vect) {//Interrupt at freq of 1kHz to measure reed switch
  //speed
  hall_speed_prev = hall_speed_val;
  hall_speed_val = digitalRead(hall_speed);//get val of A0
//  Serial.println(speed_flag);
  if (hall_speed_val == 0 && hall_speed_prev == 1){//if reed switch is closed
    if (hall_speed_counter == 0){//min time between pulses has passed
      //56.8mph = 1 inch per ms
      bike_speed = (56.8*float(circumference))/float(speed_timer);//calculate miles per hour
      speed_timer = 0;//reset speed_timer
      hall_speed_counter = maxcounter;//reset hall_speed_counter
    }
    else{
      if (hall_speed_counter > 0){//don't let hall_speed_counter go negative
        hall_speed_counter -= 1;//decrement hall_speed_counter
      }
    }
  }
  else{//if reed switch is open
    if (hall_speed_counter > 0){//don't let hall_speed_counter go negative
      hall_speed_counter -= 1;//decrement hall_speed_counter
    }
  }
  if (speed_timer > 2000){
    bike_speed = 0;//if no new pulses from reed switch- tire is still, set bike_speed to 0
  }
  else{
    speed_timer += 1;//increment speed_timer
  } 

  //cadence
  hall_cadence_prev = hall_cadence_val;
  hall_cadence_val = digitalRead(hall_cadence);//get val of A0
  if (hall_cadence_val == 0 && hall_cadence_prev == 1){//if reed switch is closed
    if (hall_cadence_counter == 0){//min time between pulses has passed
      //rpm
      //60*1000ms
      cadence = float(60)/(float(cadence_timer)/1000);//calculate rpm
      cadence_timer = 0;//reset cadence_timer
      hall_cadence_counter = cadence_maxcounter;//reset hall_cadence_counter
    }
    else{
      if (hall_cadence_counter > 0){//don't let hall_cadence_counter go negative
        hall_cadence_counter -= 1;//decrement hall_cadence_counter
      }
    }
  }
  else{//if reed switch is open
    if (hall_cadence_counter > 0){//don't let hall_cadence_counter go negative
      hall_cadence_counter -= 1;//decrement hall_cadence_counter
    }
  }
  if (cadence_timer > 3000){
    cadence = 0;//if no new pulses from reed switch- tire is still, set cadence to 0
  }
  cadence_timer += 1;//increment cadence_timer 
}

void loop()//Measure RPM
{
  delay(50);
  counter ++; 
  g_counter ++;
  // //update cadence
  // if (cadence_rev > 0){
  //   cadence = 30 * 1000 / (millis() - cadence_timeold) * cadence_rev;
  //   cadence *= 2;
  //   //reset timer
  //   cadence_timeold = millis();
      
  //   //reset counters
  //   cadence_rev = 0;
  // }
  // //update speed
  // if (speed_rev > 0){
  //   speed_rpm = 30 * 1000 / (millis() - speed_timeold) * speed_rev;
  //   speed_rpm *= 2;
  //   bike_speed = speed_rpm * (2 * 3.14) / 60 * wheel_diameter / 2.0;
  //   //reset timer
  //   speed_timeold = millis();
      
  //   //reset counters
  //   speed_rev = 0;
  // }

  //update LCD
  // (note: line 1 is the second row, since counting begins with 0):
  if (counter >= 20)
  {
    
    lcd.setCursor(0, 0);
    // print the number of seconds since reset:
    lcd.print(bike_speed);
    lcd.print("  ");
//    Serial.print(bike_speed);
//    Serial.print('\n');

//    lcd.setCursor(13, 0);
//    lcd.print(gear+1);

    lcd.setCursor(0, 1);
    lcd.print(cadence);
    lcd.print("  ");
//    Serial.println(cadence);
    counter = 0;
//    Serial.println((float)bike_speed/cadence);
  }
  // Serial.println(cadence, DEC);

  //buttons
  pre_manual = manual_buttonState;
  manual_buttonState = digitalRead(manual_buttonPin);
  pre_up = up_buttonState;
  up_buttonState = digitalRead(up_buttonPin);
  pre_down = down_buttonState;
  down_buttonState = digitalRead(down_buttonPin);

  if (manual_buttonState == HIGH && pre_manual == LOW) {
    if (control_mode == 0) {
      control_mode = 1;
      lcd.setCursor(13, 1);
      lcd.print("MT");
      Serial.println("MT");
      }
    else {
      control_mode = 0;
      lcd.setCursor(13, 1);
      lcd.print("AT");
      Serial.println("AT");
      }
  }

  if (control_mode == 1) {
    if (up_buttonState == HIGH && pre_up == LOW && gear != 6) {
      //upshift
      digitalWrite(motor_driver1, HIGH);
      digitalWrite(motor_driver2, LOW);
      delay(upshift_timing[gear]);
      digitalWrite(motor_driver1, LOW);
//      digitalWrite(motor_driver2, HIGH);
//      delay(compensate_timing[gear]);
//      digitalWrite(motor_driver2, LOW);
      gear += 1;
      lcd.setCursor(13, 0);
      lcd.print(gear+1);
      }
    else if (down_buttonState == HIGH && pre_down == LOW && gear != 0) {
      //downshift
      digitalWrite(motor_driver1, LOW);
      digitalWrite(motor_driver2, HIGH);
      delay(downshift_timing[gear]);
      digitalWrite(motor_driver2, LOW);
      gear -= 1;
      lcd.setCursor(13, 0);
      lcd.print(gear+1);
      }
  }
  else if (control_mode == 0) { 
    if (up_buttonState == HIGH && down_buttonState == HIGH) {
      control_mode = 2; 
      lcd.setCursor(13, 1);
      lcd.print("CA");
    }

//    (cadence < downshift_cadence || bike_speed < downshift_speed[gear])
    else if (bike_speed < downshift_speed[gear] && cadence != 0 && gear != 0 && g_counter >= 15){
      //downshift
      digitalWrite(motor_driver1, LOW);
      digitalWrite(motor_driver2, HIGH);
      delay(downshift_timing[gear]);
      digitalWrite(motor_driver2, LOW);
      gear -= 1;
      lcd.setCursor(13, 0);
      lcd.print(gear+1);
      g_counter = 0;
    }
    
    else if (cadence > upshift_cadence && cadence != 0 && gear != 6 && g_counter >= 40){
      //upshift
      digitalWrite(motor_driver1, HIGH);
      digitalWrite(motor_driver2, LOW);
      delay(upshift_timing[gear]);
      digitalWrite(motor_driver1, LOW);
      //upshift compensate
//      digitalWrite(motor_driver2, HIGH);
//      delay(compensate_timing[gear]);
//      digitalWrite(motor_driver2, LOW);
      gear += 1;
      lcd.setCursor(13, 0);
      lcd.print(gear+1);
      g_counter = 0;
    }
  }
  else {
    //calibration mode here
     if (up_buttonState == HIGH && pre_up == LOW) {
      //up_calibrate
      digitalWrite(motor_driver1, HIGH);
      digitalWrite(motor_driver2, LOW);
      delay(100);
      digitalWrite(motor_driver1, LOW);
      }
    else if (down_buttonState == HIGH && pre_down == LOW) {
      //down_calibrate
      digitalWrite(motor_driver1, LOW);
      digitalWrite(motor_driver2, HIGH);
      delay(100);
      digitalWrite(motor_driver2, LOW);
      }
  }
}

