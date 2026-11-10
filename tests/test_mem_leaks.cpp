#include <obs.h>
#include <util/bmem.h>
#include <util/dstr.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
    // Forward declaration of mock functions (must be extern "C" for linker wrap)
    char *__wrap_obs_find_module_file(obs_module_t *module, const char *file);
    const char *__wrap_obs_module_text(const char *val);
    obs_module_t *__wrap_obs_current_module(void);

    extern struct obs_source_info emulens_filter_info;

    // Stub instance counting functions from plugin-main.c
    // Since we don't link plugin-main.c, we must provide these symbols.
    void emulens_inc_instances(void) {}
    void emulens_dec_instances(void) {}

    // We also define obs_module_pointer to satisfy any extern references
    // from lens-core.c (if it uses OBS_DECLARE_MODULE, which it doesn't,
    // but if it uses obs_module_file macro that references it).
    // In lens-core.c: obs_module_file("file") -> obs_find_module_file(obs_module_pointer, "file")
    // So we need this variable to exist.
    obs_module_t *obs_module_pointer = NULL;

    // Mock for obs_module_text - just return the key as-is
    const char *__wrap_obs_module_text(const char *val) {
        return val;  // Return the key itself for testing
    }

    // Mock for obs_current_module - return our dummy module pointer
    obs_module_t *__wrap_obs_current_module(void) {
        return obs_module_pointer;
    }

    // Mock implementation of obs_find_module_file using linker wrap
    // This function will be called instead of the real obs_find_module_file
    char *__wrap_obs_find_module_file(obs_module_t *module, const char *file) {
        (void)module; // Unused

        // Try to find the file in known locations
        const char *paths[] = {
            "../data",
            "../../data",
            "../../../data",
            "./data"
        };

        for (size_t i = 0; i < sizeof(paths)/sizeof(paths[0]); i++) {
            struct dstr path;
            memset(&path, 0, sizeof(path));
            dstr_printf(&path, "%s/%s", paths[i], file);

            FILE *f = fopen(path.array, "rb");
            if (f) {
                fclose(f);
                printf("[TestMock] Found effect file at: %s\n", path.array);
                char *ret = bstrdup(path.array);
                dstr_free(&path);
                return ret;
            }
            dstr_free(&path);
        }

        fprintf(stderr, "[TestMock] Could not find effect file: %s\n", file);
        return NULL;
    }

    void RegisterMyPluginSources() {
        obs_register_source(&emulens_filter_info);
    }
} // extern "C"

void test_lifecycle() {
    printf("Starting LifeCycle Test...\n");

    // 1. Initialize libobs core
    if (!obs_startup("en-US", NULL, NULL)) {
        fprintf(stderr, "Failed to start OBS context\n");
        exit(1);
    }
    printf("OBS Started.\n");

    // 2. Register your plugin components manually
    RegisterMyPluginSources();
    printf("Source Registered.\n");
    // Public name changes, while the persistent OBS source type ID does not.
    const char *display = obs_source_get_display_name("emulens_fx");
    if (!display || strcmp(display, "EmuLens") != 0) {
        fprintf(stderr, "Expected EmuLens display on stable emulens_fx type, got %s\n",
                display ? display : "(missing)");
        obs_shutdown();
        exit(2);
    }
    printf("PUBLIC_BRANDING=EmuLens ID=emulens_fx\n");

    // 3. Create and Destroy your object
    obs_source_t *source = obs_source_create("emulens_fx", "Test Source", NULL, NULL);
    if (source == NULL) {
        fprintf(stderr, "Failed to create source!\n");
        // Ensure we shut down even if we fail
        obs_shutdown();
        exit(1);
    }
    printf("Source Created.\n");

    // Simulate some usage
    obs_data_t *settings = obs_data_create();
    obs_data_set_string(settings, "test_property", "test_value");
    obs_source_update(source, settings);
    obs_data_release(settings);
    printf("Source Updated.\n");

    // Destroy
    obs_source_release(source); // Ref count reaches 0, triggers destroy callback
    printf("Source Released.\n");

    // 4. Shutdown OBS
    obs_shutdown();
    printf("OBS Shutdown.\n");

    // 5. Verification: Check the leak counter
    long leaks = bnum_allocs();
    if (leaks > 0) {
        fprintf(stderr, "TEST FAILED: %ld memory allocations leaked!\n", leaks);
        exit(1);
    } else {
        printf("TEST PASSED: 0 Leaks detected.\n");
    }
}

int main() {
    test_lifecycle();
    return 0;
}
