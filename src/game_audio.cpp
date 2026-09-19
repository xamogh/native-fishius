#include "aquarium/game_audio.hpp"
#include <algorithm>
#include <cmath>
namespace aq {
GameAudio::GameAudio(const std::filesystem::path& assets){
 SDL_AudioSpec spec{};Uint8* data=nullptr;Uint32 length=0;
 if(!SDL_LoadWAV((assets/"audio/coral-promenade-loop.wav").string().c_str(),&spec,&data,&length))return;
 const int frameBytes=SDL_AUDIO_BYTESIZE(spec.format)*spec.channels;
 if(frameBytes>0&&spec.freq>0&&length>0&&length%frameBytes==0){
  loop_.assign(data,data+length);chunkBytes_=std::max(1,spec.freq/8)*frameBytes;
  stream_=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,nullptr,nullptr);
  if(stream_)SDL_SetAudioStreamGain(stream_,0);
 }
 SDL_free(data);
}
GameAudio::~GameAudio(){if(stream_)SDL_DestroyAudioStream(stream_);}
void GameAudio::update(const Settings& settings,double seconds,bool suspended){
 if(!stream_)return;
 const float target=settings.music&&!suspended?float(settings.volume):0;
 const float step=float(std::clamp(seconds,0.,.25))/.08f;
 gain_=suspended?0:gain_+std::clamp(target-gain_,-step,step);
 SDL_SetAudioStreamGain(stream_,gain_);
 if(gain_<=0){
  if(playing_){SDL_PauseAudioStreamDevice(stream_);SDL_ClearAudioStream(stream_);playing_=false;}
  return;
 }
 // Bound queued audio to 250 ms. Copy whole frames, wrapping at the exact
 // authored loop boundary without inserting silence or replaying a fade.
 int queued=SDL_GetAudioStreamQueued(stream_);
 if(queued<0)return;
 while(queued<chunkBytes_*2){
  const auto count=std::min({std::size_t(chunkBytes_),loop_.size()-cursor_,std::size_t(chunkBytes_*2-queued)});
  if(!SDL_PutAudioStreamData(stream_,loop_.data()+cursor_,int(count)))return;
  cursor_=(cursor_+count)%loop_.size();queued+=int(count);
 }
 if(!playing_)playing_=SDL_ResumeAudioStreamDevice(stream_);
}
}
