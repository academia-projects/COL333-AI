#include <iostream>
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <algorithm>
#include <string>
#include <fstream>
using namespace std;

struct MetroProblem
{
    int scenario, N, M, K, J, P;
    vector<pair<int, int>> starts, ends, popular_cells;

    void print() const
    {
        cout << "MetroProblem(N=" << N << ", M=" << M << ", K=" << K
             << ", J=" << J << ", P=" << P << ", Scenario=" << scenario << ")" << endl;
    }
};

class OptimizedSATEncoder
{
private:
    int N, M, K, J;
    int next_var;

    // Compact variable arrays
    vector<vector<vector<int>>> present;    // [k][r][c] -> cell (r,c) has metro k
    vector<vector<vector<int>>> horizontal; // [k][r][c] -> horizontal segment from (r,c) to (r+1,c)
    vector<vector<vector<int>>> vertical;   // [k][r][c] -> vertical segment from (r,c) to (r,c+1)

    inline int get_var() { return ++next_var; }

    string add_clause(const vector<int> &vars)
    {
        string clause = "";
        for (int var : vars)
        {
            clause += to_string(var) + " ";
        }
        return clause + "0";
    }

    vector<string> at_most_one(const vector<int> &vars)
    {
        vector<string> clauses;
        for (size_t i = 0; i < vars.size(); i++)
        {
            for (size_t j = i + 1; j < vars.size(); j++)
            {
                clauses.push_back(add_clause({-vars[i], -vars[j]}));
            }
        }
        return clauses;
    }

    vector<string> at_least_one(const vector<int> &vars)
    {
        vector<string> clauses;
        if (!vars.empty())
        {
            clauses.push_back(add_clause(vars));
        }
        return clauses;
    }

    vector<string> exactly_one(const vector<int> &vars)
    {
        auto clauses = at_least_one(vars);
        auto amo = at_most_one(vars);
        clauses.insert(clauses.end(), amo.begin(), amo.end());
        return clauses;
    }

    vector<string> exactly_two(const vector<int> &vars)
    {
        vector<string> clauses;

        if (vars.size() < 2)
        {
            return clauses;
        }

        clauses.push_back(add_clause(vars));

        for (size_t i = 0; i < vars.size(); i++)
        {
            vector<int> clause = {-vars[i]};
            for (size_t j = 0; j < vars.size(); j++)
            {
                if (j != i)
                {
                    clause.push_back(vars[j]);
                }
            }
            clauses.push_back(add_clause(clause));
        }

        for (size_t i = 0; i < vars.size(); i++)
        {
            for (size_t j = i + 1; j < vars.size(); j++)
            {
                for (size_t k = j + 1; k < vars.size(); k++)
                {
                    clauses.push_back(add_clause({-vars[i], -vars[j], -vars[k]}));
                }
            }
        }

        return clauses;
    }

    int H(int k, int r, int c)
    {
        if (k < 0 || k >= K || r < 0 || r >= N - 1 || c < 0 || c >= M)
            return 0;
        return horizontal[k][r][c];
    }

    int V(int k, int r, int c)
    {
        if (k < 0 || k >= K || r < 0 || r >= N || c < 0 || c >= M - 1)
            return 0;
        return vertical[k][r][c];
    }

public:
    int P(int k, int r, int c)
    {
        if (k < 0 || k >= K || r < 0 || r >= N || c < 0 || c >= M)
            return 0;
        return present[k][r][c];
    }
    OptimizedSATEncoder(int n, int m, int k, int j) : N(n), M(m), K(k), J(j), next_var(0)
    {
        // Initialize all variable arrays
        present.assign(K, vector<vector<int>>(N, vector<int>(M, 0)));
        horizontal.assign(K, vector<vector<int>>(N, vector<int>(M, 0)));
        vertical.assign(K, vector<vector<int>>(N, vector<int>(M, 0)));

        // Assign variables
        for (int k = 0; k < K; k++)
        {
            for (int r = 0; r < N; r++)
            {
                for (int c = 0; c < M; c++)
                {
                    present[k][r][c] = get_var();
                    if (r < N - 1)
                        horizontal[k][r][c] = get_var();
                    if (c < M - 1)
                        vertical[k][r][c] = get_var();
                }
            }
        }
    }

