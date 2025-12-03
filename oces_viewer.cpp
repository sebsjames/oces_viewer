/*
 * To begin with, read OCES file and output data to stdout, but why not use
 * mathplot::compoundray::EyeVisual to display?
 */

#include <iostream>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <stdexcept>

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

namespace oces
{
    struct reader
    {
        std::string filename;
        std::string base_dir = "";

        sm::vvec<sm::vec<float, 3>> position = {};
        sm::vvec<sm::vec<float, 3>> orientation = {};
        sm::vvec<float> focal_offset = {};
        sm::vvec<float> diameter = {}; // Optical diameter

        reader (const std::string& _filename)
        {
            this->filename = _filename;
            this->init();
        }

        void init()
        {
            tinygltf::Model model;
            tinygltf::TinyGLTF loader;
            std::string err = "";
            std::string warn = "";

            bool r_loader = loader.LoadASCIIFromFile (&model, &err, &warn, this->filename);
            if (!warn.empty()) { std::cerr << "glTF WARNING: " << warn << std::endl; }
            if (r_loader == false) {
                std::stringstream ee;
                ee << "Failed to load GLTF file '" << filename << "': " << err;
                throw std::runtime_error (ee.str());
            }

            // We want to access extensions for the OCES stuff
            loader.SetStoreOriginalJSONForExtrasAndExtensions (true);

            // Calculate and store the path to the file bar the file iteself for relative includes
            this->base_dir = "";
            std::size_t slashPos = filename.find_last_of ("/\\") + 1; // (+1 to include the slash)
            if (slashPos != std::string::npos) { this->base_dir = filename.substr (0, slashPos); }

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

                        std::cerr << "Processing eye " << eyes.Get(i).Get("name").Get<std::string>()
                                  << " of type " << eyes.Get(i).Get("type").Get<std::string>() << std::endl;

                        // Good to go
                        eyes_OmmatidialProperties[i]["POSITION"] = op.Get("POSITION").Get<int>();
                        eyes_OmmatidialProperties[i]["ORIENTATION"] = op.Get("ORIENTATION").Get<int>();
                        eyes_OmmatidialProperties[i]["FOCAL_OFFSET"] = op.Get("FOCAL_OFFSET").Get<int>();
                        eyes_OmmatidialProperties[i]["DIAMETER"] = op.Get("DIAMETER").Get<int>();
                    }
                }

                // Can now read the buffers into our member attributes position, orientation, etc
                for (auto eye : eyes_OmmatidialProperties) {
                    int sz = static_cast<int>(ommatidialAccessors.size());
                    if (eye["POSITION"] < sz) { this->get_buffer (model, ommatidialAccessors[eye["POSITION"]], this->position); }
                    if (eye["ORIENTATION"] < sz) { this->get_buffer (model, ommatidialAccessors[eye["ORIENTATION"]], this->orientation); }
                    if (eye["FOCAL_OFFSET"] < sz) { this->get_buffer (model, ommatidialAccessors[eye["FOCAL_OFFSET"]], this->focal_offset); }
                    if (eye["DIAMETER"] < sz) { this->get_buffer (model, ommatidialAccessors[eye["DIAMETER"]], this->diameter); }
                }
            }
        }

        /**
         * Copy data from the buffer identified by accessor_idx into the output vvec.
         *
         * \tparam T The type of the data elements in output. May be a scalar such as float or double, or a
         * fixed size data type such as sm::vec<float, 3> or float3
         *
         * \param model The initialized (by a tinygltf::TinyGLTF loader) tinygltf model reference.
         *
         * \param accessor_idx An integer index for the accessor to the glTF buffer
         *
         * \param output The output vvec. This will be resized, then filled with a *copy* of the data held
         * in the TinyGLTF model.
         */
        template<typename T>
        void get_buffer (const tinygltf::Model& model, const int32_t accessor_idx, sm::vvec<T>& output)
        {
            if (accessor_idx == -1) { return; }

            const tinygltf::Accessor& gltf_accessor      = model.accessors[accessor_idx];
            const tinygltf::BufferView& gltf_buffer_view = model.bufferViews[gltf_accessor.bufferView];
            const int32_t elmt_cmpt_byte_size = tinygltf::GetComponentSizeInBytes(gltf_accessor.componentType);
            const int32_t cmpts_in_type  = tinygltf::GetNumComponentsInType(gltf_accessor.type);


            if (elmt_cmpt_byte_size == (static_cast<int32_t>(sizeof(T)) / cmpts_in_type)) {
                output.resize (gltf_accessor.count);
                // copy data from model.buffers[gltf_buffer_view.buffer].data (vector<unsigned char>) to vvec_of_vec.
                std::cerr << "Memcpy " << gltf_accessor.count * elmt_cmpt_byte_size * cmpts_in_type
                          << " bytes from accessor index " << accessor_idx << ", buffer view byte offset is " << gltf_buffer_view.byteOffset << "\n";
                std::memcpy (output.data(),
                             model.buffers[gltf_buffer_view.buffer].data.data() + gltf_buffer_view.byteOffset,
                             gltf_accessor.count * elmt_cmpt_byte_size * cmpts_in_type);
            } else {
                std::cerr << "Failed to memcpy in get_buffer!\n cmpts_in_type = " << cmpts_in_type
                          << "\n elmt_cmpt_byte_size = " <<  elmt_cmpt_byte_size
                          << "\n sizeof(T) = " << static_cast<int32_t>(sizeof(T))
                          << "\n sizeof(T)/cmpts_in_type = " << (static_cast<int32_t>(sizeof(T)) / cmpts_in_type) << std::endl;
            }
        }

        void output_compound_ray_csv()
        {
            // Compound-ray eye files use acceptance angle, rather than optical lens diameter
            sm::vvec<float> acceptance_angle (this->diameter.size(), 0.0f);
            for (size_t i = 0; i < this->diameter.size(); i++) {
                acceptance_angle[i] = 2.0f * std::atan2 (this->diameter[i] / 2.0f, std::abs (this->focal_offset[i]));
            }

            if (this->position.size() == this->orientation.size()
                && this->position.size() == this->focal_offset.size()
                && this->position.size() == acceptance_angle.size()) {
                for (size_t i = 0; i < this->position.size(); ++i) {
                    std::cout << this->position[i].str_comma_separated (' ') << " "
                              << this->orientation[i].str_comma_separated (' ')
                              << " " << acceptance_angle[i]
                              << " " << this->focal_offset[i]
                              << std::endl;
                    // (*) For compound-ray we have to convert diameter into an acceptance angle
                }
            } else {
                std::cerr << "position, orientation, focal_offset and diameter/acceptance_angle should all have the same number of elements.\n";
            }
        }
    };

} // namespace oces

int main (int argc, char** argv)
{
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " path/to/oces_file.gltf\n";
        return -1;
    }
    std::string filename (argv[1]);

    oces::reader oces_reader (filename);
    oces_reader.output_compound_ray_csv();

    return 0;
}
