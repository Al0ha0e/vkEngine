using System.Reflection;
using System.Runtime.Loader;
using System.Text.Json;
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

        var classes = GetLoadableTypes(assembly)
            .Where(type => type.IsClass && !type.IsAbstract && IsEntityScript(type))
            .OrderBy(type => type.FullName, StringComparer.Ordinal)
            .Select(CreateClassMetadata)
            .ToArray();

        var document = new MetadataDocument(assembly.GetName().Name ?? string.Empty, classes);
        string json = JsonSerializer.Serialize(document, jsonOptions);

        string? outputDirectory = Path.GetDirectoryName(outputPath);
        if (!string.IsNullOrEmpty(outputDirectory))
            Directory.CreateDirectory(outputDirectory);

        string temporaryPath = outputPath + ".tmp";
        try
        {
            File.WriteAllText(temporaryPath, json + Environment.NewLine);
            File.Move(temporaryPath, outputPath, overwrite: true);
        }
        finally
        {
            if (File.Exists(temporaryPath))
                File.Delete(temporaryPath);
        }

        Console.WriteLine($"Generated metadata for {classes.Length} EntityScript class(es): {outputPath}");
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
            throw new InvalidOperationException($"Some game types could not be loaded:{Environment.NewLine}{details}", exception);
        }
    }

    private static bool IsEntityScript(Type type)
    {
        for (Type? baseType = type.BaseType; baseType != null; baseType = baseType.BaseType)
        {
            if (baseType.FullName == EntityScriptFullName)
                return true;
        }

        return false;
    }

    private static ScriptClassMetadata CreateClassMetadata(Type type)
    {
        var fields = type
            .GetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.DeclaredOnly)
            .Select(field => (Member: (MemberInfo)field, Attribute: FindExportAttribute(field)))
            .Where(item => item.Attribute != null)
            .Select(item => CreateFieldMetadata((FieldInfo)item.Member, item.Attribute!));

        var properties = type
            .GetProperties(BindingFlags.Instance | BindingFlags.Public | BindingFlags.DeclaredOnly)
            .Where(property => property.GetIndexParameters().Length == 0)
            .Select(property => (Member: property, Attribute: FindExportAttribute(property)))
            .Where(item => item.Attribute != null)
            .Select(item => CreatePropertyMetadata(item.Member, item.Attribute!));

        ExportedMemberMetadata[] members = fields
            .Concat(properties)
            .OrderBy(member => member.Name, StringComparer.Ordinal)
            .ToArray();

        return new ScriptClassMetadata(
            type.Name,
            type.Namespace ?? string.Empty,
            type.FullName ?? type.Name,
            members);
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
        {
            return name;
        }

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

    private static ExportedMemberMetadata CreateFieldMetadata(FieldInfo field, CustomAttributeData attribute)
    {
        return new ExportedMemberMetadata(
            field.Name,
            GetExportName(field, attribute),
            "field",
            GetTypeName(field.FieldType),
            GetCppType(attribute),
            CanRead: true,
            CanWrite: !field.IsInitOnly);
    }

    private static ExportedMemberMetadata CreatePropertyMetadata(PropertyInfo property, CustomAttributeData attribute)
    {
        return new ExportedMemberMetadata(
            property.Name,
            GetExportName(property, attribute),
            "property",
            GetTypeName(property.PropertyType),
            GetCppType(attribute),
            CanRead: property.GetMethod?.IsPublic == true,
            CanWrite: property.SetMethod?.IsPublic == true);
    }

    private static string GetTypeName(Type type)
    {
        return type.FullName ?? type.Name;
    }

    private sealed record MetadataDocument(string AssemblyName, ScriptClassMetadata[] Classes);

    private sealed record ScriptClassMetadata(
        string Name,
        string Namespace,
        string FullName,
        ExportedMemberMetadata[] Members);

    private sealed record ExportedMemberMetadata(
        string Name,
        string ExportName,
        string Kind,
        string Type,
        string? CppType,
        bool CanRead,
        bool CanWrite);

    private sealed class GameAssemblyLoadContext : AssemblyLoadContext, IDisposable
    {
        private readonly string assemblyDirectory;

        public GameAssemblyLoadContext(string assemblyDirectory)
            : base(isCollectible: true)
        {
            this.assemblyDirectory = assemblyDirectory;
        }

        protected override Assembly? Load(AssemblyName assemblyName)
        {
            string candidate = Path.Combine(assemblyDirectory, $"{assemblyName.Name}.dll");
            return File.Exists(candidate) ? LoadFromAssemblyPath(candidate) : null;
        }

        public void Dispose()
        {
            Unload();
        }
    }
}
