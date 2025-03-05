// File: src/hypervolume.cpp

#include "hypervolume.hpp"
#include <iostream> 

namespace tourist {
namespace utils {

// Point implementation
HypervolumeCalculator::Point::Point(const Solution& solution) {
    this->objectives = solution.getObjectives();
}

bool HypervolumeCalculator::Point::dominates(const Point& other, size_t k) const {
    // For HSO algorithm with mixed objectives (minimization/maximization)
    // As per While et al. 2006: "A point dominates another if it's at least as good 
    // in all objectives and strictly better in at least one"
    bool strictly_better = false;
    
    for (size_t i = k; i < objectives.size(); ++i) {
        if (i == 2) { // Third objective (attractions visited) is to be maximized
            // For maximization, higher values are better
            if (objectives[i] < other.objectives[i]) return false;
            if (objectives[i] > other.objectives[i]) strictly_better = true;
        } else { // First two objectives (cost and time) are to be minimized
            // For minimization, lower values are better
            if (objectives[i] > other.objectives[i]) return false;
            if (objectives[i] < other.objectives[i]) strictly_better = true;
        }
    }
    
    return strictly_better;
}

bool HypervolumeCalculator::Point::isDominatedBy(const Point& other, size_t k) const {
    return other.dominates(*this, k);
}

// Main calculate function
double HypervolumeCalculator::calculate(
    const std::vector<Solution>& solutions, 
    const std::vector<double>& reference_point
) {
    if (solutions.empty()) return 0.0;
    
    // Convert solutions to points
    std::vector<Point> points;
    points.reserve(solutions.size());
    for (const auto& solution : solutions) {
        points.emplace_back(solution);
    }
    
    // Check for dimensionality mismatch
    size_t num_objectives = reference_point.size();
    for (const auto& point : points) {
        if (point.objectives.size() != num_objectives) {
            throw std::runtime_error("Dimensions mismatch between solutions and reference point");
        }
    }
    
    // Verify if reference point is valid (not dominated by any solution)
    // For mixed objectives, the reference point validity check needs to consider
    // direction of optimization for each objective
    bool reference_is_valid = true;
    for (const auto& point : points) {
        bool point_dominates_reference = true;
        for (size_t i = 0; i < num_objectives; ++i) {
            if (i == 2) { // Maximization objective (attractions)
                // For maximization, reference should be worse (lower) than the point
                if (point.objectives[i] <= reference_point[i]) {
                    point_dominates_reference = false;
                    break;
                }
            } else { // Minimization objectives (cost, time)
                // For minimization, reference should be worse (higher) than the point
                if (point.objectives[i] >= reference_point[i]) {
                    point_dominates_reference = false;
                    break;
                }
            }
        }
        
        if (point_dominates_reference) {
            reference_is_valid = false;
            break;
        }
    }
    
    // If reference point is invalid, adjust to a valid value
    // This ensures the reference point is worse than all points in all objectives
    std::vector<double> adjusted_reference(num_objectives);
    if (!reference_is_valid) {
        for (size_t i = 0; i < num_objectives; ++i) {
            if (i == 2) { // Maximization objective (attractions)
                // For maximization, find the maximum value and make reference worse (lower)
                double max_value = std::numeric_limits<double>::lowest();
                for (const auto& point : points) {
                    max_value = std::max(max_value, point.objectives[i]);
                }
                // Subtract a margin to ensure it's worse than all points
                double margin = std::max(0.1 * std::abs(max_value), 1.0);
                adjusted_reference[i] = max_value - margin;
            } else { // Minimization objectives (cost, time)
                // For minimization, find the minimum value and make reference worse (higher)
                double min_value = std::numeric_limits<double>::max();
                for (const auto& point : points) {
                    min_value = std::min(min_value, point.objectives[i]);
                }
                // Add a margin to ensure it's worse than all points
                double margin = std::max(0.1 * std::abs(min_value), 1.0);
                adjusted_reference[i] = min_value + margin;
            }
        }
    } else {
        adjusted_reference = reference_point;
    }
    
    // Call the recursive HSO algorithm with the adjusted reference point
    double volume = hso(std::move(points), 0, num_objectives, adjusted_reference).volume;
    
    return volume;
}

// HSO algorithm as described in While et al. (2006)
// "A Faster Algorithm for Calculating Hypervolume"
HypervolumeCalculator::HVResult HypervolumeCalculator::hso(
    std::vector<Point> points, 
    size_t k, 
    size_t n, 
    const std::vector<double>& reference_point
) {
    HVResult result;
    
    // Base case: k = n-2 means we have 2 objectives left (2D case)
    if (k == n - 2) {
        // Filter dominated points before calculating 2D hypervolume
        points = filterDominated(points, k);
        result.volume = calculate2D(points, 
                                   {reference_point[n-2], reference_point[n-1]},
                                   n-2 == 2 || n-1 == 2); // Check if either dimension is the maximization objective
        result.points = std::move(points);
        return result;
    }
    
    // Filter dominated points in current dimension and above
    points = filterDominated(points, k);
    
    // If no points left after filtering, return zero volume
    if (points.empty()) {
        result.volume = 0.0;
        return result;
    }
    
    // Sort points by kth objective
    // For minimization (objectives 0, 1): ascending order 
    // For maximization (objective 2): descending order
    if (k == 2) { // Maximization objective
        std::sort(points.begin(), points.end(), 
                [k](const Point& a, const Point& b) {
                    return a.objectives[k] > b.objectives[k]; // Descending for maximization
                });
    } else { // Minimization objectives
        std::sort(points.begin(), points.end(), 
                [k](const Point& a, const Point& b) {
                    return a.objectives[k] < b.objectives[k]; // Ascending for minimization
                });
    }
    
    // Calculate hypervolume by slicing
    // This follows the HSO algorithm in While et al. (2006)
    double volume = 0.0;
    double prev_slice = reference_point[k];
    
    for (size_t i = 0; i < points.size(); ++i) {
        // Current point defines the bound of the current slice
        double current_slice = points[i].objectives[k];
        
        // Skip if point doesn't contribute to hypervolume in this dimension
        // For minimization: skip if point's value >= reference point
        // For maximization: skip if point's value <= reference point
        if (k == 2) { // Maximization
            if (current_slice <= reference_point[k]) continue;
        } else { // Minimization
            if (current_slice >= reference_point[k]) continue;
        }
        
        // Calculate the height of the slice
        // For minimization: height = previous_value - current_value
        // For maximization: height = current_value - previous_value
        double slice_height;
        if (k == 2) { // Maximization
            slice_height = current_slice - prev_slice;
        } else { // Minimization
            slice_height = prev_slice - current_slice;
        }
        
        // Create a sublist of points for this slice (all points from i onwards)
        std::vector<Point> slice_points(points.begin() + i, points.end());
        
        // Process the next dimension recursively
        HVResult subresult = hso(slice_points, k + 1, n, reference_point);
        
        // Multiply the (n-1)-dimensional volume by the height of the slice
        volume += slice_height * subresult.volume;
        
        // Update for next slice
        prev_slice = current_slice;
    }
    
    result.volume = volume;
    result.points = std::move(points);
    return result;
}

// Alternative slice implementation (not used directly, but kept for reference)
double HypervolumeCalculator::slice(
    std::vector<Point>& points, 
    size_t k, 
    size_t n, 
    const std::vector<double>& reference_point
) {
    // Filter dominated points
    points = filterDominated(points, k);
    
    // If no points left, return zero volume
    if (points.empty()) return 0.0;
    
    // If we're down to 2 dimensions, use the optimized 2D calculation
    if (k == n - 2) {
        return calculate2D(points, 
                         {reference_point[n-2], reference_point[n-1]}, 
                         n-2 == 2 || n-1 == 2); // Check if either dimension is for maximization
    }
    
    // Sort points by kth objective (ascending for min, descending for max)
    if (k == 2) { // Maximization
        std::sort(points.begin(), points.end(), 
                [k](const Point& a, const Point& b) {
                    return a.objectives[k] > b.objectives[k]; 
                });
    } else { // Minimization
        std::sort(points.begin(), points.end(), 
                [k](const Point& a, const Point& b) {
                    return a.objectives[k] < b.objectives[k]; 
                });
    }
    
    // Calculate volume by slicing
    double volume = 0.0;
    double prev_slice = reference_point[k];
    
    for (size_t i = 0; i < points.size(); ++i) {
        double current_slice = points[i].objectives[k];
        
        // Skip if point doesn't contribute
        if (k == 2) { // Maximization
            if (current_slice <= reference_point[k]) continue;
        } else { // Minimization
            if (current_slice >= reference_point[k]) continue;
        }
        
        // Calculate slice height
        double slice_height;
        if (k == 2) { // Maximization
            slice_height = current_slice - prev_slice;
        } else { // Minimization
            slice_height = prev_slice - current_slice;
        }
        
        // Create a sublist for the next dimension
        std::vector<Point> slice_points(points.begin() + i, points.end());
        
        // Process next dimension
        double subvolume = slice(slice_points, k + 1, n, reference_point);
        
        // Accumulate volume
        volume += slice_height * subvolume;
        
        // Update for next slice
        prev_slice = current_slice;
    }
    
    return volume;
}

// Special case calculation for 2D hypervolume
// The isMaximization parameter indicates if the second dimension is to be maximized
double HypervolumeCalculator::calculate2D(
    const std::vector<Point>& points, 
    const std::vector<double>& reference_point,
    bool isMaximization
) {
    // If no points, return zero
    if (points.empty()) return 0.0;
    
    // Make a local copy of points for sorting
    std::vector<Point> sorted_points = points;
    
    // Sort points by first dimension
    // For dim0, always minimize (cost, time)
    std::sort(sorted_points.begin(), sorted_points.end(), 
              [](const Point& a, const Point& b) {
                  return a.objectives[0] < b.objectives[0];
              });
    
    // Calculate 2D volume
    double volume = 0.0;
    double prev_x = reference_point[0];
    double best_y = reference_point[1]; // Start with reference (worst) value
    
    for (const auto& point : sorted_points) {
        double x = point.objectives[0];
        double y = point.objectives[1];
        
        // Skip points that don't contribute
        // For both dims being minimization:
        //   Skip if x >= ref_x or y >= ref_y
        // If dim1 is maximization:
        //   Skip if x >= ref_x or y <= ref_y
        if (!isMaximization) {
            if (x >= reference_point[0] || y >= reference_point[1]) continue;
        } else {
            if (x >= reference_point[0] || y <= reference_point[1]) continue;
        }
        
        // Update the best y seen so far
        // For minimization: best is min value
        // For maximization: best is max value
        bool is_better;
        if (!isMaximization) {
            is_better = y < best_y;
        } else {
            is_better = y > best_y;
        }
        
        if (is_better) {
            // Calculate area contribution
            double width = prev_x - x; // Always prev - current for minimization in dim0
            double height;
            
            if (!isMaximization) {
                height = reference_point[1] - y; // ref - current for minimization
            } else {
                height = y - reference_point[1]; // current - ref for maximization
            }
            
            volume += width * height;
            best_y = y;
            prev_x = x;
        }
    }
    
    return volume;
}

// Filter dominated points based on dominance in objectives [k...n]
// As per While et al. (2006), this step is crucial for efficiency
std::vector<HypervolumeCalculator::Point> HypervolumeCalculator::filterDominated(
    const std::vector<Point>& points, 
    size_t k
) {
    // If less than 2 points, nothing can be dominated
    if (points.size() < 2) return points;
    
    std::vector<Point> non_dominated;
    non_dominated.reserve(points.size());
    
    for (const auto& point : points) {
        bool is_dominated = false;
        
        // Check if this point is dominated by any points already in non_dominated
        for (const auto& other : non_dominated) {
            if (point.isDominatedBy(other, k)) {
                is_dominated = true;
                break;
            }
        }
        
        if (!is_dominated) {
            // Before adding, remove any points in non_dominated that this point dominates
            auto it = std::remove_if(non_dominated.begin(), non_dominated.end(),
                                    [&point, k](const Point& other) {
                                        return other.isDominatedBy(point, k);
                                    });
            non_dominated.erase(it, non_dominated.end());
            
            // Add this point
            non_dominated.push_back(point);
        }
    }
    
    return non_dominated;
}

// Calculate exclusive volume contribution of a point
double HypervolumeCalculator::exclusiveVolume(
    const Point& point, 
    const std::vector<Point>& other_points,
    const std::vector<double>& reference_point
) {
    std::vector<Point> all_points = other_points;
    all_points.push_back(point);
    
    // For mixed objectives, we need to check if any dimension is maximization
    bool has_max_objective = reference_point.size() > 2; // In our case, with 3 objectives, the 3rd is maximization
    
    // Calculate total volume with all points
    double total_volume = calculate2D(all_points, reference_point, has_max_objective);
    
    // Calculate volume without the specific point
    double volume_without_point = calculate2D(other_points, reference_point, has_max_objective);
    
    // The exclusive contribution is the difference
    return total_volume - volume_without_point;
}

// HypervolumeMetrics implementation
double HypervolumeMetrics::calculateHypervolume(
    const std::vector<Solution>& solutions, 
    const std::vector<double>& reference_point
) {
    return HypervolumeCalculator::calculate(solutions, reference_point);
}

std::vector<double> HypervolumeMetrics::calculateContributions(
    const std::vector<Solution>& solutions, 
    const std::vector<double>& reference_point
) {
    std::vector<double> contributions(solutions.size(), 0.0);
    
    // Calculate total hypervolume
    double total_volume = calculateHypervolume(solutions, reference_point);
    
    // Calculate hypervolume without each solution
    for (size_t i = 0; i < solutions.size(); ++i) {
        std::vector<Solution> subset = solutions;
        subset.erase(subset.begin() + i);
        
        double volume_without_i = calculateHypervolume(subset, reference_point);
        contributions[i] = total_volume - volume_without_i;
    }
    
    return contributions;
}

std::vector<double> HypervolumeMetrics::calculateExclusiveContributions(
    const std::vector<Solution>& solutions, 
    const std::vector<double>& reference_point
) {
    // This is equivalent to calculateContributions for hypervolume
    return calculateContributions(solutions, reference_point);
}

} // namespace utils
} // namespace tourist