open Revery_Core;

module Log = (val Revery_Core.Log.withNamespace("Revery.FontCache"));

module StringFeaturesHashable = {
  type t = (string, list(Feature.t));
  let equal = ((str1, features1), (str2, features2)) =>
    String.equal(str1, str2) && features1 == features2;
  let hash = Hashtbl.hash;
};

module SkiaTypefaceIDHashable = {
  type t = option(int32);
  let equal = Option.equal(Int32.equal);
  let hash = maybeTypefaceID =>
    switch (maybeTypefaceID) {
    | Some(id) => Int32.to_int(id)
    | None => 0
    };
};

module UcharHashable = {
  type t = Uchar.t;
  let equal = Uchar.equal;
  let hash = Uchar.hash;
};

module FloatHashable = {
  type t = float;
  let equal = Float.equal;
  let hash = Float.hash;
};
module StringHash =
  Hashtbl.Make({
    type t = string;
    let equal = String.equal;
    let hash = Hashtbl.hash;
  });

type fontLoaded = Event.t(unit);
let onFontLoaded: fontLoaded = Event.create();

module MetricsWeighted = {
  type t = FontMetrics.t;
  let weight = _ => 1;
};

module ShapeResultWeighted = {
  type t = ShapeResult.t;

  let weight = _ => 1;
};

module FallbackWeighted = {
  type t = list(ShapeResult.shapeNode);
  let weight = _ => 1;
};

module FontIdWeighted = {
  type t = option(int32);
  let weight = _ => 1;
};

module MetricsCache = Lru.M.Make(FloatHashable, MetricsWeighted);
module ShapeResultCache =
  Lru.M.Make(StringFeaturesHashable, ShapeResultWeighted);
module FallbackCache = Lru.M.Make(StringFeaturesHashable, FallbackWeighted);
module FallbackCharacterCache = Lru.M.Make(UcharHashable, FontIdWeighted);

type cacheItem = {
  metricsCache: MetricsCache.t,
  shapeCache: ShapeResultCache.t,
  fallbackCharacterCache: FallbackCharacterCache.t,
};

type t = {
  skiaFace: Skia.Typeface.t,
  metricsCache: MetricsCache.t,
  shapeCache: ShapeResultCache.t,
  fallbackCharacterCache: FallbackCharacterCache.t,
};

let ofCacheItem = (item: cacheItem, typeface: Skia.Typeface.t) => {
  skiaFace: typeface,
  metricsCache: item.metricsCache,
  shapeCache: item.shapeCache,
  fallbackCharacterCache: item.fallbackCharacterCache,
};

module FontWeight = {
  type font = cacheItem;
  type t = result(font, string);
  let weight = _ => 1;
};

module FontCache = Lru.M.Make(SkiaTypefaceIDHashable, FontWeight);
module HarfbuzzMap = Ephemeron.K1.Make(Int32);

module Internal = {
  let cache = FontCache.create(8);
  let harfbuzzCache = HarfbuzzMap.create(32);
};

module Constants = {
  let unresolvedGlyphID = 0;
  let defaultUnresolvedAdvance = 0.0;
  let defaultUnresolvedOffset = 0.0;
  let defaultUnresolvedUnitsPerEm = 1.0;
};

let skiaFaceToHarfbuzzFace = skiaFace => {
  switch (Skia.Typeface.toStream(skiaFace)) {
  | Some(asset) =>
    let bytes =
      Fun.protect(
        ~finally=() => {Skia.StreamAsset.delete(asset)},
        () => {
          let stream = Skia.StreamAsset.toStream(asset);
          let length = Skia.Stream.getLength(stream);
          Skia.Data.makeStringFromStream(stream, length);
        },
      );

    Harfbuzz.hb_face_from_data(bytes);
  | None => Result.Error("failed to convert skia typeface to stream")
  };
};

