/*
 * To begin with, read OCES file and output data to stdout, but why not use
 * mathplot::compoundray::EyeVisual to display?
 */

#include <iostream>
#include <cstdint>
#include <vector>
#include <unordered_map>

#include <sm/vvec>
#include <sm/vec>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#if defined( WIN32 )
#pragma warning( push )
#pragma warning( disable : 4267 )
#endif
#include <tinygltf/tiny_gltf.h>
#if defined( WIN32 )
#pragma warning( pop )
#endif

void get_buffer (const tinygltf::Model& model, const int32_t accessor_idx,
                 sm::vvec<sm::vec<float, 3>>& vvec_of_vec)
{
    if (accessor_idx == -1) { return; }

    const tinygltf::Accessor& gltf_accessor      = model.accessors[accessor_idx];
    const tinygltf::BufferView& gltf_buffer_view = model.bufferViews[gltf_accessor.bufferView];
    const int32_t elmt_cmpt_byte_size = tinygltf::GetComponentSizeInBytes(gltf_accessor.componentType);
    const int32_t cmpts_in_type  = tinygltf::GetNumComponentsInType(gltf_accessor.type);

    if (cmpts_in_type == 3 && elmt_cmpt_byte_size == 4) {
        vvec_of_vec.resize (gltf_accessor.count);
        // copy data from model.buffers[gltf_buffer_view.buffer].data (vector<unsigned char>) to vvec_of_vec.
        std::cout << "Memcpy " << gltf_accessor.count * elmt_cmpt_byte_size * cmpts_in_type << " bytes\n";
        std::memcpy (vvec_of_vec.data(), model.buffers[gltf_buffer_view.buffer].data.data(),
                     gltf_accessor.count * elmt_cmpt_byte_size * cmpts_in_type);
    }
}

void get_buffer (const tinygltf::Model& model, const int32_t accessor_idx,
                 sm::vvec<float>& vvec_of_float)
{
    if (accessor_idx == -1) { return; }

    const tinygltf::Accessor& gltf_accessor      = model.accessors[accessor_idx];
    const tinygltf::BufferView& gltf_buffer_view = model.bufferViews[gltf_accessor.bufferView];
    const int32_t elmt_cmpt_byte_size = tinygltf::GetComponentSizeInBytes(gltf_accessor.componentType);
    const int32_t cmpts_in_type  = tinygltf::GetNumComponentsInType(gltf_accessor.type);

    if (cmpts_in_type == 1 && elmt_cmpt_byte_size == 4) {
        vvec_of_float.resize (gltf_accessor.count);
        std::cout << "Memcpy " << gltf_accessor.count * elmt_cmpt_byte_size * cmpts_in_type << " bytes\n";
        std::memcpy (vvec_of_float.data(), model.buffers[gltf_buffer_view.buffer].data.data(),
                     gltf_accessor.count * elmt_cmpt_byte_size * cmpts_in_type);
    }
}

