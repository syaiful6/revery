type hb_shape = {
  glyphId: int,
  cluster: int,
  xAdvance: float,
  yAdvance: float,
  xOffset: float,
  yOffset: float,
  unitsPerEm: float,
};

// Font table tag utilities
module FontTable = {
  // Convert 4-character string to font table tag (int32)
  let tagFromString = (str: string): int32 => {
    let len = String.length(str);
    if (len != 4) {
      failwith("Font table tag must be exactly 4 characters");
    };
    let a = Int32.of_int(Char.code(str.[0]));
    let b = Int32.of_int(Char.code(str.[1]));
    let c = Int32.of_int(Char.code(str.[2]));
    let d = Int32.of_int(Char.code(str.[3]));
    Int32.logor(
      Int32.logor(Int32.shift_left(a, 24), Int32.shift_left(b, 16)),
      Int32.logor(Int32.shift_left(c, 8), d),
    );
  };

  // Convert font table tag back to 4-character string
  let tagToString = (tag: int32): string => {
    let a = Int32.to_int(Int32.shift_right_logical(tag, 24)) land 0xFF;
    let b = Int32.to_int(Int32.shift_right_logical(tag, 16)) land 0xFF;
    let c = Int32.to_int(Int32.shift_right_logical(tag, 8)) land 0xFF;
    let d = Int32.to_int(tag) land 0xFF;
    String.make(1, Char.chr(a))
    ++ String.make(1, Char.chr(b))
    ++ String.make(1, Char.chr(c))
    ++ String.make(1, Char.chr(d));
  };

  // Common font table tags
  let head = tagFromString("head");
  let hhea = tagFromString("hhea");
  let hmtx = tagFromString("hmtx");
  let cmap = tagFromString("cmap");
  let glyf = tagFromString("glyf");
  let loca = tagFromString("loca");
  let name = tagFromString("name");
  let os2 = tagFromString("OS/2");
  let post = tagFromString("post");
  let maxp = tagFromString("maxp");

  // Color font tables
  let colr = tagFromString("COLR");
  let cpal = tagFromString("CPAL");
  let cbdt = tagFromString("CBDT");
  let cblc = tagFromString("CBLC");
  let sbix = tagFromString("sbix");
};

module Internal = {
  type face;
  type feature = {
    tag: string,
    value: int,
    start: int,
    stop: int,
  };
  external hb_face_from_path: string => result(face, string) =
    "rehb_face_from_path";
  external hb_face_from_data: (string, int) => result(face, string) =
    "rehb_face_from_bytes";
  external hb_face_from_memory_ptr:
    (nativeint, int, int) => result(face, string) =
    "rehb_face_from_memory_ptr";
  external hb_face_create_for_tables:
    ((int32, 'a) => option(string), 'a) => result(face, string) =
    "rehb_face_create_for_tables";
  external hb_shape:
    (face, string, array(feature), int, int) => array(hb_shape) =
    "rehb_shape";

  // hb-version
  external hb_version_string_compiled: unit => string =
    "rehb_version_string_compiled";
  external hb_version_string_runtime: unit => string =
    "rehb_version_string_runtime";
};

type position = [
  | `Start
  | `End
  | `Position(int)
];
type feature = {
  tag: string,
  value: int,
  start: position,
  stop: position,
};

type hb_face = {face: Internal.face};

let positionToInt = position =>
  switch (position) {
  | `Position(n) => n
  | `Start => 0
  | `End => (-1)
  };

let hb_face_from_path = str => {
  switch (Internal.hb_face_from_path(str)) {
  | Error(msg) => Error(msg)
  | Ok(face) =>
    let ret = {face: face};

    Ok(ret);
  };
};

let hb_shape = (~features=[], ~start=`Start, ~stop=`End, {face}, str) => {
  let arr =
    features
    |> List.map(
         feat => {
           tag: feat.tag,
           value: feat.value,
           start: positionToInt(feat.start),
           stop: positionToInt(feat.stop),
         }: feature => Internal.feature,
       )
    |> Array.of_list;
  let startPosition = positionToInt(start);
  let length =
    switch (stop) {
    | `Position(n) => n - startPosition
    | `Start => 0
    | `End => (-1)
    };

  Internal.hb_shape(face, str, arr, startPosition, length);
};
let hb_new_face = str => hb_face_from_path(str);

let hb_face_from_data = bytes => {
  switch (Internal.hb_face_from_data(bytes, String.length(bytes))) {
  | Error(_) as e => e
  | Ok(face) =>
    let ret = {face: face};
    Ok(ret);
  };
};

let hb_face_from_memory_ptr = (memoryPtr, length, index) => {
  switch (Internal.hb_face_from_memory_ptr(memoryPtr, length, index)) {
  | Error(_) as e => e
  | Ok(face) =>
    let ret = {face: face};
    Ok(ret);
  };
};

// Table-based font creation for efficient emoji fonts
// The callback receives a font table tag (as int32) and user data,
// and should return Some(table_data) or None
let hb_face_create_for_tables = (callback, userData) => {
  switch (Internal.hb_face_create_for_tables(callback, userData)) {
  | Error(_) as e => e
  | Ok(face) =>
    let ret = {face: face};
    Ok(ret);
  };
};

let hb_version_string_compiled = Internal.hb_version_string_compiled;
let hb_version_string_runtime = Internal.hb_version_string_runtime;
