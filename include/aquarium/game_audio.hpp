#pragma once
#include "aquarium/domain.hpp"
#include <SDL3/SDL.h>
#include <filesystem>
#include <vector>
namespace aq {
// The loop has its own stream so muting music never mutes tap/reward sounds.
class GameAudio {
 public:
 explicit GameAudio(const std::filesystem::path& assets);~GameAudio();
 GameAudio(const GameAudio&)=delete;GameAudio& operator=(const GameAudio&)=delete;
 void update(const Settings&,double seconds,bool suspended=false);
 bool available()const{return stream_!=nullptr;}
 bool playing()const{return playing_;}float gain()const{return gain_;}
 private:
 SDL_AudioStream* stream_{};std::vector<Uint8> loop_;std::size_t cursor_{};
 int chunkBytes_{};float gain_{};bool playing_{};
};
}
