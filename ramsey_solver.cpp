/* =========================================================================
 * Project Name: Ramsey Numbers
 * Description: An optimized Constraint Satisfaction Problem (CSP) solver 
 * designed to find exact Ramsey Numbers R(s, t).
 * Algorithm: Depth-First Search (DFS) with Backtracking and Forward Checking.
 * Optimizations: 
 * 1. Bitmask Adjacency Matrices for O(1) graph intersections.
 * 2. Dynamic Variable Ordering (Degree Heuristic) to force early failures.
 * 3. Hardware-level bitwise operations (__builtin_popcountll, __builtin_ctzll).
 * ========================================================================= */

#include <bits/stdc++.h>
using namespace std;

struct Solver {
    int n, s, t, m;
    
    vector<pair<int,int>> e; // Stores all edges in the complete graph K_n
    vector<int> col;         // State of each edge: -1 (uncolored), 0 (Red), 1 (Blue)
    
    // BITMASK OPTIMIZATION: 
    // We represent the adjacency matrix of the Red and Blue subgraphs using 64-bit integers.
    // This allows us to check the common neighbors of up to 64 nodes simultaneously 
    // using a single CPU clock cycle (bitwise AND), bypassing slow nested for-loops.
    vector<unsigned long long> rg, bg; 

    // Constructor: Initializes the complete graph K_n for testing
    Solver(int n_, int s_, int t_) : n(n_), s(s_), t(t_) {
        // Generate all possible edges for a graph of size n
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                e.push_back({i, j});
            }
        }
        m = (int)e.size(); // Total number of edges to color
        col.assign(m, -1);
        rg.assign(n, 0);
        bg.assign(n, 0);
    }

    // HEURISTIC: Dynamic Variable Ordering (similar to Minimum Remaining Values)
    // Instead of picking edges sequentially (0, 1, 2...), we pick the edge whose
    // endpoints already have the most colored connections. 
    // Why? We want to trigger forbidden cliques as fast as possible to prune dead branches.
    int choose_edge() {
        int id = -1, bs = -1;
        for (int i = 0; i < m; i++) {
            if (col[i] != -1) continue; // Skip edges we've already colored

            int u = e[i].first;
            int v = e[i].second;

            // Score edges based on how constrained their endpoints are.
            // __builtin_popcountll is a hardware instruction that counts the number of '1' bits.
            int sc = __builtin_popcountll(rg[u] | bg[u]) +
                     __builtin_popcountll(rg[v] | bg[v]);

            if (sc > bs) {
                bs = sc;
                id = i;
            }
        }
        return id; // Returns the index of the most constrained uncolored edge
    }

    // CORE LOGIC: Recursive Clique Detector
    // Checks if the bitmask 'cand' (candidate neighbors) contains a clique of size 'need'.
    bool kclique_from(unsigned long long cand, int need,
                      const vector<unsigned long long>& g) {
        
        if (need == 0) return true; // Base case: We successfully found the required clique size
        
        // Pruning: If there aren't even enough set bits left in the mask to form 
        // the required clique size, abort immediately.
        if (__builtin_popcountll(cand) < need) return false;

        // Iterate through all currently set bits in the candidate mask
        while (cand) {
            // __builtin_ctzll counts trailing zeros to quickly find the index of the lowest set bit
            int v = __builtin_ctzll(cand); 
            
            // Strip that specific bit away so we don't process it again in this loop
            cand &= (cand - 1); 
            
            // Recurse: Look for a smaller clique among the intersection of current candidates 
            // and the neighbors of node 'v'. (cand & g[v] is the O(1) intersection).
            if (kclique_from(cand & g[v], need - 1, g)) return true;
        }
        return false;
    }

    // CONSTRAINT CHECKER: Evaluates if coloring edge (u, v) with color 'c' breaks Ramsey rules
    bool makes_forbidden(int u, int v, int c) {
        const auto& g = (c == 0 ? rg : bg); // Check the Red graph (0) or Blue graph (1)
        int need = (c == 0 ? s : t) - 2;    // We already have 2 nodes (u and v), so we need s-2 or t-2 more
        
        if (need <= 0) return true; // Trivial fail (e.g., trying to prevent K_2)

        // Find nodes that are connected to BOTH u and v in the specified color.
        // This single bitwise AND is the mathematical equivalent of checking an entire row in a matrix.
        unsigned long long cand = g[u] & g[v]; 
        
        return kclique_from(cand, need, g);
    }

    // SEARCH ENGINE: Depth-First Search with Backtracking
    bool dfs(int rem) {
        if (rem == 0) return true; // All edges colored without forming a forbidden clique!

        int id = choose_edge();
        if (id == -1) return true; // Failsafe, should mathematically map to rem == 0

        int u = e[id].first;
        int v = e[id].second;

        // --- BRANCH 1: Try coloring the edge RED ---
        col[id] = 0;
        rg[u] |= (1ULL << v); // Set the v-th bit in u's row
        rg[v] |= (1ULL << u); // Set the u-th bit in v's row

        // Forward Checking: Does this move create a Red K_s? If not, dive deeper.
        if (!makes_forbidden(u, v, 0) && dfs(rem - 1)) return true;

        // Backtrack: The Red branch failed. Undo the bit assignments.
        rg[u] &= ~(1ULL << v);
        rg[v] &= ~(1ULL << u);

        // --- BRANCH 2: Try coloring the edge BLUE ---
        col[id] = 1;
        bg[u] |= (1ULL << v);
        bg[v] |= (1ULL << u);

        // Forward Checking: Does this move create a Blue K_t? If not, dive deeper.
        if (!makes_forbidden(u, v, 1) && dfs(rem - 1)) return true;

        // Backtrack: The Blue branch failed. Undo the bit assignments.
        bg[u] &= ~(1ULL << v);
        bg[v] &= ~(1ULL << u);

        // Both branches failed. This exact path is a dead end. Uncolor the edge and retreat.
        col[id] = -1;
        return false;
    }

    // Wrapper to start the search
    bool solve() {
        return dfs(m);
    }

    // Utility function to print the valid graphs if the AI succeeds
    void print_coloring() {
        vector<string> mat(n, string(n, '.'));
        for (int i = 0; i < n; i++) mat[i][i] = '-'; // Diagonal is self-referential

        for (int i = 0; i < m; i++) {
            int u = e[i].first;
            int v = e[i].second;

            char ch = (col[i] == 0 ? 'R' : 'B');
            mat[u][v] = mat[v][u] = ch; // Populate symmetric matrix
        }

        cout << "Coloring matrix:" << endl;
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                cout << mat[i][j] << ' ';
            }
            cout << endl;
        }

        cout << "Edge list:" << endl;
        for (int i = 0; i < m; i++) {
            int u = e[i].first;
            int v = e[i].second;

            cout << "(" << u + 1 << "," << v + 1 << ") = "
                 << (col[i] == 0 ? "Red" : "Blue") << endl;
        }
    }
};

