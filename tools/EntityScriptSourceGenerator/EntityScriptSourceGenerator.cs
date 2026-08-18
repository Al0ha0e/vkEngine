using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;
using System.Text;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.CodeAnalysis.Text;

namespace vkEngine.Tools.EntityScriptSourceGenerator;

[Generator(LanguageNames.CSharp)]
public sealed class EntityScriptSourceGenerator : IIncrementalGenerator
{
    private const string EntityScriptFullName = "vkEngine.EngineCore.EntityScript";
    private const string ExportAttributeFullName = "vkEngine.EngineCore.ExportAttribute";

    private static readonly DiagnosticDescriptor GenerationFailed = new(
        id: "VKECSG001",
        title: "EntityScript binary reader generation failed",
        messageFormat: "{0}",
        category: "vkEngine.EntityScript",
        defaultSeverity: DiagnosticSeverity.Error,
        isEnabledByDefault: true);

    public void Initialize(IncrementalGeneratorInitializationContext context)
    {
        IncrementalValueProvider<ImmutableArray<INamedTypeSymbol>> classDeclarations = context.SyntaxProvider
            .CreateSyntaxProvider(
                static (node, _) => node is ClassDeclarationSyntax,
                static (syntaxContext, _) =>
                    syntaxContext.SemanticModel.GetDeclaredSymbol((ClassDeclarationSyntax)syntaxContext.Node))
            .Where(static type => type != null)
            .Select(static (type, _) => type!)
            .Collect();

        context.RegisterSourceOutput(
            classDeclarations,
            static (output, classes) => Generate(output, classes));
    }

    private static void Generate(SourceProductionContext output, ImmutableArray<INamedTypeSymbol> declaredClasses)
    {
        try
        {
            ScriptClass[] classes = declaredClasses
                .GroupBy(GetRuntimeTypeName, StringComparer.Ordinal)
                .Select(group => group.First())
                .Where(type => type.TypeKind == TypeKind.Class && !type.IsAbstract && IsEntityScript(type))
                .OrderBy(GetRuntimeTypeName, StringComparer.Ordinal)
                .Select(CreateClassMetadata)
                .ToArray();

            string source = new BinaryReaderSourceBuilder(classes).Generate();
            output.AddSource("EntityScriptBinaryReaders.g.cs", SourceText.From(source, Encoding.UTF8));
        }
        catch (GenerationException exception)
        {
            output.ReportDiagnostic(Diagnostic.Create(GenerationFailed, exception.Location, exception.Message));
        }
        catch (Exception exception)
        {
            output.ReportDiagnostic(Diagnostic.Create(GenerationFailed, Location.None, exception.Message));
        }
    }

    private static bool IsEntityScript(INamedTypeSymbol type)
    {
        for (INamedTypeSymbol? baseType = type.BaseType; baseType != null; baseType = baseType.BaseType)
            if (GetRuntimeTypeName(baseType) == EntityScriptFullName)
                return true;
        return false;
    }

    private static ScriptClass CreateClassMetadata(INamedTypeSymbol type)
    {
        if (type.IsGenericType)
            throw Error(type, $"Generic EntityScript '{GetRuntimeTypeName(type)}' is not supported.");

        var members = new List<ExportedMember>();
        for (INamedTypeSymbol? current = type; current != null && GetRuntimeTypeName(current) != EntityScriptFullName; current = current.BaseType)
        {
            foreach (ISymbol member in current.GetMembers())
            {
                AttributeData? attribute = FindExportAttribute(member);
                if (attribute == null || member.IsStatic)
                    continue;

                if (member is IFieldSymbol field && field.DeclaredAccessibility == Accessibility.Public)
                {
                    members.Add(new ExportedMember(
                        field.Name,
                        GetExportName(field, attribute),
                        field.Type,
                        !field.IsReadOnly,
                        field.Locations.FirstOrDefault()));
                }
                else if (member is IPropertySymbol property && property.Parameters.Length == 0 &&
                         (property.GetMethod?.DeclaredAccessibility == Accessibility.Public ||
                          property.SetMethod?.DeclaredAccessibility == Accessibility.Public))
                {
                    members.Add(new ExportedMember(
                        property.Name,
                        GetExportName(property, attribute),
                        property.Type,
                        property.SetMethod?.DeclaredAccessibility == Accessibility.Public,
                        property.Locations.FirstOrDefault()));
                }
            }
        }

        ExportedMember[] ordered = members.OrderBy(member => member.Name, StringComparer.Ordinal).ToArray();
        string? duplicateExportName = ordered
            .GroupBy(member => member.ExportName, StringComparer.Ordinal)
            .FirstOrDefault(group => group.Count() != 1)?.Key;
        if (duplicateExportName != null)
            throw Error(type,
                $"EntityScript '{GetRuntimeTypeName(type)}' has multiple exported members named '{duplicateExportName}'.");

        return new ScriptClass(type, GetRuntimeTypeName(type), ordered);
    }

