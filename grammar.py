import sys
from pathlib import Path
from lark import Lark, exceptions

# Load the EBNF grammar
with open("grammar.ebnf", "r", encoding="utf-8") as f:
    grammar: str = f.read()

# Create a parser from the grammar
parser: Lark = Lark(grammar, start="start")


def check_dir(path: Path) -> None:
    """
    Parse all .grug files in the directory (non-recursive, only one level)
    and immediately exit on the first grammar error.
    """
    for subdir in path.iterdir():
        if not subdir.is_dir():
            continue
        for file in subdir.glob("*.grug"):
            try:
                parser.parse(file.read_text())  # type: ignore
            except exceptions.LarkError as e:
                print(f"❌ Grammar error in test: {file}")
                print(e)
                sys.exit(1)


# The tests directory relative to this script
root: Path = Path("tests")

# Check OK and err_runtime tests
check_dir(root / "ok")
check_dir(root / "err_runtime")

print("✅ All grammar checks passed")
