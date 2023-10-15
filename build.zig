const std = @import("std");
const vcpkg_bootstrap = @import("utils/vcpkg_bootstrap.zig");
const vcpkg_install = @import("utils/vcpkg_install.zig");

const source_files = [_][]const u8{
    "source/AI.cpp",
    "source/Account.cpp",
    "source/AlertLabel.cpp",
    "source/AmmoDisplay.cpp",
    "source/Angle.cpp",
    "source/Armament.cpp",
    "source/AsteroidField.cpp",
    "source/Audio.cpp",
    "source/BankPanel.cpp",
    "source/BatchDrawList.cpp",
    "source/BatchShader.cpp",
    "source/Bitset.cpp",
    "source/BoardingPanel.cpp",
    "source/Body.cpp",
    "source/CaptureOdds.cpp",
    "source/CargoHold.cpp",
    "source/CategoryList.cpp",
    "source/CollisionSet.cpp",
    "source/Color.cpp",
    "source/Command.cpp",
    "source/ConditionSet.cpp",
    "source/ConditionsStore.cpp",
    "source/Conversation.cpp",
    "source/ConversationPanel.cpp",
    "source/CoreStartData.cpp",
    "source/DamageProfile.cpp",
    "source/DataFile.cpp",
    "source/DataNode.cpp",
    "source/DataWriter.cpp",
    "source/Date.cpp",
    "source/Depreciation.cpp",
    "source/Dialog.cpp",
    "source/Dictionary.cpp",
    "source/DistanceCalculationSettings.cpp",
    "source/DistanceMap.cpp",
    "source/Distribution.cpp",
    "source/DrawList.cpp",
    "source/Effect.cpp",
    "source/Engine.cpp",
    "source/EsUuid.cpp",
    "source/EscortDisplay.cpp",
    "source/File.cpp",
    "source/Files.cpp",
    "source/FillShader.cpp",
    "source/FireCommand.cpp",
    "source/Fleet.cpp",
    "source/FleetCargo.cpp",
    "source/Flotsam.cpp",
    "source/FogShader.cpp",
    "source/FormationPattern.cpp",
    "source/FrameTimer.cpp",
    "source/Galaxy.cpp",
    "source/GameAction.cpp",
    "source/GameData.cpp",
    "source/GameEvent.cpp",
    "source/GameLoadingPanel.cpp",
    "source/Gamerules.cpp",
    "source/GameWindow.cpp",
    "source/Government.cpp",
    "source/HailPanel.cpp",
    "source/Hardpoint.cpp",
    "source/Hazard.cpp",
    "source/HiringPanel.cpp",
    "source/ImageBuffer.cpp",
    "source/ImageSet.cpp",
    "source/InfoPanelState.cpp",
    "source/Information.cpp",
    "source/Interface.cpp",
    "source/ItemInfoDisplay.cpp",
    "source/Logger.cpp",
    "source/LineShader.cpp",
    "source/LoadPanel.cpp",
    "source/LocationFilter.cpp",
    "source/LogbookPanel.cpp",
    "source/MainPanel.cpp",
    "source/MapDetailPanel.cpp",
    "source/MapOutfitterPanel.cpp",
    "source/MapPanel.cpp",
    "source/MapPlanetCard.cpp",
    "source/MapSalesPanel.cpp",
    "source/MapShipyardPanel.cpp",
    "source/Mask.cpp",
    "source/MaskManager.cpp",
    "source/MenuAnimationPanel.cpp",
    "source/MenuPanel.cpp",
    "source/Messages.cpp",
    "source/Minable.cpp",
    "source/Mission.cpp",
    "source/MissionAction.cpp",
    "source/MissionPanel.cpp",
    "source/Mortgage.cpp",
    "source/Music.cpp",
    "source/NPC.cpp",
    "source/NPCAction.cpp",
    "source/News.cpp",
    "source/Outfit.cpp",
    "source/OutfitInfoDisplay.cpp",
    "source/OutfitterPanel.cpp",
    "source/OutlineShader.cpp",
    "source/Panel.cpp",
    "source/Person.cpp",
    "source/Personality.cpp",
    "source/Phrase.cpp",
    "source/Planet.cpp",
    "source/PlanetLabel.cpp",
    "source/PlanetPanel.cpp",
    "source/PlayerInfo.cpp",
    "source/PlayerInfoPanel.cpp",
    "source/Plugins.cpp",
    "source/Point.cpp",
    "source/PointerShader.cpp",
    "source/Politics.cpp",
    "source/Preferences.cpp",
    "source/PreferencesPanel.cpp",
    "source/PrintData.cpp",
    "source/Projectile.cpp",
    "source/Radar.cpp",
    "source/RaidFleet.cpp",
    "source/Random.cpp",
    "source/Rectangle.cpp",
    "source/RingShader.cpp",
    "source/SavedGame.cpp",
    "source/Screen.cpp",
    "source/Shader.cpp",
    "source/Ship.cpp",
    "source/ShipEvent.cpp",
    "source/ShipInfoDisplay.cpp",
    "source/ShipInfoPanel.cpp",
    "source/ShipJumpNavigation.cpp",
    "source/ShipManager.cpp",
    "source/ShipyardPanel.cpp",
    "source/ShopPanel.cpp",
    "source/Sound.cpp",
    "source/SpaceportPanel.cpp",
    "source/Sprite.cpp",
    "source/SpriteQueue.cpp",
    "source/SpriteSet.cpp",
    "source/SpriteShader.cpp",
    "source/StarField.cpp",
    "source/StartConditions.cpp",
    "source/StartConditionsPanel.cpp",
    "source/StellarObject.cpp",
    "source/System.cpp",
    "source/Test.cpp",
    "source/TestContext.cpp",
    "source/TestData.cpp",
    "source/TextReplacements.cpp",
    "source/Trade.cpp",
    "source/TradingPanel.cpp",
    "source/UI.cpp",
    "source/UniverseObjects.cpp",
    "source/Variant.cpp",
    "source/Visual.cpp",
    "source/Weapon.cpp",
    "source/Weather.cpp",
    "source/Wormhole.cpp",
    "source/opengl.cpp",
    "source/ship/ShipAICache.cpp",
    "source/text/DisplayText.cpp",
    "source/text/Font.cpp",
    "source/text/FontSet.cpp",
    "source/text/Format.cpp",
    "source/text/Table.cpp",
    "source/text/Utf8.cpp",
    "source/text/WrappedText.cpp",
};