    private static AttributeData? FindExportAttribute(ISymbol member)
    {
        return member.GetAttributes().FirstOrDefault(attribute =>
            attribute.AttributeClass != null && GetRuntimeTypeName(attribute.AttributeClass) == ExportAttributeFullName);
    }

    private static string GetExportName(ISymbol member, AttributeData attribute)
    {
        if (attribute.ConstructorArguments.Length == 1 &&
            attribute.ConstructorArguments[0].Value is string name &&
            !string.IsNullOrWhiteSpace(name))
            return name;
        return member.Name;
    }

    private static string GetRuntimeTypeName(INamedTypeSymbol type)
    {
        var names = new Stack<string>();
        for (INamedTypeSymbol? current = type; current != null; current = current.ContainingType)
            names.Push(current.MetadataName);
        string typeName = string.Join("+", names);
        string namespaceName = type.ContainingNamespace?.ToDisplayString() ?? string.Empty;
        return namespaceName.Length == 0 ? typeName : namespaceName + "." + typeName;
    }

    private static string GetCSharpTypeName(ITypeSymbol type)
    {
        return type.ToDisplayString(SymbolDisplayFormat.FullyQualifiedFormat);
    }

    private static GenerationException Error(ISymbol symbol, string message)
    {
        return new GenerationException(message, symbol.Locations.FirstOrDefault());
    }

    private sealed class BinaryReaderSourceBuilder
    {
        private const string ReaderType = "global::vkEngine.EngineCore.EntityScriptBinaryReader";
        private readonly ScriptClass[] classes;
        private readonly StringBuilder helpers = new();
        private readonly Dictionary<ITypeSymbol, string> helperNames = new(SymbolEqualityComparer.Default);
        private readonly HashSet<ITypeSymbol> generatingHelpers = new(SymbolEqualityComparer.Default);
        private int nextHelperId;

        public BinaryReaderSourceBuilder(ScriptClass[] classes)
        {
            this.classes = classes;
        }

        public string Generate()
        {
            ValidateClasses();

            var methods = new StringBuilder();
            foreach (ScriptClass scriptClass in classes)
                AppendScriptMethod(methods, scriptClass);

            var source = new StringBuilder();
            source.AppendLine("// <auto-generated />");
            source.AppendLine("#nullable enable");
            source.AppendLine();
            source.AppendLine("namespace vkEngine.Generated");
            source.AppendLine("{");
            source.AppendLine("    public static unsafe class EntityScriptBinaryReaders");
            source.AppendLine("    {");
            AppendDispatcher(source);
            source.Append(methods);
            source.Append(helpers);
            source.AppendLine("    }");
            source.AppendLine("}");
            return source.ToString();
        }

