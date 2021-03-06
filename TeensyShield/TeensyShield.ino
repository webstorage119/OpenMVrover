// This file is part of the OpenMV project.
// Copyright (c) 2013-2017 Ibrahim Abdelkader <iabdalkader@openmv.io> & Kwabena W. Agyeman <kwagyeman@openmv.io>
// Updated for Teensy shield by Chris Anderson: https://circuits.io/circuits/5453306-openmv-teensy-shield
// This work is licensed under the MIT license, see the file LICENSE for details.

#include <Servo.h>

#define SERIAL_RX_PIN 0
#define SERIAL_TX_PIN 1
#define THROTTLE_SERVO_PIN 20
#define STEERING_SERVO_PIN 21
#define RC_THROTTLE_PIN 4
#define RC_STEERING_PIN 3
#define RC_SWITCH_PIN 5
#define LED_PIN 13
#define VOLTAGE_PIN 15

#define SERIAL_BAUD_RATE 19200

#define RC_THROTTLE_SERVO_REFRESH_RATE 20000UL // in us
#define SERIAL_THROTTLE_SERVO_REFRESH_RATE 1000000UL // in us
#define RC_THROTTLE_DEAD_ZONE_MIN 1400UL // in us
#define RC_THROTTLE_DEAD_ZONE_MAX 1600UL // in us

#define RC_STEERING_SERVO_REFRESH_RATE 20000UL // in us
#define SERIAL_STEERING_SERVO_REFRESH_RATE 1000000UL // in us
#define RC_STEERING_DEAD_ZONE_MIN 1400UL // in us
#define RC_STEERING_DEAD_ZONE_MAX 1600UL // in us

Servo throttle_servo, steering_servo;

unsigned long last_microseconds;
bool last_rc_throttle_pin_state, last_rc_steering_pin_state;
unsigned long last_rc_throttle_microseconds, last_rc_steering_microseconds;
unsigned long rc_throttle_servo_pulse_length = 0, rc_steering_servo_pulse_length = 0;
unsigned long rc_throttle_servo_pulse_refreshed = 0, rc_steering_servo_pulse_refreshed = 0;
float voltage;
float throttle_compensation = 1;

char serial_buffer[16] = {};
unsigned long serial_throttle_servo_pulse_length = 0, serial_steering_servo_pulse_length = 0;
unsigned long serial_throttle_servo_pulse_refreshed = 0, serial_steering_servo_pulse_refreshed = 0;

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    Serial1.begin(SERIAL_BAUD_RATE);
    pinMode(LED_PIN, OUTPUT);
    pinMode(RC_STEERING_PIN, INPUT);
    pinMode(RC_THROTTLE_PIN, INPUT);
    pinMode(VOLTAGE_PIN, INPUT);
    last_microseconds = micros();
    last_rc_throttle_pin_state = digitalRead(RC_THROTTLE_PIN) == HIGH;
    last_rc_steering_pin_state = digitalRead(RC_STEERING_PIN) == HIGH;
    last_rc_steering_pin_state = digitalRead(RC_SWITCH_PIN) == HIGH;
    last_rc_throttle_microseconds = last_microseconds;
    last_rc_steering_microseconds = last_microseconds;
    digitalWrite(LED_PIN,HIGH);
}

