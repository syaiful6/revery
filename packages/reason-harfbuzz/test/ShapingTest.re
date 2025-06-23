open Harfbuzz;
open TestFramework;

describe("Shaping", ({test, _}) => {
  test("whole string", ({expect, _}) => {
    let expectedResult = [|
      {
        glyphId: 69,
        cluster: 0,
      },
      {
        glyphId: 70,
        cluster: 1,
      },
      {
        glyphId: 71,
        cluster: 2,
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
      },
    |];
    let shapes =
      hb_shape(font, "aҙc", ~start=`Position(1), ~stop=`Position(3));

    expect.equal(expectedResult, shapes);
  });
});
