/*
 * This file is part of Cleanflight.
 *
 * Cleanflight is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cleanflight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cleanflight.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stdbool.h>

#include <limits.h>

#include <platform.h>

//#define DEBUG_ALTITUDE_HOLD

#define BARO

extern "C" {
    #include "debug.h"

    #include "common/axis.h"
    #include "common/maths.h"

    #include "drivers/sensor.h"
    #include "drivers/accgyro.h"

    #include "sensors/sensors.h"
    #include "sensors/acceleration.h"
    #include "sensors/barometer.h"

    #include "io/escservo.h"
    #include "io/rc_controls.h"

    #include "rx/rx.h"

    #include "flight/mixer.h"
    #include "flight/pid.h"
    #include "flight/imu.h"
    #include "flight/altitudehold.h"

    #include "config/runtime_config.h"

}

#include "unittest_macros.h"
#include "gtest/gtest.h"

#define DOWNWARDS_THRUST true
#define UPWARDS_THRUST false


extern "C" {
    bool isThrustFacingDownwards(attitudeEulerAngles_t * attitude);
    uint16_t calculateTiltAngle(attitudeEulerAngles_t * attitude);
}

typedef struct inclinationExpectation_s {
    attitudeEulerAngles_t attitude;
    bool expectDownwardsThrust;
} inclinationExpectation_t;

TEST(AltitudeHoldTest, IsThrustFacingDownwards)
{
    // given

    inclinationExpectation_t inclinationExpectations[] = {
            { {{    0,    0,    0 }}, DOWNWARDS_THRUST },
            { {{  799,  799,    0 }}, DOWNWARDS_THRUST },
            { {{  800,  799,    0 }}, UPWARDS_THRUST },
            { {{  799,  800,    0 }}, UPWARDS_THRUST },
            { {{  800,  800,    0 }}, UPWARDS_THRUST },
            { {{  801,  801,    0 }}, UPWARDS_THRUST },
            { {{ -799, -799,    0 }}, DOWNWARDS_THRUST },
            { {{ -800, -799,    0 }}, UPWARDS_THRUST },
            { {{ -799, -800,    0 }}, UPWARDS_THRUST },
            { {{ -800, -800,    0 }}, UPWARDS_THRUST },
            { {{ -801, -801,    0 }}, UPWARDS_THRUST }
    };
    uint8_t testIterationCount = sizeof(inclinationExpectations) / sizeof(inclinationExpectation_t);

    // expect

    for (uint8_t index = 0; index < testIterationCount; index ++) {
        inclinationExpectation_t *angleInclinationExpectation = &inclinationExpectations[index];
#ifdef DEBUG_ALTITUDE_HOLD
        printf("iteration: %d\n", index);
#endif
        bool result = isThrustFacingDownwards(&angleInclinationExpectation->attitude);
        EXPECT_EQ(angleInclinationExpectation->expectDownwardsThrust, result);
    }
}

TEST(AltitudeHoldTest, applyMultirotorAltHold)
{
    // given
    escAndServoConfig_t escAndServoConfig;
    rcControlsConfig_t rcControlsConfig;
    
    memset(&escAndServoConfig, 0, sizeof(escAndServoConfig));
    escAndServoConfig.minthrottle = 1150;
    escAndServoConfig.maxthrottle = 1850;
    memset(&rcControlsConfig, 0, sizeof(rcControlsConfig));
    rcControlsConfig.alt_hold_deadband = 40;
    
    configureAltitudeHold(
                          NULL,
                          NULL,
                          &rcControlsConfig,
                          &escAndServoConfig
                          );
    
    rcData[THROTTLE] = 1400;
    rcCommand[THROTTLE] = 1500;
    ACTIVATE_RC_MODE(BOXBARO);
    updateAltHoldState();
    
    // when
    applyAltHold(NULL);
    
    // expect
    EXPECT_EQ(1500, rcCommand[THROTTLE]);
    
    // and given
    rcControlsConfig.alt_hold_fast_change = 1;
    
    // when
    applyAltHold(NULL);
    
    // expect
    EXPECT_EQ(1500, rcCommand[THROTTLE]);
}

// STUBS

extern "C" {
uint32_t rcModeActivationMask;
int16_t rcCommand[4];
int16_t rcData[MAX_SUPPORTED_RC_CHANNEL_COUNT];

uint32_t accTimeSum ;        // keep track for integration of acc
int accSumCount;
float accVelScale;

attitudeEulerAngles_t attitude;

//uint16_t acc_1G;
//int16_t heading;
//gyro_t gyro;
int32_t accSum[XYZ_AXIS_COUNT];
//int16_t magADC[XYZ_AXIS_COUNT];
int32_t BaroAlt;
int16_t debug[DEBUG16_VALUE_COUNT];

uint8_t stateFlags;
uint16_t flightModeFlags;
uint8_t armingFlags;

int32_t sonarAlt;
int16_t sonarCfAltCm;
int16_t sonarMaxAltWithTiltCm;


uint16_t enableFlightMode(flightModeFlags_e mask)
{
    return flightModeFlags |= (mask);
}

uint16_t disableFlightMode(flightModeFlags_e mask)
{
    return flightModeFlags &= ~(mask);
}

void gyroUpdate(void) {};
bool sensors(uint32_t mask)
{
    UNUSED(mask);
    return false;
};
void updateAccelerationReadings(rollAndPitchTrims_t *rollAndPitchTrims)
{
    UNUSED(rollAndPitchTrims);
}

void imuResetAccelerationSum(void) {};

uint32_t micros(void) { return 0; }
bool isBaroCalibrationComplete(void) { return true; }
void performBaroCalibrationCycle(void) {}
int32_t baroCalculateAltitude(void) { return 0; }
}
