const std = @import("std");

fn doVcpkg(
    b: *std.Build,
    git: []const u8,
    cwd: []const u8,
    manifest_features: []const []const u8,
    override_vcpkg_install: ?bool,
) !void {
    var should_install = false;
    
    // TODO: Fix Lol, Use std.fmt.format() instead of arraylists
    const vcpkg_github_url = "https://github.com/Microsoft/vcpkg";
    const vcpkg_clone_into = try std.fs.path.join(b.allocator, &.{
        cwd,
        "vcpkg",
    });
    std.log.info("Vcpkg Clone Dir: {s}", .{vcpkg_clone_into});

    const git_path = try std.fs.path.join(b.allocator, &.{
        vcpkg_clone_into,
        ".git",
    });
    var vcpkg_needs_install = false;
    std.fs.accessAbsolute(git_path, .{}) catch |err| {
        if (err == error.FileNotFound) {
            should_install = true;
            vcpkg_needs_install = true;
        } else {
            std.log.err("Unexpected filesystem error while accessing {s} : {!}", .{ git_path, err });
            return err;
        }
    };
    std.log.debug("Vcpkg Needs Install: {}", .{vcpkg_needs_install});

    if (vcpkg_needs_install) {
        std.log.info("Cloning Vcpkg into {s}", .{vcpkg_clone_into});
        const vcpkg_result = b.exec(&.{ git, "clone", "--quiet", vcpkg_github_url, vcpkg_clone_into });
        std.log.info("Git: {s}", .{vcpkg_result});
    }

    const res = try std.process.Child.exec(.{
        .allocator = b.allocator,
        .argv = &.{ git, "rev-parse", "HEAD" },
        .cwd = vcpkg_clone_into,
    });
    const baseline_sha: []const u8 = res.stdout[0 .. res.stdout.len - 1];
    std.log.info("Vcpkg HEAD SHA: {s}", .{baseline_sha});
    const vcpkg_config_json_file = try std.fs.openFileAbsolute(try std.fs.path.join(b.allocator, &.{
        cwd,
        "vcpkg-configuration.json",
    }), .{ .mode = std.fs.File.OpenMode.read_only });
    defer vcpkg_config_json_file.close();
    const max_bytes = std.math.maxInt(usize);
    const vcpkg_config_json = try vcpkg_config_json_file.readToEndAlloc(b.allocator, max_bytes);

    const parsed_json = try std.json.parseFromSliceLeaky(struct {
        @"default-registry": struct {
            kind: []const u8,
            baseline: []const u8,
        },
        @"overlay-triplets": []const []const u8,
    }, b.allocator, vcpkg_config_json, .{});
    std.log.info("Current SHA: {s}", .{parsed_json.@"default-registry".baseline});
    if (std.mem.eql(u8, baseline_sha, parsed_json.@"default-registry".baseline)) {
        std.log.info("Vcpkg Up-to-Date", .{});
    } else {
        should_install = true;
        std.log.warn("Differing Stuff:\n{s}\n{s}", .{ baseline_sha, parsed_json.@"default-registry".baseline });
        std.log.info("Fetching changes from Vcpkg upstream", .{});
        const fetch_res = try std.process.Child.exec(.{
            .allocator = b.allocator,
            .argv = &.{ git, "fetch", "--quiet", "--prune" },
            .cwd = vcpkg_clone_into,
        });
        std.log.info("Git: {s}", .{fetch_res.stdout});
        std.log.info("Switching from vcpkg HEAD to {s}", .{parsed_json.@"default-registry".baseline});
        const switch_res = try std.process.Child.exec(.{
            .allocator = b.allocator,
            .argv = &.{ git, "reset", "--quiet", "--hard", parsed_json.@"default-registry".baseline },
            .cwd = vcpkg_clone_into,
        });
        std.log.info("Git: {s}", .{switch_res.stdout});
        const clean_res = try std.process.Child.exec(.{
            .allocator = b.allocator,
            .argv = &.{ git, "clean", "qfd" },
            .cwd = vcpkg_clone_into,
        });
        std.log.info("Git: {s}", .{clean_res.stdout});
    }

    const vcpkg_exe_path = try std.fs.path.join(b.allocator, &.{
        vcpkg_clone_into,
        if (b.host.target.os.tag == std.Target.Os.Tag.windows) "vcpkg.exe" else "vcpkg",
    });
    var vcpkg_needs_bootstrap = false;
    std.fs.accessAbsolute(vcpkg_exe_path, .{}) catch |err| {
        if (err == error.FileNotFound) {
            vcpkg_needs_bootstrap = true;
            should_install = true;
        } else {
            std.log.err("Unexpected filesystem error while accessing {s} : {!}", .{ git_path, err });
            return err;
        }
    };

    if (vcpkg_needs_bootstrap) {
        std.log.info("Bootstrap-Vcpkg: {s}", .{b.exec(&.{
            try std.fs.path.join(b.allocator, &.{
                vcpkg_clone_into,
                "bootstrap-vcpkg",
            }),
        })});
    }
    
    if (override_vcpkg_install) |override| {
        should_install = override;
    }
    if (should_install) {
        var exec_args = std.ArrayList([]const u8).init(b.allocator);
        try exec_args.appendSlice(&.{ vcpkg_exe_path, "install", "--x-install-root", "vcpkg/installed", "--no-print-usage" });
        for (manifest_features) |feat| {
            const feat_with_prefix = try std.mem.join(b.allocator, "", &.{
                "--x-feature=",
                feat,
            });
            try exec_args.append(feat_with_prefix);
        }
    
        var vcpkg_install_process = std.process.Child.init(exec_args.items, b.allocator);
        vcpkg_install_process.cwd = cwd;
        vcpkg_install_process.stdout = std.io.getStdOut();
        vcpkg_install_process.stderr = std.io.getStdErr();
        _ = try vcpkg_install_process.spawnAndWait();
    
        std.log.info("Vcpkg: {s}", .{b.exec(exec_args.items)});   
    }

    return;
}