    int get_num_vars() const { return next_var; }

    vector<string> generate_clauses(const MetroProblem &problem)
    {
        vector<string> clauses;

        // CONSTRAINT 1: At most one metro per cell
        for (int r = 0; r < N; r++)
        {
            for (int c = 0; c < M; c++)
            {
                vector<int> metros_at_cell;
                for (int k = 0; k < K; k++)
                {
                    metros_at_cell.push_back(P(k, r, c));
                }
                auto amo_clauses = at_most_one(metros_at_cell);
                clauses.insert(clauses.end(), amo_clauses.begin(), amo_clauses.end());
            }
        }

        // Process each metro line
        for (int k = 0; k < K; k++)
        {
            int start_r = problem.starts[k].first, start_c = problem.starts[k].second;
            int end_r = problem.ends[k].first, end_c = problem.ends[k].second;

            for (int r = 0; r < N; r++)
            {
                for (int c = 0; c < M; c++)
                {
                    // Get all possible segments for this cell
                    vector<int> segments;
                    if (r > 0)
                        segments.push_back(H(k, r - 1, c)); // Left
                    if (r < N - 1)
                        segments.push_back(H(k, r, c)); // Right
                    if (c > 0)
                        segments.push_back(V(k, r, c - 1)); // Up
                    if (c < M - 1)
                        segments.push_back(V(k, r, c)); // Down

                    // at least one segment active -> P(k, r, c)

                    for (int x : segments)
                    {
                        clauses.push_back(add_clause({-x, P(k, r, c)}));
                    }

                    bool is_start = (r == start_r && c == start_c);
                    bool is_end = (r == end_r && c == end_c);

                    if (is_start || is_end)
                    {
                        // CONSTRAINT 2: Start/end cells must be occupied and have exactly one neighbor
                        clauses.push_back(add_clause({P(k, r, c)}));

                        // No other metro can occupy start/end cells
                        // for (int other_k = 0; other_k < K; other_k++)
                        // {
                        //     if (other_k != k)
                        //     {
                        //         clauses.push_back(add_clause({-P(other_k, r, c)}));
                        //     }
                        // }

                        // Exactly one segment must be active
                        auto exactly_one_seg = exactly_one(segments);
                        clauses.insert(clauses.end(), exactly_one_seg.begin(), exactly_one_seg.end());
                    }
                    else
                    {
                        // CONSTRAINT 3: Double implication for non-start/end cells

                        // If cell has metro k, then exactly 2 segments must be active
                        if (!segments.empty())
                        {
                            auto exactly_two_segs = exactly_two(segments);
                            for (const string &clause_str : exactly_two_segs)
                            {
                                // Add -P(k,r,c) to each clause
                                clauses.push_back(to_string(-P(k, r, c)) + " " + clause_str);
                            }
                        }

                        // If exactly 2 segments are active, then cell must have metro k
                        // if (segments.size() >= 2)
                        // {
                        //     for (size_t i = 0; i < segments.size(); i++)
                        //     {
                        //         for (size_t j = i + 1; j < segments.size(); j++)
                        //         {
                        //             // If segments[i] and segments[j] are both true, then P(k,r,c) must be true
                        //             clauses.push_back(add_clause({-segments[i], -segments[j], P(k, r, c)}));
                        //         }
                        //     }
                        // }
                    }
                }
            }
        }

        // CONSTRAINT 4: Turn constraints (CORRECTED)
        for (int k = 0; k < K; k++)
        {
            int start_r = problem.starts[k].first, start_c = problem.starts[k].second;
            int end_r = problem.ends[k].first, end_c = problem.ends[k].second;

            vector<int> turn_indicator_vars;
            for (int r = 0; r < N; r++)
            {
                for (int c = 0; c < M; c++)
                {
                    // Turns can only happen at non-start/end cells
                    if ((r == start_r && c == start_c) || (r == end_r && c == end_c))
                    {
                        continue;
                    }
                    // A turn is a horizontal segment connected to a vertical one.
                    // There are four combinations for a turn at cell (r,c).
                    int v_in = V(k, r, c - 1);
                    int v_out = V(k, r, c);
                    int h_in = H(k, r - 1, c);
                    int h_out = H(k, r, c);

                    // Define a variable that is true if a turn occurs at this cell for metro k.
                    int is_turn_at_rc = get_var();
                    turn_indicator_vars.push_back(is_turn_at_rc);

                    // is_turn_at_rc <=> ( (h_in & v_out) | (h_in & v_in) | (h_out & v_in) | (h_out & v_out) )
                    // This is equivalent to:
                    // 1. If a turn happens, one of the pairs must be true.
                    //    -is_turn_at_rc OR h_in_v_out OR h_in_v_in OR ...
                    //    We can simplify: if a turn happens, (h_in OR h_out) AND (v_in OR v_out) must be true.
                    if (h_in != 0 && h_out != 0)
                    {
                        clauses.push_back(add_clause({-is_turn_at_rc, h_in, h_out}));
                    }
                    else if (h_in == 0)
                    {
                        clauses.push_back(add_clause({-is_turn_at_rc, h_out}));
                    }
                    else
                    {
                        clauses.push_back(add_clause({-is_turn_at_rc, h_in}));
                    }

                    if (v_in != 0 && v_out != 0)
                    {
                        clauses.push_back(add_clause({-is_turn_at_rc, v_in, v_out}));
                    }
                    else if (v_in == 0)
                    {
                        clauses.push_back(add_clause({-is_turn_at_rc, v_out}));
                    }
                    else
                    {
                        clauses.push_back(add_clause({-is_turn_at_rc, v_in}));
                    }
                    clauses.push_back(add_clause({-is_turn_at_rc, P(k, r, c)}));

                    // 2. If any pair of perpendicular segments is true, it's a turn.
                    //    This also implies the cell is present, which is handled by constraint 3.
                    if (h_in != 0 && v_out != 0)
                        clauses.push_back(add_clause({-h_in, -v_out, is_turn_at_rc}));
                    if (h_in != 0 && v_in != 0)
                        clauses.push_back(add_clause({-h_in, -v_in, is_turn_at_rc}));
                    if (h_out != 0 && v_in != 0)
                        clauses.push_back(add_clause({-h_out, -v_in, is_turn_at_rc}));
                    if (h_out != 0 && v_out != 0)
                        clauses.push_back(add_clause({-h_out, -v_out, is_turn_at_rc}));
                }
            }

            // Now, enforce that the sum of turn_indicator_vars is at most J.
            // We use a sequential counter for this cardinality constraint.
            // S[i][j] = true if the sum of the first i turn indicators is at least j.
            int num_possible_turns = turn_indicator_vars.size();
            vector<vector<int>> S(num_possible_turns + 1, vector<int>(J + 2, 0));

            for (int i = 0; i <= num_possible_turns; ++i)
            {
                for (int j = 0; j <= J + 1; ++j)
                {
                    S[i][j] = get_var();
                }
            }
            for (int i = 0; i <= num_possible_turns; ++i)
            {
                clauses.push_back(add_clause({S[i][0]}));
            }
            for (int j = 1; j <= J + 1; ++j)
            {
                clauses.push_back(add_clause({-S[0][j]}));
            }

            for (int i = 1; i <= num_possible_turns; ++i)
            {
                int x_i = turn_indicator_vars[i - 1];
                for (int j = 1; j <= J + 1; ++j)
                {
                    if (i < j)
                    {
                        clauses.push_back(add_clause({-S[i][j]}));
                        continue;
                    }
                    // S[i][j] <=> (S[i-1][j] OR (S[i-1][j-1] AND x_i))
                    // This gives three clauses:
                    // 1. S[i-1][j] => S[i][j]
                    clauses.push_back(add_clause({-S[i - 1][j], S[i][j]}));
                    // 2. (S[i-1][j-1] AND x_i) => S[i][j]
                    clauses.push_back(add_clause({-S[i - 1][j - 1], -x_i, S[i][j]}));
                    // 3. S[i][j] => (S[i-1][j] OR (S[i-1][j-1] AND x_i))
                    clauses.push_back(add_clause({-S[i][j], S[i - 1][j], S[i - 1][j - 1]}));
                    clauses.push_back(add_clause({-S[i][j], S[i - 1][j], x_i}));
                }
                // S[i][1] <=> (S[i-1][1] OR x_i)
                // clauses.push_back(add_clause({-S[i - 1][1], S[i][1]}));
                // clauses.push_back(add_clause({-x_i, S[i][1]}));
                // clauses.push_back(add_clause({-S[i][1], S[i - 1][1], x_i}));
            }

            // The final constraint: the total number of turns must be at most J.
            // This is equivalent to saying the number of turns is NOT (at least J+1).
            // So, S[num_possible_turns][J+1] must be false.
            clauses.push_back(add_clause({-S[num_possible_turns][J + 1]}));
        }

        // Scenario 2: Popular cells constraint
        if (problem.scenario == 2)
        {
            for (const auto &cell : problem.popular_cells)
            {
                int r = cell.first, c = cell.second;
                vector<int> metros_at_popular;
                for (int k = 0; k < K; k++)
                {
                    metros_at_popular.push_back(P(k, r, c));
                }
                // At least one metro must pass through each popular cell
                auto at_least_one_metro = at_least_one(metros_at_popular);
                clauses.insert(clauses.end(), at_least_one_metro.begin(), at_least_one_metro.end());
            }
        }

        return clauses;
    }