        private void ValidateClasses()
        {
            var methodNames = new HashSet<string>(StringComparer.Ordinal);
            foreach (ScriptClass scriptClass in classes)
            {
                if (!methodNames.Add(GetScriptMethodName(scriptClass)))
                    throw Error(scriptClass.Type, $"Generated reader name collision for '{scriptClass.FullName}'.");

                bool hasConstructor = scriptClass.Type.InstanceConstructors.Any(constructor =>
                    !constructor.IsStatic && constructor.Parameters.Length == 1 &&
                    constructor.Parameters[0].Type.SpecialType == SpecialType.System_UInt32 &&
                    constructor.DeclaredAccessibility is Accessibility.Public or Accessibility.Internal or Accessibility.ProtectedOrInternal);
                if (!hasConstructor)
                    throw Error(scriptClass.Type,
                        $"EntityScript '{scriptClass.FullName}' must have a public or internal constructor with a single UInt32 parameter for generated deserialization.");

                foreach (ExportedMember member in scriptClass.Members)
                {
                    if (!member.CanWrite)
                        throw new GenerationException(
                            $"Exported member '{scriptClass.FullName}.{member.Name}' must be publicly writable.", member.Location);
                    _ = GetAlignment(member.Type, new HashSet<ITypeSymbol>(SymbolEqualityComparer.Default));
                    ValidateDepth(member.Type, depth: 2, new HashSet<ITypeSymbol>(SymbolEqualityComparer.Default));
                }
            }
        }

        private void AppendDispatcher(StringBuilder source)
        {
            source.AppendLine("        public static global::vkEngine.EngineCore.EntityScript Parse(");
            source.AppendLine("            string className, uint entity, byte* data, int dataSize)");
            source.AppendLine("        {");
            source.AppendLine("            return className switch");
            source.AppendLine("            {");
            foreach (ScriptClass scriptClass in classes)
            {
                source.Append("                \"").Append(EscapeString(scriptClass.FullName)).Append("\" => ")
                    .Append(GetScriptMethodName(scriptClass)).AppendLine("(entity, data, dataSize),");
            }
            source.AppendLine("                _ => throw new global::System.InvalidOperationException($\"No generated EntityScript binary reader exists for '{className}'.\")");
            source.AppendLine("            };");
            source.AppendLine("        }");
            source.AppendLine();
        }

        private void AppendScriptMethod(StringBuilder source, ScriptClass scriptClass)
        {
            int alignment = GetStructAlignment(scriptClass.Members.Select(member => member.Type));
            source.Append("        public static ").Append(GetCSharpTypeName(scriptClass.Type)).Append(' ')
                .Append(GetScriptMethodName(scriptClass)).AppendLine("(uint entity, byte* data, int dataSize)");
            source.AppendLine("        {");
            source.Append("            var reader = new ").Append(ReaderType).AppendLine("(data, dataSize);");
            source.Append("            reader.Align(").Append(alignment).AppendLine(");");
            source.Append("            var value = new ").Append(GetCSharpTypeName(scriptClass.Type)).AppendLine("(entity);");
            foreach (ExportedMember member in scriptClass.Members)
            {
                source.Append("            value.").Append(EscapeIdentifier(member.Name)).Append(" = ")
                    .Append(GetReadExpression(member.Type)).AppendLine(";");
            }
            source.Append("            reader.Align(").Append(alignment).AppendLine(");");
            source.AppendLine("            reader.EnsureComplete();");
            source.AppendLine("            return value;");
            source.AppendLine("        }");
            source.AppendLine();
        }

        private string GetReadExpression(ITypeSymbol type)
        {
            return type.SpecialType switch
            {
                SpecialType.System_Byte => "reader.ReadByte()",
                SpecialType.System_Int32 => "reader.ReadInt32()",
                SpecialType.System_Int64 => "reader.ReadInt64()",
                SpecialType.System_Single => "reader.ReadFloat32()",
                SpecialType.System_Double => "reader.ReadFloat64()",
                SpecialType.System_String => "reader.ReadString()",
                _ => $"{GetOrCreateHelper(type)}(ref reader)"
            };
        }

