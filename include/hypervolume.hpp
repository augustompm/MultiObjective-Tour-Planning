// File: include/hypervolume.hpp

#pragma once

#include <vector>
#include <algorithm>
#include <functional>
#include <numeric>
#include <limits>
#include <cmath>
#include "models.hpp"

namespace tourist {
namespace utils {

/**
 * @class HypervolumeCalculator
 * @brief Implements the Hypervolume by Slicing Objectives (HSO) algorithm for calculating hypervolume
 *
 * This class provides an efficient implementation of the HSO algorithm as described in:
 * "A Faster Algorithm for Calculating Hypervolume" by While et al., IEEE Transactions on Evolutionary Computation, 2006.
 * 
 * The algorithm works by processing points one objective at a time, creating slices through the 
 * hypervolume. Each slice is processed recursively in fewer dimensions until reaching the base case
 * of two dimensions, which is handled as a special case for efficiency.
 */
class HypervolumeCalculator {
public:
    /**
     * @brief Calculates the hypervolume of a set of solutions
     * 
     * @param solutions Vector of solutions for which to calculate the hypervolume
     * @param reference_point The reference point (typically the anti-optimal point)
     * @return The hypervolume value
     */
    static double calculate(const std::vector<Solution>& solutions, 
                           const std::vector<double>& reference_point);

private:
    /**
     * @brief Internal structure to represent a point with its objective values
     */
    struct Point {
        std::vector<double> objectives;
        
        /**
         * @brief Constructs a Point from a solution
         * @param solution The solution to convert
         */
        explicit Point(const Solution& solution);
        
        /**
         * @brief Checks if this point dominates another point in the remaining objectives [k...n]
         * 
         * @param other The other point to compare against
         * @param k Starting objective index
         * @return true if this point dominates the other, false otherwise
         */
        bool dominates(const Point& other, size_t k) const;
        
        /**
         * @brief Checks if this point is dominated by another point in the remaining objectives [k...n]
         * 
         * @param other The other point to compare against
         * @param k Starting objective index
         * @return true if this point is dominated by the other, false otherwise
         */
        bool isDominatedBy(const Point& other, size_t k) const;
    };

    /**
     * @brief Result structure for the recursive hypervolume calculation
     */
    struct HVResult {
        double volume;      ///< The calculated hypervolume
        std::vector<Point> points; ///< The contributing points (after removing dominated points)
    };

    /**
     * @brief The main recursive HSO algorithm implementation
     * 
     * @param points Vector of points
     * @param k Current dimension being processed (starting from 0)
     * @param n Total number of dimensions
     * @param reference_point The reference point
     * @return HVResult containing the calculated hypervolume and relevant points
     */
    static HVResult hso(std::vector<Point> points, size_t k, size_t n, 
                        const std::vector<double>& reference_point);
    
    /**
     * @brief Slices the hypervolume in the k-th dimension
     * 
     * Takes a list of points and creates slices through the hypervolume in the k-th dimension.
     * Each slice is processed recursively in dimensions k+1...n.
     * 
     * @param points Vector of points to process
     * @param k Current dimension being processed (starting from 0)
     * @param n Total number of dimensions
     * @param reference_point The reference point
     * @return The hypervolume of the points
     */
    static double slice(std::vector<Point>& points, size_t k, size_t n,
                       const std::vector<double>& reference_point);
    
    /**
     * @brief Special case handler for 2D hypervolume calculation
     * 
     * A fast implementation for the 2D case which is used as the base case
     * for the recursive algorithm.
     * 
     * @param points Vector of points (assumed to be in 2D)
     * @param reference_point The reference point (for the 2D case)
     * @param isMaximization Whether the second dimension is to be maximized
     * @return The 2D hypervolume
     */
    static double calculate2D(const std::vector<Point>& points, 
                             const std::vector<double>& reference_point,
                             bool isMaximization = false);
    
    /**
     * @brief Removes points that are dominated in objectives [k...n]
     * 
     * @param points Vector of points to filter
     * @param k Starting objective index
     * @return Vector of non-dominated points
     */
    static std::vector<Point> filterDominated(const std::vector<Point>& points, size_t k);
    
    /**
     * @brief Computes the exclusive hypervolume contribution of a point
     * 
     * @param point The point for which to calculate the exclusive contribution
     * @param other_points The set of other points
     * @param reference_point The reference point
     * @return The exclusive hypervolume contribution
     */
    static double exclusiveVolume(const Point& point, 
                                 const std::vector<Point>& other_points,
                                 const std::vector<double>& reference_point);
};

/**
 * @class HypervolumeMetrics
 * @brief Provides utility functions related to hypervolume metrics
 */
class HypervolumeMetrics {
public:
    /**
     * @brief Calculates hypervolume for a set of solutions
     * 
     * This is the main interface function for hypervolume calculation.
     * It uses the HypervolumeCalculator internally.
     * 
     * @param solutions Vector of solutions
     * @param reference_point The reference point
     * @return The hypervolume value
     */
    static double calculateHypervolume(const std::vector<Solution>& solutions, 
                                     const std::vector<double>& reference_point);
    
    /**
     * @brief Calculates the hypervolume contribution of each solution
     * 
     * @param solutions Vector of solutions
     * @param reference_point The reference point
     * @return Vector of hypervolume contributions, one for each solution
     */
    static std::vector<double> calculateContributions(const std::vector<Solution>& solutions, 
                                                    const std::vector<double>& reference_point);
    
    /**
     * @brief Calculates the exclusive hypervolume contribution of each solution
     * 
     * @param solutions Vector of solutions
     * @param reference_point The reference point
     * @return Vector of exclusive hypervolume contributions, one for each solution
     */
    static std::vector<double> calculateExclusiveContributions(const std::vector<Solution>& solutions, 
                                                             const std::vector<double>& reference_point);
};

} // namespace utils
} // namespace tourist