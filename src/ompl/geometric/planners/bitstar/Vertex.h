/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2014, University of Toronto
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the University of Toronto nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

/* Authors: Jonathan Gammell */

#ifndef OMPL_GEOMETRIC_PLANNERS_BITSTAR_VERTEX_
#define OMPL_GEOMETRIC_PLANNERS_BITSTAR_VERTEX_

//vector
#include <vector>
//std::set and std::multiset
//#include <set>


//Boost
//shared and weak pointers
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
//For unordered sets of failed children:
#include <boost/unordered_set.hpp>
//For locking the ID generator:
#include <boost/thread/mutex.hpp>

//Forward declarations:
#include "ompl/util/ClassForward.h"
//The space information
#include "ompl/base/SpaceInformation.h"
//The optimization objective
#include "ompl/base/OptimizationObjective.h"



namespace ompl
{
    namespace geometric
    {
        OMPL_CLASS_FORWARD(Vertex);
        typedef boost::shared_ptr<const Vertex> VertexConstPtr;

        /** \brief A class to store a state as a vertex in a (tree) graph.
        Allocates and frees it's own memory on construction/destruction.
        Parent vertices are owned by their children as shared pointers,
        assuring that a parent vertex will not be deleted while the child exists.
        Child vertices are owned by their parents as weak pointers, assuring
        that the shared-pointer ownership loop is broken.

        Add/Remove functions should almost always update their children's cost.
        The only known exception is when a series of operations are being performed
        and it would be beneficial to delay the update until the last operation. In this case,
        make sure that the last call updates the children and is on the highest ancestor that has been
        changed. Updates only flow downstream.
        */
        class Vertex
        {

        public:
            /** \brief A boost::weak_ptr to a Vertex */
            typedef boost::weak_ptr<Vertex> vertex_weak_ptr_t;

            /** \brief A typedef to the id type */
            typedef unsigned int id_t;


            /** \brief Constructor */
            Vertex(const ompl::base::SpaceInformationPtr& si, const ompl::base::OptimizationObjectivePtr& opt, bool root = false);

            /** \brief Destructor */
            ~Vertex();

            /** \brief The (unique) vertex ID */
            id_t getId() const;

            /** \brief The optimization objective used by the vertex. */
            ompl::base::OptimizationObjectivePtr getOpt() const;

            /** \brief The state of a vertex as a constant pointer */
            ompl::base::State const* stateConst() const;

            /** \brief The state of a vertex as a mutable pointer*/
            ompl::base::State* state();

            /** \brief Whether the vertex is root */
            bool isRoot() const;

            /** \brief Get whether this vertex has a parent */
            bool hasParent() const;

            /** \brief Get whether a vertex is "in the graph" or not. This returns true if the vertex is the graph root or is connected to a parent. */
            bool isInTree() const;

            /** \brief Get the "depth" of the vertex from the root. A root vertex is at depth 0, a direct descendent of the root 1, etc. */
            unsigned int getDepth() const;

            /** \brief Get the parent of a vertex as a constant pointer */
            VertexConstPtr getParentConst() const;

            /** \brief Get the parent of a vertex as a mutable pointer*/
            VertexPtr getParent();

            /** \brief Set the parent of a vertex, cannot be used to replace a previous parent. Will update this vertex's cost, and can update descendent costs */
            void addParent(const VertexPtr& newParent, const ompl::base::Cost& edgeInCost, bool updateChildCosts = true);

            /** \brief Remove the parent edge. Will update this vertex's cost, and can update the descendent costs */
            void removeParent(bool updateChildCosts = true);

            /** \brief Get whether this vertex has any children */
            bool hasChildren() const;

            /** \brief Get the children of a vertex as constant pointers */
            void getChildrenConst(std::vector<VertexConstPtr>* children) const;

            /** \brief Get the children of a vertex as mutable pointers */
            void getChildren(std::vector<VertexPtr>* children);

            /** \brief Add a child vertex. Does not change this vertex's cost, and can update the child and its descendent costs */
            void addChild(const VertexPtr& newChild, bool updateChildCosts = true);