        private string GetOrCreateHelper(ITypeSymbol type)
        {
            if (helperNames.TryGetValue(type, out string? existing))
                return existing;
            if (!generatingHelpers.Add(type))
                throw Error(type, $"Cyclic binary type '{type.ToDisplayString()}' is not supported.");

            string name = $"ReadValue{nextHelperId++}";
            helperNames.Add(type, name);
            try
            {
                if (type is IArrayTypeSymbol { Rank: 1 } arrayType)
                    AppendArrayHelper(arrayType, name);
                else if (TryGetVectorInfo(type, out ITypeSymbol scalarType, out int componentCount))
                    AppendVectorHelper(type, scalarType, componentCount, name);
                else if (type is INamedTypeSymbol { IsValueType: true, TypeKind: TypeKind.Struct, IsGenericType: false } structType)
                    AppendStructHelper(structType, name);
                else
                    throw UnsupportedType(type);
            }
            finally
            {
                generatingHelpers.Remove(type);
            }
            return name;
        }

        private void AppendArrayHelper(IArrayTypeSymbol arrayType, string name)
        {
            ITypeSymbol elementType = arrayType.ElementType;
            int elementAlignment = GetAlignment(elementType, NewTypeSet());
            int arrayAlignment = Math.Max(4, elementAlignment);
            int minimumElementSize = GetMinimumSize(elementType, NewTypeSet());
            string elementExpression = GetReadExpression(elementType);
            helpers.Append("        private static ").Append(GetCSharpTypeName(arrayType)).Append(' ').Append(name)
                .Append("(ref ").Append(ReaderType).AppendLine(" reader)");
            helpers.AppendLine("        {");
            helpers.Append("            reader.Align(").Append(arrayAlignment).AppendLine(");");
            helpers.Append("            int count = reader.ReadArrayLength(").Append(elementType.SpecialType == SpecialType.System_Byte ? "true" : "false").AppendLine(");");
            helpers.Append("            reader.Align(").Append(elementAlignment).AppendLine(");");
            helpers.Append("            reader.ValidateArrayPayload(count, ").Append(minimumElementSize).AppendLine(");");
            helpers.Append("            var value = new ").Append(GetCSharpTypeName(elementType)).AppendLine("[count];");
            helpers.AppendLine("            for (int i = 0; i < count; i++)");
            helpers.Append("                value[i] = ").Append(elementExpression).AppendLine(";");
            helpers.AppendLine("            return value;");
            helpers.AppendLine("        }");
            helpers.AppendLine();
        }

        private void AppendVectorHelper(ITypeSymbol vectorType, ITypeSymbol scalarType, int componentCount, string name)
        {
            int alignment = GetAlignment(scalarType, NewTypeSet());
            helpers.Append("        private static ").Append(GetCSharpTypeName(vectorType)).Append(' ').Append(name)
                .Append("(ref ").Append(ReaderType).AppendLine(" reader)");
            helpers.AppendLine("        {");
            helpers.Append("            reader.Align(").Append(alignment).AppendLine(");");
            helpers.Append("            return new ").Append(GetCSharpTypeName(vectorType)).Append('(');
            for (int i = 0; i < componentCount; i++)
            {
                if (i != 0) helpers.Append(", ");
                helpers.Append(GetReadExpression(scalarType));
            }
            helpers.AppendLine(");");
            helpers.AppendLine("        }");
            helpers.AppendLine();
        }

        private void AppendStructHelper(INamedTypeSymbol type, string name)
        {
            ISymbol[] members = GetStructMembers(type);
            int alignment = GetStructAlignment(members.Select(GetMemberType));
            string[] memberExpressions = members.Select(member => GetReadExpression(GetMemberType(member))).ToArray();
            helpers.Append("        private static ").Append(GetCSharpTypeName(type)).Append(' ').Append(name)
                .Append("(ref ").Append(ReaderType).AppendLine(" reader)");
            helpers.AppendLine("        {");
            helpers.Append("            reader.Align(").Append(alignment).AppendLine(");");
            helpers.Append("            var value = new ").Append(GetCSharpTypeName(type)).AppendLine("();");
            for (int i = 0; i < members.Length; i++)
            {
                helpers.Append("            value.").Append(EscapeIdentifier(members[i].Name)).Append(" = ")
                    .Append(memberExpressions[i]).AppendLine(";");
            }
            helpers.Append("            reader.Align(").Append(alignment).AppendLine(");");
            helpers.AppendLine("            return value;");
            helpers.AppendLine("        }");
            helpers.AppendLine();
        }

