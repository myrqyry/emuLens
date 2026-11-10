/**
 * @file test_strategies.cpp
 * @brief Unit tests for EmuLensFX effect strategies
 *
 * Tests that all strategy functions are properly defined and the
 * global strategies[] vtable is correctly populated.
 */

#include <obs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
    #include "lens-data.h"

    // The strategies array is defined in strategies.c
    extern const lens_effect_vtable_t strategies[NUM_MODES];

    // Stub functions to satisfy linker
    void emulens_inc_instances(void) {}
    void emulens_dec_instances(void) {}
}

// Mode names for better error messages
const char* mode_names[NUM_MODES] = {
    "MODE_VIDICON",
    "MODE_STARBURST",
    "MODE_LEAK",
    "MODE_HANDHELD",
    "MODE_BOKEH",
    "MODE_PARTICLES",
    "MODE_ORTON",
    "MODE_GRIT",
    "MODE_MIRAGE",
    "MODE_SPOT",
    "MODE_BLOOM",
    "MODE_PINHOLE",
    "MODE_POLAROID",
    "MODE_DAGUERRE",
    "MODE_TECHNICOLOR",
    "MODE_WET_LENS",
    "MODE_PHONE_WET"
};

// Expected technique names for each mode
const char* expected_techniques[NUM_MODES] = {
    "DrawVidicon",
    "DrawStarburst",
    "DrawLightLeak",
    "DrawHandheld",
    "DrawBokeh",
    "DrawBokehParticles",
    "DrawOrton",
    "DrawGrit",
    "DrawMirage",
    "DrawCenterSpot",
    "DrawBloomCombine",
    "DrawPinhole",
    "DrawPolaroid",
    "DrawDaguerreotype",
    "DrawTechnicolor",
    "DrawWetLens",
    "DrawPhoneWet"
};

int test_strategies_defined() {
    printf("\n=== Testing Strategy Vtable ===\n");
    int failures = 0;

    for (int i = 0; i < NUM_MODES; i++) {
        printf("\n[%d] %s:\n", i, mode_names[i]);

        // Check cache function
        if (strategies[i].cache_params == NULL) {
            printf("  ✗ cache_params is NULL\n");
            failures++;
        } else {
            printf("  ✓ cache_params defined\n");
        }

        // Check update function
        if (strategies[i].update_settings == NULL) {
            printf("  ✗ update_settings is NULL\n");
            failures++;
        } else {
            printf("  ✓ update_settings defined\n");
        }

        // Check render function
        if (strategies[i].render == NULL) {
            printf("  ✗ render is NULL\n");
            failures++;
        } else {
            printf("  ✓ render defined\n");
        }

        // Check technique name
        if (strategies[i].tech_name == NULL) {
            printf("  ✗ tech_name is NULL\n");
            failures++;
        } else if (strcmp(strategies[i].tech_name, expected_techniques[i]) != 0) {
            printf("  ✗ tech_name mismatch: got '%s', expected '%s'\n",
                   strategies[i].tech_name, expected_techniques[i]);
            failures++;
        } else {
            printf("  ✓ tech_name = %s\n", strategies[i].tech_name);
        }
    }

    return failures;
}

int test_mode_constants() {
    printf("\n=== Testing Mode Constants ===\n");
    int failures = 0;

    if (NUM_MODES != 17) {
        printf("✗ NUM_MODES = %d, expected 17\n", NUM_MODES);
        failures++;
    } else {
        printf("✓ NUM_MODES = 17\n");
    }

    if (MODE_VIDICON != 0) {
        printf("✗ MODE_VIDICON = %d, expected 0\n", MODE_VIDICON);
        failures++;
    } else {
        printf("✓ MODE_VIDICON = 0\n");
    }

    if (MODE_BLOOM != 10) {
        printf("✗ MODE_BLOOM = %d, expected 10\n", MODE_BLOOM);
        failures++;
    } else {
        printf("✓ MODE_BLOOM = 10\n");
    }

    return failures;
}

int main() {
    printf("============================================================\n");
    printf("EmuLensFX Strategy Unit Tests\n");
    printf("============================================================\n");

    int total_failures = 0;

    total_failures += test_mode_constants();
    total_failures += test_strategies_defined();

    printf("\n============================================================\n");
    if (total_failures > 0) {
        printf("FAILED: %d test(s) failed\n", total_failures);
        return 1;
    } else {
        printf("PASSED: All strategy tests passed\n");
        return 0;
    }
}
