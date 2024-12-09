let True = \t -> \f -> t in True;
let False = \t -> \f -> f in False;
let otherwise = \x -> True in otherwise;
let if = \b -> \x -> \y -> b.x.y in if;

let eq = \n -> \m -> match n {
                       m => True |
                       _ => False
                     } in eq;

(\y -> (\x -> \y -> x).y).1.2;
(\x -> (\x -> x).x).200;

let fact = \nat -> if.(eq.nat.0).1.(nat + (fact.(nat - 1))) in fact.1;
