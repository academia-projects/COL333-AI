#include <iostream>
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
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

// --- Helper Struct to store a path ---
struct Path
{
    // A path is a sequence of grid cells
    std::vector<std::pair<int, int>> cells;
};

class PathBasedEncoder
{
private:
    int N, M, K, J;
    int next_var;

    // Stores all pre-computed paths: all_paths[k][p] is the p-th path for metro k
    std::vector<std::vector<Path>> all_paths;

    // Maps Path_kp to a SAT variable: path_vars[k][p] is the variable for that path
    std::vector<std::vector<int>> path_vars;

    // --- Path Generation using DFS ---
    void find_paths_dfs(
        int k, int r, int c, int turns, int last_dir, // State: position, turns, direction
        std::vector<std::pair<int, int>> &current_path,
        std::vector<std::vector<bool>> &visited,
        const MetroProblem &problem)
    {
        // Mark current cell as visited for this path
        visited[r][c] = true;
        current_path.push_back({r, c});

        // Base Case: We reached the destination
        if (r == problem.ends[k].first && c == problem.ends[k].second)
        {
            all_paths[k].push_back({current_path});
            // Backtrack
            visited[r][c] = false;
            current_path.pop_back();
            return;
        }

        // Explore neighbors (Right, Left, Down, Up)
        int dr[] = {0, 0, 1, -1};
        int dc[] = {1, -1, 0, 0};
        int dirs[] = {1, 2, 3, 4}; // 1:Horizontal, 2:Horizontal, 3:Vertical, 4:Vertical

        for (int i = 0; i < 4; ++i)
        {
            int next_r = r + dr[i];
            int next_c = c + dc[i];
            int current_dir_type = (dirs[i] <= 2) ? 1 : 2; // 1 for H, 2 for V

            // Check boundaries
            if (next_r >= 0 && next_r < N && next_c >= 0 && next_c < M && !visited[next_r][next_c])
            {
                int new_turns = turns;
                if (last_dir != 0 && last_dir != current_dir_type)
                {
                    new_turns++;
                }

                if (new_turns <= J)
                {
                    find_paths_dfs(k, next_r, next_c, new_turns, current_dir_type, current_path, visited, problem);
                }
            }
        }

        // Backtrack
        visited[r][c] = false;
        current_path.pop_back();
    }

    void generate_all_paths(const MetroProblem &problem)
    {
        all_paths.resize(K);
        for (int k = 0; k < K; ++k)
        {
            std::vector<std::vector<bool>> visited(N, std::vector<bool>(M, false));
            std::vector<std::pair<int, int>> current_path;
            int start_r = problem.starts[k].first;
            int start_c = problem.starts[k].second;

            find_paths_dfs(k, start_r, start_c, 0, 0, current_path, visited, problem);
        }
    }

public:
    PathBasedEncoder(int n, int m, int k, int j) : N(n), M(m), K(k), J(j), next_var(0) {}

    void save_paths_to_file(const string &filename) const
    {
        ofstream outfile(filename);
        if (!outfile.is_open())
        {
            cerr << "Error: Could not open path map file for writing." << endl;
            return;
        }

        // Format: k p cell_count r1 c1 r2 c2 ...
        for (int k = 0; k < K; ++k)
        {
            for (size_t p = 0; p < all_paths[k].size(); ++p)
            {
                outfile << k << " " << p << " " << all_paths[k][p].cells.size();
                for (const auto &cell : all_paths[k][p].cells)
                {
                    outfile << " " << cell.first << " " << cell.second;
                }
                outfile << endl;
            }
        }
    }

