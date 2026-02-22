import sys
from pathlib import Path
from typing import Iterator

from lark import Lark, Token, exceptions
from lark.indenter import Indenter

with open("grug_grammar.lark") as f:
    grammar: str = f.read()


class TreeIndenter(Indenter):
    NL_type = "_NL"  # type: ignore
    OPEN_PAREN_types = []  # type: ignore
    CLOSE_PAREN_types = []  # type: ignore
    INDENT_type = "_INDENT"  # type: ignore
    DEDENT_type = "_DEDENT"  # type: ignore
    tab_len = 4  # type: ignore

    def handle_NL(self, token: Token) -> Iterator[Token]:
        # Only spaces after newline
        indent = len(token.rsplit("\n", 1)[1])

        # Reject indent not multiple of tab_len
        if indent % self.tab_len != 0:
            raise IndentationError(
                f"Invalid indent of {indent} spaces at line {token.line}, must be multiple of {self.tab_len}"
            )

        # Delegate to base class for normal INDENT/DEDENT handling
        yield from super().handle_NL(token)


# Create a parser from the grammar
parser: Lark = Lark(
    grammar, start="start", parser="lalr", postlex=TreeIndenter(), strict=True
)


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
