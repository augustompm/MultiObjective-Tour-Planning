# File: metrics/calculate_binary_coverage.py

import numpy as np
import pandas as pd
import os
from pathlib import Path
import sys
import itertools

"""
This script implements the binary coverage metric (also known as set coverage
or C-metric) as described in:

"Performance Assessment of Multiobjective Optimizers: An Analysis and Review"
by Zitzler et al., IEEE Transactions on Evolutionary Computation, Vol. 7, No. 2, April 2003
(see p. 121 for binary coverage definition)

"Evolutionary algorithms for solving multi-objective problems" by Coello et al., 2007
(see p. 328-329 for binary indicators)

and following the implementation ideas from:

"A Faster Algorithm for Calculating Hypervolume" by While et al., 
IEEE Transactions on Evolutionary Computation, Vol. 10, No. 1, February 2006

The binary coverage metric measures the fraction of points in one set that are
dominated by at least one point in another set, giving an indication of the
relative quality of two approximation sets.
"""

# Enable debug output
DEBUG = True

def debug_print(msg, value=None):
    """Helper function for debugging output"""
    if DEBUG:
        if value is not None:
            print(f"DEBUG: {msg}: {value}")
        else:
            print(f"DEBUG: {msg}")

def dominates(p, q):
    """
    Check if solution p dominates solution q according to Pareto dominance relation.
    
    As described in Zitzler et al. (2003), p.118, a point dominates another point
    if it is not worse in any objective and better in at least one.
    
    Args:
        p: Solution vector [cost, time, -attractions, -neighborhoods]
        q: Solution vector [cost, time, -attractions, -neighborhoods]
        
    Returns:
        True if p dominates q, False otherwise
    """
    better_in_one = False
    
    # For minimization objectives (cost, time), lower values are better
    # For maximization objectives (-attractions, -neighborhoods), more negative values are better
    for i in range(len(p)):
        if i < 2:  # Cost and time (minimize)
            if p[i] > q[i]:  # p is worse than q
                return False
            if p[i] < q[i]:  # p is better than q
                better_in_one = True
        else:  # Attractions and neighborhoods (maximize, stored as negative)
            if p[i] > q[i]:  # p is worse than q (less negative)
                return False
            if p[i] < q[i]:  # p is better than q (more negative)
                better_in_one = True
    
    return better_in_one

def weakly_dominates(p, q):
    """
    Check if solution p weakly dominates solution q.
    
    As described in Zitzler et al. (2003), p.118, a point weakly dominates another
    point if it is at least as good in all objectives.
    
    Args:
        p: Solution vector [cost, time, -attractions, -neighborhoods]
        q: Solution vector [cost, time, -attractions, -neighborhoods]
        
    Returns:
        True if p weakly dominates q, False otherwise
    """
    # For minimization objectives (cost, time), lower values are better
    # For maximization objectives (-attractions, -neighborhoods), more negative values are better
    for i in range(len(p)):
        if i < 2:  # Cost and time (minimize)
            if p[i] > q[i]:  # p is worse than q
                return False
        else:  # Attractions and neighborhoods (maximize, stored as negative)
            if p[i] > q[i]:  # p is worse than q (less negative)
                return False
    
    return True

def calculate_binary_coverage(set_a, set_b):
    """
    Calculate the binary coverage metric C(A,B) as defined in Zitzler et al. (2003), p.121.
    
    C(A,B) gives the fraction of solutions in B that are weakly dominated by
    at least one solution in A.
    
    Args:
        set_a: List of solution vectors from set A
        set_b: List of solution vectors from set B
        
    Returns:
        Binary coverage value C(A,B) in [0,1]
    """
    if not set_b:
        return 0.0
    
    # Count solutions in B that are weakly dominated by at least one solution in A
    dominated_count = 0
    for b in set_b:
        for a in set_a:
            if weakly_dominates(a, b):
                dominated_count += 1
                break  # Only count b once
    
    # Return fraction
    return dominated_count / len(set_b)

