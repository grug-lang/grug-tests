import json
import sys
from pathlib import Path
from typing import Any, Dict, Iterator, List, Optional, Union

from lark import Lark, Token, Transformer, Tree, exceptions
from lark.indenter import Indenter

with open("grug_grammar.lark") as f:
    grammar: str = f.read()


class TreeIndenter(Indenter):
    NL_type = "_NL"  # type: ignore
    OPEN_PAREN_types: List[str] = []  # type: ignore
    CLOSE_PAREN_types: List[str] = []  # type: ignore
    INDENT_type = "_INDENT"  # type: ignore
    DEDENT_type = "_DEDENT"  # type: ignore
    tab_len = 4  # type: ignore

    def handle_NL(self, token: Token) -> Iterator[Token]:
        indent = len(token.rsplit("\n", 1)[1])
        if indent % self.tab_len != 0:
            raise IndentationError(
                f"Invalid indent of {indent} spaces at line {token.line}, must be multiple of {self.tab_len}"
            )
        if token.count("\n") >= 2:
            yield Token("EMPTY_LINE", "")
        yield from super().handle_NL(token)


parser: Lark = Lark(
    grammar, start="start", parser="lalr", postlex=TreeIndenter(), strict=True
)


class GrugTransformer(Transformer[Tree[Any], Any]):
    def start(self, items: List[Dict[str, Any]]) -> List[Any]:
        res: List[Any] = []
        for item in items:
            typ: Optional[str] = item.get("type")
            if typ == "VARIABLE_STATEMENT":
                item["type"] = "GLOBAL_VARIABLE"
            elif typ == "COMMENT_STATEMENT":
                item["type"] = "GLOBAL_COMMENT"
            elif typ == "EMPTY_LINE_STATEMENT":
                item["type"] = "GLOBAL_EMPTY_LINE"

            res.append(item)

        return res

    def empty_line(self, items: List[Any]) -> Dict[str, str]:
        return {"type": "EMPTY_LINE_STATEMENT"}

    def on_fn(self, items: List[Any]) -> Dict[str, Any]:
        func: Dict[str, Any] = {
            "type": "GLOBAL_ON_FN",
            "name": str(items[0]),
            "statements": items[-1],
        }
        if items[1]:
            func["arguments"] = items[1]
        return func

    def helper_fn(self, items: List[Any]) -> Dict[str, Any]:
        func: Dict[str, Any] = {
            "type": "GLOBAL_HELPER_FN",
            "name": str(items[0]),
        }
        if items[1]:
            func["arguments"] = items[1]
        if items[2]:
            func["return_type"] = items[2]
        func["statements"] = items[-1]
        return func

    def return_type(self, items: List[Any]) -> Any:
        return items[0]

    def statement(self, items: List[Any]) -> Optional[Any]:
        return items[0] if items else None

    def block_stmt(self, items: List[Any]) -> Any:
        return items[0]

    def block(self, items: List[Any]) -> List[Any]:
        return [s for s in items if s is not None]

    def vardecl(self, items: List[Any]) -> Dict[str, Any]:
        name, typ, expr = items
        return {
            "type": "VARIABLE_STATEMENT",
            "name": str(name),
            "variable_type": str(typ),
            "assignment": expr,
        }

    def ident_ref(self, items: List[Any]) -> Dict[str, Any]:
        return {"type": "IDENTIFIER_EXPR", "str": str(items[0])}

    def call_stmt(self, items: List[Any]) -> Dict[str, Any]:
        res = self.call_expr(items)
        res["type"] = "CALL_STATEMENT"
        return res

    def call_expr(self, items: List[Any]) -> Dict[str, Any]:
        result: Dict[str, Any] = {
            "type": "CALL_EXPR",
            "name": str(items[0]),
        }
        if len(items) > 1 and items[1] is not None:
            result["arguments"] = items[1]
        return result

    def type(self, items: List[Any]) -> str:
        return str(items[0])

    def reassign(self, items: List[Any]) -> Dict[str, Any]:
        name, expr = items
        return {
            "type": "VARIABLE_STATEMENT",
            "name": str(name),
            "assignment": expr,
        }

    def if_stmt(self, items: List[Union[List[Any], Dict[str, Any]]]) -> Dict[str, Any]:
        items = items[1:]  # Remove "if" token
        result: Dict[str, Any] = {
            "type": "IF_STATEMENT",
            "condition": items[0],
        }

        if len(items) > 1 and items[1]:
            result["if_statements"] = items[1]

        if len(items) > 2:
            else_block = items[2]
            if (
                isinstance(else_block, dict)
                and else_block.get("type") == "IF_STATEMENT"
            ):
                result["else_statements"] = [else_block]
            else:
                result["else_statements"] = else_block
        return result

    def while_loop(self, items: List[Any]) -> Dict[str, Any]:
        return {
            "type": "WHILE_STATEMENT",
            "condition": items[1],
            "statements": items[2],
        }

    def break_stmt(self, items: List[Any]) -> Dict[str, str]:
        return {"type": "BREAK_STATEMENT"}

    def continue_stmt(self, items: List[Any]) -> Dict[str, str]:
        return {"type": "CONTINUE_STATEMENT"}

    def return_stmt(self, items: List[Any]) -> Dict[str, Any]:
        result: Dict[str, Any] = {"type": "RETURN_STATEMENT"}
        if len(items) > 0 and items[0] is not None:
            result["expr"] = items[0]
        return result

    def _binary_expr(self, operator_token: str, items: List[Any]) -> Dict[str, Any]:
        return {
            "type": "BINARY_EXPR",
            "left_expr": items[0],
            "operator": operator_token,
            "right_expr": items[1],
        }

    # Strings
    def string_lit(self, items: List[Any]) -> Dict[str, Any]:
        s = str(items[0])
        if s.startswith('"') and s.endswith('"'):
            s = s[1:-1]
        return {"type": "STRING_EXPR", "str": s}

    def entity_lit(self, items: List[Any]) -> Dict[str, Any]:
        s = str(items[0])
        if s.startswith('"') and s.endswith('"'):
            s = s[1:-1]
        return {"type": "ENTITY_EXPR", "str": s}

    def resource_lit(self, items: List[Any]) -> Dict[str, Any]:
        s = str(items[0])
        if s.startswith('"') and s.endswith('"'):
            s = s[1:-1]
        return {"type": "RESOURCE_EXPR", "str": s}

    # Binary operations
    def add(self, items: List[Any]) -> Dict[str, Any]:
        return self._binary_expr("PLUS_TOKEN", items)

    def sub(self, items: List[Any]) -> Dict[str, Any]:
        return self._binary_expr("MINUS_TOKEN", items)

    def mul(self, items: List[Any]) -> Dict[str, Any]:
        return self._binary_expr("MULTIPLICATION_TOKEN", items)

    def div(self, items: List[Any]) -> Dict[str, Any]:
        return self._binary_expr("DIVISION_TOKEN", items)

    def eq(self, items: List[Any]) -> Dict[str, Any]:
        return self._binary_expr("EQUALS_TOKEN", items)

    def neq(self, items: List[Any]) -> Dict[str, Any]:
        return self._binary_expr("NOT_EQUALS_TOKEN", items)

    def lt(self, items: List[Any]) -> Dict[str, Any]:
        return self._binary_expr("LESS_TOKEN", items)

    def le(self, items: List[Any]) -> Dict[str, Any]:
        return self._binary_expr("LESS_OR_EQUAL_TOKEN", items)

    def gt(self, items: List[Any]) -> Dict[str, Any]:
        return self._binary_expr("GREATER_TOKEN", items)

    def ge(self, items: List[Any]) -> Dict[str, Any]:
        return self._binary_expr("GREATER_OR_EQUAL_TOKEN", items)

    # Logical operations
    def and_op(self, items: List[Any]) -> Dict[str, Any]:
        return {
            "type": "LOGICAL_EXPR",
            "left_expr": items[0],
            "operator": "AND_TOKEN",
            "right_expr": items[1],
        }

    def or_op(self, items: List[Any]) -> Dict[str, Any]:
        return {
            "type": "LOGICAL_EXPR",
            "left_expr": items[0],
            "operator": "OR_TOKEN",
            "right_expr": items[1],
        }

    def not_op(self, items: List[Any]) -> Dict[str, Any]:
        return {"type": "UNARY_EXPR", "operator": "NOT_TOKEN", "expr": items[1]}

    def neg(self, items: List[Any]) -> Dict[str, Any]:
        return {"type": "UNARY_EXPR", "operator": "MINUS_TOKEN", "expr": items[0]}

    def paren_expr(self, items: List[Any]) -> Dict[str, Any]:
        return {"type": "PARENTHESIZED_EXPR", "expr": items[0]}

    # Terminals
    def NUMBER(self, tok: Token) -> Dict[str, Any]:
        return {"type": "NUMBER_EXPR", "value": tok.value}

    def true_expr(self, items: List[Any]) -> Dict[str, str]:
        return {"type": "TRUE_EXPR"}

    def false_expr(self, items: List[Any]) -> Dict[str, str]:
        return {"type": "FALSE_EXPR"}

    def NAME(self, tok: Token) -> str:
        return str(tok)

    def ID(self, tok: Token) -> str:
        return str(tok)

    def args(self, items: List[Any]) -> List[Any]:
        return [i for i in items if i is not None]

    def params(self, items: List[Any]) -> List[Any]:
        return items

    def param(self, items: List[Any]) -> Dict[str, str]:
        return {"name": str(items[0]), "type": str(items[1])}

    def comment(self, items: List[Any]) -> Dict[str, str]:
        content: str = str(items[0]).lstrip("#").strip()
        return {"type": "COMMENT_STATEMENT", "comment": content}


