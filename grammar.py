import json
import sys
from pathlib import Path
from typing import Iterator

from lark import Lark, Token, Transformer, Tree, exceptions
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
        indent = len(token.rsplit("\n", 1)[1])
        if indent % self.tab_len != 0:
            raise IndentationError(
                f"Invalid indent of {indent} spaces at line {token.line}, must be multiple of {self.tab_len}"
            )
        yield from super().handle_NL(token)


parser: Lark = Lark(
    grammar, start="start", parser="lalr", postlex=TreeIndenter(), strict=True
)


class GrugTransformer(Transformer):
    def start(self, items):
        res = []
        for item in items:
            if item is None:
                continue

            if isinstance(item, dict):
                if item.get("type") == "VARIABLE_STATEMENT":
                    item["type"] = "GLOBAL_VARIABLE"
                elif item.get("type") == "COMMENT_STATEMENT":
                    item["type"] = "GLOBAL_COMMENT"

            res.append(item)

        while res and res[-1].get("type") == "GLOBAL_EMPTY_LINE":
            res.pop()

        return res

    def empty_line(self, items):
        return {"type": "GLOBAL_EMPTY_LINE"}

    def on_fn(self, items):
        name = str(items[0])
        statements = items[-1]
        return {
            "type": "GLOBAL_ON_FN",
            "name": name,
            "statements": statements,
        }

    def helper_fn(self, items):
        return {
            "type": "HELPER_FN",
            "name": str(items[0]),
            "statements": items[-1],
        }

    def return_type(self, items):
        return items[0]

    def statement(self, items):
        return items[0] if items else None

    def block_stmt(self, items):
        return items[0]

    def block(self, items):
        return [s for s in items if s is not None]

    def vardecl(self, items):
        name, typ, expr = items
        return {
            "type": "VARIABLE_STATEMENT",
            "name": str(name),
            "variable_type": str(typ),
            "assignment": expr,
        }

    def ident_ref(self, items):
        return {"type": "IDENTIFIER_EXPR", "str": str(items[0])}

    def call_stmt(self, items):
        res = self.call_expr(items)
        res["type"] = "CALL_STATEMENT"
        return res

    def call_expr(self, items):
        result = {
            "type": "CALL_EXPR",
            "name": str(items[0]),
        }
        if len(items) > 1 and items[1] is not None:
            result["arguments"] = items[1]
        return result

    def type(self, items):
        return str(items[0])

    def reassign(self, items):
        name, expr = items
        return {
            "type": "VARIABLE_STATEMENT",
            "name": str(name),
            "assignment": expr,
        }

    def if_stmt(self, items):
        items = [i for i in items if not isinstance(i, Token) or i.type != "IF"]
        result = {
            "type": "IF_STATEMENT",
            "condition": items[0],
            "if_statements": items[1],
        }
        if len(items) > 2:
            result["else_statements"] = items[2]
        return result

    def while_loop(self, items):
        return {
            "type": "WHILE_STATEMENT",
            "condition": items[1],
            "statements": items[2],
        }

    def break_stmt(self, items):
        return {"type": "BREAK_STATEMENT"}

    def continue_stmt(self, items):
        return {"type": "CONTINUE_STATEMENT"}

    def return_stmt(self, items):
        result = {"type": "RETURN_STATEMENT"}
        if len(items) > 0 and items[0] is not None:
            result["expression"] = items[0]
        return result

    def _binary_expr(self, operator_token, items):
        return {
            "type": "BINARY_EXPR",
            "left_expr": items[0],
            "operator": operator_token,
            "right_expr": items[1],
        }

    def add(self, items):
        return self._binary_expr("PLUS_TOKEN", items)

    def sub(self, items):
        return self._binary_expr("MINUS_TOKEN", items)

    def mul(self, items):
        return self._binary_expr("MULTIPLICATION_TOKEN", items)

    def div(self, items):
        return self._binary_expr("DIVISION_TOKEN", items)

    def eq(self, items):
        return self._binary_expr("EQUALS_TOKEN", items)

    def neq(self, items):
        return self._binary_expr("NOT_EQUALS_TOKEN", items)

    def lt(self, items):
        return self._binary_expr("LESS_THAN_TOKEN", items)

    def le(self, items):
        return self._binary_expr("LESS_THAN_OR_EQUAL_TOKEN", items)

    def gt(self, items):
        return self._binary_expr("GREATER_THAN_TOKEN", items)

    def ge(self, items):
        return self._binary_expr("GREATER_THAN_OR_EQUAL_TOKEN", items)

    def and_op(self, items):
        return {
            "type": "LOGICAL_EXPR",
            "left_expr": items[0],
            "operator": "AND_TOKEN",
            "right_expr": items[1],
        }

    def or_op(self, items):
        return {
            "type": "LOGICAL_EXPR",
            "left_expr": items[0],
            "operator": "OR_TOKEN",
            "right_expr": items[1],
        }

    def not_op(self, items):
        return {
            "type": "UNARY_EXPR",
            "operator": "NOT_TOKEN",
            "expr": items[1],
        }

    def neg(self, items):
        return {
            "type": "UNARY_EXPR",
            "operator": "MINUS_TOKEN",
            "expr": items[1],
        }

    def paren_expr(self, items):
        return {"type": "PARENTHESIZED_EXPR", "expr": items[0]}

    def NUMBER(self, tok):
        return {"type": "NUMBER_EXPR", "value": tok.value}

    def STRING(self, tok):
        return {"type": "STRING_EXPR", "value": tok.value}

    def true_expr(self, items):
        return {"type": "TRUE_EXPR"}

    def false_expr(self, items):
        return {"type": "FALSE_EXPR"}

    def NAME(self, tok):
        return str(tok)

    def ID(self, tok):
        return str(tok)

    def args(self, items):
        return [i for i in items if i is not None]

    def params(self, items):
        return items

    def param(self, items):
        return {"name": str(items[0]), "type": str(items[1])}

    def comment(self, items):
        content = str(items[0]).lstrip("#").strip()
        return {"type": "COMMENT_STATEMENT", "comment": content}


def check_dir(path: Path) -> None:
    transformer = GrugTransformer()
    for subdir in sorted(
        [d for d in path.iterdir() if d.is_dir()], key=lambda d: d.name
    ):
        expected_file = subdir / "expected.json"
        if not expected_file.exists():
            print(f"⚠️ No expected.json in {subdir}, skipping")
            continue
        expected = json.loads(expected_file.read_text())

        for file in sorted(subdir.glob("*.grug"), key=lambda f: f.name):
            print(f"Parsing {file}...")
            try:
                tree: Tree = parser.parse(file.read_text())
                ast = transformer.transform(tree)
            except exceptions.LarkError as e:
                print(f"❌ Grammar error in test: {file}")
                print(e)
                sys.exit(1)
            if ast != expected:
                print(f"❌ AST does not match expected.json for {file}")
                print("Got:")
                print(json.dumps(ast, indent=4))
                print("Expected:")
                print(json.dumps(expected, indent=4))
                sys.exit(1)


if __name__ == "__main__":
    root: Path = Path("tests")
    check_dir(root / "ok")
    check_dir(root / "err_runtime")
    print("✅ All grammar checks passed")