    // Loads paths from a file and rebuilds the internal state.
    void load_paths_from_file(const string &filename)
    {
        ifstream infile(filename);
        if (!infile.is_open())
        {
            cerr << "Error: Could not open path map file for reading." << endl;
            return;
        }

        all_paths.assign(K, {}); // Clear and resize the main paths vector

        string line;
        while (getline(infile, line))
        {
            stringstream ss(line);
            int k, p, cell_count;
            ss >> k >> p >> cell_count;

            if (k >= all_paths.size())
            {
                all_paths.resize(k + 1);
            }
            if (p >= all_paths[k].size())
            {
                all_paths[k].resize(p + 1);
            }

            Path new_path;
            for (int i = 0; i < cell_count; ++i)
            {
                int r, c;
                ss >> r >> c;
                new_path.cells.push_back({r, c});
            }
            all_paths[k][p] = new_path;
        }

        // CRITICAL: After loading the paths, we must re-generate the
        // SAT variable numbers in the exact same order as the encoder did.
        next_var = 0;
        path_vars.resize(K);
        for (int k_idx = 0; k_idx < K; ++k_idx)
        {
            path_vars[k_idx].clear();
            for (size_t p_idx = 0; p_idx < all_paths[k_idx].size(); ++p_idx)
            {
                path_vars[k_idx].push_back(++next_var);
            }
        }
    }

    vector<string> generate_clauses(const MetroProblem &problem)
    {
        // 1. Pre-computation Phase: Find all possible paths for each metro
        std::cout << "Generating all valid paths..." << std::endl;
        generate_all_paths(problem);
        std::cout << "Path generation complete." << std::endl;

        // 2. SAT Variable Assignment
        path_vars.resize(K);
        for (int k = 0; k < K; ++k)
        {
            for (size_t p = 0; p < all_paths[k].size(); ++p)
            {
                path_vars[k].push_back(++next_var);
            }
        }

        vector<string> clauses;

        // 3. Generate Clauses
        // CONSTRAINT 1: Each metro must select exactly one path.
        for (int k = 0; k < K; ++k)
        {
            // At least one path must be chosen
            clauses.push_back(add_clause(path_vars[k]));

            // At most one path can be chosen (pairwise negative clauses)
            for (size_t p1 = 0; p1 < path_vars[k].size(); ++p1)
            {
                for (size_t p2 = p1 + 1; p2 < path_vars[k].size(); ++p2)
                {
                    clauses.push_back(add_clause({-path_vars[k][p1], -path_vars[k][p2]}));
                }
            }
        }

        // CONSTRAINT 2: Selected paths for different metros must not intersect.
        for (int k1 = 0; k1 < K; ++k1)
        {
            for (int k2 = k1 + 1; k2 < K; ++k2)
            {
                for (size_t p1 = 0; p1 < all_paths[k1].size(); ++p1)
                {
                    // For efficient lookup, put cells of path p1 into a set
                    std::set<std::pair<int, int>> path1_cells(all_paths[k1][p1].cells.begin(), all_paths[k1][p1].cells.end());

                    for (size_t p2 = 0; p2 < all_paths[k2].size(); ++p2)
                    {
                        bool intersects = false;
                        // Check each cell of path p2 for collision
                        for (const auto &cell : all_paths[k2][p2].cells)
                        {
                            if (path1_cells.count(cell))
                            {
                                intersects = true;
                                break;
                            }
                        }

                        if (intersects)
                        {
                            // If path p1 (for k1) and path p2 (for k2) intersect, they cannot both be chosen.
                            clauses.push_back(add_clause({-path_vars[k1][p1], -path_vars[k2][p2]}));
                        }
                    }
                }
            }
        }

        if (problem.scenario == 2 && problem.P > 0)
        {
            // This constraint must hold for EACH popular cell.
            for (const auto &popular_cell : problem.popular_cells)
            {
                int pop_r = popular_cell.first;
                int pop_c = popular_cell.second;

                vector<int> paths_through_this_cell;

                // Collect all path variables (Path_kp) where the path p contains the popular cell.
                for (int k = 0; k < K; ++k)
                {
                    for (size_t p = 0; p < all_paths[k].size(); ++p)
                    {
                        // Check if the current path p contains the popular cell.
                        bool contains_cell = false;
                        for (const auto &cell_in_path : all_paths[k][p].cells)
                        {
                            if (cell_in_path.first == pop_r && cell_in_path.second == pop_c)
                            {
                                contains_cell = true;
                                break;
                            }
                        }

                        if (contains_cell)
                        {
                            // If it does, add its SAT variable to our list for the clause.
                            paths_through_this_cell.push_back(path_vars[k][p]);
                        }
                    }
                }

                // Add the clause to the solver.
                // This clause is a large OR of all collected variables, enforcing that
                // at least one of them must be true for the whole formula to be satisfiable.
                if (!paths_through_this_cell.empty())
                {
                    clauses.push_back(add_clause(paths_through_this_cell));
                }
                else
                {
                    // This is an important edge case. If no possible path for any metro
                    // goes through a required popular cell, the problem is unsatisfiable.
                    // We add an empty clause, which is a contradiction, to tell the solver this.
                    clauses.push_back(add_clause({}));
                }
            }
        }

        return clauses;
    }

