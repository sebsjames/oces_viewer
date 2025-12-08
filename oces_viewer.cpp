/*
 * Read OCES file then display with mathplot::compoundray::EyeVisual
 */

#include <iostream>
#include <string>
#include <oces/reader>

#include <mplot/Visual.h>
#include <mplot/ColourMap.h>
#include <mplot/SphereVisual.h>
#include <mplot/VectorVisual.h>
#include <mplot/compoundray/EyeVisual.h>

int main (int argc, char** argv)
{
    // Get user-provided info for the glTF file and any 2D projection sphere
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " path/to/oces_file.gltf [psradius] pscentre_x pscentre_y pscentre_z \n";
        return -1;
    }
    std::string filename (argv[1]);

    float psrad = 0.1f;
    sm::vec<double> pscentre_d = { 0, 0, 0 };
    //pscentre = { -0.0004, -0.0002, 0.0001 };

    if (argc > 2) {
        psrad = std::atof (argv[2]);
        std::cout << "User-supplied projection sphere radius: " << psrad << std::endl;
    }
    if (argc > 5) {
        pscentre_d = { std::atof(argv[3]), std::atof(argv[4]), std::atof(argv[5]) };
        std::cout << "User-supplied projection sphere centre: " << pscentre_d << std::endl;
    }
    sm::vec<float> pscentre = pscentre_d.as<float>();

    // Read
    oces::reader oces_reader (filename);

    // Now view
    auto v = mplot::Visual<>(1024, 768, "mplot::compoundray::EyeVisual");
    v.lightingEffects (true);

    // We read the information from the eye file into a vector of Ommatidium objects.  Ommatidium is
    // defined in "cameras/CompoundEyeDataTypes.h" in compound ray, mplot::Ommatidium is a
    // mplot/Seb's maths style equivalent. It contains 2 3D float vectors and two scalar floating point
    // values.
    auto ommatidia = std::make_unique<std::vector<mplot::compoundray::Ommatidium>>();
    std::vector<std::array<float, 3>> ommatidiaColours;

    // Copy data into the ommatidia data structure
    ommatidia->resize (oces_reader.position.size());
    std::cerr << "Copying " << oces_reader.position.size() << " ommatidia\n";
    for (size_t i = 0; i < oces_reader.position.size(); ++i) {
        (*ommatidia)[i].relativePosition = oces_reader.position[i];
        (*ommatidia)[i].relativeDirection = oces_reader.orientation[i];
        (*ommatidia)[i].focalPointOffset = oces_reader.focal_offset[i];
        (*ommatidia)[i].acceptanceAngleRadians = oces_reader.acceptance_angle[i];
    }

    // Make some dummy data to demo the eye
    sm::vvec<float> ommatidiaData;
    ommatidiaData.linspace (0, 1, ommatidia->size());
    // Colour it with a colour map
    mplot::ColourMap cm (mplot::ColourMapType::Plasma);
    ommatidiaColours.resize (ommatidia->size());
    for (size_t i = 0; i < ommatidia->size(); ++i) {
        ommatidiaColours[i] = cm.convert (ommatidiaData[i]);
    }

    oces_reader.head_mesh.single_colour = {0.345f, 0.122f, 0.082f};
    auto eyevm = std::make_unique<mplot::compoundray::EyeVisual<>> (sm::vec<>{}, &ommatidiaColours, ommatidia.get(),
                                                                    &oces_reader.head_mesh);
    v.bindmodel (eyevm);
    eyevm->name = "CompoundRay Eye";
    eyevm->show_cones = true;

    [[maybe_unused]] auto ptype = mplot::compoundray::EyeVisual<>::projection_type::equirectangular; // mercator, equirectangular or cassini
    [[maybe_unused]] sm::mat44<float> twod_tr;
    twod_tr.scale (sm::vec<>{4, 1, 1});
#if 0
    auto prange = sm::range<sm::vec<float, 3>>::search_initialized();
    for (auto p : oces_reader.position) { prange.update (p); }
    auto pspan = prange.span();
    twod_tr.translate (pspan / 4);
#endif
    // To avoid 2D, don't add spherical projections
    eyevm->add_spherical_projection (ptype, twod_tr, pscentre, psrad,
                                     0, oces_reader.position.size() / 2);
    if (oces_reader.mirrors.empty() == false) {
        pscentre = (oces_reader.mirrors[0] * pscentre).less_one_dim();
        std::cout << "New centre: " << pscentre << std::endl;
        eyevm->add_spherical_projection (ptype, twod_tr, pscentre, psrad,
                                         oces_reader.position.size() / 2, oces_reader.position.size());
    }

    eyevm->show_sphere = false;
    eyevm->show_rays = true;
    eyevm->show_cones = false;
    eyevm->show_fov = false;
    eyevm->pre_set_cone_length (0.005f);
    eyevm->finalize();

    [[maybe_unused]] auto ep = v.addVisualModel (eyevm);

    ep->scaleViewMatrix (1000.0f);

    v.keepOpen();

    return 0;
}
