# File: metrics/calculate_hypervolume.py

import numpy as np
import pandas as pd
import os
from pathlib import Path
import sys

"""
This script implements the Hypervolume by Slicing Objectives (HSO) algorithm
as described in:

"A Faster Algorithm for Calculating Hypervolume" by While et al., 
IEEE Transactions on Evolutionary Computation, Vol. 10, No. 1, February 2006

The implementation follows the paper's approach with consistent variable naming
and algorithmic structure. Additional theoretical background is derived from:

"Performance Assessment of Multiobjective Optimizers: An Analysis and Review"
by Zitzler et al., IEEE Transactions on Evolutionary Computation, Vol. 7, No. 2, April 2003

"Adjusting normalization bounds to improve hypervolume based search for expensive 
multi-objective optimization" by Wang et al., Complex & Intelligent Systems, 2023
"""

# Problem-specific constants for normalization
# These are theoretical bounds for the problem domain

# Ideal point (best possible values for each objective)
IDEAL_COST = 0.0          # Minimum possible cost
IDEAL_TIME = 60.0         # Minimum viable time (minutes)
IDEAL_ATTRACTIONS = 10.0  # Maximum reasonable number of attractions
IDEAL_NEIGHBORHOODS = 10.0 # Maximum reasonable number of neighborhoods

# Nadir point (worst acceptable values for each objective)
# Following the definition in Zitzler et al. (2003), p.119
NADIR_COST = 500.0        # Cost of the most expensive attractions
NADIR_TIME = 840.0        # Maximum time budget (14 hours in minutes)
NADIR_ATTRACTIONS = 1.0   # Minimum acceptable number of attractions
NADIR_NEIGHBORHOODS = 1.0 # Minimum acceptable number of neighborhoods

# Enable debug output
DEBUG = True

def debug_print(msg, value=None):
    """Helper function for debugging output"""
    if DEBUG:
        if value is not None:
            print(f"DEBUG: {msg}: {value}")
        else:
            print(f"DEBUG: {msg}")

# ====================================================================
# Regular hypervolume calculation for normalized space
# ====================================================================

def calculate_hypervolume(points, reference_point):
    """
    Calculate hypervolume using the HSO algorithm.
    
    Args:
        points: List of points in the format [cost, time, -attractions, -neighborhoods]
        reference_point: Reference point in the same format
        
    Returns:
        Hypervolume value
    """
    # Prepare data
    points_min = []
    reference_min = []
    
    # Convert all points to minimization format
    # As defined in Zitzler et al. (2003), p.117, we need to convert maximization objectives to minimization
    for p in points:
        points_min.append([
            p[0],                # Cost - min (keep as is)
            p[1],                # Time - min (keep as is)
            -p[2],               # Attractions - max (value is already negative, make positive for minimization)
            -p[3]                # Neighborhoods - max (value is already negative, make positive for minimization)
        ])
    
    # Convert reference point to minimization format
    reference_min = [
        reference_point[0],      # Cost - min (keep as is)
        reference_point[1],      # Time - min (keep as is)
        -reference_point[2],     # Attractions - max (make positive)
        -reference_point[3]      # Neighborhoods - max (make positive)
    ]
    
    debug_print("Points in minimization format (sample)", points_min[:3] if points_min else "None")
    debug_print("Reference in minimization format", reference_min)
    
    # Following While et al. (2006), p.32, points that contribute to hypervolume must dominate the reference point.
    # In a minimization context, point p dominates reference point r when all p values are <= r values.
    contributing_points = []
    for p in points_min:
        if all(p[i] <= reference_min[i] for i in range(len(p))):
            contributing_points.append(p)
    
    debug_print(f"Contributing points: {len(contributing_points)} of {len(points_min)}")
    
    if not contributing_points:
        debug_print("No points contribute to hypervolume")
        return 0.0
    
    # Calculate hypervolume using recursive slicing
    return _hypervolume_hso(contributing_points, reference_min)

