let timeFn = ref(Unix.gettimeofday);
let setTimeFn = f => timeFn := f;
let getTime = () => timeFn^();

let generator = () => {
  let last = ref(getTime());

  () => {
    let now = getTime();
    let delta = now -. last^;
    delta;
  };
};

let pp = (ppf, dt) => Fmt.pf(ppf, "[%6.3fs]", dt);
