import sys
from pathlib import Path

from lark import Lark, exceptions

# Load the EBNF grammar
with open("grammar.ebnf", "r", encoding="utf-8") as f:
    grammar: str = f.read()

# Create a parser from the grammar
parser: Lark = Lark(grammar, start="start")


def check_dir(path: Path) -> None:
    # Sort directories alphabetically
    for subdir in sorted(
        [d for d in path.iterdir() if d.is_dir()], key=lambda d: d.name
    ):
        # Sort .grug files alphabetically
        for file in sorted(subdir.glob("*.grug"), key=lambda f: f.name):
            print(f"Parsing {file}...")
            try:
                parser.parse(file.read_text())  # type: ignore
            except exceptions.LarkError as e:
                print(f"❌ Grammar error in test: {file}")
                print(e)
                sys.exit(1)


root: Path = Path("tests")
check_dir(root / "ok")
check_dir(root / "err_runtime")
print("✅ All grammar checks passed")
