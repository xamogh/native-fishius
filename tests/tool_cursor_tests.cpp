#include "aquarium/canvas.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
using namespace aq;
void check(bool good,const char* message){if(!good)throw std::runtime_error(message);}
bool near(double a,double b){return std::abs(a-b)<.0001;}
void checkAnimation(){
 const auto can=toolCursorPose(Tool::Food,.1),net=toolCursorPose(Tool::Sell,.125);
 check(near(can.angle,-70)&&near(can.scale,.96),"Food can does not match Phaser's tilt and scale");
 check(near(net.angle,-18)&&near(net.scale,1.08),"Net does not match Phaser's swing and scale");
 for(double t:{-.1,0.,.2,1.})check(near(toolCursorPose(Tool::Food,t).angle,-30),"Food can does not return after 200 ms");
 for(double t:{-.1,0.,.25,1.})check(near(toolCursorPose(Tool::Sell,t).angle,0),"Net does not return after 250 ms");
 check(near(toolCursorPose(Tool::Food,.1,true).angle,-30)&&near(toolCursorPose(Tool::Sell,.125,true).scale,1),"Reduced motion still swings or tips");
}
}
int main(){try{checkAnimation();std::cout<<"PASS food and net cursor timing, including reduced motion\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