    string reconstruct_path(int k, const set<int> &true_vars, const MetroProblem &problem)
    {
        int start_r = problem.starts[k].first, start_c = problem.starts[k].second;
        int end_r = problem.ends[k].first, end_c = problem.ends[k].second;
        cout << start_r << " " << start_c << " " << end_r << " " << end_c << endl;
        string path = "";
        int curr_r = start_r, curr_c = start_c;
        int parent_segment = -1; // Track which segment we came from to avoid backtracking

        int max_iterations = N * M;
        int iteration = 0;

        while ((curr_r != end_r || curr_c != end_c) && iteration < max_iterations)
        {
            iteration++;
            int next_r = -1, next_c = -1;
            char direction = ' ';
            int next_segment = -1;

            // Check all four directions using segment variables

            // Left: H segment from (curr_r-1, curr_c) to (curr_r, curr_c)
            if (curr_r > 0)
            {
                int h_var = H(k, curr_r - 1, curr_c);
                if (h_var > 0 && h_var != parent_segment && true_vars.count(h_var))
                {
                    next_r = curr_r - 1;
                    next_c = curr_c;
                    direction = 'L';
                    next_segment = h_var;
                }
            }

            // Right: H segment from (curr_r, curr_c) to (curr_r+1, curr_c)
            if (next_r == -1 && curr_r < N - 1)
            {
                int h_var = H(k, curr_r, curr_c);
                if (h_var > 0 && h_var != parent_segment && true_vars.count(h_var))
                {
                    next_r = curr_r + 1;
                    next_c = curr_c;
                    direction = 'R';
                    next_segment = h_var;
                }
            }

            // Up: V segment from (curr_r, curr_c-1) to (curr_r, curr_c)
            if (next_r == -1 && curr_c > 0)
            {
                int v_var = V(k, curr_r, curr_c - 1);
                if (v_var > 0 && v_var != parent_segment && true_vars.count(v_var))
                {
                    next_r = curr_r;
                    next_c = curr_c - 1;
                    direction = 'U';
                    next_segment = v_var;
                }
            }

            // Down: V segment from (curr_r, curr_c) to (curr_r, curr_c+1)
            if (next_r == -1 && curr_c < M - 1)
            {
                int v_var = V(k, curr_r, curr_c);
                if (v_var > 0 && v_var != parent_segment && true_vars.count(v_var))
                {
                    next_r = curr_r;
                    next_c = curr_c + 1;
                    direction = 'D';
                    next_segment = v_var;
                }
            }

            if (next_r == -1)
            {
                cout << "NO VALID MOVE" << endl;
                break; // No valid move found
            }

            path += string(1, direction) + " ";
            curr_r = next_r;
            curr_c = next_c;
            parent_segment = next_segment;
        }

        return path + "0";
    }
};

