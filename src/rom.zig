//! ROM = A compressed xbin
// TODO: finish interface (main)
const std = @import("std");

// TODO: make proper error handling
pub fn create_rom(filename: []const u8) void {
    const xbin = std.fs.cwd().openFile(filename, .{}) catch @panic("could not open xbin");
    // TODO: unhardcode max read size
    const xbin_content = xbin.readToEndAlloc(std.heap.page_allocator, 1024*1024*10) catch @panic("could not read xbin");
    defer std.heap.page_allocator.free(xbin_content);

    var buf = std.ArrayList(u8).init(std.heap.page_allocator);
    defer buf.deinit();
    
    var cmp = std.compress.zlib.compressor(buf.writer(), .{}) catch @panic("could not create compressor");
    _ = cmp.write(xbin_content) catch {};
    cmp.finish() catch {};

    // TODO: make rom name settable
    const output = std.fs.cwd().createFile("output.rom", .{}) catch @panic("could not open output rom");
    defer output.close();
    output.writeAll(buf.items) catch @panic("could not write rom");
}

pub fn load_rom(filename: []const u8) void {
    const rom = std.fs.cwd().openFile(filename, .{}) catch @panic("could not open rom");
    var buf = std.ArrayList(u8).init(std.heap.page_allocator);
    defer buf.deinit();
    
    var dcmp = std.compress.zlib.decompressor(rom.reader());
    dcmp.decompress(buf.writer()) catch @panic("could not decompress rom");

    // TODO: make bin name settable
    const output = std.fs.cwd().createFile("output.xbin", .{}) catch @panic("could not open output xbin");
    defer output.close();
    output.writeAll(buf.items) catch @panic("could not write rom");
}

pub fn main() !u8 {
    const args: [][:0]u8 = std.process.argsAlloc(std.heap.page_allocator) catch @panic("could not allocate args");
    if (args.len != 3) @panic("expected 2 arguments: mode and input");
    const mode: []u8 = args[1];
    const input: []u8 = args[2];
    
    if (std.mem.eql(u8, mode, "c")) {
        create_rom(input);
    } else {
        load_rom(input); 
    }

    return 0;
}

