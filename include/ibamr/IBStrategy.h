// ---------------------------------------------------------------------
//
// Copyright (c) 2014 - 2024 by the IBAMR developers
// All rights reserved.
//
// This file is part of IBAMR.
//
// IBAMR is free software and is distributed under the 3-clause BSD
// license. The full text of the license can be found in the file
// COPYRIGHT at the top level directory of IBAMR.
//
// ---------------------------------------------------------------------

/////////////////////////////// INCLUDE GUARD ////////////////////////////////

#ifndef included_IBAMR_IBStrategy
#define included_IBAMR_IBStrategy

/////////////////////////////// INCLUDES /////////////////////////////////////

#include <ibamr/config.h>

#include "ibtk/CartGridFunction.h"

#include "CoarsenPatchStrategy.h"
#include "IntVector.h"
#include "RefinePatchStrategy.h"
#include "StandardTagAndInitStrategy.h"
#include "VariableContext.h"
#include "tbox/Pointer.h"
#include "tbox/Serializable.h"

#include <limits>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace IBTK
{
class RobinPhysBdryPatchStrategy;
} // namespace IBTK

namespace IBTK
{
class HierarchyMathOps;
} // namespace IBTK
namespace SAMRAI
{
namespace hier
{
template <int DIM>
class BasePatchLevel;
template <int DIM>
class PatchHierarchy;
template <int DIM>
class Variable;
template <int DIM>
class BasePatchHierarchy;
} // namespace hier
namespace math
{
template <int DIM, class TYPE>
class HierarchyDataOpsReal;
} // namespace math
namespace mesh
{
template <int DIM>
class GriddingAlgorithm;
template <int DIM>
class LoadBalancer;
} // namespace mesh
namespace tbox
{
class Database;
template <class TYPE>
class Array;
} // namespace tbox
namespace xfer
{
template <int DIM>
class CoarsenAlgorithm;
template <int DIM>
class CoarsenSchedule;
template <int DIM>
class RefineAlgorithm;
template <int DIM>
class RefineSchedule;
} // namespace xfer
} // namespace SAMRAI

