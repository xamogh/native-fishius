These sources are pinned so the native editor and game build without fetching
new UI dependencies. Each library retains its upstream license.

| Library | Official source | Revision | Archive SHA-256 |
| --- | --- | --- | --- |
| Dear ImGui | https://github.com/ocornut/imgui | `7a6efe395d18d9cb37cea397f63ce0d39824dc72` (`v1.92.6-docking`) | `2213db4c362b276c664de799bf79b80bc468ee9ae05a2969d809d28b4de3f947` |
| Clay | https://github.com/nicbarker/clay | `b25a31c1a152915cd7dd6796e6592273e5a10aac` (`v0.14`) | `1cba69296adf4efb57dbabc46076f0d50912319ed703d372c655e6a45c47129d` |

ImGui includes its core, SDL3 platform backend, and SDL_Renderer backend. It is
linked only into Aquarium Studio. Clay resolves the dialog's layout in both
the game and Studio. SDL and SDL_ttf still draw all game artwork and text.
