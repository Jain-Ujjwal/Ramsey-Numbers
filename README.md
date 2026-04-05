# Ramsey Number CSP Solver

A highly optimized C++ solver designed to find exact mathematical upper bounds for two-color Ramsey Numbers $R(s, t)$. 

This project frames the Ramsey Number problem as a Constraint Satisfaction Problem (CSP). Because the search space for edge colorings grows exponentially (a complete graph $K_n$ has $2^{\binom{n}{2}}$ possible colorings), standard brute-force algorithms fail almost immediately. This solver uses a combination of Depth-First Search (DFS) backtracking, hardware-level bitmasking, and dynamic heuristics to prune structurally impossible branches early.

## Technical Optimizations

To handle the massive search space, the solver relies on three main AI and architectural optimizations:

### 1. Bitmask Adjacency Matrices for $O(1)$ Intersections
Instead of using standard 2D arrays (e.g., `bool adj[MAX][MAX]`), the Red and Blue subgraphs are represented using 64-bit integers (`unsigned long long`). 
* Finding the common neighbors of two nodes is reduced to a single bitwise `AND` operation (`cand = g[u] & g[v]`). 
* This allows the CPU to evaluate up to 64 nodes in a single clock cycle, completely bypassing slow nested `for`-loops during clique detection.

### 2. Minimum Remaining Values (MRV) / Degree Heuristic
Instead of coloring edges in numerical order, the `choose_edge()` function dynamically selects the uncolored edge whose endpoints already have the highest number of colored connections. 
* By targeting the most heavily constrained edges first (MRV), the algorithm aggressively forces early failures. This cuts off massive "dead" branches of the search tree before the algorithm wastes time exploring them.

### 3. Hardware-Level Bitwise Intrinsics
The recursive clique detector uses GCC compiler intrinsics to maximize speed:
* `__builtin_popcountll(mask)`: Counts the number of set bits (degree of the node) directly at the hardware level.
* `__builtin_ctzll(mask)`: Counts trailing zeros to instantly find the index of the next available neighbor, skipping iteration over empty bits.

##  How to Build and Run

You will need a standard C++ compiler (like GCC or Clang). Since this relies heavily on bitwise operations and recursion, compiling with the `-O3` optimization flag is highly recommended for performance.

**Compilation:**
`g++ -O3 ramsey_solver.cpp -o ramsey`

**Execution:**
`./ramsey`

### Usage
When you run the program, it will prompt you for the target clique sizes:
`Enter target clique sizes (s and t) to calculate R(s, t): 3 4`

The solver will start testing $K_n$ graphs sequentially. For every valid graph it finds, it prints:
1. An $n \times n$ adjacency matrix (where `R` is Red and `B` is Blue).
2. A complete edge list.

When the DFS exhausts all possibilities and fails to color $K_n$ without forcing a forbidden clique, the program halts and outputs the exact Ramsey Number proof.

### Example Output Extract
```text
Enter target clique sizes (s and t) to calculate R(s, t): 3 4
Testing K_8 ...
YES: valid coloring exists for K_8
Coloring matrix:
- R R R B B B B 
R - B B R R B B 
... (matrix continues)

Testing K_9 ...
NO: every coloring creates forbidden clique
Exact Match: R(3,4) = 9