    string reconstruct_path(int k, const set<int> &true_vars, const MetroProblem &problem)
    {
        int selected_path_idx = -1;
        // Find which path variable for metro k is true
        for (size_t p = 0; p < path_vars[k].size(); ++p)
        {
            if (true_vars.count(path_vars[k][p]))
            {
                selected_path_idx = p;
                break;
            }
        }

        if (selected_path_idx == -1)
            return "0"; // Should not happen in a SAT solution

        const auto &cells = all_paths[k][selected_path_idx].cells;
        string path_str = "";

        // Convert the sequence of cells to a direction string
        for (size_t i = 0; i < cells.size() - 1; ++i)
        {
            int r1 = cells[i].first, c1 = cells[i].second;
            int r2 = cells[i + 1].first, c2 = cells[i + 1].second;

            if (r2 > r1)
                path_str += 'R';
            else if (r2 < r1)
                path_str += 'L';
            else if (c2 > c1)
                path_str += 'D';
            else if (c2 < c1)
                path_str += 'U';
        }

        return path_str + "0";
    }

    // Standard helper functions
    int get_num_vars() const { return next_var; }
    string add_clause(const vector<int> &vars)
    {
        string clause = "";
        for (int var : vars)
            clause += to_string(var) + " ";
        return clause + "0";
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
    { /* ... error handling ... */
    }

    cout << "Encoding problem: ";
    problem.print();

    PathBasedEncoder encoder(problem.N, problem.M, problem.K, problem.J);

    // This function internally calls generate_all_paths first.
    vector<string> clauses = encoder.generate_clauses(problem);

    // --- NEW STEP: Save the generated paths to a temporary file ---
    string path_map_file = basename + ".pathmap";
    encoder.save_paths_to_file(path_map_file);
    cout << "Saved path map to " << path_map_file << endl;
    // --- END NEW STEP ---

    ofstream outfile(basename + ".satinput");
    // ... (rest of the function remains the same)
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
    { /* ... error handling ... */
    }

    // This creates a NEW, EMPTY encoder object.
    PathBasedEncoder encoder(problem.N, problem.M, problem.K, problem.J);

    // --- NEW STEP: Load the paths from the temporary file ---
    string path_map_file = basename + ".pathmap";
    encoder.load_paths_from_file(path_map_file);
    cout << "Loaded path map from " << path_map_file << endl;
    // --- END NEW STEP ---

    // Now, the encoder object is populated and ready to decode.

    ifstream sat_infile(basename + ".satoutput");
    // ... (rest of the function for reading SAT output and true_vars) ...

    string result;
    sat_infile >> result;

    if (result == "UNSAT")
    { /* ... handle UNSAT ... */
        return;
    }

    set<int> true_vars;
    int var;
    while (sat_infile >> var && var != 0)
    {
        if (var > 0)
            true_vars.insert(var);
    }

    ofstream map_outfile(basename + ".metromap");
    // ... (error handling for map_outfile) ...

    for (int k = 0; k < problem.K; k++)
    {
        // This call will now succeed because the encoder object has the path data.
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