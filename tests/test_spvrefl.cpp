#include <spirv_reflect.h>
#include <iostream>
#include <fstream>
#include <assert.h>

static inline void readFile(const std::string &filename, std::vector<char> &buffer)
{
    std::cout << filename << "\n";
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    buffer.resize(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
}

void PrintModuleInfo(std::ostream &os, const SpvReflectShaderModule &obj, const char *indent = "");
void PrintDescriptorSet(std::ostream &os, const SpvReflectDescriptorSet &obj, const char *indent = "");
void PrintDescriptorBinding(std::ostream &os, const SpvReflectDescriptorBinding &obj, bool write_set, const char *indent = "");

void PrintModuleInfo(std::ostream &os, const SpvReflectShaderModule &obj, const char *indent)
{
    os << "entry point     : " << obj.entry_point_name << "\n";
    os << "source lang     : " << spvReflectSourceLanguage(obj.source_language) << "\n";
    os << "source lang ver : " << obj.source_language_version << "\n";
    if (obj.source_language == SpvSourceLanguageGLSL)
    {
        os << "stage           : ";
        switch (obj.shader_stage)
        {
        default:
            break;
        case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
            os << "VS";
            break;
        case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
            os << "HS";
            break;
        case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
            os << "DS";
            break;
        case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT:
            os << "GS";
            break;
        case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
            os << "PS";
            break;
        case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:
            os << "CS";
            break;
        }
    }

    for (int i = 0; i < obj.input_variable_count; i++)
    {
        SpvReflectInterfaceVariable &input = *obj.input_variables[i];
        os << "\n------------------"
           << input.location << " " << input.name << " " << input.component << " "
           << input.member_count << " " << input.word_offset.location << "\n";
    }
}

void PrintDescriptorSet(std::ostream &os, const SpvReflectDescriptorSet &obj, const char *indent)
{
    const char *t = indent;
    std::string tt = std::string(indent) + "  ";
    std::string ttttt = std::string(indent) + "    ";

    os << t << "set           : " << obj.set << "\n";
    os << t << "binding count : " << obj.binding_count;
    os << "\n";
    for (uint32_t i = 0; i < obj.binding_count; ++i)
    {
        const SpvReflectDescriptorBinding &binding = *obj.bindings[i];
        os << tt << i << ":"
           << "\n";
        PrintDescriptorBinding(os, binding, false, ttttt.c_str());
        if (i < (obj.binding_count - 1))
        {
            os << "\n";
        }
    }
}

std::string ToStringDescriptorType(SpvReflectDescriptorType value)
{
    switch (value)
    {
    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
        return "VK_DESCRIPTOR_TYPE_SAMPLER";
    case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        return "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE";
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        return "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE";
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        return "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER";
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        return "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER";
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER";
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC";
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC";
    case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        return "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT";
    case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
        return "VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR";
    }
    // unhandled SpvReflectDescriptorType enum value
    return "VK_DESCRIPTOR_TYPE_???";
}

void PrintDescriptorBinding(std::ostream &os, const SpvReflectDescriptorBinding &obj, bool write_set, const char *indent)
{
    const char *t = indent;
    os << t << "binding : " << obj.binding << "\n";
    if (write_set)
    {
        os << t << "set     : " << obj.set << "\n";
    }
    os << t << "type    : " << ToStringDescriptorType(obj.descriptor_type) << "\n";

    // array
    if (obj.array.dims_count > 0)
    {
        os << t << "array   : ";
        for (uint32_t dim_index = 0; dim_index < obj.array.dims_count; ++dim_index)
        {
            os << "[" << obj.array.dims[dim_index] << "]";
        }
        os << "\n";
    }

    // counter
    if (obj.uav_counter_binding != nullptr)
    {
        os << t << "counter : ";
        os << "(";
        os << "set=" << obj.uav_counter_binding->set << ", ";
        os << "binding=" << obj.uav_counter_binding->binding << ", ";
        os << "name=" << obj.uav_counter_binding->name;
        os << ");";
        os << "\n";
    }

    os << t << "name    : " << obj.name;
    if ((obj.type_description->type_name != nullptr) && (strlen(obj.type_description->type_name) > 0))
    {
        os << " "
           << "(" << obj.type_description->type_name << ")";
    }
}

int main()
{
    std::vector<char> buffer;
    readFile("./builtin_assets/shader/default_vert.spv", buffer);
    std::cout << "READ OK " << buffer.size() << "\n";
    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule(buffer.size(), buffer.data(), &module);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    uint32_t count = 0;
    result = spvReflectEnumerateDescriptorSets(&module, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    std::vector<SpvReflectDescriptorSet *> sets(count);
    result = spvReflectEnumerateDescriptorSets(&module, &count, sets.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    // Log the descriptor set contents to stdout
    const char *t = "  ";
    const char *tt = "    ";

    PrintModuleInfo(std::cout, module);
    std::cout << "\n\n";

    std::cout << "Descriptor sets:"
              << "\n";
    for (size_t index = 0; index < sets.size(); ++index)
    {
        auto p_set = sets[index];

        // descriptor sets can also be retrieved directly from the module, by set
        // index
        auto p_set2 = spvReflectGetDescriptorSet(&module, p_set->set, &result);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);
        assert(p_set == p_set2);
        (void)p_set2;

        std::cout << t << index << ":"
                  << "\n";
        PrintDescriptorSet(std::cout, *p_set, tt);
        std::cout << "\n\n";
    }

    spvReflectDestroyShaderModule(&module);

    return 0;
}