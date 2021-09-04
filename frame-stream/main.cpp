#include "frame_stream.hpp"
#include "kinect.hpp"
using namespace mik;

int main(int, char*[]) {
    Kinect kinect(KinectConfig{}); // Default config
    kinect.save_frames(10);
    return 0;
}