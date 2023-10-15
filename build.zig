const std = @import("std");
const vcpkg_bootstrap = @import("utils/vcpkg_bootstrap.zig");

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

const BuildSettings = struct {
    target: std.zig.CrossTarget,
    opt_mode: std.builtin.OptimizeMode,
    use_vcpkg: bool = false,
    use_system_libraries: bool = false,
    flatpak: bool = false,
    steam: bool = false,
    apple: bool = false,
    gles: bool = false,
    bundle: bool = false,
};

pub fn getBuildSettings(b: *std.Build) BuildSettings {
    const opt_level = b.standardOptimizeOption(.{});
    const target = b.standardTargetOptions(.{});
    var settings = BuildSettings{
        .use_system_libraries = !target.isWindows() and target.isNative(),
        .use_vcpkg = b.host.target.os.tag == std.Target.Os.Tag.windows or !target.isNative(),
        .apple = target.isDarwin(),
        .opt_mode = opt_level,
        .target = target,
    };
    return settings;
}

pub fn build(b: *std.Build) !void {
    const settings = getBuildSettings(b);

    const link_libraries = [_][]const u8{
        "SDL2",
        if (settings.target.isWindows()) "glew32" else "glew",
        if (settings.target.isWindows()) "OpenAL32" else "OpenAL",
        "jpeg",
        if (settings.target.isWindows()) "libpng16" else "png16",
        "mad",
        "jpeg",
        "zlib",
        "uuid",
    };

    const use_vcpkg = settings.use_vcpkg;
    std.log.info("Settings: {}", .{settings});

    if (use_vcpkg) try vcpkg_bootstrap.bootstrap_vcpkg(b, .{});
    if (use_vcpkg) try vcpkg_bootstrap.install_vcpkg(b, .{
        .manifest_features = &.{
            if (!settings.use_system_libraries) "system-libs" else "",
            if (settings.flatpak) "flatpak-libs" else if (settings.steam) "steam-libs" else if (settings.apple) "macos-libs" else "",
        },
        .additional_arguments = &.{"--overlay-triplets=./overlays"},
        .target = try settings.target.vcpkgTriplet(b.allocator, .Dynamic),
    });

    const create_lib = b.step("check", "Only builds the static library, without building and linking the executable. Use to check for compiler errors.");

    const lib = b.addStaticLibrary(.{
        .name = "endless-sky-lib",
        .optimize = settings.opt_mode,
        .target = settings.target,
    });
    create_lib.dependOn(&lib.step);
    if (use_vcpkg) try lib.addVcpkgPaths(.dynamic);

    for (source_files) |file| {
        const lib2 = b.addStaticLibrary(.{
            .name = std.fs.path.basename(file),
            .optimize = settings.opt_mode,
            .target = settings.target,
        });
        create_lib.dependOn(&lib2.step);

        if (use_vcpkg) try lib2.addVcpkgPaths(.dynamic);
        lib2.addCSourceFile(.{
            .file = .{ .cwd_relative = file },
            .flags = &.{
                "-Wall",
                "-std=c++17",
            },
        });
        lib2.linkLibCpp();

        lib.linkLibrary(lib2);
    }

    for (link_libraries) |library| {
        lib.linkSystemLibrary(library);
    }

    const exe = b.addExecutable(.{
        .name = "endless-sky",
        .optimize = settings.opt_mode,
        .target = settings.target,
    });
    if (use_vcpkg) try exe.addVcpkgPaths(.dynamic);
    exe.step.dependOn(&lib.step);
    exe.addCSourceFile(.{
        .file = .{ .cwd_relative = "source/main.cpp" },
        .flags = &.{
            "-Wall",
            "-std=c++17",
        },
    });
    exe.linkLibrary(lib);

    b.default_step.dependOn(&exe.step);
    b.installArtifact(exe);

    const run_step = b.step("run", "Run endless-sky");
    const run = b.addRunArtifact(exe);
    run_step.dependOn(&run.step);
}
