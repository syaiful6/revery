open Revery_Core;

type measureResult = {
  width: float,
  height: float,
};

// Get scale factor for a specific typeface relative to primary font
let getScaleFactorForTypeface = (~primaryFont, ~typeface, ~size) => {
  let primaryTypeface = FontCache.getSkiaTypeface(primaryFont);
  let isUsingFallback = !Skia.Typeface.equal(typeface, primaryTypeface);

  if (isUsingFallback) {
    switch (FontCache.load(Some(typeface))) {
    | Ok(fallbackFont) =>
      let (_, scaleFactor) =
        FontSizeAdjust.adjustFallbackFont(~primaryFont, ~fallbackFont, ~size);
      scaleFactor;
    | Error(_) => 1.0
    };
  } else {
    1.0;
  };
};

let measure =
    (~smoothing: Smoothing.t, ~features=[], font, size, text: string) => {
  let {height, _}: FontMetrics.t = FontCache.getMetrics(font, size);

  let shapes = FontCache.shape(~features, font, text);

  let width =
    List.fold_left(
      (acc, run: ShapeResult.shapedRun) => {
        // Apply font size adjustment for fallback fonts
        let typeface = ShapeResult.resolveFont(run.textRun);
        let scaleFactor =
          getScaleFactorForTypeface(~primaryFont=font, ~typeface, ~size);
        let effectiveSize = size *. scaleFactor;

        List.fold_left(
          (acc2: float, node: ShapeResult.shapeNode) =>
            acc2 +. node.xAdvance *. effectiveSize /. node.unitsPerEm,
          acc,
          run.nodes,
        );
      },
      0.,
      shapes,
    );
  {
    height,
    width,
  };
};

// Legacy measure function without size adjustment (for backward compatibility)
let measureWithoutAdjustment =
    (~smoothing: Smoothing.t, ~features=[], font, size, text: string) => {
  let {height, _}: FontMetrics.t = FontCache.getMetrics(font, size);

  let shapes = FontCache.shape(~features, font, text);

  let width =
    List.fold_left(
      (acc, run: ShapeResult.shapedRun) =>
        List.fold_left(
          (acc2: float, node: ShapeResult.shapeNode) =>
            acc2 +. node.xAdvance *. size /. node.unitsPerEm,
          acc,
          run.nodes,
        ),
      0.,
      shapes,
    );
  {
    height,
    width,
  };
};
