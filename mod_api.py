#!/usr/bin/env python3
import json
import sys
from pathlib import Path
from typing import Any, Dict, List, Tuple, cast

# Define the global key priority.
# Keys not in this list will be sorted alphabetically after these.
KEY_ORDER_LIST: List[str] = [
    "entities",
    "classes",
    "host_functions",
    "name",
    "description",
    "used_generics",
    "static_methods",
    "methods",
    "export_functions",
    "return_type",
    "parameters",
    "type",
    "generics",
    "resource_extension",
    "entity_type",
]
KEY_ORDER: Dict[str, int] = {k: i for i, k in enumerate(KEY_ORDER_LIST)}


def order_data(data: Any) -> Any:
    """Recursively sorts dictionaries based on KEY_ORDER, falling back to alphabetical."""
    if isinstance(data, dict):
        # Cast explicitly tells Pyright the key/value types instead of leaving them Unknown
        d = cast(Dict[str, Any], data)

        def sort_key(k: str) -> Tuple[int, str]:
            return (KEY_ORDER.get(k, 9999), k)

        sorted_keys: List[str] = sorted(d.keys(), key=sort_key)
        return {k: order_data(d[k]) for k in sorted_keys}

    elif isinstance(data, list):
        # Cast explicitly tells Pyright the item types
        l = cast(List[Any], data)
        return [order_data(item) for item in l]

    else:
        return data


def main() -> None:
    fix: bool = "--fix" in sys.argv
    path: Path = Path("mod_api.json")

    try:
        raw: str = path.read_text(encoding="utf-8")
        data: Any = json.loads(raw)
    except Exception as e:
        print(f"Error reading or parsing {path}: {e}")
        sys.exit(1)

    ordered_data: Any = order_data(data)

    # sort_keys=False is crucial here because we already structured the dict correctly.
    expected: str = json.dumps(ordered_data, indent="\t", sort_keys=False) + "\n"

    if raw != expected:
        if fix:
            path.write_text(expected, encoding="utf-8")
            print(f"Fixed formatting: {path}")
        else:
            print(f"Error: {path} is not properly formatted.")
            print("Run 'python mod_api.py --fix' locally to fix it.")
            sys.exit(1)
    else:
        print(f"{path} is already formatted correctly.")


if __name__ == "__main__":
    main()
