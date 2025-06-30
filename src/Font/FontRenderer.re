type measureResult = {
  width: float,
  height: float,
};

let measure =
    (~smoothing: Smoothing.t, ~features=[], font, size, text: string) => {
  let {height, _}: FontMetrics.t = FontCache.getMetrics(font, size);
  let glyphStrings =
    text |> FontCache.shape(~features, font) |> ShapeResult.getGlyphStrings;

  let paint = Skia.Paint.make();
  let skiaFont = Skia.Font.make();

  Smoothing.setPaint(~smoothing, skiaFont, paint);
  Skia.Font.setSize(skiaFont, size);
  // Skia.Paint.setTextSize(paint, size);

  let width =
    glyphStrings
    |> List.fold_left(
         (acc, (skiaFace, str)) => {
           Skia.Font.setTypeface(skiaFont, skiaFace);
           acc
           +. Skia.Font.measureText(
                ~encoding=GlyphId,
                ~paint,
                skiaFont,
                str,
                (),
              );
         },
         0.,
       );
  {
    height,
    width,
  };
};