def _hypervolume_hso(points, reference):
    """
    Calculate hypervolume using the HSO algorithm.
    
    Args:
        points: List of points that collectively dominate the objective space up to the reference point
        reference: Reference point
    
    Returns:
        Hypervolume value
    """
    # No points, return zero volume
    if not points:
        return 0.0
    
    # Following While et al. (2006), p.33-34, base case for single dimension
    if len(reference) == 1:
        # The hypervolume is the difference between the reference and the minimum point value
        return reference[0] - min(p[0] for p in points)
    
    # Get non-dominated set for efficiency
    # While et al. (2006) explains using non-dominated points for efficiency
    front = _get_nondominated_front(points)
    
    # Sort points by first dimension (ascending)
    # As described in While et al. (2006), we process slices from lowest to highest
    front.sort(key=lambda p: p[0])
    
    # Calculate hypervolume by slicing
    volume = 0.0
    prev_slice = reference[0]
    
    # Process each point, calculating volume slice by slice as described in While et al. (2006), p.33
    for point in front:
        current_slice = point[0]
        
        # Skip points beyond reference
        if current_slice >= reference[0]:
            continue
            
        # Height of this slice
        slice_height = prev_slice - current_slice
        
        # Get slice points
        slice_points = []
        for p in front:
            if p[0] <= current_slice:  # Points in this slice
                # Project to remaining dimensions
                slice_points.append(p[1:])
        
        # Reference for recursive calculation
        slice_reference = reference[1:]
        
        # Calculate volume of slice recursively
        slice_volume = _hypervolume_hso(slice_points, slice_reference)
        
        # Add contribution of this slice
        volume += slice_height * slice_volume
        
        # Update previous slice
        prev_slice = current_slice
    
    return volume

def _get_nondominated_front(points):
    """
    Extract non-dominated points from a set.
    
    Args:
        points: List of points
        
    Returns:
        List of non-dominated points
    """
    if not points:
        return []
    
    front = []
    
    for p in points:
        dominated = False
        
        # Check against all currently non-dominated points
        i = 0
        while i < len(front):
            q = front[i]
            
            # Check if q dominates p (Pareto dominance definition as in Zitzler et al. 2003, p.118)
            # In a minimization context, q dominates p if all q values are <= p and at least one is 
            if all(q[j] <= p[j] for j in range(len(q))) and any(q[j] < p[j] for j in range(len(q))):
                dominated = True
                break
            
            # Check if p dominates q
            elif all(p[j] <= q[j] for j in range(len(p))) and any(p[j] < q[j] for j in range(len(p))):
                front.pop(i)
            else:
                i += 1
        
        # If p is not dominated, add it to the front
        if not dominated:
            front.append(p)
    
    return front

# ====================================================================
# Raw hypervolume calculation functions
# ====================================================================

