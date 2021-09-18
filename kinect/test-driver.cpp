#include "kinect.hpp"
using namespace mik;

// Saves 10 frames to disk

int main(int, char*[]) {
    Kinect kinect(KinectConfig{}); // Default config
    kinect.save_frames(10);
    return 0;
}