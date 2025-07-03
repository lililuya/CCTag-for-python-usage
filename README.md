# 编译CCTag作为Python包使用
> 摘自原始仓库的issue，https://github.com/alicevision/CCTag/issues/136
## 测试环境
- ubuntu22.04 LTS
- python-3.10
- ninja-1.11.1.3
- Cython-3.0.11
- gcc (Ubuntu 11.4.0-9ubuntu1) 11.4.0
## 1.目录结构
```txt
|--CCTag-for-python-usage
    |--pycctag
        |--build (编译产生)
        |--thirdparty
            |--pybind11(https://github.com/pybind/pybind11/archive/refs/tags/v2.13.6.zip)
        |--pycctag.cpp (封装cpp接口)
        |--creat_GLIBLink.sh(将环境的libstdc++6.so链向conda环境内)
        |--CMakeLists.txt
```

## 2.编译CCTag-for-python-usage
- 下载[CCTag](https://github.com/alicevision/CCTag)
    - `git clone https://github.com/alicevision/CCTag.git`
- 安装必须的库
    - `sudo apt install libeigen3-dev`
    - `sudo apt install libboost-all-dev`
    - `sudo apt install libopencv-dev`
    - `sudo apt install libtbb-dev`
- 在项目顶层`cmakelists.txt`中设置路径防止找不到
```bash
# 查看配置文件的位置
find /usr -name "Eigen3Config.cmake" 2>/dev/null
find /usr -name "TBBConfig.cmake" 2>/dev/null

set(CMAKE_PREFIX_PATH "/usr/share/eigen3/cmake/")
set(CMAKE_INSTALL_PREFIX "/usr/local/lib/cmake/opencv4/")
```
- cmake编译(此处为非cuda,适配无GPU场景)
    - 构建cmake configuration
        - `sudo make build && cd build`
        - `cmake .. -DCCTAG_WITH_CUDA:BOOL=OFF`
    - 编译
        - `make -j nproc `
    - 结果
        - build目录下会产生so文件和CCTag的cmake config
        - 例如：`./CCTag-for-python-usage/build/src/generated/CCTagConfig.cmake`
## 3. 配置pybind11 cpp接口
- 创建pycctag.cpp
```cpp
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
```

- 创建`CMakeLists.txt`
- 配置项
```CMakeLists.txt
cmake_minimum_required(VERSION 3.5.1)
project(pycctag) 

# set the path includes CCTagConfig.cmake
find_package(CCTag CONFIG REQUIRED
	     PATHS ./CCTag-for-python-usage/build/src/generated
	     NO_DEFAULT_PATH
)
# set opencv(ignore)
set(OpenCV_DIR "/usr/local/lib/cmake/opencv4")
add_subdirectory(thirdparty/pybind11-2.13.6)

# create module
pybind11_add_module(pycctag pycctag.cpp)

# add header file
target_include_directories(pycctag PUBLIC
    ${CCTAG_INSTALL}/include/cctag
    ./CCTag-for-python-usage/src/cctag
)
target_link_libraries(pycctag PUBLIC CCTag)

# set C++14 standard to compile
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

## 4. 编译pybind11 cpp接口
- 编译
    - `sudo mkdir build && cd build`
    - `cmake ..`
    - `make -j nproc`
- 结果
    - `build`下产生`.so`文件，python可以直接调用
    - `pycctag.cpython-310-x86_64-linux-gnu.so`

## 5. 测试
- 确保`.so`与测试脚本在同一路径或者`sys.path`添加
```python
import cv2
# import sys
# sys.path.append("./CCTag-for-python-usage/python_pakage/pycctag")
import pycctag

markervector = pycctag.detect_from_file("./CCTag-for-python-usage/sample/01.png")
image = cv2.imread("./CCTag-for-python-usage/sample/01.png")
markerlist = list(markervector)
for i in range(len(markerlist)):
    print(str(markerlist[i].id)+"   "+str(markerlist[i].status))
    print(markerlist[i].x, markerlist[i].y)
    pointx = int(markerlist[i].x)
    pointy = int(markerlist[i].y)
    cv2.circle(image, (pointx, pointy), radius=10, color=(0, 255, 0), thickness=-1)
cv2.imwrite("./CCTag-for-python-usage/sample/01_result.png", image)
```

## 6.报错解决
```bash
libstdc++.so.6: version `GLIBCXX_3.4.30` not found
```
- 主要在conda环境下碰到
- 将`libstdc++.so.6`链接到当前conda环境即可


## 7. 精度问题
- 在某些机器上会出现检测精度急剧下降的情况，主要是Eigen库的懒加载问题导致的
- [参考](https://github.com/alicevision/CCTag/issues/179) 
- `some issue with Eigen's lazy evaluation`