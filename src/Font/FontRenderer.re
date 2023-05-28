type measureResult = {
  width: float,
  height: float,
};

let measure = {
  let paint = Skia.Paint.make();
  let skiaFont = Skia.Font.make();
  //Skia.Paint.setTextEncoding(paint, GlyphId);

  (~smoothing: Smoothing.t, ~features=[], font, size, text: string) => {
    let {height, _}: FontMetrics.t = FontCache.getMetrics(font, size);
    let glyphStrings =
      text |> FontCache.shape(~features, font) |> ShapeResult.getGlyphStrings;

    //Smoothing.setPaint(~smoothing, paint);
    Skia.Font.setSize(skiaFont, size);
    // Skia.Paint.setTextSize(paint, size);

    let width =
      glyphStrings
      |> List.fold_left(
           (acc, (skiaFace, str)) => {
             Skia.Font.setTypeface(skiaFont, skiaFace);
             acc +. Skia.Font.measureText(~encoding=GlyphId, ~paint=paint, skiaFont, str, ());
           },
           0.,
         );
    {height, width};
  };
};
