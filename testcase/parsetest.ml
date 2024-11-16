let a = 1 in a + b * c;
let f = \x -> x + 1 in let g = \y -> y + 1 in f * g;
let f = \x -> \y -> x + y in f;
