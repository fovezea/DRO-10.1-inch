#include "cone_calc.h"
#include "machine_params.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Greatest Common Divisor using Euclidean algorithm
static uint32_t gcd(uint32_t a, uint32_t b) {
    while (b != 0) {
        uint32_t temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

void cone_calc_ratio(float angle_deg, 
                     uint32_t *z_numerator, uint32_t *z_denominator,
                     uint32_t *x_numerator, uint32_t *x_denominator) {
    // Convert angle to radians
    float angle_rad = angle_deg * M_PI / 180.0f;
    
    // Calculate ratio: X_feed / Z_feed = tan(angle)
    float x_to_z_ratio = tanf(angle_rad);
    
    // For cone turning, typically Z-axis runs at base speed (1:1 with encoder)
    // and X-axis is scaled by tan(angle)
    
    // Get machine parameters
    uint32_t motor_steps = machine_params_get_motor_steps();
    uint32_t encoder_counts = machine_params_get_encoder_counts();
    
    // Z-axis: 1:1 ratio (follow encoder directly)
    // Simplified: encoder_counts : motor_steps
    uint32_t z_num = encoder_counts;
    uint32_t z_den = motor_steps;
    uint32_t z_divisor = gcd(z_num, z_den);
    z_num /= z_divisor;
    z_den /= z_divisor;
    
    // X-axis: scaled by tan(angle)
    // X_ratio = (encoder_counts * tan(angle)) : motor_steps
    uint32_t x_num = (uint32_t)(encoder_counts * x_to_z_ratio);
    uint32_t x_den = motor_steps;
    
    // Try to simplify X ratio
    for (uint32_t test_den = 1; test_den <= 10000; test_den++) {
        uint32_t test_num = (uint32_t)roundf(x_to_z_ratio * encoder_counts * test_den / motor_steps);
        float test_ratio = (float)test_num / test_den;
        float target_ratio = (float)x_num / x_den;
        
        if (fabsf(test_ratio - target_ratio) / target_ratio < 0.001f) {
            uint32_t test_divisor = gcd(test_num, test_den);
            test_num /= test_divisor;
            test_den /= test_divisor;
            
            if (test_den < x_den) {
                x_num = test_num;
                x_den = test_den;
            }
        }
    }
    
    // Reduce X ratio
    uint32_t x_divisor = gcd(x_num, x_den);
    x_num /= x_divisor;
    x_den /= x_divisor;
    
    *z_numerator = z_num;
    *z_denominator = z_den;
    *x_numerator = x_num;
    *x_denominator = x_den;
}