def check_expected_json_format(path: Path, fix: bool) -> None:
    raw = path.read_text(encoding="utf-8")

    try:
        data = json.loads(raw)
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON in {path}")
        print(e)
        sys.exit(1)

    expected = json.dumps(data, indent=4, sort_keys=True) + "\n"

    if raw != expected:
        if fix:
            path.write_text(expected, encoding="utf-8")
            print(f"Fixed formatting: {path}")
        else:
            print(f"Error: {path} is not properly formatted")
            print("Expected format:")
            print(expected)
            print(
                "\nTip: run with '--fix' to automatically format all expected.json files."
            )
            sys.exit(1)


def check_dir(path: Path, fix: bool) -> None:
    transformer = GrugTransformer()
    for subdir in sorted(
        [d for d in path.iterdir() if d.is_dir()], key=lambda d: d.name
    ):
        print(f"Parsing {subdir}...")

        expected_file = subdir / "expected.json"

        check_expected_json_format(expected_file, fix)

        expected = json.loads(expected_file.read_text())

        for file in sorted(subdir.glob("*.grug"), key=lambda f: f.name):
            try:
                tree: Tree[Any] = parser.parse(file.read_text())  # type: ignore
                ast: Any = transformer.transform(tree)
            except exceptions.LarkError as e:
                print(f"Error: Grammar error in test: {file}")
                print(e)
                sys.exit(1)
            if ast != expected:
                print(f"Error: AST does not match expected.json for {file}")
                print("Got:")
                print(json.dumps(ast, indent=4))
                print("Expected:")
                print(json.dumps(expected, indent=4))
                sys.exit(1)


if __name__ == "__main__":
    fix = "--fix" in sys.argv

    root: Path = Path("tests")
    check_dir(root / "ok", fix)
    check_dir(root / "err_runtime", fix)

    print("All grammar checks passed")
