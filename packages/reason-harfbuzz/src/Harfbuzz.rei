type hb_face;

type hb_shape = {
  glyphId: int,
  cluster: int,
  xAdvance: float,
  yAdvance: float,
  xOffset: float,
  yOffset: float,
  unitsPerEm: float,
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

// Font table tag utilities
module FontTable: {
  // Convert 4-character string to font table tag (int32)
  let tagFromString: string => int32;

  // Convert font table tag back to 4-character string
  let tagToString: int32 => string;

  // Common font table tags
  let head: int32;
  let hhea: int32;
  let hmtx: int32;
  let cmap: int32;
  let glyf: int32;
  let loca: int32;
  let name: int32;
  let os2: int32;
  let post: int32;
  let maxp: int32;

  // Color font tables
  let colr: int32;
  let cpal: int32;
  let cbdt: int32;
  let cblc: int32;
  let sbix: int32;
};

let hb_face_from_path: string => result(hb_face, string);
let hb_face_from_data: string => result(hb_face, string);

[@ocaml.deprecated "Deprecated in favor of hb_face_from_path"]
let hb_new_face: string => result(hb_face, string);

let hb_shape:
  (
    ~features: list(feature)=?,
    ~start: position=?,
    ~stop: position=?,
    hb_face,
    string
  ) =>
  array(hb_shape);

let hb_version_string_compiled: unit => string;
let hb_version_string_runtime: unit => string;
let hb_face_from_memory_ptr: (nativeint, int, int) => result(hb_face, string);
let hb_face_create_for_tables:
  ((int32, 'a) => option(string), 'a) => result(hb_face, string);
