import glob
import json
import os
import sys

from jsonschema import ValidationError, validate


def load_json(path: str):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def main():
    if not os.path.exists("mod_api_schema.json"):
        print("Error: mod_api_schema.json not found.")
        sys.exit(1)

    schema = load_json("mod_api_schema.json")

    # Validate the root mod_api.json
    try:
        valid_data = load_json("mod_api.json")
        validate(instance=valid_data, schema=schema)
        print("SUCCESS: mod_api.json passed schema validation.")
    except ValidationError as e:
        print(f"FAIL: mod_api.json failed schema validation.\nError: {e.message}")
        sys.exit(1)
    except FileNotFoundError:
        print("Warning: mod_api.json not found, skipping valid file check.")

    # Validate that files in tests/err_mod_api/ intentionally FAIL
    err_files = glob.glob("tests/err_mod_api/*.json")
    for err_file in err_files:
        try:
            err_data = load_json(err_file)
            validate(instance=err_data, schema=schema)
            # If we reach here, the file passed when it shouldn't have
            print(f"FAIL: {err_file} SHOULD have failed validation, but it passed.")
            sys.exit(1)
        except ValidationError as e:
            # Expected outcome
            print(f"SUCCESS: {err_file} correctly failed schema validation.")
            print(f"  {e.message}")
        except Exception as e:
            print(f"Error reading {err_file}: {e}")
            sys.exit(1)


if __name__ == "__main__":
    main()