namespace IBAMR
{
class IBHierarchyIntegrator;
class INSHierarchyIntegrator;
} // namespace IBAMR

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBAMR
{
/*!
 * \brief Class IBStrategy provides a generic interface for specifying the
 * implementation details of a particular version of the IB method.
 *
 * <h2>Options Controlling Interpolation and Spreading</h2>
 * All classes inheriting from IBStrategy read the following values from the
 * input database to determine which interpolation and spreading delta functions
 * should be used:
 * <ul>
 *   <li><code>interp_delta_fcn</code>: name of the interpolation kernel,
 *   provided as a string. Defaults to <code>"IB_4"</code>.</li>
 *   <li><code>spread_delta_fcn</code>: name of the spreading kernel,
 *   provided as a string. Defaults to <code>"IB_4"</code>.</li>
 *   <li><code>IB_delta_fcn</code>: overriding alias for the two previous
 *   entries - has the same default.</li>
 * </ul>
 */
class IBStrategy : public SAMRAI::mesh::StandardTagAndInitStrategy<NDIM>, public SAMRAI::tbox::Serializable
{
public:
    /*!
     * \brief Constructor.
     */
    IBStrategy() = default;

    /*!
     * Register the IBHierarchyIntegrator object that is using this strategy
     * class.
     */
    virtual void registerIBHierarchyIntegrator(IBHierarchyIntegrator* ib_solver);

    /*!
     * Register Eulerian variables with the parent IBHierarchyIntegrator with
     * the VariableDatabase, or via the various versions of the protected method
     * IBStrategy::registerVariable().
     *
     * An empty default implementation is provided.
     */
    virtual void registerEulerianVariables();

    /*!
     * Register Eulerian refinement or coarsening algorithms with the parent
     * IBHierarchyIntegrator using the two versions of the protected methods
     * IBStrategy::registerGhostfillRefineAlgorithm(),
     * IBStrategy::registerProlongRefineAlgorithm(), and
     * IBStrategy::registerCoarsenAlgorithm().
     *
     * An empty default implementation is provided.
     */
    virtual void registerEulerianCommunicationAlgorithms();

    /*!
     * Return the number of ghost cells required by the Lagrangian-Eulerian
     * interaction routines.
     */
    virtual const SAMRAI::hier::IntVector<NDIM>& getMinimumGhostCellWidth() const = 0;

    /*!
     * Setup the tag buffer.
     *
     * A default implementation is provided that sets the tag buffer to be at
     * least the minimum ghost cell width.
     */
    virtual void setupTagBuffer(SAMRAI::tbox::Array<int>& tag_buffer,
                                SAMRAI::tbox::Pointer<SAMRAI::mesh::GriddingAlgorithm<NDIM> > gridding_alg) const;

    /*!
     * Inactivate a structure or part. Such a structure will be ignored by FSI
     * calculations: i.e., it will have its velocity set to zero and no forces
     * will be spread from the structure to the Eulerian grid.
     *
     * @param[in] structure_number Number of the structure/part.
     * @param[in] level_number Level on which the structure lives. For some
     * inheriting classes (e.g., IBAMR::IBMethod) the structure number alone
     * is not enough to establish uniqueness. The default value is interpreted
     * as the finest level in the patch hierarchy.
     *
     * @note This method should be implemented by inheriting classes for which
     * the notion of activating and inactivating a structure 'owned' by this
     * class makes sense (for example, IBAMR::IBStrategySet does not directly
     * own any structures, but IBAMR::IBMethod and IBAMR::IBFEMethod both
     * do). The default implementation unconditionally aborts the program.
     */
    virtual void inactivateLagrangianStructure(int structure_number = 0,
                                               int level_number = std::numeric_limits<int>::max());

    /*!
     * Activate a previously inactivated structure or part to be used again in
     * FSI calculations.
     *
     * @param[in] structure_number Number of the structure/part.
     * @param[in] level_number Level on which the structure lives. For some
     * inheriting classes (e.g., IBAMR::IBMethod) the structure number alone
     * is not enough to establish uniqueness. The default value is interpreted
     * as the finest level in the patch hierarchy.
     *
     * @note This method should be implemented by inheriting classes for which
     * the notion of activating and inactivating a structure 'owned' by this
     * class makes sense (for example, IBAMR::IBStrategySet does not directly
     * own any structures, but IBAMR::IBMethod and IBAMR::IBFEMethod both
     * do). The default implementation unconditionally aborts the program.
     */
    virtual void activateLagrangianStructure(int structure_number = 0,
                                             int level_number = std::numeric_limits<int>::max());

    /*!
     * Determine whether or not the given structure or part is currently
     * activated.
     *
     * @param[in] structure_number Number of the structure/part.
     * @param[in] level_number Level on which the structure lives. For some
     * inheriting classes (e.g., IBAMR::IBMethod) the structure number alone
     * is not enough to establish uniqueness. The default value is interpreted
     * as the finest level in the patch hierarchy.
     *
     * @note This method should be implemented by inheriting classes for which
     * the notion of activating and inactivating a structure 'owned' by this
     * class makes sense (for example, IBAMR::IBStrategySet does not directly
     * own any structures, but IBAMR::IBMethod and IBAMR::IBFEMethod both
     * do). The default implementation unconditionally aborts the program.
     */
    virtual bool getLagrangianStructureIsActivated(int structure_number = 0,
                                                   int level_number = std::numeric_limits<int>::max()) const;

    /*!
     * Get the ratio of the maximum point displacement of all the structures
     * owned by the current class to the cell width of the grid level on which
     * the structure is assigned. This value is useful for determining if the
     * Eulerian patch hierarchy needs to be regridded.
     *
     * @note The process of regridding is distinct, for some IBStrategy objects
     * (like IBFEMethod), from forming (or reforming) the association between
     * Lagrangian structures and patches. In particular, this function computes
     * the distance between the current position of the structure and the
     * structure at the point of the last regrid, which may not be the same point
     * at which we last rebuilt the structure-to-patch mappings. The reassociation
     * check should be implemented in postprocessIntegrateData().
     */
    virtual double getMaxPointDisplacement() const;

    /*!
     * Method to prepare to advance data from current_time to new_time.
     *
     * An empty default implementation is provided.
     */
    virtual void preprocessIntegrateData(double current_time, double new_time, int num_cycles);

    /*!
     * Method to clean up data following call(s) to integrateHierarchy().
     *
     * An empty default implementation is provided.
     */
    virtual void postprocessIntegrateData(double current_time, double new_time, int num_cycles);

    /*!
     * Indicate whether "fixed" interpolation and spreading operators should be
     * used during Lagrangian-Eulerian interaction.
     */
    void setUseFixedLEOperators(bool use_fixed_coupling_ops = true);

    /*!
     * Update the positions used for the "fixed" interpolation and spreading
     * operators.
     *
     * A default implementation is provided that emits an unrecoverable
     * exception.
     */
    virtual void updateFixedLEOperators();

    /*!
     * Interpolate the Eulerian velocity to the curvilinear mesh at the
     * specified time within the current time interval.
     */
    virtual void interpolateVelocity(
        int u_data_idx,
        const std::vector<SAMRAI::tbox::Pointer<SAMRAI::xfer::CoarsenSchedule<NDIM> > >& u_synch_scheds,
        const std::vector<SAMRAI::tbox::Pointer<SAMRAI::xfer::RefineSchedule<NDIM> > >& u_ghost_fill_scheds,
        double data_time) = 0;

    /*!
     * Indicate that multistep time stepping will be used.
     *
     * A default implementation is provided that emits an unrecoverable
     * exception.
     *
     * @param[in] n_previous_steps Number of previous solution values that can be used by the multistep scheme.
     */
    virtual void setUseMultistepTimeStepping(unsigned int n_previous_steps = 1);

    /*!
     * Advance the positions of the Lagrangian structure using the forward Euler
     * method.
     */
    virtual void forwardEulerStep(double current_time, double new_time) = 0;

    /*!
     * Advance the positions of the Lagrangian structure using the (explicit)
     * backward Euler method.
     *
     * A default implementation is provided that emits an unrecoverable
     * exception.
     */
    virtual void backwardEulerStep(double current_time, double new_time);

    /*!
     * Advance the positions of the Lagrangian structure using the (explicit)
     * midpoint rule.
     */
    virtual void midpointStep(double current_time, double new_time) = 0;

    /*!
     * Advance the positions of the Lagrangian structure using the (explicit)
     * trapezoidal rule.
     */
    virtual void trapezoidalStep(double current_time, double new_time) = 0;

    /*!
     * Advance the positions of the Lagrangian structure using the standard
     * 2nd-order Adams-Bashforth rule.
     *
     * A default implementation is provided that emits an unrecoverable
     * exception.
     */
    virtual void AB2Step(double current_time, double new_time);

    /*!
     * Compute the Lagrangian force at the specified time within the current
     * time interval.
     */
    virtual void computeLagrangianForce(double data_time) = 0;

    /*!
     * Spread the Lagrangian force to the Cartesian grid at the specified time
     * within the current time interval.
     */
    virtual void
    spreadForce(int f_data_idx,
                IBTK::RobinPhysBdryPatchStrategy* f_phys_bdry_op,
                const std::vector<SAMRAI::tbox::Pointer<SAMRAI::xfer::RefineSchedule<NDIM> > >& f_prolongation_scheds,
                double data_time) = 0;

    /*!
     * Indicate whether there are any internal fluid sources/sinks.
     *
     * A default implementation is provided that returns false.
     */
    virtual bool hasFluidSources() const;

    /*!
     * Compute the Lagrangian source/sink density at the specified time within
     * the current time interval.
     *
     * An empty default implementation is provided.
     */
    virtual void computeLagrangianFluidSource(double data_time);

    /*!
     * Spread the Lagrangian source/sink density to the Cartesian grid at the
     * specified time within the current time interval.
     *
     * An empty default implementation is provided.
     */
    virtual void spreadFluidSource(
        int q_data_idx,
        IBTK::RobinPhysBdryPatchStrategy* q_phys_bdry_op,
        const std::vector<SAMRAI::tbox::Pointer<SAMRAI::xfer::RefineSchedule<NDIM> > >& q_prolongation_scheds,
        double data_time);

    /*!
     * Compute the pressures at the positions of any distributed internal fluid
     * sources or sinks.
     *
     * An empty default implementation is provided.
     */
    virtual void interpolatePressure(
        int p_data_idx,
        const std::vector<SAMRAI::tbox::Pointer<SAMRAI::xfer::CoarsenSchedule<NDIM> > >& p_synch_scheds,
        const std::vector<SAMRAI::tbox::Pointer<SAMRAI::xfer::RefineSchedule<NDIM> > >& p_ghost_fill_scheds,
        double data_time);

    /*!
     * Execute user-defined routines just before solving the fluid equations.
     *
     * An empty default implementation is provided.
     */
    virtual void preprocessSolveFluidEquations(double current_time, double new_time, int cycle_num);

    /*!
     * Execute user-defined routines just after solving the fluid equations.
     *
     * An empty default implementation is provided.
     */
    virtual void postprocessSolveFluidEquations(double current_time, double new_time, int cycle_num);

    /*!
     * Execute user-defined post-processing operations.
     *
     * An empty default implementation is provided.
     */
    virtual void postprocessData();

    /*!
     * Initialize Lagrangian data corresponding to the given AMR patch hierarchy
     * at the start of a computation.  If the computation is begun from a
     * restart file, data may be read from the restart databases.
     *
     * A patch data descriptor is provided for the Eulerian velocity in case
     * initialization requires interpolating Eulerian data.  Ghost cells for
     * Eulerian data will be filled upon entry to this function.
     *
     * An empty default implementation is provided.
     */
    virtual void initializePatchHierarchy(
        SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
        SAMRAI::tbox::Pointer<SAMRAI::mesh::GriddingAlgorithm<NDIM> > gridding_alg,
        int u_data_idx,
        const std::vector<SAMRAI::tbox::Pointer<SAMRAI::xfer::CoarsenSchedule<NDIM> > >& u_synch_scheds,
        const std::vector<SAMRAI::tbox::Pointer<SAMRAI::xfer::RefineSchedule<NDIM> > >& u_ghost_fill_scheds,
        int integrator_step,
        double init_data_time,
        bool initial_time);

    /*!
     * Register a load balancer and work load patch data index with the IB
     * strategy object.
     *
     * An empty default implementation is provided.
     *
     * @deprecated This method is no longer necessary with the current
     * workload estimation scheme.
     */
    virtual void registerLoadBalancer(SAMRAI::tbox::Pointer<SAMRAI::mesh::LoadBalancer<NDIM> > load_balancer,
                                      int workload_data_idx);

    /*!
     * Add the estimated computational work from the current object per cell
     * into the specified <code>workload_data_idx</code>.
     *
     * An empty default implementation is provided.
     */
    virtual void addWorkloadEstimate(SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                     const int workload_data_idx);

    /*!
     * Begin redistributing Lagrangian data prior to regridding the patch
     * hierarchy.
     *
     * An empty default implementation is provided.
     */
    virtual void beginDataRedistribution(SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                         SAMRAI::tbox::Pointer<SAMRAI::mesh::GriddingAlgorithm<NDIM> > gridding_alg);

    /*!
     * Complete redistributing Lagrangian data following regridding the patch
     * hierarchy.
     *
     * An empty default implementation is provided.
     */
    virtual void endDataRedistribution(SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                       SAMRAI::tbox::Pointer<SAMRAI::mesh::GriddingAlgorithm<NDIM> > gridding_alg);

    /*!
     * Initialize data on a new level after it is inserted into an AMR patch
     * hierarchy by the gridding algorithm.
     *
     * \see SAMRAI::mesh::StandardTagAndInitStrategy::initializeLevelData
     *
     * An empty default implementation is provided.
     */
    void initializeLevelData(SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchHierarchy<NDIM> > hierarchy,
                             int level_number,
                             double init_data_time,
                             bool can_be_refined,
                             bool initial_time,
                             SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchLevel<NDIM> > old_level,
                             bool allocate_data) override;

    /*!
     * Reset cached hierarchy dependent data.
     *
     * \see SAMRAI::mesh::StandardTagAndInitStrategy::resetHierarchyConfiguration
     *
     * An empty default implementation is provided.
     */
    void resetHierarchyConfiguration(SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchHierarchy<NDIM> > hierarchy,
                                     int coarsest_level,
                                     int finest_level) override;

    /*!
     * Set integer tags to "one" in cells where refinement of the given level
     * should occur according to user-supplied feature detection criteria.
     *
     * \see SAMRAI::mesh::StandardTagAndInitStrategy::applyGradientDetector
     *
     * An empty default implementation is provided.
     */
    void applyGradientDetector(SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchHierarchy<NDIM> > hierarchy,
                               int level_number,
                               double error_data_time,
                               int tag_index,
                               bool initial_time,
                               bool uses_richardson_extrapolation_too) override;

    /*!
     * Write out object state to the given database.
     *
     * An empty default implementation is provided.
     */
    void putToDatabase(SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> db) override;

protected:
    /*!
     * Return a pointer to the INSHierarchyIntegrator object being used with the
     * IBHierarchyIntegrator class registered with this IBStrategy object.
     */
    INSHierarchyIntegrator* getINSHierarchyIntegrator() const;

    /*!
     * Return a pointer to the HierarchyDataOpsReal object associated with
     * velocity-like variables.
     */
    SAMRAI::tbox::Pointer<SAMRAI::math::HierarchyDataOpsReal<NDIM, double> > getVelocityHierarchyDataOps() const;

    /*!
     * Return a pointer to the HierarchyDataOpsReal object associated with
     * pressure-like variables.
     */
    SAMRAI::tbox::Pointer<SAMRAI::math::HierarchyDataOpsReal<NDIM, double> > getPressureHierarchyDataOps() const;

    /*!
     * Return a pointer to a HierarchyMathOps object.
     */
    SAMRAI::tbox::Pointer<IBTK::HierarchyMathOps> getHierarchyMathOps() const;

    /*!
     * Register a state variable with the integrator.  When a refine operator is
     * specified, the data for the variable are automatically maintained as the
     * patch hierarchy evolves.
     *
     * All state variables are registered with three contexts: current, new, and
     * scratch.  The current context of a state variable is maintained from time
     * step to time step and, if the necessary coarsen and refine operators are
     * specified, as the patch hierarchy evolves.
     */
    void registerVariable(int& current_idx,
                          int& new_idx,
                          int& scratch_idx,
                          SAMRAI::tbox::Pointer<SAMRAI::hier::Variable<NDIM> > variable,
                          const SAMRAI::hier::IntVector<NDIM>& scratch_ghosts = SAMRAI::hier::IntVector<NDIM>(0),
                          const std::string& coarsen_name = "NO_COARSEN",
                          const std::string& refine_name = "NO_REFINE",
                          SAMRAI::tbox::Pointer<IBTK::CartGridFunction> init_fcn = nullptr,
                          const bool register_for_restart = true);

    /*!
     * Register a variable with the integrator that may not be maintained from
     * time step to time step.
     *
     * By default, variables are registered with the scratch context, which is
     * deallocated after each time step.
     */
    void registerVariable(int& idx,
                          SAMRAI::tbox::Pointer<SAMRAI::hier::Variable<NDIM> > variable,
                          const SAMRAI::hier::IntVector<NDIM>& ghosts = SAMRAI::hier::IntVector<NDIM>(0),
                          SAMRAI::tbox::Pointer<SAMRAI::hier::VariableContext> ctx =
                              SAMRAI::tbox::Pointer<SAMRAI::hier::VariableContext>(nullptr),
                          const bool register_for_restart = true);

    /*!
     * Register a ghost cell-filling refine algorithm.
     */
    void registerGhostfillRefineAlgorithm(
        const std::string& name,
        SAMRAI::tbox::Pointer<SAMRAI::xfer::RefineAlgorithm<NDIM> > ghostfill_alg,
        std::unique_ptr<SAMRAI::xfer::RefinePatchStrategy<NDIM> > ghostfill_patch_strategy = nullptr);

    /*!
     * Register a data-prolonging refine algorithm.
     */
    void registerProlongRefineAlgorithm(
        const std::string& name,
        SAMRAI::tbox::Pointer<SAMRAI::xfer::RefineAlgorithm<NDIM> > prolong_alg,
        std::unique_ptr<SAMRAI::xfer::RefinePatchStrategy<NDIM> > prolong_patch_strategy = nullptr);

    /*!
     * Register a coarsen algorithm.
     */
    void registerCoarsenAlgorithm(
        const std::string& name,
        SAMRAI::tbox::Pointer<SAMRAI::xfer::CoarsenAlgorithm<NDIM> > coarsen_alg,
        std::unique_ptr<SAMRAI::xfer::CoarsenPatchStrategy<NDIM> > coarsen_patch_strategy = nullptr);

    /*!
     * Get ghost cell-filling refine algorithm.
     */
    SAMRAI::tbox::Pointer<SAMRAI::xfer::RefineAlgorithm<NDIM> >
    getGhostfillRefineAlgorithm(const std::string& name) const;

    /*!
     * Get data-prolonging refine algorithm.
     */
    SAMRAI::tbox::Pointer<SAMRAI::xfer::RefineAlgorithm<NDIM> >
    getProlongRefineAlgorithm(const std::string& name) const;

    /*!
     * Get coarsen algorithm.
     */
    SAMRAI::tbox::Pointer<SAMRAI::xfer::CoarsenAlgorithm<NDIM> > getCoarsenAlgorithm(const std::string& name) const;

    /*!
     * Get ghost cell-filling refine schedules.
     */
    const std::vector<SAMRAI::tbox::Pointer<SAMRAI::xfer::RefineSchedule<NDIM> > >&
    getGhostfillRefineSchedules(const std::string& name) const;

    /*!
     * Get data-prolonging refine schedules.
     *
     * \note These schedules are allocated only for level numbers >= 1.
     */
    const std::vector<SAMRAI::tbox::Pointer<SAMRAI::xfer::RefineSchedule<NDIM> > >&
    getProlongRefineSchedules(const std::string& name) const;

    /*!
     * Get coarsen schedules.
     *
     * \note These schedules are allocated only for level numbers >= 1.
     */
    const std::vector<SAMRAI::tbox::Pointer<SAMRAI::xfer::CoarsenSchedule<NDIM> > >&
    getCoarsenSchedules(const std::string& name) const;

    /*!
     * The IBHierarchyIntegrator object that is using this strategy class.
     */
    IBHierarchyIntegrator* d_ib_solver = nullptr;

    /*!
     * Whether to use "fixed" Lagrangian-Eulerian coupling operators.
     */
    bool d_use_fixed_coupling_ops = false;

private:
    /*!
     * \brief Copy constructor.
     *
     * \note This constructor is not implemented and should not be used.
     *
     * \param from The value to copy to this object.
     */
    IBStrategy(const IBStrategy& from) = delete;

    /*!
     * \brief Assignment operator.
     *
     * \note This operator is not implemented and should not be used.
     *
     * \param that The value to assign to this object.
     *
     * \return A reference to this object.
     */
    IBStrategy& operator=(const IBStrategy& that) = delete;
};
} // namespace IBAMR

//////////////////////////////////////////////////////////////////////////////

#endif // #ifndef included_IBAMR_IBStrategy
