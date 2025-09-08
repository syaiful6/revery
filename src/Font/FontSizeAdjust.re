module Log = (val Revery_Core.Log.withNamespace("Revery.FontSizeAdjust"));

type adjustmentMethod =
  | IdeographWidth
  | ExHeight
  | CapHeight
  | LineHeight;

let calculateScaleFactor =
    (
      ~primaryMetrics: FontMetrics.t,
      ~fallbackMetrics: FontMetrics.t,
      ~primarySize: float,
      ~fallbackSize: float,
    ) => {
  let tryAdjustment = (_method, getPrimaryValue, getFallbackValue) => {
    let primaryValue = getPrimaryValue(primaryMetrics, primarySize);
    let fallbackValue = getFallbackValue(fallbackMetrics, fallbackSize);

    if (primaryValue > 0.0 && fallbackValue > 0.0) {
      let primaryNormalized = primaryValue /. primarySize;
      let fallbackNormalized = fallbackValue /. fallbackSize;

      if (fallbackNormalized > 0.0) {
        let factor = primaryNormalized /. fallbackNormalized;
        Some(factor);
      } else {
        None;
      };
    } else {
      None;
    };
  };

  let ideographWidthAdjustment =
    tryAdjustment(
      IdeographWidth,
      (metrics, _size) => metrics.avgCharWidth,
      (metrics, _size) => metrics.avgCharWidth,
    );

  let exHeightAdjustment =
    tryAdjustment(
      ExHeight,
      (metrics, _size) => {metrics.xHeight},
      (metrics, _size) => {metrics.xHeight},
    );

  let capHeightAdjustment =
    tryAdjustment(
      CapHeight,
      (metrics, _size) => metrics.capHeight,
      (metrics, _size) => metrics.capHeight,
    );

  let lineHeightAdjustment =
    tryAdjustment(
      LineHeight,
      (metrics, _size) => metrics.lineHeight,
      (metrics, _size) => metrics.lineHeight,
    );

  // Filter out unreasonable scale factors (outside 0.5-2.0 range)
  let isReasonableFactor = factor => factor >= 0.5 && factor <= 2.0;

  let reasonableIdeographWidth =
    switch (ideographWidthAdjustment) {
    | Some(factor) when isReasonableFactor(factor) => Some(factor)
    | _ => None
    };

  let reasonableExHeight =
    switch (exHeightAdjustment) {
    | Some(factor) when isReasonableFactor(factor) => Some(factor)
    | _ => None
    };

  let reasonableCapHeight =
    switch (capHeightAdjustment) {
    | Some(factor) when isReasonableFactor(factor) => Some(factor)
    | _ => None
    };

  let reasonableLineHeight =
    switch (lineHeightAdjustment) {
    | Some(factor) when isReasonableFactor(factor) => Some(factor)
    | _ => None
    };

  switch (
    reasonableIdeographWidth,
    reasonableExHeight,
    reasonableCapHeight,
    reasonableLineHeight,
  ) {
  | (Some(factor), _, _, _) => factor
  | (None, Some(factor), _, _) => factor
  | (None, None, Some(factor), _) => factor
  | (None, None, None, Some(factor)) => factor
  | (None, None, None, None) =>
    Log.debugf(m =>
      m("No valid metrics found for font size adjustment, using 1.0")
    );
    1.0;
  };
};

let adjustFallbackFont =
    (~primaryFont: FontCache.t, ~fallbackFont: FontCache.t, ~size: float) => {
  let primaryMetrics = FontCache.getMetrics(primaryFont, size);
  let fallbackMetrics = FontCache.getMetrics(fallbackFont, size);

  let scaleFactor =
    calculateScaleFactor(
      ~primaryMetrics,
      ~fallbackMetrics,
      ~primarySize=size,
      ~fallbackSize=size,
    );

  let adjustedSize = size *. scaleFactor;

  (adjustedSize, scaleFactor);
};
