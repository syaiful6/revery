type t;

module Fallback: {
  /* [strategy] encapsulates the fallback logic for discovering a font, based on a character [Uchar.t] */
  type strategy;

  // [none] never falls back to a font (always fails)
  let none: strategy;

  // [constant(typeface)] always falls back to [typeface]
  let constant: Skia.Typeface.t => strategy;

  // [skia(font)] uses skia's [matchFamilyStyleCharacter] API to fall-back
  let skia: t => strategy;

  // [custom] provides a custom matching strategy
  let custom: (Uchar.t => option(Skia.Typeface.t)) => strategy;
};

let load: option(Skia.Typeface.t) => result(t, string);

let getMetrics: (t, float) => FontMetrics.t;

let getSkiaTypeface: t => Skia.Typeface.t;

let createTextRun:
  (~text: string, ~font: t, ~features: list(Feature.t)) => ShapeResult.textRun;

let shape:
  (~fallback: Fallback.strategy=?, ~features: list(Feature.t)=?, t, string) =>
  ShapeResult.t;

let onFontLoaded: Revery_Core.Event.t(unit);
