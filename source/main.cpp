#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "ei_run_classifier.h"
#include "hardware/i2c.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"

/* Private variables ------------------------------------------------------- */
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal

const uint8_t ADDRESS = 0x1C; //in MMA845X it's C, not D

//hardware registers
const uint8_t REG_X_MSB = 0x01;
const uint8_t REG_X_LSB = 0x02;
const uint8_t REG_Y_MSB = 0x03;
const uint8_t REG_Y_LSB = 0x04;
const uint8_t REG_Z_MSB = 0x05;
const uint8_t REG_Z_LSB = 0x06;
const uint8_t REG_DATA_CFG = 0x0E;
const uint8_t REG_CTRL_REG1 = 0x2A;

// Set the range and precision for the data
const uint8_t range_config = 0x01; // 0x00 for ±2g, 0x01 for ±4g, 0x02 for ±8g
const float count = 2048; // 4096 for ±2g, 2048 for ±4g, 1024 for ±8g

uint8_t buf[2];

float mma845X_convert_accel(uint16_t raw_accel) {
    float acceleration;
    // Acceleration is read as a multiple of g (gravitational acceleration on the Earth's surface)
    // Check if acceleration < 0 and convert to decimal accordingly
    if ((raw_accel & 0x2000) == 0x2000) {
        raw_accel &= 0x1FFF;
        acceleration = (-8192 + (float) raw_accel) / count;
    } else {
        acceleration = (float) raw_accel / count;
    }
    acceleration *= 9.81f;
    return acceleration;
}

#ifdef i2c_default
void mma845X_set_state(uint8_t state) {
    buf[0] = REG_CTRL_REG1;
    buf[1] = state; // Set RST bit to 1
    i2c_write_blocking(i2c_default, ADDRESS, buf, 2, false);
}
#endif


int main() {
    stdio_init_all();
    
    //For time in pico
    char datetime_buf[256];
    char *datetime_str = &datetime_buf[0];
    
    // Start on Tuesday 11th of July 2023 15:45:00
        datetime_t t = {
                .year  = 2023,
                .month = 11,
                .day   = 15,
                .dotw  = 3, // 0 is Sunday, so 5 is Friday
                .hour  = 15,
                .min   = 45,
                .sec   = 00
        };
    
    // Start the RTC
        rtc_init();
        rtc_set_datetime(&t);
    
    // clk_sys is >2000x faster than clk_rtc, so datetime is not updated immediately when rtc_get_datetime() is called.
        // tbe delay is up to 3 RTC clock cycles (which is 64us with the default clock settings)
        sleep_us(64);

    // Using I2C0 on the default SDA and SCL pins (4, 5 on a Pico)
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

    //Those variables are used to get the acceleration on 3-axes
    float x_acceleration;
    float y_acceleration;
    float z_acceleration;

    // Enable standby mode
    mma845X_set_state(0x00);

    // Edit configuration while in standby mode
    buf[0] = REG_DATA_CFG;
    buf[1] = range_config;
    i2c_write_blocking(i2c_default, ADDRESS, buf, 2, false);

    // Enable active mode
    mma845X_set_state(0x01);

    //Set the event id
    int id = 0;
    
    while (true) {
    
        float pred_value = 0;
        size_t jx = 0;
        //ei_printf("\nStart your move in 3 seconds...\n");
        sleep_ms(3000);
        //ei_printf("Predicting...\n");
        
        // Allocate a buffer here for the values we'll read from the IMU
        float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = { 0 };
        
        for (size_t ix = 0; ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; ix += 3) {
            // Determine the next tick (and then sleep later)
            uint64_t next_tick = ei_read_timer_us() + (EI_CLASSIFIER_INTERVAL_MS * 1000);
            
            // Start reading acceleration registers for 2 bytes
            //for x acceleration
            i2c_write_blocking(i2c_default, ADDRESS, &REG_X_MSB, 1, true);
            i2c_read_blocking(i2c_default, ADDRESS, buf, 2, false);
            buffer[ix] = mma845X_convert_accel(buf[0] << 6 | buf[1] >> 2);
            x_acceleration = buffer[ix];
            
            //for y acceleration
            i2c_write_blocking(i2c_default, ADDRESS, &REG_Y_MSB, 1, true);
            i2c_read_blocking(i2c_default, ADDRESS, buf, 2, false);
            buffer[ix + 1] = mma845X_convert_accel(buf[0] << 6 | buf[1] >> 2);
            y_acceleration = buffer[ix + 1];
            
            //for z acceleration
            i2c_write_blocking(i2c_default, ADDRESS, &REG_Z_MSB, 1, true);
            i2c_read_blocking(i2c_default, ADDRESS, buf, 2, false);
            buffer[ix + 2] = mma845X_convert_accel(buf[0] << 6 | buf[1] >> 2);
            z_acceleration = buffer[ix + 2];
            
            sleep_us(next_tick - ei_read_timer_us());
        }
        
        // Turn the raw buffer in a signal which we can the classify
        signal_t signal;
        int err = numpy::signal_from_buffer(buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
        if (err != 0) {
            ei_printf("Failed to create signal from buffer (%d)\n", err);
            return 1;
        }
        
        // Run the classifier
        ei_impulse_result_t result = { 0 };
        
        err = run_classifier(&signal, &result, debug_nn);
        if (err != EI_IMPULSE_OK) {
            ei_printf("ERR: Failed to run classifier (%d)\n", err);
            return 1;
        }
        
        //get date and time
        rtc_get_datetime(&t);
        datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
        
        // print the predictions
        //ei_printf("Predictions: ");
        //ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
        //          result.timing.dsp, result.timing.classification, result.timing.anomaly);
        //ei_printf(": \n");
        
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            //ei_printf("    %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);
            if (result.classification[ix].value > pred_value) {
                pred_value = result.classification[ix].value;
                jx = ix;
            }
        }
        
        //ei_printf(" \n");
        //ei_printf("************************************************************************************\n");
        absolute_time_t was = get_absolute_time();
        uint64_t us = to_us_since_boot(was); // / 1000000
        //ei_printf("Overhead for get_absolute_time() = %llu us\n", us);
        ei_printf(" Event ID: %d -> The movement is %s: %.5f  at time: \r%s and absolute time (us) is: %llu \n", id,  result.classification[jx].label, result.classification[jx].value, datetime_str, us);
        //ei_printf("************************************************************************************\n");
 //       pred = 0;
        id ++;
        
#if EI_CLASSIFIER_HAS_ANOMALY == 1
        //ei_printf("    anomaly score: %.3f\n", result.anomaly);
#endif
        
#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_ACCELEROMETER
#error "Invalid model for current sensor"
#endif
        
    }
return 0;
}

