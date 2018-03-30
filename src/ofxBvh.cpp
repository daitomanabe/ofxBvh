#include "ofxBvh.h"
#include "ofMain.h"
#include "euler.h"

using namespace std;

ofVec3f matToEuler(ofMatrix4x4 matrix, int order) {
    HMatrix hm;
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            hm[i][j] = matrix._mat[j][i];
        }
    }
    EulerAngles result = Eul_FromHMatrix(hm, order);
    ofVec3f euler;
    euler[EulAxI(order)] = result.x;
    euler[EulAxJ(order)] = result.y;
    euler[EulAxK(order)] = result.z;
    return euler;
}

// strip \r or \n from the end of a string
string strip(const string& s) {
    if (s.empty()) return s;
    int i = s.size() - 1;
    while (s[i] == '\r' || s[i] == '\n') i--;
    return s.substr(0, i + 1);
}

void ofxBvhJoint::dumpHierarchy(ostream& output, string tabs) {
    string token;
    if (isRoot()) {
        output << "HIERARCHY" << endl;
        token = "ROOT";
    } else {
        token = isSite() ? "End" : "JOINT";
    }
    output << tabs << token << " " << name << endl;
    output << tabs << "{" << endl;
    string indented = tabs + "  "; // use two spaces for tabs
    ofVec3f& p = offset;
    output << indented << "OFFSET " << p.x << " " << p.y << " " << p.z << endl;
    if (!isSite()) {
        output << indented << "CHANNELS " << channels;
        if (channels == 6) {
            output << " Xposition Yposition Zposition";
        }
        for (char axis : rotationOrder) {
            output << " " << axis << "rotation";
        }
        output << endl;
    }
    for (auto child : children) {
        child->dumpHierarchy(output, indented);
    }
    output << tabs << "}" << endl;
}

void ofxBvhJoint::drawHierarchy(bool drawNames) {
    ofSetColor(ofColor::white);
    ofDrawLine(ofVec3f(), offset);
    
    ofPushMatrix();
    ofMultMatrix(localMat);
    
    if (isSite()) {
        ofSetColor(ofColor::yellow);
        ofDrawBox(0, 0, 0, 2, 2, 2);
    } else {
        ofSetColor(ofColor::white);
        ofDrawBox(0, 0, 0, 4, 4, 4);
        if (drawNames) {
            ofDrawBitmapString(name, 0, 0);
        }
    }
    
    for (auto child : children) {
        child->drawHierarchy(drawNames);
    }
    
    ofPopMatrix();
}

void ofxBvhJoint::updateRaw(vector<double>::const_iterator& frame) {
    raw.resize(channels);
    for (int channel = 0; channel < channels; channel++) {
        raw[channel] = *frame++;
    }
    for (auto child : children) {
        child->updateRaw(frame);
    }
}

void ofxBvhJoint::updateMatrix(ofMatrix4x4 global) {
    vector<double>::iterator itr = raw.begin();
    ofMatrix4x4 local;
    if (isSite()) {
        local.preMultTranslate(offset);
    } else {
        if (channels == 6) {
            ofVec3f p;
            p.x = *itr++;
            p.y = *itr++;
            p.z = *itr++;
            local.preMultTranslate(p);
        } else {
            local.preMultTranslate(offset);
        }
        
        for (char axis : rotationOrder) {
            float angle = *itr++;
            switch(axis) {
                case 'X': local.preMult(ofMatrix4x4::newRotationMatrix(angle, 1, 0, 0)); break;
                case 'Y': local.preMult(ofMatrix4x4::newRotationMatrix(angle, 0, 1, 0)); break;
                case 'Z': local.preMult(ofMatrix4x4::newRotationMatrix(angle, 0, 0, 1)); break;
            }
        }
    }
    
    global.preMult(local);
    localMat = local;
    globalMat = global;
    
    for (auto child : children) {
        child->updateMatrix(global);
    }
}

void ofxBvhJoint::readRaw(std::vector<double>::iterator& frame) {
    for (auto channel : raw) {
        *frame++ = channel;
    }
    for (auto child : children) {
        child->readRaw(frame);
    }
}