void loop()
{  
    unsigned long microseconds = micros();
    bool rc_throttle_pin_state = digitalRead(RC_THROTTLE_PIN) == HIGH;
    bool rc_steering_pin_state = digitalRead(RC_STEERING_PIN) == HIGH;
    bool rc_steering_pin_state = digitalRead(RC_SWITCH_PIN) == HIGH;
    voltage = analogRead(VOLTAGE_PIN);
    voltage = voltage/64.8; //divisor for the resistor divider
//    Serial.print("Voltage: ");
//    Serial.println(voltage);
    throttle_compensation = 7.4/voltage;
    if(rc_throttle_pin_state && (!last_rc_throttle_pin_state)) // rising edge
    {
        last_rc_throttle_microseconds = microseconds;
    }

    if((!rc_throttle_pin_state) && last_rc_throttle_pin_state) // falling edge
    {
        unsigned long temp = microseconds - last_rc_throttle_microseconds;

        if(!rc_throttle_servo_pulse_length)
        {
           rc_throttle_servo_pulse_length = temp;
        }
        else
        {
           rc_throttle_servo_pulse_length = ((rc_throttle_servo_pulse_length * 3) + temp) >> 2;
        }

        rc_throttle_servo_pulse_refreshed = microseconds;
    }

    if(rc_throttle_servo_pulse_length // zero servo if not refreshed
    && ((microseconds - rc_throttle_servo_pulse_refreshed) > (2UL * RC_THROTTLE_SERVO_REFRESH_RATE)))
    {
        rc_throttle_servo_pulse_length = 0;
    }

    if(rc_steering_pin_state && (!last_rc_steering_pin_state)) // rising edge
    {
        last_rc_steering_microseconds = microseconds;
    }

    if((!rc_steering_pin_state) && last_rc_steering_pin_state) // falling edge
    {
        unsigned long temp = microseconds - last_rc_steering_microseconds;

        if(!rc_steering_servo_pulse_length)
        {
           rc_steering_servo_pulse_length = temp;
        }
        else
        {
           rc_steering_servo_pulse_length = ((rc_steering_servo_pulse_length * 3) + temp) >> 2;
//           Serial.print("Steer: ");
//           Serial.println(rc_steering_servo_pulse_length);
        }

        rc_steering_servo_pulse_refreshed = microseconds;
    }

    if(rc_steering_servo_pulse_length // zero servo if not refreshed
    && ((microseconds - rc_steering_servo_pulse_refreshed) > (2UL * RC_STEERING_SERVO_REFRESH_RATE)))
    {
        rc_steering_servo_pulse_length = 0;
    }

    last_microseconds = microseconds;
    last_rc_throttle_pin_state = rc_throttle_pin_state;
    last_rc_steering_pin_state = rc_steering_pin_state;

    while(Serial.available())
    {
        int m = Serial.read();
        Serial1.write(m);  // echo USB serial to the OpenMV
    }
    while(Serial1.available())
    {
        int c = Serial1.read();
        Serial.write(c);  //echo the incoming serial stream from the OpenMV
        memmove(serial_buffer, serial_buffer + 1, sizeof(serial_buffer) - 2);
        serial_buffer[sizeof(serial_buffer) - 2] = c;

        if(c == '\n')
        {
            unsigned long serial_throttle_servo_pulse_length_tmp, serial_steering_servo_pulse_length_tmp;

            if(sscanf(serial_buffer, "{%lu,%lu}", &serial_throttle_servo_pulse_length_tmp, &serial_steering_servo_pulse_length_tmp) == 2)
            {
                if(!serial_throttle_servo_pulse_length)
                {
                   serial_throttle_servo_pulse_length = serial_throttle_servo_pulse_length_tmp;
                }
                else
                {
                   serial_throttle_servo_pulse_length = ((serial_throttle_servo_pulse_length * 3) + serial_throttle_servo_pulse_length_tmp) >> 2;
                }

                serial_throttle_servo_pulse_refreshed = microseconds;

                if(!serial_steering_servo_pulse_length)
                {
                   serial_steering_servo_pulse_length = serial_steering_servo_pulse_length_tmp;
                }
                else
                {
                   serial_steering_servo_pulse_length = ((serial_steering_servo_pulse_length * 3) + serial_steering_servo_pulse_length_tmp) >> 2;
                }

                serial_steering_servo_pulse_refreshed = microseconds;

                digitalWrite(LED_PIN, (digitalRead(LED_PIN) == HIGH) ? LOW : HIGH);
            }
            else
            {
                serial_throttle_servo_pulse_length = 0;
                serial_steering_servo_pulse_length = 0;
            }
        }
    }

    if(serial_throttle_servo_pulse_length // zero servo if not refreshed
    && ((microseconds - serial_throttle_servo_pulse_refreshed) > (2UL * SERIAL_THROTTLE_SERVO_REFRESH_RATE)))
    {
        serial_throttle_servo_pulse_length = 0;
    }

    if(serial_steering_servo_pulse_length // zero servo if not refreshed
    && ((microseconds - serial_steering_servo_pulse_refreshed) > (2UL * SERIAL_STEERING_SERVO_REFRESH_RATE)))
    {
        serial_steering_servo_pulse_length = 0;
    }

    if(rc_steering_servo_pulse_length)
    {
        if(!steering_servo.attached())
        {
            throttle_servo.attach(THROTTLE_SERVO_PIN);
            steering_servo.attach(STEERING_SERVO_PIN);
        }

        if(serial_steering_servo_pulse_length)
        {
            if((rc_throttle_servo_pulse_length < RC_THROTTLE_DEAD_ZONE_MIN)
            || (rc_throttle_servo_pulse_length > RC_THROTTLE_DEAD_ZONE_MAX))
            {
                throttle_servo.writeMicroseconds(serial_throttle_servo_pulse_length);
                Serial.print("Throttle: ");
                Serial.println(serial_throttle_servo_pulse_length);
            }
            else
            {
                throttle_servo.writeMicroseconds(1500);
            }

            if((rc_steering_servo_pulse_length < RC_STEERING_DEAD_ZONE_MIN)
            || (rc_steering_servo_pulse_length > RC_STEERING_DEAD_ZONE_MAX))
            {
                steering_servo.writeMicroseconds(rc_steering_servo_pulse_length);
            }
            else
            {
                steering_servo.writeMicroseconds(serial_steering_servo_pulse_length);
            }
        }
        else
        {
            throttle_servo.writeMicroseconds(rc_throttle_servo_pulse_length);
            steering_servo.writeMicroseconds(rc_steering_servo_pulse_length);
        }
    }
    else if(steering_servo.attached())
    {
        throttle_servo.detach();
        steering_servo.detach();
    }
}
