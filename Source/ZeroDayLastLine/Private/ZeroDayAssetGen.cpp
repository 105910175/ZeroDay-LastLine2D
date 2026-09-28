#include "ZeroDayAssetGen.h" // This class's own declaration.
#include "ZeroDayPhysics.h" // The physics material specs.

#if WITH_EDITOR // Everything that touches asset saving only exists in the editor.
#include "Misc/FileHelper.h" // Reads the PNG files.
#include "Misc/Paths.h" // Builds file paths.
#include "Misc/PackageName.h" // Converts package names to file names.
#include "HAL/FileManager.h" // Lists the PNG files.
#include "Modules/ModuleManager.h" // Loads the image decoder module.
#include "IImageWrapper.h" // Decodes PNG data.
#include "IImageWrapperModule.h" // Creates the decoder.
#include "Engine/Texture2D.h" // The texture asset type.
#include "PhysicalMaterials/PhysicalMaterial.h" // The physics material asset type.
#include "PaperSprite.h" // The sprite asset type.
#include "SpriteEditorOnlyTypes.h" // Sprite creation parameters.
#include "AssetRegistry/AssetRegistryModule.h" // Tells the editor an asset now exists.
#include "UObject/Package.h" // Asset packages.
#include "UObject/SavePackage.h" // Saving a package to disk.
#endif

UZeroDayAssetGenCommandlet::UZeroDayAssetGenCommandlet()
{
	IsClient = false; // Not a game client.
	IsEditor = true; // Needs the editor's asset tools.
	IsServer = false; // Not a server.
	LogToConsole = true; // Print progress to the console.
}

#if WITH_EDITOR

namespace // Only this file uses these.
{
	// Finds an existing asset on disk to update in place, or creates a new one in a fresh package.
	template <typename AssetType>
	AssetType* LoadOrCreate(const FString& PackageName, const FString& AssetName)
	{
		const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackageName, *AssetName); // e.g. /Game/Sprites/T_threat.T_threat
		if (AssetType* Existing = LoadObject<AssetType>(nullptr, *ObjectPath, nullptr, LOAD_NoWarn | LOAD_Quiet)) // Already saved before?
		{
			return Existing; // Update it rather than making a duplicate.
		}
		UPackage* Package = CreatePackage(*PackageName); // A new package to hold the asset.
		return NewObject<AssetType>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional); // Create the asset inside it.
	}

	// Registers an asset with the editor and writes its package to disk.
	bool SaveAsset(UObject* Asset)
	{
		UPackage* Package = Asset->GetOutermost(); // The package the asset lives in.
		Package->MarkPackageDirty(); // Flag it as needing a save.
		FAssetRegistryModule::AssetCreated(Asset); // Let the asset registry know about it.
		const FString FileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension()); // Where the .uasset goes.
		FSavePackageArgs Args; // Save settings.
		Args.TopLevelFlags = RF_Public | RF_Standalone; // Keep the asset visible to other packages.
		Args.SaveFlags = SAVE_NoError; // Don't raise a dialog on problems.
		return UPackage::SavePackage(Package, Asset, *FileName, Args); // True if it was written.
	}

	// Decodes a PNG and stores its pixels in a texture asset.
	UTexture2D* ImportTexture(const FString& PngPath, const FString& BaseName)
	{
		TArray<uint8> FileData; // The raw bytes of the PNG file.
		if (!FFileHelper::LoadFileToArray(FileData, *PngPath)) // Read it.
		{
			return nullptr; // Could not read the file.
		}

		IImageWrapperModule& Module = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper")); // The PNG decoder.
		TSharedPtr<IImageWrapper> Wrapper = Module.CreateImageWrapper(EImageFormat::PNG); // A decoder for PNG data.
		if (!Wrapper.IsValid() || !Wrapper->SetCompressed(FileData.GetData(), FileData.Num())) // Hand it the file.
		{
			return nullptr; // Not a valid PNG.
		}

		TArray64<uint8> Pixels; // Decoded pixels.
		if (!Wrapper->GetRaw(ERGBFormat::BGRA, 8, Pixels)) // Ask for 8-bit BGRA, which is what textures store.
		{
			return nullptr; // Decoding failed.
		}

		const FString AssetName = TEXT("T_") + BaseName; // e.g. T_threat.
		UTexture2D* Texture = LoadOrCreate<UTexture2D>(TEXT("/Game/Sprites/") + AssetName, AssetName); // Find or create the texture.
		Texture->Source.Init(Wrapper->GetWidth(), Wrapper->GetHeight(), 1, 1, TSF_BGRA8, Pixels.GetData()); // Store the pixels as the texture's source.
		Texture->SRGB = true; // The art is in ordinary colour space.
		Texture->CompressionSettings = TC_EditorIcon; // Keep it sharp: no lossy compression.
		Texture->MipGenSettings = TMGS_NoMipmaps; // 2D art is drawn at a fixed size; mipmaps are not needed.
		Texture->LODGroup = TEXTUREGROUP_UI; // Not streamed, and behaves like UI art.
		Texture->NeverStream = true; // Always fully loaded.
		Texture->Filter = TF_Bilinear; // Smooth scaling.
		Texture->UpdateResource(); // Rebuild the render resource.
		Texture->PostEditChange(); // Tell the editor it changed.
		return Texture; // Ready to save.
	}

	// Creates a Paper2D sprite that shows a whole texture.
	UPaperSprite* MakeSprite(UTexture2D* Texture, const FString& BaseName)
	{
		const FString AssetName = TEXT("S_") + BaseName; // e.g. S_threat.
		UPaperSprite* Sprite = LoadOrCreate<UPaperSprite>(TEXT("/Game/Sprites/") + AssetName, AssetName); // Find or create the sprite.

		FSpriteAssetInitParameters Init; // How to build the sprite.
		Init.SetTextureAndFill(Texture); // Use the whole texture.
		UMaterialInterface* SpriteMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial")); // Unlit and soft-edged, so it looks the same regardless of lighting.
		Init.DefaultMaterialOverride = SpriteMaterial; // Draw with it normally...
		Init.AlternateMaterialOverride = SpriteMaterial; // ...and in the alternate slot too.
		Sprite->InitializeSprite(Init); // Build the sprite from the parameters.
		return Sprite; // Ready to save.
	}
}

