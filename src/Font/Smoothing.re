type t =
  | None
  | Antialiased
  | SubpixelAntialiased;

// Default to subpixel-antialiased, as it has the most reliable
// scaling characteristics - see Onivim 2 bugs:
// - https://github.com/onivim/oni2/issues/1475
// - https://github.com/onivim/oni2/issues/1592
let default = SubpixelAntialiased;

let setPaint = (~smoothing: t, font: Skia.Font.t, paint: Skia.Paint.t) => {
  switch (smoothing) {
  | None =>
    Skia.Paint.setAntiAlias(paint, false);
    Skia.Font.setSubpixel(font, false);
  | Antialiased =>
    Skia.Paint.setAntiAlias(paint, true);
    Skia.Font.setSubpixel(font, false);
  | SubpixelAntialiased =>
    Skia.Paint.setAntiAlias(paint, true);
    Skia.Font.setSubpixel(font, true);
  };
};
