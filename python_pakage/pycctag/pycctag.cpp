#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/functional.h>

#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <vector>

#include <CCTag.hpp>
#include <ICCTag.hpp>

// ++++++++++++++ ADAPTATION CODE FOR pycctag +++++++++++++++++++++++++++++++++++++++++++++

struct marker_st
{
    float x, y;
    int status, id;

    float getx() { return x; }
    float gety() { return y; }

    int getStatus() { return status; }
    int getID() { return id; }

};


auto detect_from_img(const cv::Mat graySrc)
{
    int err = 0;
    return err;
}

auto detect_from_file(const std::string image_filename)
{
    
    std::vector<marker_st> marker_list;

    // load the image e.g. from file
    cv::Mat src = cv::imread(image_filename);
    cv::Mat graySrc;
    cv::cvtColor(src, graySrc, cv::COLOR_BGR2GRAY);

    // set up the parameters
    const std::size_t nCrowns{ 3 };
    cctag::Parameters params(nCrowns);

    // choose a cuda pipe
    const int pipeId{ 0 };

    // an arbitrary id for the frame
    const int frameId{ 0 };

    // process the image
    boost::ptr_list<cctag::ICCTag> markers{};
    
    cctagDetection(markers, pipeId, frameId, graySrc, params);
    
    for (const auto& marker : markers)
    {
        marker_st tmp_st;
        
        tmp_st.status = marker.getStatus();
        tmp_st.x = marker.x();
        tmp_st.y = marker.y();
        tmp_st.id = marker.id();

        marker_list.push_back(tmp_st);
    }

    return marker_list;
}

// ++++++++++++++ START BINDING CODE pycctag +++++++++++++++++++++++++++++++++++++++++++++
PYBIND11_MAKE_OPAQUE(std::vector<marker_st>);

PYBIND11_MODULE(pycctag, m)
{
    m.doc() = "CCTag python wrapper for core functions";

    pybind11::bind_vector<std::vector<marker_st>>(m, "MarkerVector");
    pybind11::class_<marker_st>(m, "Marker")
        .def(pybind11::init())
        .def_property_readonly("x", &marker_st::getx)
        .def_property_readonly("y", &marker_st::gety)
        .def_property_readonly("status", &marker_st::getStatus)
        .def_property_readonly("id", &marker_st::getID);

    m.def("detect_from_file", &detect_from_file, "Function to detetct markers from image file");
    m.def("detect_from_img", &detect_from_img, "Function to detetct markers from image matrix");

}