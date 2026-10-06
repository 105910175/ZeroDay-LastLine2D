# Attributions

Zero-Day: Last Line (COS30031 Games Programming, Assessment 2). Built in Unreal Engine 5.8 with C++.

## 1. Assets created by our team

| Asset | Files | Created by |
|---|---|---|
| Gameplay code (walls, threats, spawners, stations, scan and validate, HUD, level flow, tests) | `Source/ZeroDayLastLine/` | Diesel Williams, with audio hooks added by Viresh |
| Sprite art: walls (intact, damaged, breached), Power/Food/Medical stations, threat, pickups, survivor icon, projectile | `Art/Source/*.png`, `Content/Sprites/` (28 assets) | Diesel Williams (drawn by `Art/generate_sprites.py`) |
| Physical materials (8 tuned materials) | `Content/Physics/` | Diesel Williams |
| Level layout, game mode and character Blueprints, project configuration | `Content/Levels/`, `Content/Blueprints/`, `Config/` | Viresh |
| Player walking flipbooks (Up, Down, Left, Right) and the scripts that build them | `Content/PlayerAnimations/FB_Player_*`, `Content/Python/` | Viresh |
| Audio integration (the code and Blueprint wiring that plays each sound) | `Source/ZeroDayLastLine/Private/Audio/` | Viresh |

## 2. Third-party assets

| Asset | Where it is used | Source | Author / creator | Licence |
|---|---|---|---|---|
| Sound effects (16): `SFX_Fire`, `SFX_Scan`, `SFX_Validate`, `SFX_Pickup`, `SFX_WallHit`, `SFX_WallBreached`, `SFX_WallRepaired`, `SFX_Surge` and others | `Content/Audio/SFX/*.ogg` | Kenney, *Sci-fi Sounds* pack: https://kenney.nl/assets/sci-fi-sounds | Kenney (kenney.nl) | Creative Commons CC0 1.0 (public domain) |
| Background music (`BackgroundMusic`) | `Content/Audio/Music/BackgroundMusic.mp3` | OpenGameArt.org, *Old Music* collection: https://opengameart.org/content/old-music | Alexandr Zhelanov | Creative Commons Attribution 3.0 (CC-BY 3.0): https://creativecommons.org/licenses/by/3.0/ |
| Desert ground and map textures | `Content/Environments/T_DesertGround`, `T_DesertMap` | TO CONFIRM (Viresh) | TO CONFIRM (Viresh) | TO CONFIRM (Viresh) |
| Player character sprite sheet (`pixel_character_dark_blue`, the dark blue variant) | `Content/PlayerAnimations/` | OpenGameArt.org, *Pixel character with gun*: https://opengameart.org/content/pixel-character-with-gun | Bonsaiheldin | Creative Commons CC0 1.0 (public domain) |
| Unreal Engine 5.8, including the Paper2D plugin and the engine's built-in cylinder mesh used as a stand-in body | Whole project | https://www.unrealengine.com | Epic Games, Inc. | Unreal Engine End User License Agreement |
| Pillow (Python imaging library) | `Art/generate_sprites.py` draws the sprites with it | https://python-pillow.org | Jeffrey A. Clark and contributors | MIT-CMU (HPND) |

### Required credit (CC-BY 3.0)

The background music is licensed under CC-BY 3.0, which requires this credit to be shown with the game. It is repeated on the itch.io page and in the README.

> Music: "Old Music" by Alexandr Zhelanov, from OpenGameArt.org (https://opengameart.org/content/old-music), licensed under CC-BY 3.0 (https://creativecommons.org/licenses/by/3.0/). The track was imported into Unreal Engine unchanged apart from engine import settings.

The sound effects (Kenney) and the player sprite sheet (Bonsaiheldin) are CC0, so credit is not required by the licence. They are listed here as a courtesy.

## 3. References consulted

These informed the design and the code. No assets were copied from them.

- Epic Games (2026) *Paper 2D Overview in Unreal Engine*, Unreal Engine 5.8 Documentation. https://dev.epicgames.com/documentation/en-us/unreal-engine/paper-2d-overview-in-unreal-engine
- Epic Games (2026) *Physical Materials User Guide for Unreal Engine*, Unreal Engine 5.8 Documentation. https://dev.epicgames.com/documentation/unreal-engine/physical-materials-user-guide-for-unreal-engine
- Epic Games (2026) *Automation Test Framework in Unreal Engine*, Unreal Engine 5.8 Documentation. https://dev.epicgames.com/documentation/en-us/unreal-engine/automation-test-framework-in-unreal-engine
- Qualys, Inc. (2025) *Introducing TruConfirm for Enterprise TruRisk Management: Automated Exposure Validation*, Qualys Blog, 15 October. https://blog.qualys.com/product-tech/2025/10/15/introducing-truconfirm-for-enterprise-trurisk-management-automated-exposure-validation
- Qualys, Inc. (2026) *Meet Agent Val: Closing the Validation Gap in Exposure Management at Machine Speed with Agentic AI*, Qualys Blog, 23 March. https://blog.qualys.com/product-tech/2026/03/23/meet-agent-val-closing-the-validation-gap-in-exposure-management-at-machine-speed-with-agentic-ai
- Qualys, Inc. (2026) *Zero-Day Remediation Meets Operational Resiliency*, Qualys Blog, 3 August. https://blog.qualys.com/product-tech/2026/08/03/zero-day-remediation-operational-resiliency-trurisk-eliminate
- Swinburne University of Technology (2026) *COS30031 Games Programming: Assignment 2 Brief* [unpublished course material]. Melbourne: Swinburne University of Technology.

The game takes its theme from the Qualys "Zero-Day: Race to Remediate" challenge. It does not copy the Qualys product interface or branding.
