type measureResult = {
  width: float,
  height: float,
};

let measure = (~features=[], font, size, text: string) => {
  let {height, _}: FontMetrics.t = FontCache.getMetrics(font, size);
  let shapeNodes = text |> FontCache.shape(~features, font);

  let width =
    shapeNodes
    |> List.fold_left(
         (acc, ShapeResult.{xAdvance, unitsPerEm, _}) => {
           let scaled_x_advance = xAdvance *. size /. unitsPerEm;
           acc +. scaled_x_advance;
         },
         0.,
       );
  {
    height,
    width,
  };
};
