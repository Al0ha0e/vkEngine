using System.Reflection;
using System.Runtime.Loader;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.Json.Serialization;

namespace vkEngine.Tools.EntityScriptMetadataExporter;

internal static class Program
{
    private const string EntityScriptFullName = "vkEngine.EngineCore.EntityScript";
    private const string ExportAttributeFullName = "vkEngine.EngineCore.ExportAttribute";

    private static readonly JsonSerializerOptions jsonOptions = new()
    {
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
        DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull,
        WriteIndented = true
    };

    public static int Main(string[] args)
    {
        if (args.Length != 2)
        {
            Console.Error.WriteLine("Usage: EntityScriptMetadataExporter <game-assembly> <output-json>");
            return 2;
        }

        try
        {
            Export(Path.GetFullPath(args[0]), Path.GetFullPath(args[1]));
            return 0;
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine($"Entity script metadata export failed: {exception.Message}");
            return 1;
        }
    }

    private static void Export(string assemblyPath, string outputPath)
    {
        if (!File.Exists(assemblyPath))
            throw new FileNotFoundException("The game assembly was not found.", assemblyPath);

        using var loadContext = new GameAssemblyLoadContext(Path.GetDirectoryName(assemblyPath)!);
        Assembly assembly = loadContext.LoadFromAssemblyPath(assemblyPath);

        ScriptClassMetadata[] classes = GetLoadableTypes(assembly)
            .Where(type => type.IsClass && !type.IsAbstract && IsEntityScript(type))
            .OrderBy(type => type.FullName, StringComparer.Ordinal)
            .Select(CreateClassMetadata)
            .ToArray();

        JsonObject[] types = classes.Select(CreateScriptTypeInfo).ToArray();
        var document = new MetadataDocument(assembly.GetName().Name ?? string.Empty, types);
        string json = JsonSerializer.Serialize(document, jsonOptions);
        WriteAtomically(outputPath, json + Environment.NewLine);

        Console.WriteLine($"Generated metadata for {classes.Length} EntityScript class(es): {outputPath}");
    }

    private static void WriteAtomically(string path, string contents)
    {
        if (File.Exists(path) && string.Equals(File.ReadAllText(path), contents, StringComparison.Ordinal))
            return;

        string? directory = Path.GetDirectoryName(path);
        if (!string.IsNullOrEmpty(directory))
            Directory.CreateDirectory(directory);

        string temporaryPath = path + ".tmp";
        try
        {
            File.WriteAllText(temporaryPath, contents, new UTF8Encoding(encoderShouldEmitUTF8Identifier: false));
            File.Move(temporaryPath, path, overwrite: true);
        }
        finally
        {
            if (File.Exists(temporaryPath))
                File.Delete(temporaryPath);
        }
    }

    private static Type[] GetLoadableTypes(Assembly assembly)
    {
        try
        {
            return assembly.GetTypes();
        }
        catch (ReflectionTypeLoadException exception)
        {
            string details = string.Join(
                Environment.NewLine,
                exception.LoaderExceptions.Where(error => error != null).Select(error => error!.Message));
            throw new InvalidOperationException(
                $"Some game types could not be loaded:{Environment.NewLine}{details}", exception);
        }
    }

    private static bool IsEntityScript(Type type)
    {
        for (Type? baseType = type.BaseType; baseType != null; baseType = baseType.BaseType)
            if (baseType.FullName == EntityScriptFullName)
                return true;
        return false;
    }

