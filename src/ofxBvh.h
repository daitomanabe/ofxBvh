#pragma once

#include "ofVectorMath.h"
#include <map>

class ofxBvhJoint {
    friend class ofxBvh;
    
private:
    int channels = 0;
    std::string rotationOrder;
    ofxBvhJoint* parent = nullptr;
    std::vector<std::shared_ptr<ofxBvhJoint>> children;
    
    void dumpHierarchy(std::ostream& output, std::string tabs="");
    void drawHierarchy(bool drawNames=false);
    
    void updateRaw(std::vector<double>::const_iterator& frame); // splits frame into raw
    void updateMatrix(ofMatrix4x4 global=ofMatrix4x4()); // converts raw to localMat and globalMat
    void readRaw(std::vector<double>::iterator& frame); // joins raw into frame
    void readMatrix(); // converts localMat to raw
    
public:
    std::string name;
    ofVec3f offset;
    ofMatrix4x4 localMat, globalMat;
    std::vector<double> raw;
    
    inline ofVec3f getPosition() const { return globalMat.getTranslation(); }
    inline ofVec3f getPositionLocal() const { return localMat.getTranslation(); }
    inline ofQuaternion getRotation() const { return globalMat.getRotate(); }
    inline ofQuaternion getRotationLocal() const { return localMat.getRotate(); }
    
    inline ofxBvhJoint* getParent() const { return parent; }
    inline const std::vector<std::shared_ptr<ofxBvhJoint>>& getChildren() const { return children; }
    
    inline bool isSite() const { return children.empty(); }
    inline bool isRoot() const { return !parent; }
};

class ofxBvh {
protected:
    std::shared_ptr<ofxBvhJoint> root;
    std::vector<ofxBvhJoint*> joints;
    std::map<std::string, ofxBvhJoint*> jointMap;
    double frameTime = 0;
    std::vector<std::vector<double>> motion;
    
    float playRate = 1;
    float startTime = 0;
    unsigned int startFrame = 0;
    unsigned int frameNumber = 0;
    bool playing = false;
    bool loop = true;
    bool frameNew = false;
    
    static void dumpMotion(std::ostream& output, float frameTime, const std::vector<std::vector<double>>& motion);
    bool checkReady() const;
    
public:
    
    ofxBvh() {}
    ofxBvh(std::string filename);
    void save(std::string filename) const;
    inline void update() { updatePlayTime(); updateJointsRaw(); updateJointsMatrix(); }
    void updatePlayTime(); // update the frameNumber from the current time
    void updateJointsRaw(); // write motion data into all joints raw data
    void updateJointsMatrix(); // write joints raw data into joints matrices
    inline void read() { readJointsMatrix(); readJointsRaw(); }
    void readJointsMatrix(); // read all joint matrices into joints raw data
    void readJointsRaw(); // read joints raw data into motion data
    bool isFrameNew() const;
    bool ready() const;
    void draw(bool drawNames=false) const;
    std::string info() const;
    
    const std::vector<ofxBvhJoint*>& getJoints() const;
    ofxBvhJoint* getJoint(const std::string& name);
    
    void play();
    void stop();
    void setRate(float playRate);
    float getRate() const;
    void togglePlaying();
    bool isPlaying() const;
    
    void setLoop(bool loop);
    bool isLoop() const;
    
    float getDuration() const; // in seconds
    unsigned int getNumFrames() const; // total frame count
    float getFrameDuration() const; // in seconds
    float getFrameRate() const; // in frames per second
    unsigned int getFrame() const;
    float getTime() const; // current time in seconds
    float getPosition() const; // current position 0-1
    
    void setFrame(unsigned int frameNumber);
    void setTime(float seconds); // set time in seconds
    void setPosition(float ratio); // set position 0-1
    
    // cropping will cause playback to stop
    void cropToFrame(unsigned int beginFrameNumber, unsigned int endFrameNumber=0);
    void cropToTime(float beginSeconds, float endSeconds=0);
    void cropToPosition(float beginRatio, float endRatio=0);
};
