#include "screw_calc.h"
#include "machine_params.h"
#include <math.h>

// Greatest Common Divisor using Euclidean algorithm
static uint32_t gcd(uint32_t a, uint32_t b) {
    while (b != 0) {
        uint32_t temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

void screw_calc_ratio(float target_pitch_mm, uint32_t *numerator, uint32_t *denominator) {
    // Get machine parameters
    float leadscrew_pitch = machine_params_get_leadscrew_pitch();
    uint32_t motor_steps = machine_params_get_motor_steps();
    uint32_t encoder_counts = machine_params_get_encoder_counts();
    
    // Calculate raw ratio: (Target_Pitch * Steps_Per_Rev) / (Leadscrew_Pitch * Encoder_Counts)
    float ratio = (target_pitch_mm * motor_steps) / (leadscrew_pitch * encoder_counts);
    
    // Convert ratio to fraction with precision
    // Multiply by a large factor to preserve precision, then simplify
    const uint32_t PRECISION = 1000000;
    uint32_t num = (uint32_t)(ratio * PRECISION);
    uint32_t den = PRECISION;
    
    // Reduce to lowest terms
    uint32_t divisor = gcd(num, den);
    num /= divisor;
    den /= divisor;
    
    // Further reduce if ratio is very precise (e.g., 1/3 instead of 333333/1000000)
    // Try to find simpler equivalent within tolerance
    for (uint32_t test_den = 1; test_den <= 10000; test_den++) {
        uint32_t test_num = (uint32_t)roundf(ratio * test_den);
        float test_ratio = (float)test_num / test_den;
        
        // If within 0.01% tolerance, use simpler fraction
        if (fabsf(test_ratio - ratio) / ratio < 0.0001f) {
            uint32_t test_divisor = gcd(test_num, test_den);
            test_num /= test_divisor;
            test_den /= test_divisor;
            
            // Use if simpler (smaller denominator)
            if (test_den < den) {
                num = test_num;
                den = test_den;
            }
        }
    }
    
    *numerator = num;
    *denominator = den;
}
