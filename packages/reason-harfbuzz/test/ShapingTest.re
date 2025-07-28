open Harfbuzz;
open TestFramework;

describe("Shaping", ({test, _}) => {
  test("whole string", ({expect, _}) => {
    let expectedResult = [|
      {
        glyphId: 69,
        cluster: 0,
        xAdvance: 1114.0,
        yAdvance: 0.0,
        xOffset: 0.0,
        yOffset: 0.0,
        unitsPerEm: 2048.0,
      },
      {
        glyphId: 70,
        cluster: 1,
        xAdvance: 1149.0,
        yAdvance: 0.0,
        xOffset: 0.0,
        yOffset: 0.0,
        unitsPerEm: 2048.0,
      },
      {
        glyphId: 71,
        cluster: 2,
        xAdvance: 1072.0,
        yAdvance: 0.0,
        xOffset: 0.0,
        yOffset: 0.0,
        unitsPerEm: 2048.0,
      },
    |];

    let shapes = hb_shape(font, "abc");

    expect.equal(expectedResult, shapes);
  });

  test("substring", ({expect, _}) => {
    let expectedResult = [|
      {
        glyphId: 70,
        cluster: 1,
        xAdvance: 1149.0,
        yAdvance: 0.0,
        xOffset: 0.0,
        yOffset: 0.0,
        unitsPerEm: 2048.0,
      },
    |];
    let shapes =
      hb_shape(font, "abc", ~start=`Position(1), ~stop=`Position(2));


    expect.equal(expectedResult, shapes);
  });

  test("substring UTF-8", ({expect, _}) => {
    let expectedResult = [|
      {
        glyphId: 1007,
        cluster: 1,
        xAdvance: 1040.0,
        yAdvance: 0.0,
        xOffset: 0.0,
        yOffset: 0.0,
        unitsPerEm: 2048.0,
      },
    |];
    let shapes =
      hb_shape(font, "aҙc", ~start=`Position(1), ~stop=`Position(3));


    expect.equal(expectedResult, shapes);
  });
});
