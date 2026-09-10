#pragma once
#include "aquarium/domain.hpp"
#include <filesystem>
namespace aq {
struct LoadResult {std::optional<State> state;std::string message;bool preservedOriginal{};};
// Single session owns one Storage. All paths are supplied by the host.
class Storage {
 public:explicit Storage(std::filesystem::path path):path_(std::move(path)){}
 LoadResult load(const Content&);std::string save(const State&);const auto& path()const{return path_;}
 private:std::filesystem::path path_;bool protectOriginal_{};
};
class Session {
 public:Session(Content, std::filesystem::path saveFile,Millis wallNow,bool ephemeral=false);
 Domain& domain(){return domain_;}const Domain& domain()const{return domain_;}
 Result command(const Command&);void update(Millis elapsed,Millis wall,Tool tool,FishId held={});
 void suspend(Millis wall);void resume(Millis wall);void checkpoint(Millis wall);
 bool paused()const{return paused_;}double interpolation()const{return double(accumulator_)/20.;}
 const std::string& status()const{return status_;}
 private:Domain domain_;Storage storage_;bool paused_{},ephemeral_{};Millis accumulator_{},sinceSave_{};std::string status_;
};
}
