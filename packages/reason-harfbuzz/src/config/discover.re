module Configurator = Configurator.V1;

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

let c_flags = ["-fPIC", "-I", Sys.getenv("HARFBUZZ_INCLUDE_PATH")];

let ccopt = s => ["-ccopt", s];
let cclib = s => ["-cclib", s];

let cLibraryFlags = os => {
  let extraFlags =
    switch (os) {
    | Windows => ["-lpthread"]
    | Linux => ["-L/usr/lib"]
    | Mac
    | IOS => [
        "-framework",
        "CoreFoundation",
        "-framework",
        "CoreGraphics",
        "-framework",
        "CoreText",
      ]
    | _ => []
    };

  ["-L" ++ Sys.getenv("HARFBUZZ_LIB_PATH")] @ ["-lharfbuzz"] @ extraFlags;
};

let flags = os => {
  [] @ (cLibraryFlags(os) |> List.map(cclib) |> List.flatten);
};

let cxx_flags = os =>
  switch (os) {
  | Windows => c_flags @ ["-fno-exceptions", "-fno-rtti", "-lstdc++"]
  | _ => c_flags
  };

Configurator.main(~name="reason-harfbuzz", conf => {
  let os = get_os(conf);
  Configurator.Flags.write_sexp("c_flags.sexp", c_flags);
  Configurator.Flags.write_sexp("cxx_flags.sexp", cxx_flags(os));
  Configurator.Flags.write_sexp("flags.sexp", flags(os));
  Configurator.Flags.write_sexp("c_library_flags.sexp", cLibraryFlags(os));
});