            /** \brief Remove a child vertex. Does not change this vertex's cost, and can update the child and its descendent costs. Will throw an exception if the given vertex pointer is not in the list of children. The VertexPtr to be removed is \e not passed by const ref to assure that the function cannot delete it out from under itself. */
            void removeChild(VertexPtr oldChild, bool updateChildCosts = true);

            /** \brief Get the cost-to-come of a vertex. Return infinity if the edge is disconnected */
            ompl::base::Cost getCost() const;

            /** \brief Get the incremental cost-to-come of a vertex */
            ompl::base::Cost getEdgeInCost() const;

            /** \brief Returns true if the vertex is marked as new. Vertices are new until marked old. */
            bool isNew() const;

            /** \brief Mark the vertex as new. */
            void markNew();

            /** \brief Mark the vertex as old. */
            void markOld();

            /** \brief Whether the vertex has been pruned */
            bool isPruned() const;

            /** \brief Mark the vertex as pruned. */
            void markPruned();

            /** \brief Mark the given vertex as a \e failed connection from this vertex */
            void markAsFailedChild(const VertexPtr& failedChild);

            /** \brief Check if the given vertex has previously been marked as a failed child of this vertex */
            bool hasAlreadyFailed(const VertexPtr& potentialChild) const;

        protected:
            /** \brief Calculates the updated cost and depth of the current state, as well as calling all children's updateCostAndDepth() functions and thus updating everything down-stream (if desired).*/
            void updateCostAndDepth(bool cascadeUpdates = true);

        private:
            /** \brief The vertex ID */
            id_t                                                     vId_;

            /** \brief The state space used by the planner */
            ompl::base::SpaceInformationPtr                          si_;

            /** \brief The optimization objective used by the planner */
            ompl::base::OptimizationObjectivePtr                     opt_;

            /** \brief The state itself */
            ompl::base::State*                                       state_;

            /** \brief Whether the vertex is a root */
            bool                                                     isRoot_;

            /** \brief Whether the vertex is a new. Vertices are new until marked old. */
            bool                                                     isNew_;

            /** \brief Whether the vertex is pruned. Vertices throw if any member function other than isPruned() is access after they are pruned. */
            bool                                                     isPruned_;

            /** \brief The depth of the state  */
            unsigned int                                             depth_;

            /** \brief The parent state as a shared pointer such that the parent will not be deleted until all the children are. */
            VertexPtr                                                parentSPtr_;

            /** \brief The incremental cost to get to the state. I.e., the cost of the parent -> state edge */
            ompl::base::Cost                                         edgeCost_;

            /** \brief The cost of the state  */
            ompl::base::Cost                                         cost_;

            /** \brief The child states as weak pointers, such that the ownership loop is broken and a state can be deleted once it's children are.*/
            std::vector<vertex_weak_ptr_t>                           childWPtrs_;

            /** \brief The unordered set of failed child vertices*/
            //std::set<id_t>                                           failedVIds_;
            boost::unordered_set<id_t>                               failedVIds_;


            /** \brief A helper function to check that the vertex is not pruned and throw if so */
            void assertNotPruned() const;
        }; //class: Vertex

        //A generator class for vertex ids. I'm not sure that the mutex is right (or that it has the right effect) as I haven't had a need to test it.
        //This is not actually used yet.
        class idGenerator
        {
        public:
            inline idGenerator()
            {
                nextVId_ = 0u;
            }

            inline Vertex::id_t getNextId()
            {
                //Create a scoped mutex copy of mutex_ that unlocks when it goes out of scope:
                boost::mutex::scoped_lock scoped_lock(mutex_);

                //Return the current ID and increment:
                return nextVId_++;
            }

        private:
            Vertex::id_t                                             nextVId_;
            boost::mutex                                             mutex_;
        }; //class: idGenerator
    } //geometric
} //ompl
#endif //OMPL_GEOMETRIC_PLANNERS_BITSTAR_VERTEX_

