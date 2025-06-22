module Configurator = Configurator.V1;

let getenv = name =>
  try(Sys.getenv(name)) {
  | Not_found => failwith("Error: Undefined environment variable: " ++ name)
  };

let find_xcode_sysroot = sdk => {
  let ic = Unix.open_process_in("xcrun --sdk " ++ sdk ++ " --show-sdk-path");
  let path = input_line(ic);
  close_in(ic);
  path;
};

type os =
  | Android
  | IOS
  | Linux
  | Mac
  | Windows;

let detect_system_header = {|
  #if __APPLE__
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
      #define PLATFORM_NAME "ios"
    #else
      #define PLATFORM_NAME "mac"
    #endif
  #elif __linux__
    #if __ANDROID__
      #define PLATFORM_NAME "android"
    #else
      #define PLATFORM_NAME "linux"
    #endif
  #elif WIN32
    #define PLATFORM_NAME "windows"
  #endif
|};

let sdl2FilePath = Sys.getenv("SDL2_LIB_PATH") ++ "/libSDL2.a";

let get_os = t => {
  let header = {
    let file = Filename.temp_file("discover", "os.h");
    let fd = open_out(file);
    output_string(fd, detect_system_header);
    close_out(fd);
    file;
  };
  let platform =
    Configurator.C_define.import(
      t,
      ~includes=[header],
      [("PLATFORM_NAME", String)],
    );
  switch (platform) {
  | [(_, String("android"))] => Android
  | [(_, String("ios"))] => IOS
  | [(_, String("linux"))] => Linux
  | [(_, String("mac"))] => Mac
  | [(_, String("windows"))] => Windows
  | _ => failwith("Unknown operating system")
  };
};

let ccopt = s => ["-ccopt", s];
let cclib = s => ["-cclib", s];
let framework = s => ["-framework", s];
let flags = os =>
  switch (os) {
  | Android =>
    []
    @ ["-verbose"]
    @ cclib("-lfreetype")
    @ cclib("-lz")
    @ cclib("-lsvg")
    @ cclib("-lskia")
    @ cclib("-lGLESv2")
    @ cclib("-lGLESv1_CM")
    @ cclib("-lm")
    @ cclib("-llog")
    @ cclib("-landroid")
    @ ccopt("-L" ++ getenv("FREETYPE2_LIB_PATH"))
    @ ccopt("-L" ++ getenv("SDL2_LIB_PATH"))
    @ ccopt("-L" ++ getenv("SKIA_LIB_PATH"))
    @ ccopt("-L" ++ getenv("JPEG_LIB_PATH"))
    @ ccopt("-I" ++ getenv("FREETYPE2_INCLUDE_PATH"))
    @ ccopt("-I" ++ getenv("SKIA_INCLUDE_PATH"))
    @ cclib("-ljpeg")
    @ ccopt("-I/usr/include")
    @ ccopt("-lstdc++")
  | IOS
  | Mac => [] @ cclib(sdl2FilePath)
  | Linux =>
    []
    @ ["-verbose"]
    @ cclib("-lfontconfig")
    @ cclib("-lfreetype")
    @ cclib("-lz")
    @ cclib("-lbz2")
    @ cclib("-lsvg")
    @ cclib("-lskia")
    @ cclib(sdl2FilePath)
    @ ccopt("-L" ++ getenv("FREETYPE2_LIB_PATH"))
    @ ccopt("-L" ++ getenv("SDL2_LIB_PATH"))
    @ ccopt("-L" ++ getenv("SKIA_LIB_PATH"))
    @ ccopt("-L" ++ getenv("JPEG_LIB_PATH"))
    @ ccopt("-I" ++ getenv("FREETYPE2_INCLUDE_PATH"))
    @ ccopt("-I" ++ getenv("SKIA_INCLUDE_PATH"))
    @ cclib("-ljpeg")
    @ ccopt("-I/usr/include")
    @ ccopt("-lstdc++")
    @ ccopt("-fPIC")
  | Windows =>
    []
    @ cclib("-lsvg")
    @ cclib("-lskia")
    @ cclib("-lSDL2")
    @ ccopt("-L" ++ getenv("SDL2_LIB_PATH"))
    @ ccopt("-L" ++ getenv("SKIA_LIB_PATH"))
  };

let skiaIncludeFlags = {
  let skiaIncludePath = getenv("SKIA_INCLUDE_PATH");
  ["-I" ++ skiaIncludePath]
  // Sys.readdir(skiaIncludePath)
  // |> Array.map(path => "-I" ++ skiaIncludePath ++ "/" ++ path)
  // |> Array.append([|"-I" ++ skiaIncludePath|])
  // |> Array.to_list;
};
let cflags = os => {
  switch (os) {
  | Android =>
    []
    @ [sdl2FilePath]
    @ ["-lGLESv2"]
    @ ["-lGLESv1_CM"]
    @ ["-lm"]
    @ ["-llog"]
    @ ["-landroid"]
    @ ["-lsvg"]
    @ ["-lskia"]
    @ ["-I" ++ getenv("SKIA_PREFIX_PATH")]
    @ ["-I" ++ getenv("SDL2_INCLUDE_PATH")]
    @ skiaIncludeFlags
    @ ["-L" ++ getenv("SKIA_LIB_PATH")]
    @ ["-L" ++ getenv("SDL2_LIB_PATH")]
    @ ["-L" ++ getenv("JPEG_LIB_PATH")]
    @ ["-lstdc++"]
    @ ["-ljpeg"]
  | Linux =>
    []
    @ [sdl2FilePath]
    @ ["-lsvg"]
    @ ["-lskia"]
    @ ["-I" ++ getenv("SKIA_PREFIX_PATH")]
    @ ["-I" ++ getenv("SDL2_INCLUDE_PATH")]
    @ skiaIncludeFlags
    @ ["-L" ++ getenv("SKIA_LIB_PATH")]
    @ ["-L" ++ getenv("SDL2_LIB_PATH")]
    @ ["-L" ++ getenv("JPEG_LIB_PATH")]
    @ ["-lstdc++"]
    @ ["-ljpeg"]
    @ ["-fPIC"]
  | IOS
  | Mac => {
      let sdk_path = find_xcode_sysroot("macosx");
      []
      @ ["-isysroot" ++ sdk_path]
      @ ["-I" ++ getenv("SDL2_INCLUDE_PATH"),]
      @ ["-I" ++ getenv("SKIA_PREFIX_PATH")]
      @ skiaIncludeFlags
  }
  | Windows =>
    []
    @ ["-std=c++1y"]
    @ ["-I" ++ getenv("SDL2_INCLUDE_PATH")]
    @ ["-L" ++ getenv("SKIA_LIB_PATH")]
    @ ["-lskia"]
    @ skiaIncludeFlags
  };
};