int main (int argc, char** argv)
{
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " path/to/oces_file.gltf\n";
        return -1;
    }
    std::string filename (argv[1]);

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err = "";
    std::string warn = "";

    bool r_loader = loader.LoadASCIIFromFile (&model, &err, &warn, filename);

    if (!warn.empty()) { std::cerr << "glTF WARNING: " << warn << std::endl; }
    if (r_loader == false) {
        std::cerr << "Failed to load GLTF file '" << filename << "': " << err << std::endl;
        return -1;
    }

    // We want to access extensions for the OCES stuff
    loader.SetStoreOriginalJSONForExtrasAndExtensions (true);

    // Calculate and store the path to the file bar the file iteself for relative includes
    std::string glTFdir = "";
    std::size_t slashPos = filename.find_last_of ("/\\") + 1; // (+1 to include the slash)
    if (slashPos != std::string::npos) { glTFdir = filename.substr (0, slashPos); }

    bool oces_eyes_used = false;
    for (const auto& eu : model.extensionsUsed) {
        if (eu == "OCES_eyes") { oces_eyes_used = true; }
    }
    if (!oces_eyes_used) {
        std::cout << "Warning: Did not find \"OCES_eyes\" in \"extensions\" section of glTF. Carrying on anyway...\n";
    }

    std::vector<std::unordered_map<std::string, int>> eyes_OmmatidialProperties;
    std::vector<int> ommatidialAccessors;

    // model.extensions is a map<string, Value>
    for (const auto& e : model.extensions) {

        // e.first is string, e.second is a tinygltf Value
        if (e.first != "OCES_eyes") { continue; } // We only have eyes for one extension
        auto ev = e.second;

        if (!ev.IsObject()) { continue; }
        if (!ev.Has ("ommatidialProperties")) { continue; }
        if (!ev.Has ("eyes")) { continue; }

        auto ommatidialProperties = ev.Get ("ommatidialProperties");
        if (!ommatidialProperties.IsArray()) {
            std::cout << "This ommatidialProperties is not an array; try next extension\n";
            continue;
        }

        // Load ommatidialProperties
        ommatidialAccessors.resize (ommatidialProperties.Size(), 0);
        for (size_t i = 0; i < ommatidialProperties.Size(); ++i) {
            auto ommprop = ommatidialProperties.Get(i);

            if (ommprop.IsObject()) {
                if (ommprop.Has("type")) {
                    auto ot = ommprop.Get("type");
                    if (ot.IsString() && ot.Get<std::string>() == "ACCESSOR") {
                        if (ommprop.Has("value")) {
                            auto ov = ommprop.Get("value");
                            if (ov.IsInt()) {
                                // ommatidialProperty[i] is ACCESSOR with value ov.Get<int>()
                                ommatidialAccessors[i] = ov.Get<int>();
                            }
                        }
                    }
                }
            }
        }

        // Load eyes
        auto eyes = ev.Get ("eyes");
        if (eyes.IsArray()) {

            eyes_OmmatidialProperties.resize (eyes.Size());

            for (size_t i = 0; i < eyes.Size(); ++i) {
                // Process eye
                if (eyes.Get(i).Get("type").Get<std::string>() != "POINT_OMMATIDIAL") {
                    std::cout << "Don't know how to process an OCES eye of type '"
                              << eyes.Get(i).Get("type").Get<std::string>() << "'\n";
                    continue;
                }

                auto op = eyes.Get(i).Get("ommatidialProperties");

                if (!op.IsObject()) {
                    std::cout << "Badly formed OCES glTF (OCES_eyes.ommatidialProperties is not a JSON object)\n";
                    continue;
                }
                if (!(op.Has("POSITION") && op.Has("ORIENTATION") && op.Has("FOCAL_OFFSET") && op.Has("DIAMETER"))) {
                    std::cout << "Badly formed OCES glTF (OCES_eyes.ommatidialProperties is not a JSON object)\n";
                    continue;
                }

                std::cout << "Processing eye " << eyes.Get(i).Get("name").Get<std::string>()
                          << " of type " << eyes.Get(i).Get("type").Get<std::string>() << std::endl;

                // Good to go
                eyes_OmmatidialProperties[i]["POSITION"] = op.Get("POSITION").Get<int>();
                eyes_OmmatidialProperties[i]["ORIENTATION"] = op.Get("ORIENTATION").Get<int>();
                eyes_OmmatidialProperties[i]["FOCAL_OFFSET"] = op.Get("FOCAL_OFFSET").Get<int>();
                eyes_OmmatidialProperties[i]["DIAMETER"] = op.Get("DIAMETER").Get<int>();
            }
        }

        // Could now read the buffers
        sm::vvec<sm::vec<float, 3>> position = {};
        sm::vvec<sm::vec<float, 3>> orientation = {};
        sm::vvec<float> focal_offset = {};
        sm::vvec<float> diameter = {};

        for (auto eye : eyes_OmmatidialProperties) {
            int sz = static_cast<int>(ommatidialAccessors.size());
            if (eye["POSITION"] < sz) {
                get_buffer (model, ommatidialAccessors[eye["POSITION"]], position);
            }
            if (eye["ORIENTATION"] < sz) {
                get_buffer (model, ommatidialAccessors[eye["ORIENTATION"]], orientation);
            }
            if (eye["FOCAL_OFFSET"] < sz) {
                get_buffer (model, ommatidialAccessors[eye["FOCAL_OFFSET"]], focal_offset);
            }
            if (eye["DIAMETER"] < sz) {
                get_buffer (model, ommatidialAccessors[eye["DIAMETER"]], diameter);
            }
        }

        if (position.size() == orientation.size()
            && position.size() == focal_offset.size()
            && position.size() == diameter.size()) {
            std::cout << "Success, can output csv!\n";
        }
    }
    return 0;
}
