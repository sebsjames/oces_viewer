/*
 * To begin with, read OCES file and output data to stdout, but why not use
 * mathplot::compoundray::EyeVisual to display?
 */

#include <iostream>
#include <cstdint>
#include <vector>
#include <unordered_map>

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

int main (int argc, char** argv)
{
    constexpr bool debug_gltf = true;

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

    // Show buffers (will need these as they will contain the eye data)
    for (const auto& gltf_buffer : model.buffers) { // model.buffers is: std::vector<Buffer> buffers;

        const uint64_t buf_size = gltf_buffer.data.size();
        if constexpr (debug_gltf == true) {
            std::cerr << "Processing glTF buffer '" << gltf_buffer.name << "'\n"
                      << "\tbyte size: " << buf_size << "\n"
                      << "\turi      : " << (buf_size > 128u ? gltf_buffer.uri.substr(0, 128) + std::string("...") : gltf_buffer.uri) << std::endl;
        }
#if 0
        for (auto bel : gltf_buffer.data) {
            std::cout << static_cast<uint32_t>(bel) << ",";
        }
        std::cout << std::endl;
#endif
    }

    for (const auto& eu : model.extensionsUsed) {
        std::cout << "Extension used: " << eu << std::endl;
    }

    std::vector<std::unordered_map<std::string, int>> eyes_OmmatidialProperties;
    std::vector<int> ommatidialAccessors;

    // model.extensions is a map<string, Value>
    for (const auto& e : model.extensions) {

        // e.first is string, e.second is a tinygltf Value
        if (e.first != "OCES_eyes") { continue; } // We only have eyes for one extension
        auto ev = e.second;

        if (!ev.IsObject()) { continue; }

        // Load ommatidialProperties
        if (ev.Has ("ommatidialProperties")) {
            auto ommatidialProperties = ev.Get ("ommatidialProperties");
            if (ommatidialProperties.IsArray()) {
                std::cout << "ommatidialProperties is an array of size " << ommatidialProperties.Size() << "\n";
            }
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
                                    std::cout << "ommatidialProperty["<<i<<"] is ACCESSOR with value " << ov.Get<int>() << std::endl;
                                    ommatidialAccessors[i] = ov.Get<int>();
                                }
                            }
                        }
                    }
                }
            }
        }

        // Load eyes
        if (ev.Has ("eyes")) {
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
        }

        // Could now read the buffers
        for (auto eye : eyes_OmmatidialProperties) {
            int sz = static_cast<int>(ommatidialAccessors.size());
            if (eye["POSITION"] < sz) {
                std::cout << "POSITION accessor index: " << eye["POSITION"] << " which is accessor "
                          << ommatidialAccessors[eye["POSITION"]]
                          << std::endl;
            }
            if (eye["ORIENTATION"] < sz) {
                std::cout << "ORIENTATION accessor index: " << eye["ORIENTATION"] << " which is accessor "
                          << ommatidialAccessors[eye["ORIENTATION"]]
                          << std::endl;
            }
            if (eye["FOCAL_OFFSET"] < sz) {
                std::cout << "FOCAL_OFFSET accessor index: " << eye["FOCAL_OFFSET"] << " which is accessor "
                          << ommatidialAccessors[eye["FOCAL_OFFSET"]]
                          << std::endl;
            }
            if (eye["DIAMETER"] < sz) {
                std::cout << "DIAMETER accessor index: " << eye["DIAMETER"] << " which is accessor "
                          << ommatidialAccessors[eye["DIAMETER"]]
                          << std::endl;
            }
        }
    }
    return 0;
}
