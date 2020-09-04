#ifndef ofxBvhExt_hpp
#define ofxBvhExt_hpp

#include <stdio.h>
#include "ofxBvh.h"

class ofxBvhExt : public ofxBvh{
public:
    std::vector<std::vector<double>> & getMotions(){
        return motion;
    }
    std::vector<double> & getMotion(int cur){
        return motion[cur];
    }
    
    //他のbvhのmotionにエフェクトをかけたものを保存する
    
    void store_motion(int cur, const std::vector<double> & motions);
    void store_motion(int cur, const ofxBvh & b);

    // effect 系
    // joint_name がallだと全部にかかるとか
    void rotate_joint(std::vector<std::string> joint_names, float val);
    void translate_joint(std::vector<std::string> joint_names, float val);
    
    // てだけ、かただけ、などグループネームを指定できる
    void rotate_joint(std::string joint_group_name, float val);
    
    // なにもなければ全部
    void rotate_joint(float val);

    //その他 NOISE など
//    perlin_joint
//    noise_joint // whitenoise
//    
};
#endif /* ofxBvhExt_hpp */
