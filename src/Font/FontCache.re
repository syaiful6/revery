open Revery_Core;

module Log = (val Revery_Core.Log.withNamespace("Revery.FontCache"));

module TextRunHashable = {
  type t = ShapeResult.textRun;
  let equal = (run1: t, run2: t) =>
    String.equal(run1.text, run2.text)
    && Skia.Typeface.equal(run1.face, run2.face)
    && List.length(run1.features) == List.length(run2.features);
  let hash = (run: t) =>
    Hashtbl.hash((
      run.text,
      Skia.Typeface.getUniqueID(run.face),
      List.length(run.features),
    ));
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
  type item = {
    familyName: string,
    style: Skia.FontStyle.t,
  };
  type t = option(item);
  let weight = _ => 1;
};

module MetricsCache = Lru.M.Make(FloatHashable, MetricsWeighted);
module ShapeResultCache = Lru.M.Make(TextRunHashable, ShapeResultWeighted);
module FallbackCache = Lru.M.Make(TextRunHashable, FallbackWeighted);
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

let tableCallback = (tag, typeface) => {
  Skia.Typeface.copyTableDataInt32(typeface, tag);
};

let skiaFaceToHarfbuzzFace = skiaFace => {
  let familyName = Skia.Typeface.getFamilyName(skiaFace);
  if (familyName == "Apple Color Emoji") {
    Harfbuzz.hb_face_create_for_tables(tableCallback, skiaFace);
  } else {
    switch (Skia.Typeface.toStreamIndex(skiaFace)) {
    | (Some(asset), idx) =>
      let stream = Skia.StreamAsset.toStream(asset);
      let length = Skia.Stream.getLength(stream);
      // Try zero-copy approach first using getMemoryBase
      let memoryBase = Skia.Stream.getMemoryBase(stream);
      let face =
        if (Ctypes.is_null(memoryBase)) {
          // Fallback to copying approach if memory base is not available
          let bytes = Skia.Data.makeStringFromStream(stream, length);
          Harfbuzz.hb_face_from_data(bytes);
        } else {
          // Zero-copy approach: use memory base directly
          let memoryPtr = Ctypes.raw_address_of_ptr(memoryBase);
          Harfbuzz.hb_face_from_memory_ptr(memoryPtr, length, idx);
        };
      let destroyAsset = _face => {
        Skia.StreamAsset.delete(asset);
      };
      Gc.finalise(destroyAsset, face);
      face;
    | (None, _) => Result.Error("failed to convert skia typeface to stream")
    };
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

let createTextRun = (~text, ~font: t, ~features) => {
  let face = getSkiaTypeface(font);
  ShapeResult.{
    text,
    face,
    features,
  };
};

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
    | Some(maybeItem) =>
      FallbackCharacterCache.promote(uchar, fallbackCharacterCache);
      switch (maybeItem) {
      | Some(item) =>
        Skia.FontManager.matchFamilyStyle(
          FontManager.instance,
          item.familyName,
          item.style,
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
      let item =
        maybeTypeface
        |> Option.map(typeface =>
             {
               FontIdWeighted.familyName:
                 Skia.Typeface.getFamilyName(typeface),
               style: Skia.Typeface.getFontStyle(typeface),
             }
           );
      FallbackCharacterCache.add(uchar, item, fallbackCharacterCache);
      FallbackCharacterCache.trim(fallbackCharacterCache);
      maybeTypeface;
    };
  };

  let custom = (f: strategy) => f;
};

let generateShapedNodes:
  (~fallback: Fallback.strategy, ~features: list(Feature.t), t, string) =>
  list((Skia.Typeface.t, ShapeResult.shapeNode)) =
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
              (
                font.skiaFace,
                ShapeResult.{
                  glyphId: Constants.unresolvedGlyphID,
                  xAdvance: Constants.defaultUnresolvedAdvance,
                  yAdvance: Constants.defaultUnresolvedAdvance,
                  xOffset: Constants.defaultUnresolvedOffset,
                  yOffset: Constants.defaultUnresolvedOffset,
                  unitsPerEm: Constants.defaultUnresolvedUnitsPerEm,
                  cluster: start,
                },
              ),
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
              (
                font.skiaFace,
                ShapeResult.{
                  glyphId: Constants.unresolvedGlyphID,
                  xAdvance: Constants.defaultUnresolvedAdvance,
                  yAdvance: Constants.defaultUnresolvedAdvance,
                  xOffset: Constants.defaultUnresolvedOffset,
                  yOffset: Constants.defaultUnresolvedOffset,
                  unitsPerEm: Constants.defaultUnresolvedUnitsPerEm,
                  cluster: start,
                },
              ),
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
            (
              skiaFace,
              ShapeResult.{
                glyphId,
                xAdvance,
                yAdvance,
                xOffset,
                yOffset,
                unitsPerEm,
                cluster,
              },
            ),
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
            (
              font.skiaFace,
              ShapeResult.{
                glyphId: Constants.unresolvedGlyphID,
                xAdvance: Constants.defaultUnresolvedAdvance,
                yAdvance: Constants.defaultUnresolvedAdvance,
                xOffset: Constants.defaultUnresolvedOffset,
                yOffset: Constants.defaultUnresolvedOffset,
                unitsPerEm: Constants.defaultUnresolvedUnitsPerEm,
                cluster: start,
              },
            ),
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

    // Create a textRun for the primary font to check cache
    let primaryTextRun = createTextRun(~text=str, ~font, ~features);

    switch (ShapeResultCache.find(primaryTextRun, shapeCache)) {
    | Some(cachedResult) =>
      // Cache hit - return complete cached result including fallback fonts
      ShapeResultCache.promote(primaryTextRun, shapeCache);
      cachedResult;

    | None =>
      // Cache miss - generate and cache complete result
      let shapedNodes =
        generateShapedNodes(~fallback=fallbackToUse, ~features, font, str);

      // Group consecutive nodes by typeface
      let rec groupConsecutiveByTypeface = (nodes, acc) => {
        switch (nodes) {
        | [] => List.rev(acc)
        | [(typeface, node), ...rest] =>
          let typefaceId = Skia.Typeface.getUniqueID(typeface);

          // Collect all consecutive nodes with the same typeface
          let rec collectSameTypeface = (currentNodes, remainingNodes) => {
            switch (remainingNodes) {
            | [(tf, n), ...r]
                when Int32.equal(Skia.Typeface.getUniqueID(tf), typefaceId) =>
              collectSameTypeface([n, ...currentNodes], r)
            | _ => (List.rev(currentNodes), remainingNodes)
            };
          };

          let (sameTypefaceNodes, remaining) =
            collectSameTypeface([node], rest);

          let textRun =
            ShapeResult.{
              text: str, // TODO: extract actual text portion for this run
              face: typeface,
              features,
            };

          let shapedRun =
            ShapeResult.{
              textRun,
              nodes: sameTypefaceNodes,
            };
          groupConsecutiveByTypeface(remaining, [shapedRun, ...acc]);
        };
      };

      let result = groupConsecutiveByTypeface(shapedNodes, []);

      // Cache the complete result including fallback fonts
      ShapeResultCache.add(primaryTextRun, result, shapeCache);
      ShapeResultCache.trim(shapeCache);

      result;
    };
  };