bool parse_city_file(const string &city_file_path, MetroProblem &problem)
{
    ifstream infile(city_file_path);
    if (!infile.is_open())
        return false;

    if (!(infile >> problem.scenario))
        return false;

    if (problem.scenario == 1)
    {
        if (!(infile >> problem.N >> problem.M >> problem.K >> problem.J))
            return false;
        problem.P = 0;
    }
    else
    {
        if (!(infile >> problem.N >> problem.M >> problem.K >> problem.J >> problem.P))
            return false;
    }

    problem.starts.resize(problem.K);
    problem.ends.resize(problem.K);

    for (int i = 0; i < problem.K; i++)
    {
        if (!(infile >> problem.starts[i].first >> problem.starts[i].second >>
              problem.ends[i].first >> problem.ends[i].second))
            return false;
    }

    if (problem.scenario == 2 && problem.P > 0)
    {
        // cout << "reached" << endl;
        problem.popular_cells.resize(problem.P);
        for (int i = 0; i < problem.P; i++)
        {
            if (!(infile >> problem.popular_cells[i].first >> problem.popular_cells[i].second))
                return false;
        }
    }

    return true;
}

void run_encoder(const string &basename)
{
    MetroProblem problem;
    if (!parse_city_file(basename + ".city", problem))
    {
        cerr << "Error parsing city file." << endl;
        exit(1);
    }

    cout << "Encoding problem: ";
    problem.print();

    OptimizedSATEncoder encoder(problem.N, problem.M, problem.K, problem.J);
    vector<string> clauses = encoder.generate_clauses(problem);

    ofstream outfile(basename + ".satinput");
    if (!outfile.is_open())
    {
        cerr << "Error: Could not open satinput file." << endl;
        exit(1);
    }

    outfile << "p cnf " << encoder.get_num_vars() << " " << clauses.size() << endl;
    for (const string &clause : clauses)
    {
        outfile << clause << endl;
    }

    cout << "Generated " << basename << ".satinput" << endl;
}