        private static void ValidateDepth(ITypeSymbol type, int depth, HashSet<ITypeSymbol> path)
        {
            bool isArray = type is IArrayTypeSymbol { Rank: 1 };
            bool isStruct = type is INamedTypeSymbol { IsValueType: true, TypeKind: TypeKind.Struct } &&
                !TryGetVectorInfo(type, out _, out _);
            if (!isArray && !isStruct)
                return;
            if (depth > 64)
                throw Error(type, $"Binary type '{type.ToDisplayString()}' exceeds the format nesting depth limit of 64.");
            if (!path.Add(type))
                throw Error(type, $"Cyclic binary type '{type.ToDisplayString()}' is not supported.");
            try
            {
                if (type is IArrayTypeSymbol arrayType)
                    ValidateDepth(arrayType.ElementType, depth + 1, path);
                else
                    foreach (ISymbol member in GetStructMembers((INamedTypeSymbol)type))
                        ValidateDepth(GetMemberType(member), depth + 1, path);
            }
            finally
            {
                path.Remove(type);
            }
        }

        private static int GetAlignment(ITypeSymbol type, HashSet<ITypeSymbol> path)
        {
            switch (type.SpecialType)
            {
                case SpecialType.System_Byte: return 1;
                case SpecialType.System_Int32:
                case SpecialType.System_Single:
                case SpecialType.System_String: return 4;
                case SpecialType.System_Int64:
                case SpecialType.System_Double: return 8;
            }
            if (type is IArrayTypeSymbol { Rank: 1 } arrayType)
                return Math.Max(4, GetAlignment(arrayType.ElementType, path));
            if (TryGetVectorInfo(type, out ITypeSymbol scalarType, out _))
                return GetAlignment(scalarType, path);
            if (type is not INamedTypeSymbol { IsValueType: true, TypeKind: TypeKind.Struct } structType)
                throw UnsupportedType(type);
            if (!path.Add(type))
                throw Error(type, $"Cyclic binary type '{type.ToDisplayString()}' is not supported.");
            try
            {
                return GetStructAlignment(GetStructMembers(structType).Select(GetMemberType), path);
            }
            finally
            {
                path.Remove(type);
            }
        }

        private static int GetStructAlignment(IEnumerable<ITypeSymbol> types, HashSet<ITypeSymbol>? path = null)
        {
            path ??= NewTypeSet();
            int alignment = 1;
            foreach (ITypeSymbol type in types)
                alignment = Math.Max(alignment, GetAlignment(type, path));
            return alignment;
        }

        private static int GetMinimumSize(ITypeSymbol type, HashSet<ITypeSymbol> path)
        {
            switch (type.SpecialType)
            {
                case SpecialType.System_Byte: return 1;
                case SpecialType.System_Int32:
                case SpecialType.System_Single:
                case SpecialType.System_String: return 4;
                case SpecialType.System_Int64:
                case SpecialType.System_Double: return 8;
            }
            if (TryGetVectorInfo(type, out ITypeSymbol scalarType, out int componentCount))
                return checked(GetMinimumSize(scalarType, path) * componentCount);
            if (type is IArrayTypeSymbol { Rank: 1 } arrayType)
                return AlignUp(4, GetAlignment(arrayType.ElementType, path));
            if (type is not INamedTypeSymbol { IsValueType: true, TypeKind: TypeKind.Struct } structType)
                throw UnsupportedType(type);
            if (!path.Add(type))
                throw Error(type, $"Cyclic binary type '{type.ToDisplayString()}' is not supported.");
            try
            {
                int offset = 0;
                int structAlignment = 1;
                foreach (ISymbol member in GetStructMembers(structType))
                {
                    ITypeSymbol memberType = GetMemberType(member);
                    int memberAlignment = GetAlignment(memberType, path);
                    structAlignment = Math.Max(structAlignment, memberAlignment);
                    offset = checked(AlignUp(offset, memberAlignment) + GetMinimumSize(memberType, path));
                }
                return AlignUp(offset, structAlignment);
            }
            finally
            {
                path.Remove(type);
            }
        }

