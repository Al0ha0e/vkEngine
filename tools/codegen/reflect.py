import os
import sys
from pathlib import Path
from cppinfo import Namespace, ClassDef, Field

namespace_stack: list[Namespace] = []
namespace_dict: dict[str, Namespace] = {}

namespace_stack.append(Namespace("", None, -1))


def is_namechar(c: str) -> bool:
    return c.isalnum() or c == "_"


def skip_comment1(content: str, now: int, l: int) -> int:  # skip //
    while now < l:
        if content[now] == "\n":
            return now + 1
        now += 1
    return l


def skip_comment2(content: str, now: int, l: int) -> int:  # skip /**/
    now += 2
    met = False
    while now < l:
        c = content[now]
        if met and c == "/":
            return now + 1
        elif c == "*":
            met = True
        else:
            met = False
        now += 1
    return l


def skip_char(content: str, now: int, l: int) -> int:  # skip character
    if now + 1 >= l:
        return l
    if content[now + 1] == "\\":
        return now + 4
    return now + 3


def skip_string(content: str, now: int, l: int) -> int:  # skip string
    now += 1
    met = False
    while now < l:
        c = content[now]
        if not met:
            if c == '"':
                return now + 1
            if c == "\\":
                met = True
        else:
            met = False
        now += 1
    return l


def handle_namespace(content: str, now: int, l: int, depth: int) -> int:
    now += len("namespace")
    while now < l and content[now] == " ":
        now += 1
    en = now
    while en < l and is_namechar(content[en]):
        en += 1
    name = content[now:en]

    if len(namespace_stack) > 1:
        name = namespace_stack[-1].name + "::" + name

    if name in namespace_dict:
        namespace = namespace_dict[name]
    else:
        namespace = Namespace(
            name, None if len(namespace_stack) == 1 else namespace_stack[-1], depth
        )
        namespace_dict[name] = namespace

    print(namespace)
    namespace_stack.append(namespace)
    return en


def match_word(target: str, content: str, now: int, l: int) -> bool:
    return content[now : now + len(target)] == target


def match_list(
    lr: tuple[str, str], content: str, now: int, l: int
) -> tuple[int, list[str]]:
    while now < l and content[now] != lr[0]:
        now += 1
    now += 1
    if now >= l:
        return (l, [])

    ret = []
    st = now
    while now < l:
        c = content[now]
        if c == lr[1]:
            ret.append(content[st:now].strip())
            now += 1
            break
        if c == ",":
            ret.append(content[st:now].strip())
            st = now + 1
        now += 1

    return (now, ret)


def handle_field(class_def: ClassDef, content: str, now: int, l: int) -> int:
    now += len("REFLECT_FIELD")
    now, kv = match_list(("(", ")"), content, now, l)
    field = Field(kv[0], kv[1])
    class_def.fields[kv[1]] = field
    return now


def handle_classdef(is_struct: bool, content: str, now: int, l: int) -> int:
    now += len("REFLECT_STRUCT") if is_struct else len("REFLECT_CLASS")

    # match name
    now, name = match_list(("(", ")"), content, now, l)
    print(f"class {name[0]}")

    class_def = ClassDef(name=name[0])

    # match body
    depth = 0
    while now < l:
        c = content[now]
        nxt = now + 1
        if c == "{":
            depth += 1
        elif c == "R" and match_word("REFLECT_FIELD", content, now, l):
            nxt = handle_field(class_def, content, now, l)
        elif c == "}":
            depth -= 1
            if depth == 0:
                now += 2
                break
        now = nxt
    namespace_stack[-1].classes[class_def.name] = class_def
    return now


def do_analyse(content: str):
    l = len(content)
    now = 0
    scope_depth = 0
    while now < l:
        c = content[now]
        nxt = now + 1
        if c == "/" and now + 1 < l:
            if content[now + 1] == "/":
                nxt = skip_comment1(content, now, l)
            elif content[now + 1] == "*":
                nxt = skip_comment2(content, now, l)
        elif c == "'":
            nxt = skip_char(content, now, l)
        elif c == '"':
            nxt = skip_string(content, now, l)
        elif c == "{":
            scope_depth += 1
            print("+1=", scope_depth)
        elif c == "}":
            scope_depth -= 1
            print("-1=", scope_depth)
            if len(namespace_stack) > 1 and namespace_stack[-1].depth == scope_depth:
                print("pop", namespace_stack.pop())
        elif c == "n" and match_word("namespace", content, now, l):
            nxt = handle_namespace(content, now, l, scope_depth)
        elif c == "R":
            if match_word("REFLECT_STRUCT", content, now, l):
                nxt = handle_classdef(True, content, now, l)
            elif match_word("REFLECT_CLASS", content, now, l):
                nxt = handle_classdef(False, content, now, l)
        now = nxt


path = sys.argv[1]
content = Path(path).read_text(encoding="utf-8")
do_analyse(content)

cpp_filename = path[len("./include/") :]

with open("./src/generated/serialize.cpp", "w") as f:
    for namespace in namespace_dict.values():
        f.write("#include <nlohmann/json.hpp>\n")
        f.write(f"#include<{cpp_filename}>\n")
        f.write(f"namespace {namespace.name}" + "{\n")
        for class_def in namespace.classes.values():
            f.write(
                f"void {class_def.name}::loadJSON(const nlohmann::json &json)" + "{\n"
            )

            for field_def in class_def.fields.values():
                f.write(f'{field_def.name} = json["{field_def.name}"];')

            f.write("}\n")
        f.write("}\n")
