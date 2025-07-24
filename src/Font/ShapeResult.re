type shapeNode = {
  glyphId: int,
  cluster: int,
  xAdvance: float,
  yAdvance: float,
  xOffset: float,
  yOffset: float,
  unitsPerEm: float,
};

type textRun = {
  text: string,
  face: Skia.Typeface.t,
  features: list(Feature.t),
};

type shapedRun = {
  textRun,
  nodes: list(shapeNode),
};

type t = list(shapedRun);

let resolveFont = (textRun: textRun) => textRun.face;
