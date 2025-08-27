let True = \t -> \f -> t in True;
let False = \t -> \f -> f in False;
let if = \bool -> \p -> \q -> bool.p.q in if;

let eq = \n -> \m -> match n {
                       m => True |
                       _ => False
                     } in eq;

let Y = \f -> (\x -> f.(x.x)).(\x -> f.(x.x)) in
let factgen = \f -> \n -> if.(eq.n.0).1.(n * (f.(n-1))) in
let fact = Y.factgen in
stdout.(fact.6);

let fibogen = \f -> \n -> if.(eq.n.0).0.(
                            if.(eq.n.1).1.((f.(n-1)) + (f.(n-2)))
                          ) in
let fibo = Y.fibogen in
stdout.(fibo.20);
