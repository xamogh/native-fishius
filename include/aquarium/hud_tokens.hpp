#pragma once

namespace aq::hudTokens {
// Author UI measurements on a four-unit grid, then scale them for the window.
consteval float measure(int value){
 if(value<0||value%4!=0)throw "HUD measurements must be multiples of four";
 return static_cast<float>(value);
}
inline constexpr float canvasWidth=measure(1608),canvasHeight=measure(908);
inline constexpr float smallGap=measure(4),overlap=measure(8),padding=measure(12),gap=measure(16),margin=measure(24);
inline constexpr float icon=measure(64);
inline constexpr float shop=measure(144),food=measure(64),bagWidth=measure(108),bagHeight=measure(108);
inline constexpr float barHeight=measure(40),xpWidth=measure(344),coinWidth=measure(240),pearlWidth=measure(120);
inline constexpr float textSmall=measure(20),textBody=measure(24),textLarge=measure(32);
inline constexpr float lineHeight=measure(32),smallLineHeight=measure(24);
inline constexpr float radius=measure(8),panelRadius=measure(16),border=measure(4),dot=measure(16);
static_assert(food==icon&&pearlWidth*2==coinWidth);
}
