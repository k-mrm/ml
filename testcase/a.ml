let a' = 10 in
let b' = 10 in
  a' + b' * a' - b';

let b' = 200 in b';

a';
b';

let a = 10 in
let a = 810 in
  a - a' - b';

let a = 10 in
let a = 810 in
  a - (a' - b');

let c = 114 in
  c + a;

let True = \x -> \y -> x in True;
let False = \x -> \y -> y in False;

let f = \n -> n + 10 in f.20;
let f' = \x -> \y -> x+y in f'.20;
let f' = \x -> \y -> x+y in f'.20.120;

let a = \n -> n + 10 in a;
let g' = \x -> \y -> \z -> x - y * z in g';
let h' = g'.10 in h';
let z' = h'.20 in z';
stdout.(z'.30).(z'.40).(z'.(a.40));

stdout.(g'.10.20.30);

stdout.((\x -> \y -> \z -> x * y * z).100.200.300);

let add = \x -> \y -> x + y in
let inc = add.1 in
stdout.(inc.(inc.(inc.(inc.(inc.(inc.(inc.0)))))));

stdout.((\y -> (\x -> \y -> x).y).1.2).((\x -> (\x -> x).x).200);