void run_decoder(const string &basename)
{
    MetroProblem problem;
    if (!parse_city_file(basename + ".city", problem))
    {
        cerr << "Error parsing city file." << endl;
        exit(1);
    }

    ifstream sat_infile(basename + ".satoutput");
    if (!sat_infile.is_open())
    {
        cerr << "Error: Could not open satoutput file." << endl;
        exit(1);
    }

    string result;
    sat_infile >> result;

    ofstream map_outfile(basename + ".metromap");
    if (!map_outfile.is_open())
    {
        cerr << "Error: Could not open metromap file." << endl;
        exit(1);
    }

    if (result == "UNSAT")
    {
        map_outfile << "0" << endl;
        cout << "Problem is UNSATISFIABLE." << endl;
        return;
    }

    set<int> true_vars;
    int var;
    while (sat_infile >> var && var != 0)
    {
        if (var > 0)
        {
            true_vars.insert(var);
            // cout << var << " ";
        }
    }
    cout << endl;

    OptimizedSATEncoder encoder(problem.N, problem.M, problem.K, problem.J);
    // for(int k = 0; k < problem.K; k++){
    //     for(int r = 0; r < problem.N; r++){
    //         for(int c = 0; c < problem.M; c++){
    //             cout << k << " " << r << " " << c << " " << encoder.P(k, r, c) <<endl;
    //         }
    //     }
    // }

    for (int k = 0; k < problem.K; k++)
    {
        string path = encoder.reconstruct_path(k, true_vars, problem);
        map_outfile << path << endl;
    }

    cout << "Generated " << basename << ".metromap" << endl;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cerr << "Usage: " << argv[0] << " <run1|run2> <basename>" << endl;
        return 1;
    }

    string command = argv[1];
    string basename = argv[2];

    if (command == "run1")
    {
        run_encoder(basename);
    }
    else if (command == "run2")
    {
        run_decoder(basename);
    }
    else
    {
        cerr << "Unknown command: " << command << ". Use 'run1' or 'run2'." << endl;
        return 1;
    }

    return 0;
}