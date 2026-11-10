import sys
import argparse
import os

try:
    import obspython as obs
except ImportError:
    print("SKIP: obspython not found")
    sys.exit(77)


def test_source_creation(plugin_path=None):
    print("Starting Python Integration Test...")

    # 1. Initialize OBS
    if not obs.obs_startup("en-US", None, None):
        print("Failed to start OBS")
        sys.exit(1)

    # 2. Load the plugin module if path provided
    if plugin_path:
        print(f"Loading plugin from: {plugin_path}")
        if not os.path.exists(plugin_path):
            print(f"ERROR: Plugin file not found at {plugin_path}")
            sys.exit(1)

        # Use obs_open_module and obs_init_module
        module_ptr = obs.obs_open_module(plugin_path)
        if not module_ptr:
            print("ERROR: Failed to open module.")
            sys.exit(1)

        if not obs.obs_init_module(module_ptr):
            print("ERROR: Failed to init module.")
            sys.exit(1)

        print("Plugin loaded successfully.")
    else:
        print(
            "WARNING: No plugin path provided. Assuming plugin is already loaded or in search path."
        )

    # 3. Create the source
    source_id = "emulens_fx"  # This MUST match the .id in lens-core.c
    source = obs.obs_source_create(source_id, "Test Source", None, None)

    if not source:
        print(f"FAILURE: Could not create source '{source_id}'. Is the plugin loaded?")
        obs.obs_shutdown()
        sys.exit(1)

    print(f"SUCCESS: Source '{source_id}' created.")

    # 4. Test a real plugin setting
    settings = obs.obs_data_create()
    obs.obs_data_set_int(settings, "mode", 0)
    obs.obs_source_update(source, settings)

    # Verify the setting matches the plugin's current public interface.
    current_settings = obs.obs_source_get_settings(source)
    val = obs.obs_data_get_int(current_settings, "mode")

    if val != 0:
        print(f"FAILURE: Expected mode not set. Got {val}")
        sys.exit(1)
    else:
        print("SUCCESS: Settings updated correctly")

    # Cleanup
    obs.obs_data_release(settings)
    obs.obs_data_release(current_settings)
    obs.obs_source_release(source)

    obs.obs_shutdown()
    print("Test Complete.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run OBS Plugin Integration Tests")
    parser.add_argument(
        "--plugin-path", type=str, help="Path to the compiled plugin binary (.so/.dll)"
    )
    args = parser.parse_args()

    try:
        test_source_creation(args.plugin_path)
    except Exception as e:
        print(f"ERROR: {e}")
        import traceback

        traceback.print_exc()
        sys.exit(1)