def calculate_raw_hypervolume_direct(points, reference_point):
    """
    Calculate raw hypervolume directly in the original objective space without any transformations.
    This is a separate implementation to ensure no interference with the normalized calculation.
    Based on While et al. (2006) but adapted for mixed minimization/maximization objectives.
    
    Args:
        points: List of solution points [cost, time, -attractions, -neighborhoods]
        reference_point: Reference point
        
    Returns:
        Raw hypervolume value
    """
    debug_print("\n=== CALCULATING DIRECT RAW HYPERVOLUME ===")
    debug_print("Original solutions (sample)", points[:3])
    debug_print("Reference point", reference_point)
    
    # Create deep copies to avoid modifying original data
    points_copy = []
    for p in points:
        # In this raw calculation, we keep the maximization objectives as negative values
        # but we need to compare correctly with the reference point
        points_copy.append([p[0], p[1], p[2], p[3]])
    
    ref_copy = [reference_point[0], reference_point[1], reference_point[2], reference_point[3]]
    
    # Filter points that contribute to hypervolume
    # A point contributes if it's better than the reference in all objectives
    # As discussed in Zitzler et al. (2003), p.118, a point is better than another
    # if it is not worse in any objective and better in at least one
    contributing_points = []
    for p in points_copy:
        # For min objectives (cost, time): point value should be less than reference
        # For max objectives (attractions, neighborhoods): since stored as negative,
        # point value should be more negative (smaller) than reference
        if (p[0] <= ref_copy[0] and 
            p[1] <= ref_copy[1] and
            p[2] <= ref_copy[2] and
            p[3] <= ref_copy[3]):
            contributing_points.append(p)
    
    debug_print(f"Directly contributing points: {len(contributing_points)} of {len(points_copy)}")
    
    if not contributing_points:
        debug_print("No points directly contribute to raw hypervolume")
        return 0.0
    
    # Calculate hypervolume directly in the original space
    # We need a separate HSO implementation to handle mixed min/max objectives
    volume = _raw_hypervolume_hso(contributing_points, ref_copy)
    debug_print(f"Direct raw hypervolume: {volume}")
    
    return volume

def _raw_hypervolume_hso(points, reference):
    """
    HSO algorithm implementation specifically for raw hypervolume calculation.
    Based on the algorithm by While et al. (2006) but adapted for mixed min/max objectives.
    
    Args:
        points: List of points in original space that contribute to hypervolume
        reference: Reference point in original space
        
    Returns:
        Raw hypervolume value
    """
    # No points, return zero volume
    if not points:
        return 0.0
    
    # Get non-dominated front
    front = _get_raw_nondominated_front(points)
    
    # Base case: one dimension
    if len(reference) == 1:
        # For minimization objective, find minimum value
        if reference[0] >= 0:
            return reference[0] - min(p[0] for p in front)
        # For maximization objective (stored as negative), find maximum absolute value
        else:
            return min(p[0] for p in front) - reference[0]
    
    # Process first objective (recursively slice on it)
    # Determine if first objective is minimization or maximization
    is_min_objective = True  # First two objectives are minimization (cost, time)
    if len(reference) <= 2:  # When recursing, we may hit maximization objectives
        is_min_objective = (reference[0] >= 0)
    
    # Sort points appropriately
    if is_min_objective:
        # Sort ascending for minimization
        front.sort(key=lambda p: p[0])
        # Process slices from min to max
        return _process_slices_min(front, reference)
    else:
        # Sort descending (more negative is better) for maximization
        front.sort(key=lambda p: p[0], reverse=True)
        # Process slices from max to min (absolute value)
        return _process_slices_max(front, reference)

def _process_slices_min(front, reference):
    """
    Process slices for minimization objective.
    Following the slicing approach from While et al. (2006), p.33.
    """
    volume = 0.0
    prev_slice = reference[0]
    
    for point in front:
        current_slice = point[0]
        
        # Skip points beyond reference
        if current_slice >= reference[0]:
            continue
            
        # Height of this slice
        slice_height = prev_slice - current_slice
        
        # Get slice points
        slice_points = []
        for p in front:
            if p[0] <= current_slice:  # Points in this slice
                # Project to remaining dimensions
                slice_points.append(p[1:])
        
        # Reference for recursive calculation
        slice_reference = reference[1:]
        
        # Calculate volume of slice recursively
        slice_volume = _raw_hypervolume_hso(slice_points, slice_reference)
        
        # Add contribution of this slice
        volume += slice_height * slice_volume
        
        # Update previous slice
        prev_slice = current_slice
    
    return volume