let load: option(Skia.Typeface.t) => result(t, string) =
  (skiaTypeface: option(Skia.Typeface.t)) => {
    let typefaceID = Option.map(Skia.Typeface.getUniqueID, skiaTypeface);
    switch (FontCache.find(typefaceID, Internal.cache)) {
    | Some(v) =>
      FontCache.promote(typefaceID, Internal.cache);
      v
      |> Result.map(cacheItem =>
           ofCacheItem(cacheItem, Option.get(skiaTypeface))
         );
    | None =>
      let metricsCache = MetricsCache.create(8);
      let shapeCache = ShapeResultCache.create(256);
      let fallbackCharacterCache = FallbackCharacterCache.create(256);

      let ret =
        switch (skiaTypeface) {
        | Some(skiaFace) =>
          Event.dispatch(onFontLoaded, ());
          Log.infof(m =>
            m("Loaded: %s", Skia.Typeface.getFamilyName(skiaFace))
          );
          Ok({
            metricsCache,
            shapeCache,
            fallbackCharacterCache,
          });
        | None =>
          Log.warn("Error loading typeface (skia)");
          Error("Error loading typeface.");
        };

      FontCache.add(typefaceID, ret, Internal.cache);
      FontCache.trim(Internal.cache);
      ret
      |> Result.map(cacheItem =>
           ofCacheItem(cacheItem, Option.get(skiaTypeface))
         );
    };
  };

let getSkiaTypeface: t => Skia.Typeface.t = ({skiaFace, _}) => skiaFace;

let getMetrics: (t, float) => FontMetrics.t =
  ({skiaFace, metricsCache, _}, size) => {
    switch (MetricsCache.find(size, metricsCache)) {
    | Some(v) =>
      MetricsCache.promote(size, metricsCache);
      v;
    | None =>
      let font = Skia.Font.make();
      Skia.Font.setTypeface(font, skiaFace);
      Skia.Font.setSize(font, size);

      let metrics = Skia.FontMetrics.make();
      let lineHeight = Skia.Font.getFontMetrics(font, metrics);

      let ret = FontMetrics.ofSkia(size, lineHeight, metrics);
      MetricsCache.add(size, ret, metricsCache);
      MetricsCache.trim(metricsCache);
      ret;
    };
  };

/* [Fallback.strategy] encapsulates the logic for discovering a font, based on a character [Uchar.t] */
module Fallback = {
  type strategy = Uchar.t => option(Skia.Typeface.t);

  let none = _uchar => None;

  let constant = (typeface, _uchar) => Some(typeface);

  let skia = ({skiaFace, fallbackCharacterCache, _}: t, uchar) => {
    let familyName = skiaFace |> Skia.Typeface.getFamilyName;
    let fontStyle = skiaFace |> Skia.Typeface.getFontStyle;

    switch (FallbackCharacterCache.find(uchar, fallbackCharacterCache)) {
    | Some(maybeFontId) =>
      FallbackCharacterCache.promote(uchar, fallbackCharacterCache);
      // Re-query font manager to get the actual typeface
      // Since we cached the ID, we know this character is supported
      switch (maybeFontId) {
      | Some(_fontId) =>
        Skia.FontManager.matchFamilyStyleCharacter(
          FontManager.instance,
          familyName,
          fontStyle,
          [Environment.userLocale],
          uchar,
        )
      | None => None
      };
    | None =>
      let maybeTypeface =
        Skia.FontManager.matchFamilyStyleCharacter(
          FontManager.instance,
          familyName,
          fontStyle,
          [Environment.userLocale],
          uchar,
        );

      Log.debugf(m =>
        m(
          "Unresolved glyph: character : U+%04X font: %s",
          Uchar.to_int(uchar),
          familyName,
        )
      );

      // Cache the font ID instead of the typeface
      let maybeFontId = Option.map(Skia.Typeface.getUniqueID, maybeTypeface);
      FallbackCharacterCache.add(uchar, maybeFontId, fallbackCharacterCache);
      FallbackCharacterCache.trim(fallbackCharacterCache);
      maybeTypeface;
    };
  };

  let custom = (f: strategy) => f;
};

