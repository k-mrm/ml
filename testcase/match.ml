let True = \t -> \f -> t in True;
let False = \t -> \f -> f in False;
let otherwise = \x -> True in otherwise;
let if = \b -> \x -> \y -> b.x.y in if;

let eq = \x -> \y -> match x {
                       y => True |
                       _ => False
                     } in eq;

let a = if.(eq.100.100).0.9999 in a;
let a = if.(eq.100.200).0.9999 in a;