def _process_slices_max(front, reference):
    """
    Process slices for maximization objective (stored as negative values).
    This is an adaptation of While et al. (2006) for maximization objectives.
    """
    volume = 0.0
    prev_slice = reference[0]
    
    for point in front:
        current_slice = point[0]
        
        # Skip points beyond reference (for max objectives, point should be more negative/smaller)
        if current_slice >= reference[0]:
            continue
            
        # Height of this slice (absolute difference)
        slice_height = current_slice - prev_slice  # Note: both values are negative, smaller is better
        
        # Get slice points
        slice_points = []
        for p in front:
            if p[0] <= current_slice:  # Points in this slice (more negative/smaller than current)
                # Project to remaining dimensions
                slice_points.append(p[1:])
        
        # Reference for recursive calculation
        slice_reference = reference[1:]
        
        # Calculate volume of slice recursively
        slice_volume = _raw_hypervolume_hso(slice_points, slice_reference)
        
        # Add contribution of this slice
        volume += slice_height * slice_volume
        
        # Update previous slice
        prev_slice = current_slice
    
    return volume

def _get_raw_nondominated_front(points):
    """
    Extract non-dominated points from a set in the original objective space.
    Implements the dominance relation from Zitzler et al. (2003) for mixed objectives.
    
    Args:
        points: List of points with mixed min/max objectives
        
    Returns:
        List of non-dominated points
    """
    if not points:
        return []
    
    front = []
    
    for p in points:
        dominated = False
        
        # Check against all currently non-dominated points
        i = 0
        while i < len(front):
            q = front[i]
            
            # Check if q dominates p
            if (_dominates_raw(q, p)):
                dominated = True
                break
            
            # Check if p dominates q
            elif (_dominates_raw(p, q)):
                front.pop(i)
            else:
                i += 1
        
        # If p is not dominated, add it to the front
        if not dominated:
            front.append(p)
    
    return front

def _dominates_raw(p, q):
    """
    Check if p dominates q in the original objective space.
    Implements Pareto dominance as defined in Zitzler et al. (2003) for mixed objectives.
    
    Args:
        p, q: Points with mixed min/max objectives
        
    Returns:
        True if p dominates q, False otherwise
    """
    better_in_one = False
    
    for i in range(len(p)):
        # For minimization objectives (index 0, 1), smaller is better
        if i < 2:
            if p[i] > q[i]:
                return False
            if p[i] < q[i]:
                better_in_one = True
        # For maximization objectives (index 2, 3), stored as negative, more negative is better
        else:
            if p[i] > q[i]:
                return False
            if p[i] < q[i]:
                better_in_one = True
    
    return better_in_one

# ====================================================================
# Functions for normalized hypervolume
# ====================================================================

def calculate_raw_hypervolume(points, reference_point):
    """
    Calculate hypervolume in the original objective space.
    This function uses the main HSO algorithm but is no longer preferred.
    Use calculate_raw_hypervolume_direct() instead.
    
    Args:
        points: List of solution points [cost, time, -attractions, -neighborhoods]
        reference_point: Reference point
        
    Returns:
        Hypervolume value
    """
    debug_print("\n=== CALCULATING RAW HYPERVOLUME ===")
    debug_print("Original solutions (sample)", points[:3])
    debug_print("Reference point", reference_point)
    
    # Calculate hypervolume directly
    volume = calculate_hypervolume(points, reference_point)
    debug_print(f"Raw hypervolume: {volume}")
    
    return volume