    private static ScriptClassMetadata CreateClassMetadata(Type type)
    {
        IEnumerable<ExportedMemberMetadata> fields = type
            .GetFields(BindingFlags.Instance | BindingFlags.Public)
            .Select(field => (Member: (MemberInfo)field, Attribute: FindExportAttribute(field)))
            .Where(item => item.Attribute != null)
            .Select(item => new ExportedMemberMetadata(
                item.Member.Name,
                GetExportName(item.Member, item.Attribute!),
                GetCppType(item.Attribute!),
                ((FieldInfo)item.Member).FieldType));

        IEnumerable<ExportedMemberMetadata> properties = type
            .GetProperties(BindingFlags.Instance | BindingFlags.Public)
            .Where(property => property.GetIndexParameters().Length == 0)
            .Select(property => (Member: (MemberInfo)property, Attribute: FindExportAttribute(property)))
            .Where(item => item.Attribute != null)
            .Select(item => new ExportedMemberMetadata(
                item.Member.Name,
                GetExportName(item.Member, item.Attribute!),
                GetCppType(item.Attribute!),
                ((PropertyInfo)item.Member).PropertyType));

        ExportedMemberMetadata[] members = fields.Concat(properties)
            .OrderBy(member => member.Name, StringComparer.Ordinal)
            .ToArray();

        string? duplicateExportName = members
            .GroupBy(member => member.ExportName, StringComparer.Ordinal)
            .FirstOrDefault(group => group.Count() != 1)?.Key;
        if (duplicateExportName != null)
            throw new InvalidOperationException(
                $"EntityScript '{type.FullName}' has multiple exported members named '{duplicateExportName}'.");

        return new ScriptClassMetadata(type.FullName ?? type.Name, members);
    }

    private static CustomAttributeData? FindExportAttribute(MemberInfo member)
    {
        return member.CustomAttributes.FirstOrDefault(
            attribute => attribute.AttributeType.FullName == ExportAttributeFullName);
    }

    private static string GetExportName(MemberInfo member, CustomAttributeData attribute)
    {
        if (attribute.ConstructorArguments.Count == 1 &&
            attribute.ConstructorArguments[0].Value is string name &&
            !string.IsNullOrWhiteSpace(name))
            return name;
        return member.Name;
    }

    private static string? GetCppType(CustomAttributeData attribute)
    {
        CustomAttributeNamedArgument argument = attribute.NamedArguments.FirstOrDefault(
            item => item.MemberName == "CppType");
        return argument.TypedValue.Value is string cppType && !string.IsNullOrWhiteSpace(cppType)
            ? cppType
            : null;
    }

    private static JsonObject CreateScriptTypeInfo(ScriptClassMetadata scriptClass)
    {
        var path = new HashSet<Type>();
        var fields = new JsonArray();
        foreach (ExportedMemberMetadata member in scriptClass.Members)
        {
            fields.Add(new JsonObject
            {
                ["name"] = member.ExportName,
                ["type"] = CreateTypeInfo(member.RuntimeType, member.CppType, path)
            });
        }

        return new JsonObject
        {
            ["kind"] = "struct",
            ["name"] = scriptClass.FullName,
            ["fields"] = fields
        };
    }

    private static JsonObject CreateTypeInfo(Type type, string? nameOverride, HashSet<Type> path)
    {
        string? primitiveKind = type == typeof(byte) ? "byte" :
            type == typeof(int) ? "int32" :
            type == typeof(long) ? "int64" :
            type == typeof(float) ? "float32" :
            type == typeof(double) ? "float64" :
            type == typeof(string) ? "string" : null;
        if (primitiveKind != null)
        {
            if (nameOverride != null)
                throw new InvalidOperationException(
                    $"CppType cannot override the fixed TypeInfo name of primitive C# type '{GetTypeName(type)}'.");
            return new JsonObject { ["kind"] = primitiveKind };
        }

        if (!path.Add(type))
            throw new InvalidOperationException($"Cyclic binary type '{GetTypeName(type)}' is not supported.");
        try
        {
            if (type.IsArray && type.GetArrayRank() == 1)
            {
                Type elementType = type.GetElementType()!;
                return new JsonObject
                {
                    ["kind"] = "array",
                    ["name"] = nameOverride ?? GetTypeName(type),
                    ["elementType"] = CreateTypeInfo(elementType, null, path)
                };
            }

            if (TryGetVectorInfo(type, out Type scalarType, out int componentCount))
            {
                return new JsonObject
                {
                    ["kind"] = "vector",
                    ["name"] = nameOverride ?? GetTypeName(type),
                    ["scalarType"] = CreateTypeInfo(scalarType, null, path),
                    ["componentCount"] = componentCount
                };
            }

            if (!type.IsValueType || type.IsPrimitive || type.IsEnum)
                throw UnsupportedType(type);

            var fields = new JsonArray();
            foreach (MemberInfo member in GetStructMembers(type))
            {
                fields.Add(new JsonObject
                {
                    ["name"] = member.Name,
                    ["type"] = CreateTypeInfo(GetMemberType(member), null, path)
                });
            }
            return new JsonObject
            {
                ["kind"] = "struct",
                ["name"] = nameOverride ?? GetTypeName(type),
                ["fields"] = fields
            };
        }
        finally
        {
            path.Remove(type);
        }
    }