void ofxBvhJoint::readMatrix() {
    vector<double>::iterator itr = raw.begin();
    if (!isSite()) {
        if (channels == 6) {
            ofVec3f translation = localMat.getTranslation();
            *itr++ = translation.x;
            *itr++ = translation.y;
            *itr++ = translation.z;
        }
        
        string order = rotationOrder;
        int orderType;
        // the EulOrd is "backwards" from the order rotations are applied
        if (order == "YXZ") orderType = EulOrdZXYs;
        else if (order == "ZXY") orderType = EulOrdYXZs;
        else {
            cout << "Rotation order '" << order << "' is not implemented. Please add it to ofxBvhJoint::readMatrix()" << endl;
            return;
        }
        ofVec3f euler = matToEuler(localMat, orderType);
        for (char axis : order) {
            float angle;
            switch(axis) {
                case 'X': angle = euler.x; break;
                case 'Y': angle = euler.y; break;
                case 'Z': angle = euler.z; break;
            }
            *itr++ = ofRadToDeg(angle);
        }
    }
    
    for (auto child : children) {
        child->readMatrix();
    }
}

void ofxBvh::dumpMotion(ostream& output, float frameTime, const vector<vector<double>>& motion) {
    output << "MOTION" << endl;
    output << "Frames:\t" << motion.size() << endl;
    output << "Frame Time:\t" << frameTime << endl;
    for (auto& frame : motion) {
        for (auto& channel : frame) {
            output << channel << " ";
        }
        output << endl;
    }
}

bool ofxBvh::checkReady() const {
    if (motion.empty()) {
        cout << "Not ready: no motion to update." << endl;
        false;
    }
    if (!root) {
        cout << "Not ready: no hierarchy to read." << endl;
        false;
    }
    return true;
}

bool ofxBvh::ready() const {
    return !motion.empty() && root;
}

ofxBvh::ofxBvh(string filename) {
    string path = ofToDataPath(filename);
    
    ofxBvhJoint* cur = nullptr;
    ifstream ifs(path);
    string s;
    ifs >> s; // skip "HIERARCHY"
    int channels = 0;
    while(true) {
        ifs >> s;
        if (s == "ROOT" || s == "JOINT" || s == "End") {
            ofxBvhJoint* child = new ofxBvhJoint();
            if (cur != nullptr) {
                cur->children.emplace_back(child);
                child->parent = cur;
            }
            cur = child;
            ifs.ignore(); // skip " "
            getline(ifs, cur->name);
            cur->name = strip(cur->name);
            joints.push_back(cur);
            if (s != "End") {
                jointMap[cur->name] = cur;
            }
            ifs >> s; // skip "{"
        } else if (s == "}") {
            if (!cur->isRoot()) {
                cur = cur->parent;
            }
        } else if (s == "OFFSET") {
            ofVec3f& p = cur->offset;
            ifs >> p.x >> p.y >> p.z;
        } else if (s == "CHANNELS") {
            int& n = cur->channels;
            ifs >> n;
            channels += n;
            ifs.ignore(); // skip " "
            vector<string> tokens;
            for (int i = 0; i < n; i++) {
                ifs >> s;
                tokens.push_back(s);
            }
            string& order = cur->rotationOrder;
            order += tokens[n-3][0];
            order += tokens[n-2][0];
            order += tokens[n-1][0];
        } else if (s == "MOTION") {
            break;
        } else {
            cout << "Unknown token: " << s << endl;
            return;
        }
    }
    
    root = shared_ptr<ofxBvhJoint>(cur);
    
    char c;
    int frames;
    ifs >> s >> frames; // skip "Frames:"
    ifs >> s >> c >> s >> frameTime; // skip "Frame Time:"
    getline(ifs, s); // finish off line?
    
    // getline + strtod is 3x faster than ifs
    motion = vector<vector<double>>(frames, vector<double>(channels));
    for (auto& frame : motion) {
        getline(ifs, s);
        const char* sc = s.c_str();
        char* end;
        for (auto& channel : frame) {
            channel = strtod(sc, &end);
            sc = end;
        }
    }
    ifs.close();
}

void ofxBvh::save(string filename) const {
    string path = ofToDataPath(filename);
    ofstream ofs(path);
    root->dumpHierarchy(ofs);
    dumpMotion(ofs, frameTime, motion);
    ofs.close();
}

void ofxBvh::updatePlayTime() {
    if (!checkReady()) return;
    
    // update the time
    bool previousFrameNumber = frameNumber;
    if (playing) {
        float elapsed = ofGetElapsedTimef() - startTime;
        int progress = elapsed * getFrameRate() * playRate;
        frameNumber = startFrame + progress;
        if (loop) {
            frameNumber %= getNumFrames();
        } else {
            frameNumber = std::min(frameNumber, getNumFrames()-1);
        }
    }
    frameNew = previousFrameNumber != frameNumber;
}