#endif // WITH_EDITOR

// Creates or refreshes every physics material and sprite asset.
int32 UZeroDayAssetGenCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	int32 Failures = 0; // How many assets could not be created or saved.

	// 1. Physics materials, one per entry in the spec table.
	for (int32 Index = 0; Index < static_cast<int32>(EZeroDayMaterial::Count); ++Index) // Every material.
	{
		const EZeroDayMaterial Kind = static_cast<EZeroDayMaterial>(Index); // Which one.
		const FZeroDayMaterialSpec& Spec = ZeroDayPhysics::GetSpec(Kind); // Its numbers.
		UPhysicalMaterial* Material = LoadOrCreate<UPhysicalMaterial>(FString(TEXT("/Game/Physics/")) + Spec.Name, Spec.Name); // Find or create the asset.
		ZeroDayPhysics::ApplySpec(Material, Kind); // Write the numbers into it.
		const bool bSaved = SaveAsset(Material); // Save it.
		UE_LOG(LogTemp, Display, TEXT("ZeroDayAssetGen: physics material %s friction=%.2f restitution=%.2f density=%.2f saved=%d"), Spec.Name, Spec.Friction, Spec.Restitution, Spec.Density, bSaved ? 1 : 0); // Report it.
		Failures += bSaved ? 0 : 1; // Count a failure if it did not save.
	}

	// 2. A texture and a sprite for every PNG in Art/Source (files starting with "_" are ignored).
	TArray<FString> PngFiles; // The file names found.
	IFileManager::Get().FindFiles(PngFiles, *(FPaths::ProjectDir() / TEXT("Art/Source/*.png")), true, false); // List them.
	for (const FString& File : PngFiles) // Every PNG.
	{
		if (File.StartsWith(TEXT("_"))) // Contact sheets and other helper images.
		{
			continue; // Not game art.
		}

		const FString BaseName = FPaths::GetBaseFilename(File); // e.g. "threat".
		UTexture2D* Texture = ImportTexture(FPaths::ProjectDir() / TEXT("Art/Source") / File, BaseName); // Build the texture.
		if (!Texture) // Import failed.
		{
			UE_LOG(LogTemp, Error, TEXT("ZeroDayAssetGen: could not import %s"), *File); // Say which file.
			++Failures; // Count it.
			continue; // Move on.
		}

		UPaperSprite* Sprite = MakeSprite(Texture, BaseName); // Build the sprite from it.
		const bool bTextureSaved = SaveAsset(Texture); // Save the texture.
		const bool bSpriteSaved = SaveAsset(Sprite); // Save the sprite.
		UE_LOG(LogTemp, Display, TEXT("ZeroDayAssetGen: %s -> %dx%d texture saved=%d sprite saved=%d"), *File, Texture->Source.GetSizeX(), Texture->Source.GetSizeY(), bTextureSaved ? 1 : 0, bSpriteSaved ? 1 : 0); // Report it.
		Failures += (bTextureSaved ? 0 : 1) + (bSpriteSaved ? 0 : 1); // Count any failures.
	}

	UE_LOG(LogTemp, Display, TEXT("ZeroDayAssetGen: finished with %d failure(s)"), Failures); // Final summary.
	return Failures == 0 ? 0 : 1; // Zero means success.
#else
	return 1; // Asset generation is only possible in the editor.
#endif
}
