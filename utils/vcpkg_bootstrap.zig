const std = @import("std");

pub const BootstrapConfig = struct {
    /// The current working directory. If not supplied here, it will allocate with std.process.getCwdAlloc
    cwd: ?[]const u8 = null,
    /// Directory of the vcpkg executable. Relative to the cwd. Defaults to [cwd]/vcpkg
    vcpkg_dir: []const u8 = "vcpkg",
    /// Path to the git executable. Defaults to finding from $PATH.
    git_executable: []const u8 = "git",
};
pub const InstallConfig = struct {
    /// The current working directory. If not supplied here, it will allocate with std.process.getCwdAlloc
    cwd: ?[]const u8 = null,
    /// Directory of the vcpkg executable. Relative to the cwd. Defaults to [cwd]/vcpkg
    vcpkg_dir: []const u8 = "vcpkg",
    vcpkg_install_dir: []const u8 = "vcpkg/installed",
    target: std.zig.CrossTarget,
    manifest_features: []const []const u8 = &.{},
    default_features: bool = true,
    additional_arguments: []const []const u8 = &.{},
};

fn join_p(b: *std.Build, s1: []const u8, s2: []const u8) ![]const u8 {
    return std.fs.path.join(b.allocator, &.{ s1, s2 });
}
fn join_s(b: *std.Build, s1: []const u8, s2: []const u8) ![]const u8 {
    return std.mem.join(b.allocator, "", &.{ s1, s2 });
}
fn spawn_and_pipe(b: *std.Build, args: []const []const u8, cwd: []const u8, stdout: ?std.fs.File, stderr: ?std.fs.File) !void {
    var proc = std.process.Child.init(args, b.allocator);
    proc.stdout = stdout orelse std.io.getStdOut();
    proc.stderr = stderr orelse std.io.getStdErr();
    proc.cwd = cwd;
    const term = try proc.spawnAndWait();
    switch (term) {
        .Exited => |code| {
            if (code != 0) {
                return error.Exited;
            }
        },
        // TODO: Handle Everything Else
        else => {},
    }
}
fn exists(path: []const u8) !bool {
    if (std.fs.accessAbsolute(path, .{})) return true else |e| {
        switch (e) {
            error.FileNotFound => return false,
            else => return e,
        }
    }
}

/// Returns true if a reconfiguration is needed.
pub fn bootstrap_vcpkg(b: *std.Build, cfg: BootstrapConfig) !void {
    const cwd = cfg.cwd orelse try std.process.getCwdAlloc(b.allocator);

    const vcpkg_github_url = "https://github.com/Microsoft/vcpkg";
    const vcpkg_dir = try join_p(b, cwd, cfg.vcpkg_dir);

    const vcpkg_git_path = try join_p(b, vcpkg_dir, ".git");
    const needs_clone = !try exists(vcpkg_git_path);
    if (needs_clone) {
        std.log.info("Bootstrap: Cloning Vcpkg into {s}", .{vcpkg_dir});
        try spawn_and_pipe(b, &.{
            cfg.git_executable,
            "clone",
            vcpkg_github_url,
            vcpkg_dir,
        }, cwd, null, null);
    }

    const res = try std.process.Child.exec(.{
        .allocator = b.allocator,
        .argv = &.{ cfg.git_executable, "rev-parse", "HEAD" },
        .cwd = vcpkg_dir,
    });
    const current_sha = res.stdout[0..(res.stdout.len - 1)];

    const vcpkg_config_json_file = try std.fs.openFileAbsolute(try join_p(b, cwd, "vcpkg-configuration.json"), .{ .mode = std.fs.File.OpenMode.read_only });
    defer vcpkg_config_json_file.close();
    const vcpkg_config_json = try vcpkg_config_json_file.readToEndAlloc(b.allocator, std.math.maxInt(usize));
    const parsed_json = try std.json.parseFromSliceLeaky(struct {
        @"default-registry": struct {
            kind: []const u8,
            baseline: []const u8,
        },
        @"overlay-triplets": []const []const u8,
    }, b.allocator, vcpkg_config_json, .{});
    const baseline_sha = parsed_json.@"default-registry".baseline;

    if (!std.mem.eql(u8, baseline_sha, current_sha)) {
        std.log.info("Bootstrap: Fetching changes from Vcpkg upstream", .{});
        try spawn_and_pipe(b, &.{
            cfg.git_executable,
            "fetch",
            "--prune",
        }, vcpkg_dir, null, null);
        std.log.info("Bootstrap: Switching from commit {s} to {s}", .{ current_sha, baseline_sha });
        try spawn_and_pipe(b, &.{
            cfg.git_executable,
            "reset",
            "--hard",
            baseline_sha,
        }, vcpkg_dir, null, null);
        try spawn_and_pipe(b, &.{
            cfg.git_executable,
            "clean",
            "qfd",
            baseline_sha,
        }, vcpkg_dir, null, null);
    }

    const vcpkg_exe_path = try join_p(b, vcpkg_dir, if (b.host.target.os.tag == .windows) "vcpkg.exe" else "vcpkg");
    if (!try exists(vcpkg_exe_path)) {
        try spawn_and_pipe(b, &.{try join_p(b, vcpkg_dir, "bootstrap-vcpkg")}, cwd, null, null);
    }
}

pub fn install_vcpkg(b: *std.Build, cfg: InstallConfig) !void {
    const cwd = cfg.cwd orelse try std.process.getCwdAlloc(b.allocator);
    const vcpkg_dir = try join_p(b, cwd, cfg.vcpkg_dir);

    const vcpkg_exe_path = try join_p(b, vcpkg_dir, if (b.host.target.os.tag == .windows) "vcpkg.exe" else "vcpkg");

    var vcpkg_args = std.ArrayList([]const u8).init(b.allocator);
    try vcpkg_args.appendSlice(&.{ vcpkg_exe_path, "install" });

    for (cfg.manifest_features) |feat| {
        try vcpkg_args.append(try std.fmt.allocPrint(b.allocator, "--x-feature={s}", .{feat}));
    }

    if (!cfg.default_features) {
        try vcpkg_args.append("--x-no-default-features");
    }

    try vcpkg_args.append(std.fs.path.join(b.allocator, &.{
        cwd,
        cfg.vcpkg_install_dir,
    }));

    vcpkg_args.appendSlice(cfg.additional_arguments);

    const triplet = try cfg.target.vcpkgTriplet(b.allocator, .Dynamic);
    std.log.info("Building for triplet: {s}", .{triplet});
    try vcpkg_args.append(try std.fmt.allocPrint(b.allocator, "--triplet={s}", .{triplet}));

    std.log.info("Running Vcpkg Install", .{});
    try spawn_and_pipe(b, vcpkg_args.items, cwd, null, null);
}