    private static MemberInfo[] GetStructMembers(Type type)
    {
        FieldInfo[] publicFields = type.GetFields(BindingFlags.Instance | BindingFlags.Public);
        FieldInfo? readOnlyField = publicFields.FirstOrDefault(field => field.IsInitOnly);
        if (readOnlyField != null)
            throw new InvalidOperationException(
                $"Binary struct member '{GetTypeName(type)}.{readOnlyField.Name}' is readonly and cannot be deserialized.");

        IEnumerable<MemberInfo> properties = type
            .GetProperties(BindingFlags.Instance | BindingFlags.Public)
            .Where(property => property.GetIndexParameters().Length == 0 && property.SetMethod?.IsPublic == true);
        return publicFields.Cast<MemberInfo>().Concat(properties)
            .OrderBy(member => member.Name, StringComparer.Ordinal).ToArray();
    }

    private static bool TryGetVectorInfo(Type type, out Type scalarType, out int componentCount)
    {
        scalarType = typeof(float);
        componentCount = type.FullName switch
        {
            "vkEngine.EngineCore.NVec2" => 2,
            "vkEngine.EngineCore.NVec3" => 3,
            "vkEngine.EngineCore.NVec4" => 4,
            "vkEngine.EngineCore.NQuat" => 4,
            _ => 0
        };
        return componentCount != 0;
    }

    private static Type GetMemberType(MemberInfo member)
    {
        return member switch
        {
            FieldInfo field => field.FieldType,
            PropertyInfo property => property.PropertyType,
            _ => throw new InvalidOperationException($"Unsupported member kind '{member.MemberType}'.")
        };
    }

    private static string GetTypeName(Type type) => type.FullName ?? type.Name;

    private static NotSupportedException UnsupportedType(Type type)
    {
        return new NotSupportedException(
            $"C# type '{GetTypeName(type)}' cannot be represented by the EntityScript binary format. " +
            "Supported types are byte, Int32, Int64, Single, Double, String, one-dimensional arrays, EngineCore vectors, and value structs composed from those types.");
    }

    private sealed record MetadataDocument(string AssemblyName, JsonObject[] Types);
    private sealed record ScriptClassMetadata(string FullName, ExportedMemberMetadata[] Members);
    private sealed record ExportedMemberMetadata(
        string Name,
        string ExportName,
        string? CppType,
        [property: JsonIgnore] Type RuntimeType);

    private sealed class GameAssemblyLoadContext : AssemblyLoadContext, IDisposable
    {
        private readonly string assemblyDirectory;

        public GameAssemblyLoadContext(string assemblyDirectory) : base(isCollectible: true)
        {
            this.assemblyDirectory = assemblyDirectory;
        }

        protected override Assembly? Load(AssemblyName assemblyName)
        {
            string candidate = Path.Combine(assemblyDirectory, $"{assemblyName.Name}.dll");
            return File.Exists(candidate) ? LoadFromAssemblyPath(candidate) : null;
        }

        public void Dispose() => Unload();
    }
}
