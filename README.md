# PrattCalc - (A visulizer for Pratt parsing algorithm)
Pratt parsing is the algorithm used to calculate expressions like: 5*2-4+(20-1),
This expressions cannot be calculated in linear way because each operator has its own precedence some higher and some lower;
the way to parse it correctly is the Pratt parsing algorithm.

In this algorithm we first tokenize the whole expression then assign each operator its own Binding Power (more like a measure to its precedence higher means it has more precedence)
then we use a recursive behaviour to create a binary tree of the expression after that we use a solver function to calculate the generated tree to gain the answer of the expression.

Power of this algorithm unfold when you find out that you can create your own operand precedence (BP) or introduce your own operands to calculate your own custom expressions.

I used [this article](https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html) to implement my own C++ version and then created a nice UI to represent how the final parsed expression looks like with binary tree.


# Screenshots
![3](https://github.com/user-attachments/assets/4b5b986f-86dd-43a9-a95d-6c95256afb67)
![3](https://github.com/user-attachments/assets/3335849f-fab2-46cc-bb15-2d030898a87a)
![3](https://github.com/user-attachments/assets/abf4bb1f-f47b-4091-9b30-3149f8355b94)

# Notes
I configured the ImGui in a single source file to use Dx11 which make the whole software be bound to windows OS.
so to build it all you need to do is either clone the repo and used visual studio or use : 
```
cmake .
cmake --build .
```

# Contact
You can send me message in telegram: @KVMSwitch
or email me: 
javadejadidi@gmail.com