let libs = os =>
  switch (os) {
  | Android =>
    []
    @ [
      "-L" ++ getenv("SDL2_LIB_PATH"),
      "-L" ++ getenv("SKIA_LIB_PATH"),
      "-L" ++ getenv("FREETYPE2_LIB_PATH"),
      sdl2FilePath,
      "-lGLESv2",
      "-lGLESv1_CM",
      "-lm",
      "-llog",
      "-landroid",
      "-lsvg",
      "-lskia",
      "-lfreetype",
      "-lz",
      "-L" ++ getenv("JPEG_LIB_PATH"),
      "-ljpeg",
      "-lstdc++",
    ]
  | IOS =>
    []
    @ ["-L" ++ getenv("JPEG_LIB_PATH")]
    @ ["-L" ++ getenv("SKIA_LIB_PATH")]
    @ ["-L" ++ getenv("FREETYPE2_LIB_PATH")]
    @ ["-L" ++ getenv("SDL2_LIB_PATH")]
    @ framework("OpenGLES")
    @ framework("UIKit")
    @ framework("Foundation")
    @ framework("GameController")
    @ framework("AVFoundation")
    @ framework("QuartzCore")
    @ framework("CoreMotion")
    @ framework("CoreFoundation")
    @ framework("CoreAudio")
    @ framework("CoreVideo")
    @ framework("CoreServices")
    @ framework("CoreGraphics")
    @ framework("CoreText")
    @ framework("CoreFoundation")
    @ framework("AudioToolbox")
    @ framework("IOKit")
    @ framework("Metal")
    @ ["-liconv"]
    @ [sdl2FilePath]
    @ ["-lskia"]
    @ ["-lstdc++"]
    @ [getenv("JPEG_LIB_PATH") ++ "/libturbojpeg.a"]
  | Linux =>
    []
    @ [
      "-L" ++ getenv("SDL2_LIB_PATH"),
      "-L" ++ getenv("SKIA_LIB_PATH"),
      "-L" ++ getenv("FREETYPE2_LIB_PATH"),
      sdl2FilePath,
      "-lsvg",
      "-lskia",
      "-lfreetype",
      "-lfontconfig",
      "-lz",
      "-lbz2",
      "-L" ++ getenv("JPEG_LIB_PATH"),
      "-ljpeg",
      "-lpthread",
      "-lstdc++",
      "-fPIC",
    ]
  | Mac =>
    []
    @ ["-L" ++ getenv("JPEG_LIB_PATH")]
    @ ["-L" ++ getenv("SKIA_LIB_PATH")]
    @ ["-L" ++ getenv("FREETYPE2_LIB_PATH")]
    @ ["-L" ++ getenv("SDL2_LIB_PATH")]
    @ framework("Carbon")
    @ framework("Cocoa")
    @ framework("CoreFoundation")
    @ framework("CoreAudio")
    @ framework("CoreVideo")
    @ framework("CoreServices")
    @ framework("CoreGraphics")
    @ framework("CoreText")
    @ framework("CoreFoundation")
    @ framework("AudioToolbox")
    @ framework("ForceFeedback")
    @ framework("IOKit")
    @ framework("Metal")
    @ ["-liconv"]
    @ ["-lSDL2"]
    @ ["-lsvg"]
    @ ["-lskia"]
    @ ["-lstdc++"]
    @ [getenv("JPEG_LIB_PATH") ++ "/libturbojpeg.a"]
  | Windows =>
    []
    @ ["-luser32"]
    @ ["-lsvg"]
    @ ["-lskia"]
    @ ["-lSDL2"]
    @ ["-lstdc++"]
    @ ["-L" ++ getenv("SDL2_LIB_PATH")]
    @ ["-L" ++ getenv("SKIA_LIB_PATH")]
  };

Configurator.main(~name="reason-sdl2", conf => {
  let os = get_os(conf);
  Configurator.Flags.write_sexp("flags.sexp", flags(os));
  Configurator.Flags.write_lines("c_flags.txt", cflags(os));
  Configurator.Flags.write_sexp("c_flags.sexp", cflags(os));
  Configurator.Flags.write_sexp("c_library_flags.sexp", libs(os));
  Configurator.Flags.write_lines("c_library_flags.txt", libs(os));
  Configurator.Flags.write_sexp(
    "cclib_c_library_flags.sexp",
    libs(os) |> List.map(o => ["-cclib", o]) |> List.flatten,
  );
});