int main() {
    // Optimizes C++ standard stream speeds for faster console output
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    cout << "Enter target clique sizes (s and t) to calculate R(s, t):" << endl;
    int s, t;
    if (!(cin >> s >> t)) return 0; // Take target clique sizes from user input

    // Start testing at the smallest theoretically possible graph size
    int start = max(2, max(s, t) - 1);
    int last_ok = -1;

    // The core loop: Incrementally test larger graphs (K_n) until the DFS fails to 
    // find a valid coloring. The point of failure is exactly our Ramsey Number.
    for (int n = start; ; n++) { 
        cout << "Testing K_" << n << " ..." << endl;

        Solver sv(n, s, t);
        bool ok = sv.solve();

        if (ok) {
            // A valid coloring was found, meaning R(s,t) must be strictly greater than 'n'
            last_ok = n;
            cout << "YES: valid coloring exists for K_" << n << endl;
            sv.print_coloring();
            cout << endl;
        } else {
            // The search space was exhausted and all branches failed.
            // By definition, every possible coloring forces a forbidden clique.
            cout << "NO: every coloring creates forbidden clique" << endl;
            
            // Outputs the exact mathematical proof result
            cout << "Exact Match: R(" << s << "," << t << ") = " << n << endl;
            break; // Stop the program, the upper bound has been found
        }
    }

    return 0;
}