def load_solutions_from_csv(csv_path):
    """
    Load solutions from a results CSV file
    
    Args:
        csv_path: Path to the CSV file
        
    Returns:
        List of solution vectors
    """
    solutions = []
    
    try:
        # Read the file as text first to handle potential formatting issues
        with open(csv_path, 'r', encoding='utf-8') as file:
            lines = file.readlines()
        
        # Get the header line
        header = lines[0].strip().split(';')
        
        # Find indexes of the required columns
        try:
            custo_idx = header.index('CustoTotal')
            tempo_idx = header.index('TempoTotal')
            atracoes_idx = header.index('NumAtracoes')
            bairros_idx = header.index('NumBairros')
        except ValueError:
            print(f"Required columns not found in header: {header}")
            return solutions
        
        # Process each line
        for line_idx, line in enumerate(lines[1:], 2):  # Start from index 2 for error reporting
            try:
                # Split by semicolon
                parts = line.strip().split(';')
                
                if len(parts) <= max(custo_idx, tempo_idx, atracoes_idx, bairros_idx):
                    print(f"Line {line_idx} has fewer fields than expected: {len(parts)}")
                    continue
                
                # Extract and convert values
                custo = float(parts[custo_idx].replace(',', '.'))
                tempo = float(parts[tempo_idx].replace(',', '.'))
                atracoes = float(parts[atracoes_idx].replace(',', '.'))
                bairros = float(parts[bairros_idx].replace(',', '.'))
                
                # Create solution vector with negated values for maximization objectives
                solution = [
                    custo,      # Cost (minimize)
                    tempo,      # Time (minimize)
                    -atracoes,  # Attractions (maximize, stored negated)
                    -bairros    # Neighborhoods (maximize, stored negated)
                ]
                solutions.append(solution)
            except Exception as e:
                print(f"Error processing line {line_idx}: {e}")
                # Continue processing other lines
    
    except Exception as e:
        print(f"Error reading CSV file: {e}")
    
    if solutions:
        print(f"Loaded {len(solutions)} solutions. Sample: {solutions[:2]}")
    else:
        print("WARNING: No solutions could be loaded from the CSV file.")
    
    return solutions

def extract_algorithm_name(filename):
    """
    Extract algorithm name from filename
    Example: "nsga2-resultados.csv" -> "NSGA-II"
    
    Args:
        filename: Name of the file
        
    Returns:
        Algorithm name in a readable format
    """
    # Extract the base part before "resultados.csv"
    # This handles both with and without hyphen
    base_name = os.path.basename(filename).lower()
    if "-resultados.csv" in base_name:
        base_name = base_name.split('-resultados.csv')[0]
    elif "resultados.csv" in base_name:
        base_name = base_name.split('resultados.csv')[0]
        # Remove trailing dash if present
        if base_name.endswith('-'):
            base_name = base_name[:-1]
    
    # Format algorithm name
    if base_name == "nsga2":
        return "NSGA-II"
    elif base_name == "moead":
        return "MOEA/D"
    elif base_name == "spea2":
        return "SPEA2"
    elif base_name == "movns":
        return "MOVNS"
    else:
        # Convert to title case and replace underscores with spaces
        return base_name.replace('_', ' ').title()

def get_nondominated_front(solutions):
    """
    Extract the non-dominated front from a set of solutions.
    
    This follows the concept of Pareto optimality as described in
    Zitzler et al. (2003), p.118.
    
    Args:
        solutions: List of solution vectors
        
    Returns:
        List of non-dominated solution vectors
    """
    if not solutions:
        return []
    
    front = []
    
    for solution in solutions:
        is_dominated = False
        
        # Check against existing front
        i = 0
        while i < len(front):
            # If front[i] dominates solution, solution is dominated
            if dominates(front[i], solution):
                is_dominated = True
                break
            
            # If solution dominates front[i], remove front[i]
            elif dominates(solution, front[i]):
                front.pop(i)
            else:
                i += 1
        
        # If solution is not dominated, add it to front
        if not is_dominated:
            front.append(solution)
    
    return front

