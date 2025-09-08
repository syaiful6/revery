open TestFramework;
open Revery_Font;

describe("FontSizeAdjust", ({describe, _}) => {
  describe("calculateScaleFactor", ({test, _}) => {
    test("returns 1.0 when no valid metrics found", ({expect, _}) => {
      let primaryMetrics = FontMetrics.empty(12.0);
      let fallbackMetrics = FontMetrics.empty(12.0);

      let scaleFactor =
        SizeAdjust.calculateScaleFactor(
          ~primaryMetrics,
          ~fallbackMetrics,
          ~primarySize=12.0,
          ~fallbackSize=12.0,
        );

      expect.float(scaleFactor).toBeCloseTo(1.0);
    });

    test(
      "calculates scale factor based on ideograph width (highest priority metric)",
      ({expect, _}) => {
      let primaryMetrics = {
        FontMetrics.height: 12.0,
        lineHeight: 14.0,
        ascent: 10.0,
        descent: (-2.0),
        underlinePosition: (-1.0),
        underlineThickness: 1.0,
        avgCharWidth: 7.0,
        maxCharWidth: 12.0,
        capHeight: 9.0,
        xHeight: 6.0 // Actual x-height from font metrics
      };

      let fallbackMetrics = {
        FontMetrics.height: 12.0,
        lineHeight: 16.0,
        ascent: 11.0,
        descent: (-3.0),
        underlinePosition: (-1.5),
        underlineThickness: 1.2,
        avgCharWidth: 8.0,
        maxCharWidth: 14.0,
        capHeight: 10.0,
        xHeight: 7.5 // Actual x-height from font metrics
      };

      let scaleFactor =
        SizeAdjust.calculateScaleFactor(
          ~primaryMetrics,
          ~fallbackMetrics,
          ~primarySize=12.0,
          ~fallbackSize=12.0,
        );

      // Ideograph width (avgCharWidth) has highest priority, not x-height
      // Primary normalized: 7.0 / 12.0 = 0.583...
      // Fallback normalized: 8.0 / 12.0 = 0.667...
      // Scale factor should be 0.583... / 0.667... ≈ 0.875
      expect.float(scaleFactor).toBeCloseTo(0.875);
    });

    test(
      "rejects unreasonable scale factors and falls back to next metric",
      ({expect, _}) => {
      let primaryMetrics = {
        FontMetrics.height: 12.0,
        lineHeight: 14.0,
        ascent: 10.0,
        descent: (-2.0),
        underlinePosition: (-1.0),
        underlineThickness: 1.0,
        avgCharWidth: 7.0, // This will give unreasonable ideograph width factor
        maxCharWidth: 12.0,
        capHeight: 9.0,
        xHeight: 6.0,
      };

      let fallbackMetrics = {
        FontMetrics.height: 12.0,
        lineHeight: 16.0,
        ascent: 11.0,
        descent: (-3.0),
        underlinePosition: (-1.5),
        underlineThickness: 1.2,
        avgCharWidth: 1.0, // Very small, would give factor > 2.0 if used
        maxCharWidth: 14.0,
        capHeight: 10.0,
        xHeight: 7.5,
      };

      let scaleFactor =
        SizeAdjust.calculateScaleFactor(
          ~primaryMetrics,
          ~fallbackMetrics,
          ~primarySize=12.0,
          ~fallbackSize=12.0,
        );

      // Should fall back to x-height since ideograph width would be unreasonable
      // X-height: Primary = 6.0/12.0 = 0.5, Fallback = 7.5/12.0 = 0.625
      // Scale factor = 0.5 / 0.625 = 0.8
      expect.float(scaleFactor).toBeCloseTo(0.8);
    });

    test(
      "returns 1.0 when all metrics give unreasonable scale factors",
      ({expect, _}) => {
      let primaryMetrics = {
        FontMetrics.height: 12.0,
        lineHeight: 100.0, // Very large, would give unreasonable factors
        ascent: 100.0,
        descent: (-100.0),
        underlinePosition: (-1.0),
        underlineThickness: 1.0,
        avgCharWidth: 100.0,
        maxCharWidth: 100.0,
        capHeight: 100.0, // Very large
        xHeight: 100.0 // Very large
      };

      let fallbackMetrics = {
        FontMetrics.height: 12.0,
        lineHeight: 1.0, // Very small, creates extreme ratios
        ascent: 1.0,
        descent: (-1.0),
        underlinePosition: (-1.5),
        underlineThickness: 1.2,
        avgCharWidth: 1.0,
        maxCharWidth: 1.0,
        capHeight: 1.0, // Very small
        xHeight: 1.0 // Very small
      };

      let scaleFactor =
        SizeAdjust.calculateScaleFactor(
          ~primaryMetrics,
          ~fallbackMetrics,
          ~primarySize=12.0,
          ~fallbackSize=12.0,
        );

      // All metrics should give unreasonable factors, so should default to 1.0
      expect.float(scaleFactor).toBeCloseTo(1.0);
    });

    test(
      "uses x-height when ideograph width gives unreasonable factor",
      ({expect, _}) => {
      let primaryMetrics = {
        FontMetrics.height: 12.0,
        lineHeight: 14.0, // Would give factor = (14/12) / (20/12) = 0.7
        ascent: 8.0,
        descent: (-2.0),
        underlinePosition: (-1.0),
        underlineThickness: 1.0,
        avgCharWidth: 10.0, // Would give factor = (10/12) / (4/12) = 2.5 (rejected as > 2.0)
        maxCharWidth: 12.0,
        capHeight: 9.0, // Would give factor = (9/12) / (10/12) = 0.9
        xHeight: 6.0 // Should be used when ideograph width is rejected
      };

      let fallbackMetrics = {
        FontMetrics.height: 12.0,
        lineHeight: 20.0, // Would give different factor if used
        ascent: 9.0,
        descent: (-3.0),
        underlinePosition: (-1.5),
        underlineThickness: 1.2,
        avgCharWidth: 4.0, // Would give factor > 2.0 if used (rejected)
        maxCharWidth: 14.0,
        capHeight: 10.0, // Would give different factor if used
        xHeight: 7.5 // Should be used when ideograph width is rejected
      };

      let scaleFactor =
        SizeAdjust.calculateScaleFactor(
          ~primaryMetrics,
          ~fallbackMetrics,
          ~primarySize=12.0,
          ~fallbackSize=12.0,
        );

      // Should use x-height: (6.0/12.0) / (7.5/12.0) = 6.0/7.5 = 0.8
      expect.float(scaleFactor).toBeCloseTo(0.8);
    });

    test(
      "falls back to cap height when x-height is unavailable", ({expect, _}) => {
      let primaryMetrics = {
        FontMetrics.height: 12.0,
        lineHeight: 14.0,
        ascent: 8.0,
        descent: (-2.0),
        underlinePosition: (-1.0),
        underlineThickness: 1.0,
        avgCharWidth: 10.0, // Would give unreasonable factor
        maxCharWidth: 12.0,
        capHeight: 9.0, // Should be used when xHeight is 0
        xHeight: 0.0 // Unavailable
      };

      let fallbackMetrics = {
        FontMetrics.height: 12.0,
        lineHeight: 20.0,
        ascent: 9.0,
        descent: (-3.0),
        underlinePosition: (-1.5),
        underlineThickness: 1.2,
        avgCharWidth: 2.0, // Would give unreasonable factor
        maxCharWidth: 14.0,
        capHeight: 10.8, // Should be used when xHeight is 0
        xHeight: 0.0 // Unavailable
      };

      let scaleFactor =
        SizeAdjust.calculateScaleFactor(
          ~primaryMetrics,
          ~fallbackMetrics,
          ~primarySize=12.0,
          ~fallbackSize=12.0,
        );

      // Should use cap height: (9.0/12.0) / (10.8/12.0) = 9.0/10.8 = 0.833...
      expect.float(scaleFactor).toBeCloseTo(0.8333);
    });

    test(
      "uses ideograph width as highest priority when available",
      ({expect, _}) => {
      let primaryMetrics = {
        FontMetrics.height: 12.0,
        lineHeight: 14.0,
        ascent: 8.0,
        descent: (-2.0),
        underlinePosition: (-1.0),
        underlineThickness: 1.0,
        avgCharWidth: 9.0, // Should be used as highest priority
        maxCharWidth: 12.0,
        capHeight: 9.0,
        xHeight: 6.0,
      };

      let fallbackMetrics = {
        FontMetrics.height: 12.0,
        lineHeight: 20.0,
        ascent: 9.0,
        descent: (-3.0),
        underlinePosition: (-1.5),
        underlineThickness: 1.2,
        avgCharWidth: 7.2, // Should be used as highest priority
        maxCharWidth: 14.0,
        capHeight: 10.0,
        xHeight: 7.5,
      };

      let scaleFactor =
        SizeAdjust.calculateScaleFactor(
          ~primaryMetrics,
          ~fallbackMetrics,
          ~primarySize=12.0,
          ~fallbackSize=12.0,
        );

      // Should use ideograph width: (9.0/12.0) / (7.2/12.0) = 9.0/7.2 = 1.25
      expect.float(scaleFactor).toBeCloseTo(1.25);
    });
  })
});