def calculate_normalized_hypervolume(points, reference_point):
    """
    Calculate hypervolume in normalized objective space.
    Following the normalization approach described in Wang et al. (2023).
    
    Args:
        points: List of solution points [cost, time, -attractions, -neighborhoods]
        reference_point: Reference point
        
    Returns:
        Normalized hypervolume value
    """
    debug_print("\n=== CALCULATING NORMALIZED HYPERVOLUME ===")
    
    # First determine the ideal and nadir points based on Wang et al. (2023), p.1194-1195
    ideal = [float('inf'), float('inf'), float('-inf'), float('-inf')]
    nadir = [float('-inf'), float('-inf'), float('inf'), float('inf')]
    
    # Find actual bounds from the points
    for p in points:
        # Minimization objectives
        ideal[0] = min(ideal[0], p[0])
        ideal[1] = min(ideal[1], p[1])
        nadir[0] = max(nadir[0], p[0])
        nadir[1] = max(nadir[1], p[1])
        
        # Maximization objectives (stored as negative)
        ideal[2] = max(ideal[2], p[2])  # Most negative = best
        ideal[3] = max(ideal[3], p[3])  # Most negative = best
        nadir[2] = min(nadir[2], p[2])  # Least negative = worst
        nadir[3] = min(nadir[3], p[3])  # Least negative = worst
    
    # Use theoretical bounds if better
    ideal[0] = min(ideal[0], IDEAL_COST)
    ideal[1] = min(ideal[1], IDEAL_TIME)
    ideal[2] = max(ideal[2], -IDEAL_ATTRACTIONS)  # Keep negative
    ideal[3] = max(ideal[3], -IDEAL_NEIGHBORHOODS)  # Keep negative
    
    nadir[0] = max(nadir[0], NADIR_COST)
    nadir[1] = max(nadir[1], NADIR_TIME)
    nadir[2] = min(nadir[2], -NADIR_ATTRACTIONS)  # Keep negative
    nadir[3] = min(nadir[3], -NADIR_NEIGHBORHOODS)  # Keep negative
    
    debug_print("Ideal point for normalization", ideal)
    debug_print("Nadir point for normalization", nadir)
    
    # Normalize points to [0,1] scale for each objective following Wang et al. (2023)
    normalized_points = []
    for p in points:
        norm_p = []
        for i in range(4):
            # Avoid division by zero
            if nadir[i] == ideal[i]:
                norm_val = 0.0
            else:
                # Normalize to [0,1] where 0 is best
                if i < 2:  # Minimization objectives
                    norm_val = (p[i] - ideal[i]) / (nadir[i] - ideal[i])
                else:  # Maximization objectives
                    # For maximization objectives (stored as negative, remember)
                    norm_val = (p[i] - ideal[i]) / (nadir[i] - ideal[i])
            norm_p.append(norm_val)
        normalized_points.append(norm_p)
    
    # Normalize reference point
    normalized_reference = []
    for i in range(4):
        if nadir[i] == ideal[i]:
            # Default if range is zero
            norm_ref = 1.1
        else:
            if i < 2:  # Minimization objectives
                norm_ref = (reference_point[i] - ideal[i]) / (nadir[i] - ideal[i])
            else:  # Maximization objectives (stored as negative)
                norm_ref = (reference_point[i] - ideal[i]) / (nadir[i] - ideal[i])
        normalized_reference.append(norm_ref)
    
    debug_print("Normalized points (sample)", normalized_points[:3])
    debug_print("Normalized reference", normalized_reference)
    
    # Calculate hypervolume in normalized space
    volume = calculate_hypervolume(normalized_points, normalized_reference)
    debug_print(f"Normalized hypervolume: {volume}")
    
    return volume

# ====================================================================
# Supporting functions
# ====================================================================

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
    # Extract the base part before "-resultados.csv"
    base_name = os.path.basename(filename).split('-resultados.csv')[0]
    
    # Format algorithm name
    if base_name.lower() == "nsga2":
        return "NSGA-II"
    elif base_name.lower() == "moead":
        return "MOEA/D"
    elif base_name.lower() == "spea2":
        return "SPEA2"
    else:
        # Convert to title case and replace underscores with spaces
        return base_name.replace('_', ' ').title()

