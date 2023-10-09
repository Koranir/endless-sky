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
    
    const link_libs = [_][]const u8 {
        "SDL2",
        "glew32",
        "OpenAL32",
        "jpeg",
        "libpng16",
        "OpenGL32",
        "mad",
    };

    const es_lib = b.addStaticLibrary(.{
        .name = "endless-sky-lib",
        .optimize = opt_level,
        .target = target,
    });
    es_lib.addCSourceFiles(
        &.{
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
        },
        &.{
            "-Wall",
            "-std=c++14",
        },
    );
    es_lib.linkLibCpp();
    for (link_libs) |lib| {
        es_lib.linkSystemLibrary(lib);
    }

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
            "-std=c++14",
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
