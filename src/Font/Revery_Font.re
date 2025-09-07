/**
    Revery_Font.re

    Module exposing font-related functionality, like:
    - Discovering fonts
    - Loading fonts
*/
module Weight = FontWeight;
module Width = FontWidth;
module FontMetrics = FontMetrics;
module FontCache = FontCache;
module FontRenderer = FontRenderer;
module ShapeResult = ShapeResult;
module Smoothing = Smoothing;
module Family = FontFamily;
module Feature = Feature;
module Features = Features;
module SizeAdjust = FontSizeAdjust;

type t = FontCache.t;

type measureResult = FontRenderer.measureResult;

let load = FontCache.load;
let getMetrics = FontCache.getMetrics;
let getSkiaTypeface = FontCache.getSkiaTypeface;

// Main API functions now use size adjustment by default
let measure = FontRenderer.measure;
let shape = FontCache.shape;
let getScaleFactorForTypeface = FontRenderer.getScaleFactorForTypeface;

// Legacy functions without size adjustment (for backward compatibility if needed)
let measureWithoutAdjustment = FontRenderer.measureWithoutAdjustment;
let shapeWithoutAdjustment = FontCache.shape;

module Discovery = {
  type t = option(Skia.Typeface.t);

  let find = Discovery.find;
};
