#pragma once
#include "aquarium/domain.hpp"

namespace aq {
struct FishDetailsContent {std::string title,care,growth,sale;FishReward reward,adultReward;float progress{};int stage{};Care condition{Care::Fed};};
FishDetailsContent fishDetailsContent(const Species&,const Fish&,Millis now);
}
