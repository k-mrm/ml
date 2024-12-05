let a' = 10 in
let b' = 10 in
  a' + b' * a' - b';

let a' = 10 in
let b' = 20 in
  a' + b' * (a' - b');

let b' = 200 in b';

a';
b';

let a = 10 in
let a = 810 in
  a - a' - b';

let a = 10 in
let a = 810 in
  (a - a') - b';

let c = 114 in
  c + a;

let True = \x -> \y -> x in True;
let False = \x -> \y -> y in False;

let a = \n -> n + 10 in a;
let f = \n -> n + 10 in f.20;
let f' = \x -> \y -> x+y in f'.20;
let f' = \x -> \y -> x+y in (f'.20).120;
let g' = \x -> \y -> \z -> x - y * z in g';
let h' = g'.10 in h';
let z' = h'.20 in z';
z'.30;
z'.40;
