/*
 * To begin with, read OCES file and output data to stdout, but why not use
 * mathplot::compoundray::EyeVisual to display?
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
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " path/to/oces_file.gltf\n";
        return -1;
    }
    std::string filename (argv[1]);

    // Read
    oces::reader oces_reader (filename);

    // Now view
    [[maybe_unused]] float psrad = 0.5f;
    if (argc > 2) { psrad = std::atof (argv[2]); }

    auto v = mplot::Visual<>(1024, 768, "mplot::compoundray::EyeVisual");

    // We read the information from the eye file into a vector of Ommatidium objects.  Ommatidium is
    // defined in "cameras/CompoundEyeDataTypes.h" in compound ray, mplot::Ommatidium is a
    // mplot/Seb's maths style equivalent. It contains 2 3D float vectors and two scalar floating point
    // values.
    auto ommatidia = std::make_unique<std::vector<mplot::compoundray::Ommatidium>>();
    std::vector<std::array<float, 3>> ommatidiaColours;

    // Copy data into the ommatidia data structure
    ommatidia->resize (oces_reader.position.size());
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

    auto eyevm = std::make_unique<mplot::compoundray::EyeVisual<>> (sm::vec<>{}, &ommatidiaColours, ommatidia.get());
    v.bindmodel (eyevm);
    eyevm->name = "CompoundRay Eye";
    eyevm->show_cones = true;

    [[maybe_unused]] auto ptype = mplot::compoundray::EyeVisual<>::projection_type::equirectangular; // mercator, equirectangular or cassini
    [[maybe_unused]] sm::vec<> centre = { 0, 0, 0 };
    [[maybe_unused]] sm::mat44<float> twod_tr;
    twod_tr.translate (sm::vec<>{0,0,-0.1});

    // To avoid 2D, don't add spherical projections
    //eyevm->add_spherical_projection (ptype, twod_tr, centre, psrad);

    eyevm->pre_set_cone_length (4e-6f);
    //eyevm->pre_set_disc_width (0.6e-5f);
    eyevm->show_sphere = true;
    eyevm->show_rays = false;
    eyevm->finalize();

    [[maybe_unused]] auto ep = v.addVisualModel (eyevm);

    ep->scaleViewMatrix (1000.0f);

    v.keepOpen();

    return 0;
}