pub fn build(b: *std.Build) !void {
    const opt_level = b.standardOptimizeOption(.{});
    const target = b.standardTargetOptions(.{});

    const use_vcpkg = true;
    const use_gles = false;
    _ = use_gles;
    const flatpak = false;
    const steam = false;
    const system_libs = false;

    if (use_vcpkg) {
        var vcpkg_manifest_features = std.ArrayList([]const u8).init(b.allocator);
        if (!system_libs) {
            try vcpkg_manifest_features.append("system-libs");
        }
    
        if (flatpak) {
            try vcpkg_manifest_features.append("flatpack-libs");
        } else if (steam) {
            try vcpkg_manifest_features.append("steam-libs");
        } else if (target.getOsTag().isDarwin()) {
            try vcpkg_manifest_features.append("macos-libs");
        }
        
        try doVcpkg(b, "git", try std.process.getCwdAlloc(b.allocator), vcpkg_manifest_features.items, null);
    }

    const es_lib = b.addStaticLibrary(.{
        .name = "endless-sky-lib",
        .optimize = opt_level,
        .target = target,
    });
    es_lib.addCSourceFiles(
        &.{
            "source/AI.cpp",
            "source/AI.h",
            "source/Account.cpp",
            "source/Account.h",
            "source/AlertLabel.cpp",
            "source/AlertLabel.h",
            "source/AmmoDisplay.cpp",
            "source/AmmoDisplay.h",
            "source/Angle.cpp",
            "source/Angle.h",
            "source/Armament.cpp",
            "source/Armament.h",
            "source/AsteroidField.cpp",
            "source/AsteroidField.h",
            "source/Audio.cpp",
            "source/Audio.h",
            "source/BankPanel.cpp",
            "source/BankPanel.h",
            "source/BatchDrawList.cpp",
            "source/BatchDrawList.h",
            "source/BatchShader.cpp",
            "source/BatchShader.h",
            "source/Bitset.cpp",
            "source/Bitset.h",
            "source/BoardingPanel.cpp",
            "source/BoardingPanel.h",
            "source/Body.cpp",
            "source/Body.h",
            "source/CaptureOdds.cpp",
            "source/CaptureOdds.h",
            "source/CargoHold.cpp",
            "source/CargoHold.h",
            "source/CategoryList.cpp",
            "source/CategoryList.h",
            "source/CategoryTypes.h",
            "source/ClickZone.h",
            "source/CollisionSet.cpp",
            "source/CollisionSet.h",
            "source/Color.cpp",
            "source/Color.h",
            "source/Command.cpp",
            "source/Command.h",
            "source/ConditionSet.cpp",
            "source/ConditionSet.h",
            "source/ConditionsStore.cpp",
            "source/ConditionsStore.h",
            "source/Conversation.cpp",
            "source/Conversation.h",
            "source/ConversationPanel.cpp",
            "source/ConversationPanel.h",
            "source/CoreStartData.cpp",
            "source/CoreStartData.h",
            "source/DamageDealt.h",
            "source/DamageProfile.cpp",
            "source/DamageProfile.h",
            "source/DataFile.cpp",
            "source/DataFile.h",
            "source/DataNode.cpp",
            "source/DataNode.h",
            "source/DataWriter.cpp",
            "source/DataWriter.h",
            "source/Date.cpp",
            "source/Date.h",
            "source/Depreciation.cpp",
            "source/Depreciation.h",
            "source/Dialog.cpp",
            "source/Dialog.h",
            "source/Dictionary.cpp",
            "source/Dictionary.h",
            "source/DistanceCalculationSettings.h",
            "source/DistanceCalculationSettings.cpp",
            "source/DistanceMap.cpp",
            "source/DistanceMap.h",
            "source/Distribution.cpp",
            "source/Distribution.h",
            "source/DrawList.cpp",
            "source/DrawList.h",
            "source/Effect.cpp",
            "source/Effect.h",
            "source/Engine.cpp",
            "source/Engine.h",
            "source/EsUuid.cpp",
            "source/EsUuid.h",
            "source/EscortDisplay.cpp",
            "source/EscortDisplay.h",
            "source/ExclusiveItem.h",
            "source/File.cpp",
            "source/File.h",
            "source/Files.cpp",
            "source/Files.h",
            "source/FillShader.cpp",
            "source/FillShader.h",
            "source/FireCommand.cpp",
            "source/FireCommand.h",
            "source/Fleet.cpp",
            "source/Fleet.h",
            "source/FleetCargo.cpp",
            "source/FleetCargo.h",
            "source/Flotsam.cpp",
            "source/Flotsam.h",
            "source/FogShader.cpp",
            "source/FogShader.h",
            "source/FormationPattern.cpp",
            "source/FormationPattern.h",
            "source/FrameTimer.cpp",
            "source/FrameTimer.h",
            "source/Galaxy.cpp",
            "source/Galaxy.h",
            "source/GameAction.cpp",
            "source/GameAction.h",
            "source/GameData.cpp",
            "source/GameData.h",
            "source/GameEvent.cpp",
            "source/GameEvent.h",
            "source/GameLoadingPanel.cpp",
            "source/GameLoadingPanel.h",
            "source/Gamerules.cpp",
            "source/Gamerules.h",
            "source/GameWindow.cpp",
            "source/GameWindow.h",
            "source/Government.cpp",
            "source/Government.h",
            "source/HailPanel.cpp",
            "source/HailPanel.h",
            "source/Hardpoint.cpp",
            "source/Hardpoint.h",
            "source/Hazard.cpp",
            "source/Hazard.h",
            "source/HiringPanel.cpp",
            "source/HiringPanel.h",
            "source/ImageBuffer.cpp",
            "source/ImageBuffer.h",
            "source/ImageSet.cpp",
            "source/ImageSet.h",
            "source/InfoPanelState.cpp",
            "source/InfoPanelState.h",
            "source/Information.cpp",
            "source/Information.h",
            "source/Interface.cpp",
            "source/Interface.h",
            "source/ItemInfoDisplay.cpp",
            "source/ItemInfoDisplay.h",
            "source/JumpTypes.h",
            "source/Logger.cpp",
            "source/Logger.h",
            "source/LineShader.cpp",
            "source/LineShader.h",
            "source/LoadPanel.cpp",
            "source/LoadPanel.h",
            "source/LocationFilter.cpp",
            "source/LocationFilter.h",
            "source/LogbookPanel.cpp",
            "source/LogbookPanel.h",
            "source/MainPanel.cpp",
            "source/MainPanel.h",
            "source/MapDetailPanel.cpp",
            "source/MapDetailPanel.h",
            "source/MapOutfitterPanel.cpp",
            "source/MapOutfitterPanel.h",
            "source/MapPanel.cpp",
            "source/MapPanel.h",
            "source/MapPlanetCard.cpp",
            "source/MapPlanetCard.h",
            "source/MapSalesPanel.cpp",
            "source/MapSalesPanel.h",
            "source/MapShipyardPanel.cpp",
            "source/MapShipyardPanel.h",
            "source/Mask.cpp",
            "source/Mask.h",
            "source/MaskManager.cpp",
            "source/MaskManager.h",
            "source/MenuAnimationPanel.cpp",
            "source/MenuAnimationPanel.h",
            "source/MenuPanel.cpp",
            "source/MenuPanel.h",
            "source/Messages.cpp",
            "source/Messages.h",
            "source/Minable.cpp",
            "source/Minable.h",
            "source/Mission.cpp",
            "source/Mission.h",
            "source/MissionAction.cpp",
            "source/MissionAction.h",
            "source/MissionPanel.cpp",
            "source/MissionPanel.h",
            "source/Mortgage.cpp",
            "source/Mortgage.h",
            "source/Music.cpp",
            "source/Music.h",
            "source/NPC.cpp",
            "source/NPC.h",
            "source/NPCAction.cpp",
            "source/NPCAction.h",
            "source/News.cpp",
            "source/News.h",
            "source/Outfit.cpp",
            "source/Outfit.h",
            "source/OutfitInfoDisplay.cpp",
            "source/OutfitInfoDisplay.h",
            "source/OutfitterPanel.cpp",
            "source/OutfitterPanel.h",
            "source/OutlineShader.cpp",
            "source/OutlineShader.h",
            "source/Panel.cpp",
            "source/Panel.h",
            "source/Person.cpp",
            "source/Person.h",
            "source/Personality.cpp",
            "source/Personality.h",
            "source/Phrase.cpp",
            "source/Phrase.h",
            "source/Planet.cpp",
            "source/Planet.h",
            "source/PlanetLabel.cpp",
            "source/PlanetLabel.h",
            "source/PlanetPanel.cpp",
            "source/PlanetPanel.h",
            "source/PlayerInfo.cpp",
            "source/PlayerInfo.h",
            "source/PlayerInfoPanel.cpp",
            "source/PlayerInfoPanel.h",
            "source/Plugins.cpp",
            "source/Plugins.h",
            "source/Point.cpp",
            "source/Point.h",
            "source/PointerShader.cpp",
            "source/PointerShader.h",
            "source/Politics.cpp",
            "source/Politics.h",
            "source/Preferences.cpp",
            "source/Preferences.h",
            "source/PreferencesPanel.cpp",
            "source/PreferencesPanel.h",
            "source/PrintData.cpp",
            "source/PrintData.h",
            "source/Projectile.cpp",
            "source/Projectile.h",
            "source/Radar.cpp",
            "source/Radar.h",
            "source/RaidFleet.cpp",
            "source/RaidFleet.h",
            "source/Random.cpp",
            "source/Random.h",
            "source/RandomEvent.h",
            "source/Rectangle.cpp",
            "source/Rectangle.h",
            "source/RingShader.cpp",
            "source/RingShader.h",
            "source/Sale.h",
            "source/SavedGame.cpp",
            "source/SavedGame.h",
            "source/Screen.cpp",
            "source/Screen.h",
            "source/Set.h",
            "source/Shader.cpp",
            "source/Shader.h",
            "source/Ship.cpp",
            "source/Ship.h",
            "source/ShipEvent.cpp",
            "source/ShipEvent.h",
            "source/ShipInfoDisplay.cpp",
            "source/ShipInfoDisplay.h",
            "source/ShipInfoPanel.cpp",
            "source/ShipInfoPanel.h",
            "source/ShipJumpNavigation.cpp",
            "source/ShipJumpNavigation.h",
            "source/ShipManager.cpp",
            "source/ShipManager.h",
            "source/ShipyardPanel.cpp",
            "source/ShipyardPanel.h",
            "source/ShopPanel.cpp",
            "source/ShopPanel.h",
            "source/Sound.cpp",
            "source/Sound.h",
            "source/SpaceportPanel.cpp",
            "source/SpaceportPanel.h",
            "source/Sprite.cpp",
            "source/Sprite.h",
            "source/SpriteQueue.cpp",
            "source/SpriteQueue.h",
            "source/SpriteSet.cpp",
            "source/SpriteSet.h",
            "source/SpriteShader.cpp",
            "source/SpriteShader.h",
            "source/StarField.cpp",
            "source/StarField.h",
            "source/StartConditions.cpp",
            "source/StartConditions.h",
            "source/StartConditionsPanel.cpp",
            "source/StartConditionsPanel.h",
            "source/StellarObject.cpp",
            "source/StellarObject.h",
            "source/System.cpp",
            "source/System.h",
            "source/SystemEntry.h",
            "source/Test.cpp",
            "source/Test.h",
            "source/TestContext.cpp",
            "source/TestContext.h",
            "source/TestData.cpp",
            "source/TestData.h",
            "source/TextReplacements.cpp",
            "source/TextReplacements.h",
            "source/Trade.cpp",
            "source/Trade.h",
            "source/TradingPanel.cpp",
            "source/TradingPanel.h",
            "source/UI.cpp",
            "source/UI.h",
            "source/UniverseObjects.cpp",
            "source/UniverseObjects.h",
            "source/Variant.cpp",
            "source/Variant.h",
            "source/Visual.cpp",
            "source/Visual.h",
            "source/Weapon.cpp",
            "source/Weapon.h",
            "source/Weather.cpp",
            "source/Weather.h",
            "source/WeightedList.h",
            "source/Wormhole.cpp",
            "source/Wormhole.h",
            "source/WormholeStrategy.h",
            "source/opengl.cpp",
            "source/opengl.h",
            "source/pi.h",
            "source/shift.h",
            "source/comparators/ByGivenOrder.h",
            "source/comparators/ByName.h",
            "source/comparators/BySeriesAndIndex.h",
            "source/ship/ShipAICache.cpp",
            "source/ship/ShipAICache.h",
            "source/text/DisplayText.cpp",
            "source/text/DisplayText.h",
            "source/text/Font.cpp",
            "source/text/Font.h",
            "source/text/FontSet.cpp",
            "source/text/FontSet.h",
            "source/text/Format.cpp",
            "source/text/Format.h",
            "source/text/Table.cpp",
            "source/text/Table.h",
            "source/text/Utf8.cpp",
            "source/text/Utf8.h",
            "source/text/WrappedText.cpp",
            "source/text/WrappedText.h",
            "source/text/alignment.h",
            "source/text/layout.h",
            "source/text/truncate.h",
        },
        &.{
            "-Wall",
            "-Werror",
            "-std=c++17",
        },
    );
    es_lib.linkLibCpp();

    const exe = b.addExecutable(.{
        .name = "endless-sky",
        .target = target,
        .optimize = opt_level,
    });
    exe.strip = (opt_level != .Debug) and (opt_level != .ReleaseSafe);

    exe.addCSourceFile(.{
        .file = .{ .cwd_relative = "source/main.cpp" },
        .flags = &.{
            "-Wall",
            "-Werror",
            "-std=c++17",
        },
    });
    exe.linkLibrary(es_lib);

    if (use_vcpkg) {
        b.vcpkg_root = .{ .found = try std.fs.path.join(b.allocator, &.{
            try std.process.getCwdAlloc(b.allocator),
            "vcpkg",
        }) };

        try exe.addVcpkgPaths(.dynamic);
        try es_lib.addVcpkgPaths(.dynamic);

        for (exe.include_dirs.items) |item| {
            switch (item) {
                .path => |path| {
                    std.debug.print("Path: {s}\n", .{path.getDisplayName()});
                },
                else => {},
            }
        }
    }
    exe.linkLibCpp();

    b.default_step.dependOn(&exe.step);
    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);

    // const dir = "source/";

    // const use_system_libs = false;

    // const exe = b.addExecutable(.{
    //     .name = "endless-sky",
    //     .target = target,
    //     .optimize = opt_level,
    // });
    // exe.linkLibCpp();
    // exe.strip = (opt_level != .Debug) and (opt_level != .ReleaseSafe);
    // exe.addIncludePath(.{
    //     .cwd_relative = dir,
    // });
    // exe.addCSourceFile(.{
    //     .file = .{
    //         .cwd_relative = dir ++ "main.cpp",
    //     },
    //     .flags = &[_][]const u8{
    //         "-std=c++17",
    //     },
    // });

    // if (!use_system_libs) {
    //     b.vcpkg_root = .{ .found = try std.fs.path.join(b.allocator, &.{
    //         try std.process.getCwdAlloc(b.allocator),
    //         "vcpkg",
    //     }) };
    //     try std.io.getStdOut().writeAll(b.exec(&.{
    //         try std.fs.path.join(b.allocator, &.{
    //             b.vcpkg_root.found,
    //             "vcpkg",
    //         }),
    //         "install",
    //     }));
    //     exe.addVcpkgPaths(.dynamic) catch return error.VcpkgNotFound;
    //     if (exe.vcpkg_bin_path) |path| {
    //         const sdl_path = try std.fs.path.join(
    //             b.allocator,
    //             &.{ path, "SDL2.dll" },
    //         );
    //         if (target.isWindows()) {
    //             b.installBinFile(sdl_path, "SDL2.dll");
    //         } else {
    //             b.installBinFile(sdl_path, "SDL2.so");
    //         }
    //     }
    // }
    // exe.linkSystemLibrary("SDL2");

    // b.default_step.dependOn(&exe.step);
}