def main():
    """
    Main function to calculate binary coverage metrics for all algorithm result files
    """
    script_dir = Path(os.path.dirname(os.path.abspath(__file__)))
    project_root = script_dir.parent
    
    # Find results directory
    results_dir = project_root / "results"
    if not results_dir.exists():
        print(f"Results directory not found: {results_dir}")
        return
    
    print(f"Using results directory: {results_dir}")
    
    # Create a debug log file
    debug_log = results_dir / "binary_coverage_debug.log"
    if DEBUG:
        # Redirect stdout to both console and log file
        sys.stdout = DebugTee(sys.stdout, open(debug_log, 'w'))
    
    # Look for files with flexible patterns
    result_files = []
    patterns = ["*-resultados.csv", "*resultados.csv"]
    for pattern in patterns:
        result_files.extend(list(results_dir.glob(pattern)))
    
    # Remove duplicates
    result_files = list(set(result_files))
    
    if not result_files or len(result_files) < 2:
        print(f"Not enough result files found for binary coverage calculation. Found {len(result_files)} files.")
        return
    
    print(f"Found {len(result_files)} result files:")
    for file in result_files:
        print(f" - {file.name}")
    
    # Load solutions from all files
    algorithm_solutions = {}
    
    for file_path in result_files:
        algo_name = extract_algorithm_name(file_path.name)
        solutions = load_solutions_from_csv(file_path)
        # Get only non-dominated solutions
        nondominated_solutions = get_nondominated_front(solutions)
        algorithm_solutions[algo_name] = nondominated_solutions
        print(f"Loaded {len(solutions)} solutions for {algo_name}")
        print(f"Filtered to {len(nondominated_solutions)} non-dominated solutions")
    
    # Calculate binary coverage for each pair of algorithms
    coverage_results = []
    
    for algo_a, algo_b in itertools.permutations(algorithm_solutions.keys(), 2):
        solutions_a = algorithm_solutions[algo_a]
        solutions_b = algorithm_solutions[algo_b]
        
        if not solutions_a or not solutions_b:
            print(f"Skipping binary coverage calculation for {algo_a} and {algo_b} due to empty solution set")
            coverage_results.append({
                "Algorithm_A": algo_a,
                "Algorithm_B": algo_b,
                "Coverage_A_B": 0.0,
                "Solutions_A": len(solutions_a),
                "Solutions_B": len(solutions_b)
            })
            continue
        
        # Calculate binary coverage C(A,B)
        coverage_a_b = calculate_binary_coverage(solutions_a, solutions_b)
        
        # Add result
        coverage_results.append({
            "Algorithm_A": algo_a,
            "Algorithm_B": algo_b,
            "Coverage_A_B": coverage_a_b,
            "Solutions_A": len(solutions_a),
            "Solutions_B": len(solutions_b)
        })
        
        print(f"C({algo_a}, {algo_b}) = {coverage_a_b:.4f}")
        print(f"This means {coverage_a_b*100:.2f}% of {algo_b}'s solutions are weakly dominated by at least one solution from {algo_a}.")
    
    # Save metrics to CSV
    metrics_df = pd.DataFrame(coverage_results)
    metrics_file = results_dir / "binary-coverage-metrics.csv"
    metrics_df.to_csv(metrics_file, index=False)
    print(f"Binary coverage metrics saved to {metrics_file}")
    
    # Generate a detailed report
    report_file = results_dir / "binary_coverage_report.txt"
    with open(report_file, "w") as f:
        f.write("Binary Coverage Calculation Report\n")
        f.write("=================================\n\n")
        f.write("Implementation following:\n")
        f.write("1. 'Performance Assessment of Multiobjective Optimizers' (Zitzler et al., 2003)\n")
        f.write("2. 'Evolutionary algorithms for solving multi-objective problems' (Coello et al., 2007)\n\n")
        
        f.write("Binary Coverage Definition:\n")
        f.write("-------------------------\n")
        f.write("The binary coverage metric C(A,B) measures the fraction of solutions in set B\n")
        f.write("that are weakly dominated by at least one solution in set A.\n\n")
        
        f.write("Interpretation:\n")
        f.write("- C(A,B) = 1 means that all solutions in B are weakly dominated by solutions in A\n")
        f.write("- C(A,B) = 0 means that no solution in B is weakly dominated by any solution in A\n")
        f.write("- In general, higher values of C(A,B) indicate that A is better than B\n")
        f.write("- Note that C(A,B) and C(B,A) should be considered together for a complete comparison\n\n")
        
        f.write("Algorithm Results:\n")
        f.write("-----------------\n")
        
        # Add a summary table
        f.write("Summary Table:\n")
        f.write("Algorithm A | Algorithm B | C(A,B) | Interpretation\n")
        f.write("-----------|------------|---------|---------------\n")
        
        for result in coverage_results:
            algo_a = result["Algorithm_A"]
            algo_b = result["Algorithm_B"]
            coverage = result["Coverage_A_B"]
            
            # Generate interpretation
            if coverage > 0.75:
                interpretation = f"{algo_a} strongly dominates {algo_b}"
            elif coverage > 0.5:
                interpretation = f"{algo_a} moderately dominates {algo_b}"
            elif coverage > 0.25:
                interpretation = f"{algo_a} slightly dominates {algo_b}"
            elif coverage > 0:
                interpretation = f"{algo_a} weakly dominates some solutions of {algo_b}"
            else:
                interpretation = f"{algo_a} does not dominate any solution of {algo_b}"
            
            f.write(f"{algo_a:<11} | {algo_b:<10} | {coverage:.4f} | {interpretation}\n")
        
        f.write("\n\nDetailed Results:\n")
        f.write("----------------\n")
        
        for result in coverage_results:
            algo_a = result["Algorithm_A"]
            algo_b = result["Algorithm_B"]
            coverage = result["Coverage_A_B"]
            solutions_a = result["Solutions_A"]
            solutions_b = result["Solutions_B"]
            
            f.write(f"C({algo_a}, {algo_b}) = {coverage:.4f}\n")
            f.write(f"- {algo_a}: {solutions_a} non-dominated solutions\n")
            f.write(f"- {algo_b}: {solutions_b} non-dominated solutions\n")
            f.write(f"- {coverage*100:.2f}% of {algo_b}'s solutions are weakly dominated by at least one solution from {algo_a}\n\n")
    
    print(f"Detailed report saved to {report_file}")
    print(f"Debug log saved to {debug_log}")

class DebugTee:
    """Class to redirect stdout to both console and file"""
    def __init__(self, console_stream, file_stream):
        self.console_stream = console_stream
        self.file_stream = file_stream
        
    def write(self, message):
        self.console_stream.write(message)
        self.file_stream.write(message)
        
    def flush(self):
        self.console_stream.flush()
        self.file_stream.flush()

if __name__ == "__main__":
    main()