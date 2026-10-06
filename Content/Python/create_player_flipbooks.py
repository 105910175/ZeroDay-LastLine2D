import unreal

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

sprite_path = "/Game/PlayerAnimations"
flipbook_path = "/Game/PlayerAnimations/Flipbooks"

unreal.EditorAssetLibrary.make_directory(flipbook_path)

for group in range(8):
    sprites = []

    start = group * 8
    end = min(start + 8, 58)

    for i in range(start, end):
        path = f"{sprite_path}/pixel_character_dark_blue_Sprite_{i}"
        sprite = unreal.EditorAssetLibrary.load_asset(path)

        if sprite:
            sprites.append(sprite)

    if not sprites:
        continue

    name = f"FB_Player_Test_{group}"

    if unreal.EditorAssetLibrary.does_asset_exist(
        f"{flipbook_path}/{name}"
    ):
        unreal.EditorAssetLibrary.delete_asset(
            f"{flipbook_path}/{name}"
        )

    factory = unreal.PaperFlipbookFactory()

    flipbook = asset_tools.create_asset(
        name,
        flipbook_path,
        unreal.PaperFlipbook,
        factory
    )

    key_frames = []

    for sprite in sprites:
        key_frame = unreal.PaperFlipbookKeyFrame()
        key_frame.sprite = sprite
        key_frame.frame_run = 1
        key_frames.append(key_frame)

    flipbook.set_editor_property("key_frames", key_frames)
    flipbook.set_editor_property(
        "frames_per_second",
        unreal.FrameRate(8, 1)
    )

    unreal.EditorAssetLibrary.save_asset(
        f"{flipbook_path}/{name}"
    )

    print(f"Created {name}: sprites {start} - {end - 1}")

print("DONE")
