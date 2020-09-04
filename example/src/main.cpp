#include "ofMain.h"
#include "ofxBvh.h"
#include "ofxBvhExt.hpp"
#include "ofxPubSubOsc.h"

constexpr int OSC_IN_PORT = 9876;

class ofApp : public ofBaseApp {
public:
    ofxBvhExt bvh_src;
    ofxBvhExt bvh_dst;

    ofEasyCam cam;
    
    void setup() {
    }
    void update() {
    }
    void draw() {
    }
};

int main() {
    ofSetupOpenGL(1280, 720, OF_WINDOW);
    ofRunApp(new ofApp());
}