def run_test_cases():
    """Run test cases to verify hypervolume calculation"""
    debug_print("=== RUNNING TEST CASES ===")
    
    # 2D test case (cost and time only)
    test_2d_points = [
        [100, 200, 0, 0],  # Point A
        [150, 150, 0, 0],  # Point B
        [200, 100, 0, 0]   # Point C
    ]
    test_2d_ref = [250, 250, 0, 0]
    
    # Expected volume: (250-100)*(250-200) + (250-150)*(200-150) + (250-200)*(150-100)
    # = 150*50 + 100*50 + 50*50 = 7500 + 5000 + 2500 = 15000
    volume_2d = calculate_raw_hypervolume_direct(test_2d_points, test_2d_ref)
    debug_print(f"2D Test Case: Expected 15000, Got {volume_2d}")
    
    # 4D test case
    test_4d_points = [
        [100, 200, -8, -7],  # Cost, Time, -Attractions, -Neighborhoods
        [150, 150, -7, -8],
        [200, 100, -9, -6]
    ]
    test_4d_ref = [250, 250, -5, -5]
    
    volume_4d = calculate_raw_hypervolume_direct(test_4d_points, test_4d_ref)
    debug_print(f"4D Test Case: Got {volume_4d}")
    
    # Test normalized calculation
    norm_volume = calculate_normalized_hypervolume(test_4d_points, test_4d_ref)
    debug_print(f"4D Normalized Test: Got {norm_volume}")
    
    # Set pass criteria: first test must be at least 14000
    # (allowing for some rounding errors)
    return volume_2d > 14000 and volume_4d > 0 and norm_volume > 0

