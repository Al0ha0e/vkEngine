from dataclasses import dataclass, field


class Namespace:
    def __init__(self, name: str, fa, depth: int) -> None:
        self.name = name
        self.fa = fa
        self.depth = depth
        self.classes: dict[str, ClassDef] = dict()

    def __str__(self) -> str:
        return f"namespace:{self.name} depth:{self.depth}"

    def __hash__(self):
        return hash(self.name)

    def __eq__(self, ano):
        return self.name == ano.name


@dataclass
class Field:
    type: str
    name: str

    def __str__(self) -> str:
        return f"<field type:{self.type} name:{self.name}>"


@dataclass
class ClassDef:
    name: str
    fields: dict[str, Field] = field(default_factory=dict)

    def __str__(self) -> str:
        ret = f"class {self.name}" + "{"
        for f in self.fields.values():
            ret += str(f)
        ret += "}"
        return ret
