type measureResult = {
  width: float,
  height: float,
};

let measure =
    (~smoothing: Smoothing.t, ~features=[], font, size, text: string) => {
  let {height, _}: FontMetrics.t = FontCache.getMetrics(font, size);

  let paint = Skia.Paint.make();
  let skiaFont = Skia.Font.make();

  Smoothing.setPaint(~smoothing, skiaFont, paint);
  Skia.Font.setSize(skiaFont, size);
  Skia.Font.setTypeface(skiaFont, FontCache.getSkiaTypeface(font));

  let width =
    Skia.Font.measureText(~paint, ~encoding=Utf8, skiaFont, text, ());

  // Skia.Paint.setTextSize(paint, size);
  {
    height,
    width,
  };
};