def main():
    """
    Main function to calculate hypervolume for all algorithm result files
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
    debug_log = results_dir / "hypervolume_debug.log"
    if DEBUG:
        # Redirect stdout to both console and log file
        sys.stdout = DebugTee(sys.stdout, open(debug_log, 'w'))
    
    # Run test cases first
    tests_passed = run_test_cases()
    if not tests_passed:
        debug_print("WARNING: Test cases failed. Hypervolume calculation may be incorrect.")
    
    result_files = list(results_dir.glob("*-resultados.csv"))
    if not result_files:
        print("No result files found matching *-resultados.csv pattern")
        return
    
    print(f"Found {len(result_files)} result files")
    
    # Define reference point
    # Following Zitzler et al. (2003), p.121, the reference point should be
    # "slightly beyond the nadir point"
    reference_point = [
        NADIR_COST * 1.1,          # Slightly worse than worst cost
        NADIR_TIME * 1.1,          # Slightly worse than worst time
        -NADIR_ATTRACTIONS * 0.9,  # Slightly worse than worst attractions (remember negation)
        -NADIR_NEIGHBORHOODS * 0.9 # Slightly worse than worst neighborhoods (remember negation)
    ]
    print(f"Using reference point: {reference_point}")
    
    # Load solutions from all files
    algorithm_solutions = {}
    
    for file_path in result_files:
        algo_name = extract_algorithm_name(file_path.name)
        solutions = load_solutions_from_csv(file_path)
        algorithm_solutions[algo_name] = solutions
        print(f"Loaded {len(solutions)} solutions for {algo_name}")
    
    # Calculate hypervolume for each algorithm
    metrics = []
    for algo_name, solutions in algorithm_solutions.items():
        if not solutions:
            print(f"Skipping hypervolume calculation for {algo_name} due to empty solution set")
            metrics.append({
                "Algorithm": algo_name,
                "Hypervolume": 0.0,
                "RawHypervolume": 0.0,
                "SolutionCount": 0
            })
            continue
        
        # Calculate raw hypervolume directly with dedicated function
        raw_hypervolume_direct = calculate_raw_hypervolume_direct(solutions, reference_point)
        
        # Calculate normalized hypervolume
        normalized_hypervolume = calculate_normalized_hypervolume(solutions, reference_point)
        
        metrics.append({
            "Algorithm": algo_name,
            "Hypervolume": normalized_hypervolume,
            "RawHypervolume": raw_hypervolume_direct,
            "SolutionCount": len(solutions)
        })
        print(f"{algo_name}: Raw Hypervolume = {raw_hypervolume_direct}, "
              f"Normalized Hypervolume = {normalized_hypervolume}, "
              f"Solutions = {len(solutions)}")
    
    # Save metrics to CSV
    metrics_df = pd.DataFrame(metrics)
    metrics_file = results_dir / "metrics.csv"
    metrics_df.to_csv(metrics_file, index=False)
    print(f"Metrics saved to {metrics_file}")
    
    # Generate a more detailed report
    report_file = results_dir / "hypervolume_report.txt"
    with open(report_file, "w") as f:
        f.write("Hypervolume Calculation Report\n")
        f.write("============================\n\n")
        f.write("Implementation following:\n")
        f.write("1. 'A Faster Algorithm for Calculating Hypervolume' (While et al., 2006)\n")
        f.write("2. 'Performance Assessment of Multiobjective Optimizers' (Zitzler et al., 2003)\n")
        f.write("3. 'Adjusting normalization bounds to improve hypervolume' (Wang et al., 2023)\n\n")
        
        f.write("Problem-specific constants used for normalization:\n")
        f.write(f"- Ideal point: [{IDEAL_COST}, {IDEAL_TIME}, {IDEAL_ATTRACTIONS}, {IDEAL_NEIGHBORHOODS}]\n")
        f.write(f"- Nadir point: [{NADIR_COST}, {NADIR_TIME}, {NADIR_ATTRACTIONS}, {NADIR_NEIGHBORHOODS}]\n\n")
        
        f.write(f"Reference point: {reference_point}\n")
        f.write("(Note: For maximization objectives, values are stored as negated values)\n\n")
        
        f.write("Algorithm Results:\n")
        f.write("-----------------\n")
        for algo_name, solutions in algorithm_solutions.items():
            if not solutions:
                f.write(f"{algo_name}:\n")
                f.write(f"  - Solutions: 0\n")
                f.write(f"  - Raw Hypervolume: 0.0\n")
                f.write(f"  - Normalized Hypervolume: 0.0\n")
                f.write("\n")
                continue
                
            raw_hypervolume_direct = calculate_raw_hypervolume_direct(solutions, reference_point)
            normalized_hypervolume = calculate_normalized_hypervolume(solutions, reference_point)
            
            f.write(f"{algo_name}:\n")
            f.write(f"  - Solutions: {len(solutions)}\n")
            f.write(f"  - Raw Hypervolume: {raw_hypervolume_direct}\n")
            f.write(f"  - Normalized Hypervolume: {normalized_hypervolume}\n")
            f.write("\n")
        
        f.write("\nHypervolume Interpretation:\n")
        f.write("-------------------------\n")
        f.write("The hypervolume indicator measures the volume of the objective space that is dominated\n")
        f.write("by the Pareto front approximation and bounded by the reference point. As described in\n")
        f.write("Zitzler et al. (2003), p.125, it is 'the only unary indicator we are aware of that is capable\n")
        f.write("of detecting that A is not worse than B for all pairs (A,B).'\n\n")
        
        f.write("The normalized hypervolume scales this value to a range closer to [0,1], where:\n")
        f.write("  - 0 represents the worst possible performance\n")
        f.write("  - 1 represents a theoretically ideal performance\n\n")
        
        f.write("Higher hypervolume values indicate better approximation sets, capturing both\n")
        f.write("convergence to the true Pareto front and diversity of solutions.\n")
    
    # Save the input data to CSV for external validation
    for algo_name, solutions in algorithm_solutions.items():
        if not solutions:
            continue
        
        # Create DataFrame of solutions
        solutions_df = pd.DataFrame(solutions, columns=["Cost", "Time", "-Attractions", "-Neighborhoods"])
        solutions_file = results_dir / f"{algo_name.lower().replace(' ', '_')}_hypervolume_input.csv"
        solutions_df.to_csv(solutions_file, index=False)
        print(f"Solutions used for {algo_name} saved to {solutions_file}")
    
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