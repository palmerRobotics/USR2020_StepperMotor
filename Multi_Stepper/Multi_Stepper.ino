#include <SPI.h>
#include "HighPowerStepperDriver.h"
#include "TeensyThreads.h"

#define MOTOR_COUNT 2
#define CURRENT_LIMIT 2800 //steppers are rated for 2.8A max continuous current

const uint8_t CSPinM1 = 10;
const uint8_t CSPinM2 = 9;
//const uint8_t FaultPin = 2;
//const uint8_t StallPin = 2;
const uint8_t HomingPinM0 = 7;

const double degrees_per_step = 1.8;
const double microstep = 4; //2^1, n = 0 - 8 //was 4
const double degreesM0 = 180;
const double degreesM1 = -90;
const double gear_ratio = 46.656;

const double StepPeriodUs = 200;

double stepper_degrees[MOTOR_COUNT] = {degreesM0, degreesM1};
double stepper_degrees_R[MOTOR_COUNT] = {-degreesM0, -degreesM1};

HighPowerStepperDriver sd0;
HighPowerStepperDriver sd1;
HighPowerStepperDriver *sd_ptr[MOTOR_COUNT] = {&sd0, &sd1};

void setup()
{
  Serial.begin(9600);
  delay(1000);
  
  SPI.begin();

  pinMode(HomingPinM0,INPUT);
  //pinMode(SleepPin, OUTPUT);
  //digitalWrite(SleepPin, HIGH);
  sd0.setChipSelectPin(CSPinM1);
  sd1.setChipSelectPin(CSPinM2);
  Serial.println("Initializing drivers");

  // Give the driver some time to power up.
  delay(100);

  //Print Stepper config data, such as microsteps, step delay, gear ratio, and predicted RPM
  Serial.print("microstep: ");
  Serial.println(microstep);
  Serial.print("step delay: ");
  Serial.print(StepPeriodUs);
  Serial.println(" microseconds");
  Serial.print("degrees of rotation: ");
  Serial.println(degreesM0);
  Serial.print("gear ratio: ");
  Serial.println(gear_ratio);
  double steps_approx = abs(degreesM0/(degrees_per_step/(gear_ratio*microstep)));
  double time_approx = (steps_approx*StepPeriodUs)/1000000;
  Serial.print("Approximate seconds per rotation: ");
  Serial.println(time_approx);
  float rpm_approx = (60/time_approx)*degreesM0/float(360);
  Serial.print("Approximate rpm: ");
  Serial.println(rpm_approx);
  Serial.print("Motor rotations (in degrees): ");
  for(int i=0;i < MOTOR_COUNT; i++){
    Serial.print(stepper_degrees[i]);
    Serial.print(" ");
  }
  Serial.println();
  
  // Set the number of microsteps that correspond to one full step.
  for(int i=0; i < MOTOR_COUNT; i++){
    sd_ptr[i]->resetSettings();
    sd_ptr[i]->clearStatus();
    sd_ptr[i]->setDecayMode(HPSDDecayMode::AutoMixed);
    sd_ptr[i]->setCurrentMilliamps36v4(CURRENT_LIMIT); 
    sd_ptr[i]->setStepMode(microstep);
    sd_ptr[i]->enableDriver();
  }

  // Enable the motor outputs.
  for(int i=0; i < MOTOR_COUNT; i++){
    sd_ptr[i]->enableDriver();
  }
  Serial.println("Drivers enabled\n");
  
  //pinMode(FaultPin, INPUT);
  //pinMode(StallPin, INPUT);
}

void loop()
{
  Serial.println("stepping...");
  turn_degrees(sd_ptr, stepper_degrees);
  Serial.println("Steppers reached targets");
  Serial.println();

  // Wait for 3 s
  delay(3000);
  Serial.println("stepping reverse...");
  turn_degrees(sd_ptr, stepper_degrees_R);
  Serial.println("Steppers reached targets");
  Serial.println();

  // Wait for 3 s
  delay(3000);
}

void turn_degrees(HighPowerStepperDriver *sd_ptr[], double stepper_degrees[]){
  int max_steps = 0;
  int steps[4] = {};
  for(int i=0; i < MOTOR_COUNT; i++){
    steps[i] = abs(stepper_degrees[i]/(degrees_per_step/(gear_ratio*microstep)));
    if(steps[i] > max_steps){
      max_steps = steps[i];
    }
  }
  Serial.print("steps in array: ");
  for(int i=0;i < MOTOR_COUNT; i++){
    Serial.print(steps[i]);
    Serial.print(", ");
  }
  Serial.println();
  Serial.print("max steps: ");
  Serial.println(max_steps);
  
  for(int i=0; i < MOTOR_COUNT; i++){
    if(stepper_degrees[i] >= 0){
      Serial.print("M");
      Serial.print(i);
      Serial.println(" set CCW");
      sd_ptr[i]->setDirection(0);
    }
    else{
      Serial.print("M");
      Serial.print(i);
      Serial.println(" set CW");
      sd_ptr[i]->setDirection(1);
    }
  }
  for(int i = 0; i <= max_steps; i++)
  {

    for(int j=0; j < MOTOR_COUNT; j++){
      if(i < steps[j]){
        sd_ptr[j]->step();
      }
      else if(i == steps[j]){
        Serial.print("M");
        Serial.print(j);
        Serial.print(" reached target rotation at ");
        Serial.print(i);
        Serial.println(" steps");
      }
    }
    if(digitalRead(HomingPinM0)){
      Serial.println("HOMING SWITCH TRIGGERED");
    }
    delayMicroseconds(StepPeriodUs);
  }
}