void ofxBvh::updateJointsRaw() {
    if (!checkReady()) return;
    vector<double>::const_iterator frame = motion[frameNumber].begin();
    root->updateRaw(frame);
}

void ofxBvh::updateJointsMatrix() {
    if (!checkReady()) return;
    root->updateMatrix();
}

void ofxBvh::readJointsMatrix() {
    if (!checkReady()) return;
    root->readMatrix();
}

void ofxBvh::readJointsRaw() {
    if (!checkReady()) return;
    vector<double>::iterator frame = motion[frameNumber].begin();
    root->readRaw(frame);
}

bool ofxBvh::isFrameNew() const {
    return frameNew;
}

void ofxBvh::draw(bool drawNames) const {
    if (!checkReady()) return;
    ofPushStyle();
    root->drawHierarchy(drawNames);
    ofPopStyle();
}

string ofxBvh::info() const {
    if (!checkReady()) return "";
    stringstream ss;
    float duration = getDuration();
    int minutes = (duration / 60);
    int seconds = round(duration - (minutes * 60));
    ss << getNumFrames() << " frames, " << motion[0].size() << " channels, "
        << minutes << "m" << seconds << "s duration, @ "
        << getFrameDuration() << "s or " << getFrameRate() << "fps";
    return ss.str();
}

const vector<ofxBvhJoint*>& ofxBvh::getJoints() const {
    return joints;
}

ofxBvhJoint* ofxBvh::getJoint(const std::string& name) {
    return jointMap[name];
}

void ofxBvh::play() {
    playing = true;
    startTime = ofGetElapsedTimef();
}
void ofxBvh::stop() {
    playing = false;
    startFrame = frameNumber;
}
void ofxBvh::setRate(float playRate) {
    this->playRate = playRate;
    startFrame = frameNumber;
    startTime = ofGetElapsedTimef();
}
float ofxBvh::getRate() const {
    return playRate;
}
void ofxBvh::togglePlaying() {
    if (playing) {
        stop();
    } else {
        play();
    }
}
bool ofxBvh::isPlaying() const {
    return playing;
}
void ofxBvh::setLoop(bool loop) {
    this->loop = loop;
}
bool ofxBvh::isLoop() const {
    return loop;
}

float ofxBvh::getDuration() const {
    return getNumFrames() * frameTime;
}
unsigned int ofxBvh::getNumFrames() const {
    return motion.size();
}
float ofxBvh::getFrameDuration() const {
    return frameTime;
}
float ofxBvh::getFrameRate() const {
    return 1 / frameTime;
}
unsigned int ofxBvh::getFrame() const {
    return frameNumber;
}
float ofxBvh::getTime() const {
    return frameNumber * frameTime;
}
float ofxBvh::getPosition() const {
    // approximate
    return float(frameNumber) / getNumFrames();
}
void ofxBvh::setFrame(unsigned int frameNumber) {
    if (loop) {
        frameNumber %= getNumFrames();
    } else {
        frameNumber = std::min(frameNumber, getNumFrames()-1);
    }
    startTime = ofGetElapsedTimef();
    this->frameNumber = frameNumber;
    startFrame = frameNumber;
}
void ofxBvh::setTime(float seconds) {
    seconds = std::max(seconds, 0.f);
    setFrame(seconds * getFrameRate());
}
void ofxBvh::setPosition(float ratio) {
    setTime(ratio * getDuration());
}

void ofxBvh::cropToFrame(unsigned int beginFrameNumber, unsigned int endFrameNumber) {
    if (!checkReady()) return;
    
    // it's possible to keep playing/looping while cropping,
    // but complicated to implement correctly... so just stop.
    frameNumber = 0;
    stop();
    
    motion.erase(motion.begin(), motion.begin() + beginFrameNumber);
    if(endFrameNumber > beginFrameNumber) {
        motion.resize(endFrameNumber - beginFrameNumber);
    }
}
void ofxBvh::cropToTime(float beginSeconds, float endSeconds) {
    unsigned int beginFrameNumber = beginSeconds * getFrameRate();
    unsigned int endFrameNumber = endSeconds * getFrameRate();
    cropToFrame(beginFrameNumber, endFrameNumber);
}
void ofxBvh::cropToPosition(float beginRatio, float endRatio) {
    float beginSeconds = beginRatio * getDuration();
    float endSeconds = endRatio * getDuration();
    cropToTime(beginSeconds, endSeconds);
}
