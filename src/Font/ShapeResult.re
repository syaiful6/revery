type shapeNode = {
  skiaFace: Skia.Typeface.t,
  glyphId: int,
  cluster: int,
  xAdvance: float,
  yAdvance: float,
  xOffset: float,
  yOffset: float,
  unitsPerEm: float,
};

type t = list(shapeNode);