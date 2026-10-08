# Metro Planner - MiniSat Integration

This project implements a SAT-based solution for the metro planning problem using MiniSat solver.

## Prerequisites

1. **MiniSat Solver**: Download and compile from http://minisat.se/MiniSat.html
   ```bash
   wget http://minisat.se/downloads/minisat-2.2.0.tar.gz
   tar -xzf minisat-2.2.0.tar.gz
   cd minisat
   make
   # Copy minisat executable to your A3 directory
   cp minisat /path/to/A3/directory/
   ```

2. **C++ Compiler**: Ensure you have g++ with C++11 support

## File Structure

- `mtero_planner_optimized.cpp` - Main SAT encoder/decoder implementation
- `run1.sh` - Compilation and SAT encoding script
- `solve_sat.sh` - MiniSat solver execution script  
- `run2.sh` - Solution decoding script
- `workflow_demo.sh` - Complete workflow demonstration
- `testcase_gen.py` - Test case generator
- `format_checker.py` - Solution format validator

## Usage

### Method 1: Step-by-step execution

1. **Encode the problem**:
   ```bash
   ./run1.sh <problem_name>
   ```
   This reads `<problem_name>.city` and generates `<problem_name>.satinput`

2. **Solve with MiniSat**:
   ```bash
   ./solve_sat.sh <problem_name>  
   ```
   This runs MiniSat on `<problem_name>.satinput` and creates `<problem_name>.satoutput`

3. **Decode the solution**:
   ```bash
   ./run2.sh <problem_name>
   ```
   This reads `<problem_name>.satoutput` and generates `<problem_name>.metromap`

### Method 2: Complete workflow

```bash
./workflow_demo.sh <problem_name>
```
This runs all three steps automatically.

## Testing

1. **Generate test cases**:
   ```bash
   python3 testcase_gen.py --N 5 --M 5 --K 2 --J 1 --mode constructive --prefix test
   ```

2. **Run the workflow**:
   ```bash
   ./workflow_demo.sh test_000
   ```

3. **Validate the solution**:
   ```bash
   python3 format_checker.py test_000 --verbose
   ```

## File Formats

- **Input**: `<name>.city` - Problem specification
- **SAT Input**: `<name>.satinput` - DIMACS CNF format for MiniSat
- **SAT Output**: `<name>.satoutput` - MiniSat solution (SAT/UNSAT + variable assignments)
- **Output**: `<name>.metromap` - Metro path solution

## Examples

```bash
# Generate a simple test case
python3 testcase_gen.py --N 6 --M 6 --K 3 --J 2 --mode constructive --prefix simple

# Run complete workflow  
./workflow_demo.sh simple_000

# Check solution
python3 format_checker.py simple_000
```

## Troubleshooting

- **"minisat not found"**: Ensure MiniSat executable is in your PATH or current directory
- **Compilation errors**: Check that you have g++ with C++11 support
- **Permission denied**: Make scripts executable with `chmod +x *.sh`
- **SAT solving timeout**: Large problems may take time; consider adding timeout to solve_sat.sh

## Assignment Requirements

This implementation follows the assignment specification:
- Separates SAT encoding, solving, and decoding into distinct scripts
- Uses MiniSat as the external SAT solver
- Maintains proper file naming conventions
- Supports both scenario 1 (basic) and scenario 2 (with popular cells)