let generateShapes:
  (~fallback: Fallback.strategy, ~features: list(Feature.t), t, string) =>
  list(ShapeResult.shapeNode) =
  (~fallback, ~features, font, str) => {
    let fallbackFor = (~byteOffset, str) => {
      Log.debugf(m =>
        m(
          "Resolving fallback for: %s at byte offset %d - source font is: %s",
          str,
          byteOffset,
          font.skiaFace |> Skia.Typeface.getFamilyName,
        )
      );
      let maybeUchar =
        try(Some(Zed_utf8.extract(str, byteOffset))) {
        | exn =>
          Log.debugf(m =>
            m("Unable to get uchar from string: %s", Printexc.to_string(exn))
          );
          None;
        };
      Option.bind(maybeUchar, uchar
        // Only fallback if the character is non-ASCII (UTF-8)
        =>
          if (Uchar.to_int(uchar) > 256) {
            fallback(uchar);
          } else {
            None;
          }
        )
      |> (
        fun
        | Some(_) as font => {
            load(font);
          }
        | None => {
            Error("No fallback font found");
          }
      );
    };

    /* A hole is a space in a string where the current font
       can't render the text. For instance, most standard fonts
       don't include emojis, and Latin fonts often don't include
       CJK characters. This module contains functions that
       relate to the creation and resolution of these "holes" */
    let rec resolveHole = (~attempts, ~acc, ~start, ~stop) =>
      if (start >= stop) {
        acc;
      } else {
        switch (fallbackFor(~byteOffset=start, str)) {
        | Ok(fallbackFont)
            when Skia.Typeface.equal(fallbackFont.skiaFace, font.skiaFace) =>
          resolveHole(
            ~acc=[
              ShapeResult.{
                skiaFace: font.skiaFace,
                glyphId: Constants.unresolvedGlyphID,
                xAdvance: Constants.defaultUnresolvedAdvance,
                yAdvance: Constants.defaultUnresolvedAdvance,
                xOffset: Constants.defaultUnresolvedOffset,
                yOffset: Constants.defaultUnresolvedOffset,
                unitsPerEm: Constants.defaultUnresolvedUnitsPerEm,
                cluster: start,
              },
              ...acc,
            ],
            ~start=start + 1,
            ~stop,
            ~attempts=0 // Reset attempts, because we've moved to the next character
          )
        | Error(_) =>
          // Just because we can't find a font for this character doesn't mean
          // the rest of the hole can't be resolved. Here we insert the "unknown"
          // glyph and try to resolve the rest of the string.
          resolveHole(
            ~acc=[
              ShapeResult.{
                skiaFace: font.skiaFace,
                glyphId: Constants.unresolvedGlyphID,
                xAdvance: Constants.defaultUnresolvedAdvance,
                yAdvance: Constants.defaultUnresolvedAdvance,
                xOffset: Constants.defaultUnresolvedOffset,
                yOffset: Constants.defaultUnresolvedOffset,
                unitsPerEm: Constants.defaultUnresolvedUnitsPerEm,
                cluster: start,
              },
              ...acc,
            ],
            ~start=start + 1,
            ~stop,
            ~attempts=0 // Reset attempts, becaused we've moved to the next character
          )
        | Ok(fallbackFont) =>
          Log.debugf(m =>
            m(
              "Got fallback font - id: %d name: %s (from source font - id: %d %s) - attempt %d",
              fallbackFont.skiaFace
              |> Skia.Typeface.getUniqueID
              |> Int32.to_int,
              fallbackFont.skiaFace |> Skia.Typeface.getFamilyName,
              font.skiaFace |> Skia.Typeface.getUniqueID |> Int32.to_int,
              font.skiaFace |> Skia.Typeface.getFamilyName,
              attempts,
            )
          );

          // We found a fallback font! Now we just have to shape it the same way
          // we shape the super-string.
          loop(~attempts=attempts + 1, ~start, ~stop, ~acc, fallbackFont);
        };
      }
    and loopShapes =
        (
          ~attempts,
          ~stopCluster,
          ~acc,
          ~holeStart=?,
          ~index,
          {skiaFace, _} as font,
          shapes,
        ) => {
      let resolvePossibleHole = (~stop) => {
        switch (holeStart) {
        | Some(start) => resolveHole(~attempts, ~acc, ~start, ~stop)
        | None => acc
        };
      };

      if (index == Array.length(shapes)) {
        resolvePossibleHole(~stop=stopCluster);
      } else {
        let Harfbuzz.{
          glyphId,
          cluster,
          xAdvance,
          yAdvance,
          xOffset,
          yOffset,
          unitsPerEm,
        } = shapes[index];

        // If we have an unknown glyph (part of a hole), extend
        // the current hole to encapsulate it. We cannot resolve unresolved
        // glyphs individually since a character can span several code points,
        // and an unresolved glyph only represents a single code point.
        if (glyphId == Constants.unresolvedGlyphID) {
          let holeStart = Option.value(holeStart, ~default=cluster);
          loopShapes(
            ~attempts,
            ~stopCluster,
            ~acc,
            ~holeStart,
            ~index=index + 1,
            font,
            shapes,
          );
        } else {
          // Otherwise resolve any hole the preceded this one and add the
          // current glyph to the list.
          let acc = resolvePossibleHole(~stop=cluster);
          let acc = [
            ShapeResult.{
              skiaFace,
              glyphId,
              xAdvance,
              yAdvance,
              xOffset,
              yOffset,
              unitsPerEm,
              cluster,
            },
            ...acc,
          ];
          loopShapes(
            ~attempts,
            ~stopCluster,
            ~acc,
            ~index=index + 1,
            font,
            shapes,
          );
        };
      };
    }

    and loop = (~attempts, ~acc, ~start, ~stop, font) =>
      // This [attempts] counter is used to 'circuit-break' - verify
      // we don't end up in an infinite loop. If we've tried multiple times
      // to fallback, give up, use the unresolved glyph ID, and move on.
      if (attempts >= 3) {
        loop(
          ~attempts=0,
          ~acc=[
            ShapeResult.{
              skiaFace: font.skiaFace,
              glyphId: Constants.unresolvedGlyphID,
              xAdvance: Constants.defaultUnresolvedAdvance,
              yAdvance: Constants.defaultUnresolvedAdvance,
              xOffset: Constants.defaultUnresolvedOffset,
              yOffset: Constants.defaultUnresolvedOffset,
              unitsPerEm: Constants.defaultUnresolvedUnitsPerEm,
              cluster: start,
            },
            ...acc,
          ],
          ~start=start + 1,
          ~stop,
          font,
        );
      } else {
        let typefaceId = Skia.Typeface.getUniqueID(font.skiaFace);
        let hbFace =
          switch (HarfbuzzMap.find_opt(Internal.harfbuzzCache, typefaceId)) {
          | Some(hbFace) => hbFace
          | None =>
            switch (skiaFaceToHarfbuzzFace(font.skiaFace)) {
            | Ok(hbFace) =>
              // Cache the HarfBuzz face for reuse
              HarfbuzzMap.add(Internal.harfbuzzCache, typefaceId, hbFace);
              hbFace;
            | Error(msg) => failwith(msg)
            }
          };
        Harfbuzz.hb_shape(
          ~features,
          ~start=`Position(start),
          ~stop=`Position(stop),
          hbFace,
          str,
        )
        |> loopShapes(~attempts, ~stopCluster=stop, ~acc, ~index=0, font);
      };

    loop(~attempts=0, ~start=0, ~stop=String.length(str), ~acc=[], font)
    |> List.rev;
  };

let shape:
  (~fallback: Fallback.strategy=?, ~features: list(Feature.t)=?, t, string) =>
  ShapeResult.t =
  (~fallback=?, ~features=[], {shapeCache, _} as font, str) => {
    // Default to skia fallback strategy
    let fallbackToUse =
      switch (fallback) {
      | None => Fallback.skia(font)
      | Some(fallbackStrategy) => fallbackStrategy
      };

    switch (ShapeResultCache.find((str, features), shapeCache)) {
    | Some(result) =>
      ShapeResultCache.promote((str, features), shapeCache);
      result;
    | None =>
      let result =
        generateShapes(~fallback=fallbackToUse, ~features, font, str);
      ShapeResultCache.add((str, features), result, shapeCache);
      ShapeResultCache.trim(shapeCache);
      result;
    };
  };
