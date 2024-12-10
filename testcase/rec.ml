let True = \t -> \f -> t in True;
let False = \t -> \f -> f in False;
let if = \bool -> \p -> \q -> bool.p.q in if;

let eq = \a -> \b -> match a {
                       b => True |
                       _ => False
                     } in eq;

let Y = \fn -> (\x -> fn.(x.x)).(\y -> fn.(y.y)) in
let factgen = \fa -> \n -> if.(eq.n.0).1.(n * (fa.(n-1))) in
let fact = Y.factgen in
fact.5;
