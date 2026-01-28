/*
 * Read OCES file then display with mathplot::compoundray::EyeVisual
 */

#include <iostream>
#include <string>
#include <oces/reader>
#define ARGS_NOEXCEPT 1
#include <args/args.hxx> // github.com/Taywee/args

#include <mplot/Visual.h>
#include <mplot/ColourMap.h>
#include <mplot/SphereVisual.h>
#include <mplot/compoundray/EyeVisual.h>

int main (int argc, char** argv)
{
    args::ArgumentParser ap ("OCES viewer", "Have a nice day.");

    args::ValueFlag<std::string> a_fname  (ap, "filepath", "path/to/oces_file.gltf",       {'f'});
    args::ValueFlag<float>       a_psrad  (ap, "radius",   "The projection sphere radius (numeric value)", {'r'});
    args::ValueFlag<std::string> a_centre (ap, "centre",   "The projection sphere centre (comma separated coordinates)", {'c'});
    args::ValueFlag<std::string> a_psrax (ap, "psrax",   "The projection sphere rotation axis (comma separated coordinates)", {'x'});
    args::ValueFlag<std::string> a_2dshift (ap, "twodshift",   "A translation of the 2d right eye (comma separated coordinates, mirrored for left eye)", {'d'});
    args::ValueFlag<float>       a_psr  (ap, "psr",   "The projection sphere rotation radians (numeric value)", {'o'});
    args::ValueFlag<std::string> a_proj (ap, "proj",   "The projection type (equirectangular, mercator or cassini)", {'p'});
    args::Flag a_fov (ap, "fov", "Show field of view with acceptance angle cones", {'v', "fov"});
    args::Flag a_hidehead (ap, "hidehead", "Hide the head, even if it was read from OCES file", {'i', "hidehead"});
    args::Flag a_showsphere (ap, "showsphere", "Show the 2D projection sphere", {'s', "showsphere"});
    args::Flag a_showrays (ap, "showrays", "Show the 2D ommatidia projection rays", {'y', "showrays"});

    ap.ParseCLI (argc, argv);

    std::string filename = "";
    float psrad = 0.1f;
    sm::vec<float> pscentre = { 0, 0, 0 };
    float psr = 0.0f;
    sm::vec<float> psrax = { 0, 1, 0 };
    sm::vec<> twod_shift = { -0.0005, 0.0006, 0 }; // A shift of the twod representation of the
                                                   // right eye (the purple one in the example
                                                   // velox-head filef)

    if (a_fname) {
        filename = args::get (a_fname);
    } else {
        std::cerr << ap;
        return -1;
    }

    if (a_psrad) {
        psrad = args::get (a_psrad);
        std::cerr << "User-supplied projection sphere radius: " << psrad << std::endl;
    }

    if (a_centre) {
        pscentre.set_from (args::get (a_centre));
        std::cerr << "User-supplied projection sphere centre: " << pscentre << std::endl;
    }

    if (a_psrax) {
        psrax.set_from (args::get (a_psrax));
        std::cerr << "User-supplied projection rotation axis: " << psrax << std::endl;
    }

    if (a_2dshift) {
        twod_shift.set_from (args::get (a_2dshift));
        std::cerr << "User-supplied 2D eye shift: " << twod_shift << std::endl;
    }

    if (a_psr) {
        psr = args::get (a_psr);
        std::cerr << "User-supplied projection rotation radians: " << psr << std::endl;
    }

    std::string projstr = "equirectangular";
    if (a_proj) {
        projstr = args::get (a_proj);
        std::cerr << "User-supplied projection type: " << projstr << std::endl;
    }

    // Read
    oces::reader oces_reader (filename);
    if (oces_reader.read_success == false) { return -1; }

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

    mplot::meshgroup* head_mesh_ptr = nullptr;
    if (!a_hidehead) { head_mesh_ptr = reinterpret_cast<mplot::meshgroup*>(&oces_reader.head_mesh); }

    oces_reader.head_mesh.single_colour = {0.345f, 0.122f, 0.082f};
    auto eyevm = std::make_unique<mplot::compoundray::EyeVisual<>> (sm::vec<>{}, &ommatidiaColours, ommatidia.get(), head_mesh_ptr);
    v.bindmodel (eyevm);
    eyevm->name = "CompoundRay Eye";
    eyevm->show_cones = true;

    [[maybe_unused]] auto ptype = mplot::compoundray::EyeVisual<>::projection_type::equirectangular; // mercator, equirectangular or cassini
    if (projstr.find ("merc") != std::string::npos) {
        ptype = mplot::compoundray::EyeVisual<>::projection_type::mercator;
    } else if (projstr.find ("cass") != std::string::npos) {
        ptype = mplot::compoundray::EyeVisual<>::projection_type::cassini;
    }

    sm::mat<float, 4> twod_tr;
    twod_tr.translate (twod_shift);

    // To avoid 2D, don't add spherical projections
    std::cout << "Rotation about axis " << psrax << " by amount " << psr << " radians\n";
    sm::quaternion<float> psrotn (psrax, psr);
    eyevm->add_spherical_projection (ptype, twod_tr, pscentre, psrad, psrotn, 0, oces_reader.position.size() / 2);
    if (oces_reader.mirrors.empty() == false) {
        pscentre = (oces_reader.mirrors[0] * pscentre).less_one_dim();
        std::cout << "New centre: " << pscentre << std::endl;

        sm::vec<> twod_shift_left = twod_shift;
        twod_shift_left[0] *= -1.0f;
        twod_tr.set_identity();
        twod_tr.translate (twod_shift_left);
        eyevm->add_spherical_projection (ptype, twod_tr, pscentre, psrad, psrotn.invert(),
                                         oces_reader.position.size() / 2, oces_reader.position.size());
    }

    eyevm->show_sphere = (a_showsphere ? true : false);
    eyevm->show_rays = (a_showrays ? true : false);
    eyevm->show_cones = false;
    eyevm->show_fov = (a_fov ? true : false);
    eyevm->pre_set_cone_length (0.005f);
    eyevm->finalize();

    [[maybe_unused]] auto ep = v.addVisualModel (eyevm);

    ep->scaleViewMatrix (1000.0f);

    v.keepOpen();

    return 0;
}