        private static ISymbol[] GetStructMembers(INamedTypeSymbol type)
        {
            IFieldSymbol[] publicFields = type.GetMembers().OfType<IFieldSymbol>()
                .Where(field => !field.IsStatic && field.DeclaredAccessibility == Accessibility.Public)
                .ToArray();
            IFieldSymbol? readOnlyField = publicFields.FirstOrDefault(field => field.IsReadOnly);
            if (readOnlyField != null)
                throw Error(readOnlyField,
                    $"Binary struct member '{type.ToDisplayString()}.{readOnlyField.Name}' is readonly and cannot be deserialized.");

            IEnumerable<ISymbol> properties = type.GetMembers().OfType<IPropertySymbol>()
                .Where(property => !property.IsStatic && property.Parameters.Length == 0 &&
                                   property.SetMethod?.DeclaredAccessibility == Accessibility.Public);
            return publicFields.Cast<ISymbol>().Concat(properties)
                .OrderBy(member => member.Name, StringComparer.Ordinal).ToArray();
        }

        private static ITypeSymbol GetMemberType(ISymbol member)
        {
            return member switch
            {
                IFieldSymbol field => field.Type,
                IPropertySymbol property => property.Type,
                _ => throw Error(member, $"Unsupported member kind '{member.Kind}'.")
            };
        }

        private static bool TryGetVectorInfo(ITypeSymbol type, out ITypeSymbol scalarType, out int componentCount)
        {
            componentCount = type is INamedTypeSymbol namedType ? GetRuntimeTypeName(namedType) switch
            {
                "vkEngine.EngineCore.NVec2" => 2,
                "vkEngine.EngineCore.NVec3" => 3,
                "vkEngine.EngineCore.NVec4" => 4,
                "vkEngine.EngineCore.NQuat" => 4,
                _ => 0
            } : 0;
            if (componentCount == 0 || type is not INamedTypeSymbol vectorType)
            {
                scalarType = type;
                return false;
            }

            scalarType = vectorType.GetMembers().OfType<IFieldSymbol>()
                .First(field => !field.IsStatic && field.Type.SpecialType == SpecialType.System_Single).Type;
            return true;
        }

        private static GenerationException UnsupportedType(ITypeSymbol type)
        {
            return Error(type,
                $"C# type '{type.ToDisplayString()}' cannot be represented by the EntityScript binary format. " +
                "Supported types are byte, Int32, Int64, Single, Double, String, one-dimensional arrays, EngineCore vectors, and value structs composed from those types.");
        }

        private static HashSet<ITypeSymbol> NewTypeSet() => new(SymbolEqualityComparer.Default);
        private static int AlignUp(int value, int alignment) => checked((value + alignment - 1) & -alignment);

        private static string GetScriptMethodName(ScriptClass scriptClass)
        {
            var result = new StringBuilder("Parse_");
            foreach (char character in scriptClass.FullName)
                result.Append(char.IsLetterOrDigit(character) || character == '_' ? character : '_');
            return result.ToString();
        }

        private static string EscapeIdentifier(string value) => "@" + value;
        private static string EscapeString(string value) => value.Replace("\\", "\\\\").Replace("\"", "\\\"");
    }

    private sealed class ScriptClass
    {
        public ScriptClass(INamedTypeSymbol type, string fullName, ExportedMember[] members)
        {
            Type = type;
            FullName = fullName;
            Members = members;
        }

        public INamedTypeSymbol Type { get; }
        public string FullName { get; }
        public ExportedMember[] Members { get; }
    }

    private sealed class ExportedMember
    {
        public ExportedMember(string name, string exportName, ITypeSymbol type, bool canWrite, Location? location)
        {
            Name = name;
            ExportName = exportName;
            Type = type;
            CanWrite = canWrite;
            Location = location;
        }

        public string Name { get; }
        public string ExportName { get; }
        public ITypeSymbol Type { get; }
        public bool CanWrite { get; }
        public Location? Location { get; }
    }

    private sealed class GenerationException : Exception
    {
        public GenerationException(string message, Location? location) : base(message)
        {
            Location = location;
        }

        public Location? Location { get; }
    }
}
