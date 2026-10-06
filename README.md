# Zero-Day: Last Line

A top-down 2D survival game for COS30031 Games Programming (Assessment 2), built in **Unreal Engine 5.8** with **C++** and Paper2D. Team: Diesel Williams, Viresh Joshi, Shashimal, Sean Wilson.

## Game description

A zero-day vulnerability has been announced, and the city's four walls are the systems under attack. You are the city's response team. Zombies are the attacker, arriving at the walls faster than you can shoot them one by one. You have to work out which threats are real, then fix what breaks while keeping the city running.

## Connection to the challenge (Qualys, "Zero-Day: Race to Remediate")

The game plays out the Qualys **Detect, Validate, Remediate** loop.

| Stage | In the game |
|---|---|
| **Detect** | **Insta Scan (Q)** flags every inbound threat at once. Flagged threats show a "?" marker and the wall shows an exposure warning. |
| **Validate** | **Validate (F)** reveals each flagged threat's severity, or marks it as a **false positive** (harmless). Shooting false positives wastes time, and skipping Validate is a risk. |
| **Remediate** | **Patch** a breached wall with **E** (costs a Patch kit), or contain it by shooting the threats. On level 3, feed and power the city stations and keep survivors alive. |

The game does not copy the Qualys product interface or branding.

## Controls

| Key | Action |
|---|---|
| W A S D | Move |
| Mouse | Aim |
| Left click | Shoot |
| Q | Insta Scan (Detect) |
| F | Validate |
| E | Fix a breached wall, or attend a Power/Food/Medical station |
| R | Restart the level |
| N | Go to the next level (after a win) |

## How to play

1. Read the objective banner at the start of each level.
2. Press **Q** to scan, then **F** near a wall to find out which threats are real.
3. Shoot the real threats before they reach the wall. Ignore the false positives.
4. If a wall breaks, you have a short countdown. Press **E** beside it to patch it, or clear the threats attacking it.
5. Collect the pickups dropped inside the city. They give you Patch kits and Validate charges, and keep the stations running.
6. Survive until the "Hold out" timer ends. If a wall stays breached too long, or a station or the survivors fail, you lose.

| Level | What changes |
|---|---|
| 1. Detect | One wall (north). Learn to scan and shoot. |
| 2. Validate | All four walls and false positives. Validate is needed. |
| 3. Remediate | Surges hit the walls, plus Power, Food and Medical stations and survivors to look after. |

## How to run the build

- **Platform:** Windows 64-bit (a packaged desktop build, no install needed).
- Unzip the build and run **`ZeroDayLastLine.exe`**.
- The game needs a DirectX 11 capable graphics card. Windows may show a SmartScreen prompt for the unsigned build: choose "More info", then "Run anyway".

## How to build the project from source

1. Install **Git** with **Git LFS**, then run `git lfs install`.
2. Clone the repository: `git clone https://github.com/105910175/GameMakersAssignment2.git`. Alternatively, unzip `ZeroDayLastLine_Project.zip` from the repository root.
3. Install **Unreal Engine 5.8** and Visual Studio with the "Game development with C++" workload.
4. Right-click `ZeroDayLastLine.uproject`, choose **Generate Visual Studio project files**, and open the solution.
5. Build the `ZeroDayLastLineEditor` target (Development Editor, Win64), then open the project in the editor and press Play. The map is `Content/Levels/L_Level1_Detect`.
6. To run the automated tests: `UnrealEditor-Cmd.exe ZeroDayLastLine.uproject -ExecCmds="Automation RunTests ZeroDay; Quit" -nullrhi -unattended`. There are 12 tests.

## Key programming systems

Each system is a reusable component or actor, not written for a single object.

- **Health and breach** (`BreachableComponent`): health and Safe/Threatened/Breached status, shared by all four walls.
- **Timers and cooldowns** (`CooldownComponent`): used for the breach countdown, station decay, spawners, and Insta Scan and Validate.
- **Task and decay** (`CityTaskComponent`): one decaying condition meter reused by the Power, Food and Medical stations.
- **Spawning** (`ThreatSpawnerActor`): one spawner per wall, with per-level difficulty from `ZeroDayLevelConfig`.
- **Detection** (`DetectionComponent`): Insta Scan and Validate.
- **Pickups** (`ResourcePickup`): physics-simulated resources with a material per type.
- **Audio** (`ZeroDayAudioComponent`): one component that plays every sound effect and the music.

**Physics and collision:** custom channels for walls, threats and projectiles, with overlap and block rules so shots hit threats but threats are stopped by walls, and **8 physical materials** (wall, threat, projectile and the five pickups) with different friction, bounce and mass.

## Team contributions

| Member | Contribution |
|---|---|
| **Diesel Williams** | Gameplay code (walls, threats, spawners, stations, scan and validate, level flow, HUD), generated sprite art, physical materials, automated tests, documentation |
| **Viresh Joshi** | Project setup, level and desert environment, player character and walking animation, all audio and its integration, video demo |
| **Shashimal** | Video editing |
| **Sean Wilson** | Team member (contribution to be added) |

Full file-by-file credit is in the development report, and every asset's source is in [ATTRIBUTIONS.md](ATTRIBUTIONS.md).

## Known issues

- Wall and station collision bodies are larger than their drawn art, because they were set up before the sprite scale was measured.
- The city is large, so Validate (range 700) only works when you are near a wall.
- Packaged builds start in the level 1 map. Use **N** after a win to move through the levels.

## Credits

Sound effects: *Sci-fi Sounds* by Kenney (CC0). Player sprite: *Pixel character with gun* by Bonsaiheldin (CC0). Music: "Old Music" by Alexandr Zhelanov, from OpenGameArt.org (https://opengameart.org/content/old-music), licensed under CC-BY 3.0 (https://creativecommons.org/licenses/by/3.0/). The full list is in [ATTRIBUTIONS.md](ATTRIBUTIONS.md).