const link_libraries = [_][]const u8{
    "SDL2",
    "glew32",
    "OpenAL32",
    "jpeg",
    "libpng16",
    // "OpenGL32",
    "mad",
    "turbojpeg",
    "zlib",
};

pub fn build(b: *std.Build) !void {
    const opt_level = b.standardOptimizeOption(.{});
    const target = b.standardTargetOptions(.{});

    try vcpkg_bootstrap.bootstrap_vcpkg(b, .{});

    // try vcpkg_bootstrap.install_vcpkg(b, .{
    //     .target = target,
    //     .manifest_features = &.{
    //         "system-libs",
    //     },
    // });

    const create_lib = b.step("ESLib", "Creates the Endless-Sky dylib");

    const lib = b.addStaticLibrary(.{
        .name = "endless-sky-lib",
        .optimize = opt_level,
        .target = target,
    });
    create_lib.dependOn(&lib.step);
    // lib.setVerboseCC(true);
    // lib.setVerboseLink(true);

    for (source_files) |file| {
        const lib2 = b.addStaticLibrary(.{
            .name = std.fs.path.basename(file),
            .optimize = opt_level,
            .target = target,
        });
        create_lib.dependOn(&lib2.step);

        lib2.addCSourceFile(.{
            .file = .{ .cwd_relative = file },
            .flags = &.{
                "-Wall",
                "-std=c++17",
            },
        });
        lib2.linkLibCpp();
        try lib2.addVcpkgPaths(.dynamic);

        lib.linkLibrary(lib2);
    }

    lib.linkLibCpp();
    try lib.addVcpkgPaths(.dynamic);
    for (link_libraries) |library| {
        lib.linkSystemLibrary(library);
    }

    const exe = b.addExecutable(.{
        .name = "endless-sky",
        .optimize = opt_level,
        .target = target,
    });
    try exe.addVcpkgPaths(.dynamic);
    exe.step.dependOn(&lib.step);
    // exe.bundle_compiler_rt = false;
    exe.addCSourceFile(.{
        .file = .{ .cwd_relative = "source/main.cpp" },
        .flags = &.{
            "-Wall",
            "-std=c++17",
        },
    });
    exe.linkLibrary(lib);
    exe.linkLibCpp();
    exe.linkLibC();
    exe.setVerboseLink(true);

    b.default_step.dependOn(&exe.step);
    b.installArtifact(lib);
    b.installArtifact(exe);

    // const use_vcpkg = true;
    // const use_gles = false;
    // _ = use_gles;
    // const flatpak = false;
    // const steam = false;
    // const system_libs = false;

    // if (use_vcpkg) {
    //     var vcpkg_manifest_features = std.ArrayList([]const u8).init(b.allocator);
    //     if (!system_libs) {
    //         try vcpkg_manifest_features.append("system-libs");
    //     }

    //     if (flatpak) {
    //         try vcpkg_manifest_features.append("flatpack-libs");
    //     } else if (steam) {
    //         try vcpkg_manifest_features.append("steam-libs");
    //     } else if (target.getOsTag().isDarwin()) {
    //         try vcpkg_manifest_features.append("macos-libs");
    //     }

    // }

    // const link_libs = [_][]const u8{
    //     "SDL2",
    //     "glew32",
    //     "OpenAL32",
    //     "jpeg",
    //     "libpng16",
    //     "OpenGL32",
    //     "mad",
    //     "turbojpeg",
    //     "zlib",
    // };

    // const exe = b.addExecutable(.{
    //     .name = "endless-sky",
    //     .target = target,
    //     .optimize = opt_level,
    // });
    // exe.strip = (opt_level != .Debug) and (opt_level != .ReleaseSafe);
    // for (link_libs) |lib| {
    //     exe.linkSystemLibrary(lib);
    // }
    // exe.linkLibCpp();

    // exe.addCSourceFiles(
    //     &.{
    //         "source/main.cpp",
    //         "source/AI.cpp",
    //         "source/Account.cpp",
    //         "source/AlertLabel.cpp",
    //         "source/AmmoDisplay.cpp",
    //         "source/Angle.cpp",
    //         "source/Armament.cpp",
    //         "source/AsteroidField.cpp",
    //         "source/Audio.cpp",
    //         "source/BankPanel.cpp",
    //         "source/BatchDrawList.cpp",
    //         "source/BatchShader.cpp",
    //         "source/Bitset.cpp",
    //         "source/BoardingPanel.cpp",
    //         "source/Body.cpp",
    //         "source/CaptureOdds.cpp",
    //         "source/CargoHold.cpp",
    //         "source/CategoryList.cpp",
    //         "source/CollisionSet.cpp",
    //         "source/Color.cpp",
    //         "source/Command.cpp",
    //         "source/ConditionSet.cpp",
    //         "source/ConditionsStore.cpp",
    //         "source/Conversation.cpp",
    //         "source/ConversationPanel.cpp",
    //         "source/CoreStartData.cpp",
    //         "source/DamageProfile.cpp",
    //         "source/DataFile.cpp",
    //         "source/DataNode.cpp",
    //         "source/DataWriter.cpp",
    //         "source/Date.cpp",
    //         "source/Depreciation.cpp",
    //         "source/Dialog.cpp",
    //         "source/Dictionary.cpp",
    //         "source/DistanceCalculationSettings.cpp",
    //         "source/DistanceMap.cpp",
    //         "source/Distribution.cpp",
    //         "source/DrawList.cpp",
    //         "source/Effect.cpp",
    //         "source/Engine.cpp",
    //         "source/EsUuid.cpp",
    //         "source/EscortDisplay.cpp",
    //         "source/File.cpp",
    //         "source/Files.cpp",
    //         "source/FillShader.cpp",
    //         "source/FireCommand.cpp",
    //         "source/Fleet.cpp",
    //         "source/FleetCargo.cpp",
    //         "source/Flotsam.cpp",
    //         "source/FogShader.cpp",
    //         "source/FormationPattern.cpp",
    //         "source/FrameTimer.cpp",
    //         "source/Galaxy.cpp",
    //         "source/GameAction.cpp",
    //         "source/GameData.cpp",
    //         "source/GameEvent.cpp",
    //         "source/GameLoadingPanel.cpp",
    //         "source/Gamerules.cpp",
    //         "source/GameWindow.cpp",
    //         "source/Government.cpp",
    //         "source/HailPanel.cpp",
    //         "source/Hardpoint.cpp",
    //         "source/Hazard.cpp",
    //         "source/HiringPanel.cpp",
    //         "source/ImageBuffer.cpp",
    //         "source/ImageSet.cpp",
    //         "source/InfoPanelState.cpp",
    //         "source/Information.cpp",
    //         "source/Interface.cpp",
    //         "source/ItemInfoDisplay.cpp",
    //         "source/Logger.cpp",
    //         "source/LineShader.cpp",
    //         "source/LoadPanel.cpp",
    //         "source/LocationFilter.cpp",
    //         "source/LogbookPanel.cpp",
    //         "source/MainPanel.cpp",
    //         "source/MapDetailPanel.cpp",
    //         "source/MapOutfitterPanel.cpp",
    //         "source/MapPanel.cpp",
    //         "source/MapPlanetCard.cpp",
    //         "source/MapSalesPanel.cpp",
    //         "source/MapShipyardPanel.cpp",
    //         "source/Mask.cpp",
    //         "source/MaskManager.cpp",
    //         "source/MenuAnimationPanel.cpp",
    //         "source/MenuPanel.cpp",
    //         "source/Messages.cpp",
    //         "source/Minable.cpp",
    //         "source/Mission.cpp",
    //         "source/MissionAction.cpp",
    //         "source/MissionPanel.cpp",
    //         "source/Mortgage.cpp",
    //         "source/Music.cpp",
    //         "source/NPC.cpp",
    //         "source/NPCAction.cpp",
    //         "source/News.cpp",
    //         "source/Outfit.cpp",
    //         "source/OutfitInfoDisplay.cpp",
    //         "source/OutfitterPanel.cpp",
    //         "source/OutlineShader.cpp",
    //         "source/Panel.cpp",
    //         "source/Person.cpp",
    //         "source/Personality.cpp",
    //         "source/Phrase.cpp",
    //         "source/Planet.cpp",
    //         "source/PlanetLabel.cpp",
    //         "source/PlanetPanel.cpp",
    //         "source/PlayerInfo.cpp",
    //         "source/PlayerInfoPanel.cpp",
    //         "source/Plugins.cpp",
    //         "source/Point.cpp",
    //         "source/PointerShader.cpp",
    //         "source/Politics.cpp",
    //         "source/Preferences.cpp",
    //         "source/PreferencesPanel.cpp",
    //         "source/PrintData.cpp",
    //         "source/Projectile.cpp",
    //         "source/Radar.cpp",
    //         "source/RaidFleet.cpp",
    //         "source/Random.cpp",
    //         "source/Rectangle.cpp",
    //         "source/RingShader.cpp",
    //         "source/SavedGame.cpp",
    //         "source/Screen.cpp",
    //         "source/Shader.cpp",
    //         "source/Ship.cpp",
    //         "source/ShipEvent.cpp",
    //         "source/ShipInfoDisplay.cpp",
    //         "source/ShipInfoPanel.cpp",
    //         "source/ShipJumpNavigation.cpp",
    //         "source/ShipManager.cpp",
    //         "source/ShipyardPanel.cpp",
    //         "source/ShopPanel.cpp",
    //         "source/Sound.cpp",
    //         "source/SpaceportPanel.cpp",
    //         "source/Sprite.cpp",
    //         "source/SpriteQueue.cpp",
    //         "source/SpriteSet.cpp",
    //         "source/SpriteShader.cpp",
    //         "source/StarField.cpp",
    //         "source/StartConditions.cpp",
    //         "source/StartConditionsPanel.cpp",
    //         "source/StellarObject.cpp",
    //         "source/System.cpp",
    //         "source/Test.cpp",
    //         "source/TestContext.cpp",
    //         "source/TestData.cpp",
    //         "source/TextReplacements.cpp",
    //         "source/Trade.cpp",
    //         "source/TradingPanel.cpp",
    //         "source/UI.cpp",
    //         "source/UniverseObjects.cpp",
    //         "source/Variant.cpp",
    //         "source/Visual.cpp",
    //         "source/Weapon.cpp",
    //         "source/Weather.cpp",
    //         "source/Wormhole.cpp",
    //         "source/opengl.cpp",
    //         "source/ship/ShipAICache.cpp",
    //         "source/text/DisplayText.cpp",
    //         "source/text/Font.cpp",
    //         "source/text/FontSet.cpp",
    //         "source/text/Format.cpp",
    //         "source/text/Table.cpp",
    //         "source/text/Utf8.cpp",
    //         "source/text/WrappedText.cpp",
    //     },
    //     &.{
    //         "-Wall",
    //         "-std=c++11",
    //     },
    // );

    // if (use_vcpkg) {
    //     b.vcpkg_root = .{ .found = try std.fs.path.join(b.allocator, &.{
    //         try std.process.getCwdAlloc(b.allocator),
    //         "vcpkg",
    //     }) };

    //     try exe.addVcpkgPaths(.dynamic);
    // }

    // b.default_step.dependOn(&exe.step);
    // b.installArtifact(exe);

    // const run_cmd = b.addRunArtifact(exe);
    // run_cmd.step.dependOn(b.getInstallStep());
    // if (b.args) |args| {
    //     run_cmd.addArgs(args);
    // }

    // const run_step = b.step("run", "Run the app");
    // run_step.dependOn(&run_cmd.step);
}
