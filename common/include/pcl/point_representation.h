/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2010, Willow Garage, Inc.
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
 *   * Neither the name of Willow Garage, Inc. nor the names of its
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
 *
 * $Id$
 *
 */
#ifndef PCL_POINT_REPRESENTATION_H_
#define PCL_POINT_REPRESENTATION_H_

#include "pcl/point_types.h"
#include "pcl/win32_macros.h"
//#include "pcl/conversions.h"
#include "pcl/ros/for_each_type.h"

namespace pcl
{
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief @b PointRepresentation provides a set of methods for converting a point structs/object into an
   *  n-dimensional vector.
   *  @note This is an abstract class.  Subclasses must set nr_dimensions_ to the appropriate value in the constructor 
   *  and provide an implemention of the pure virtual copyToFloatArray method.
   */
  template <typename PointT> 
  class PointRepresentation
  {
    protected:
      /** \brief The number of dimensions in this point's vector (i.e. the "k" in "k-D") */
      int nr_dimensions_;
      /** \brief A vector containing the rescale factor to apply to each dimension. */
      std::vector<float> alpha_;
      
    public:
      typedef boost::shared_ptr<PointRepresentation<PointT> > Ptr;
      typedef boost::shared_ptr<const PointRepresentation<PointT> > ConstPtr;

      /** \brief Empty constructor */
      PointRepresentation () : nr_dimensions_ (0), alpha_ (0) {}
      
      /** \brief Copy point data from input point to a float array. This method must be overriden in all subclasses. 
       *  \param p The input point
       *  \param out A pointer to a float array.
       */
      virtual void copyToFloatArray (const PointT &p, float * out) const = 0;
      
      /** \brief Verify that the input point is valid.
       *  \param p The point to validate
       */
      virtual bool 
      isValid (const PointT &p) const
      {
        float *temp = new float[nr_dimensions_];
        copyToFloatArray (p, temp);
        bool is_valid = true;
        for (int i = 0; i < nr_dimensions_; ++i)
        {
          if (!pcl_isfinite (temp[i]))
          {
            is_valid = false;
            break;
          }
        }
        delete [] temp;
        return (is_valid);
      }
      
      /** \brief Convert input point into a vector representation, rescaling by \a alpha.
       *  \param p
       *  \param out The output vector.  Can be of any type that implements the [] operator.
       */
      template <typename OutputType> void
        vectorize (const PointT &p, OutputType &out) const
      {
        float *temp = new float[nr_dimensions_];
        copyToFloatArray (p, temp);
        if (alpha_.empty ())
        {
          for (int i = 0; i < nr_dimensions_; ++i)
            out[i] = temp[i];
        }
        else
        {
          for (int i = 0; i < nr_dimensions_; ++i)
            out[i] = temp[i] * alpha_[i];
        }
        delete [] temp;
      }
      
      /** \brief Set the rescale values to use when vectorizing points
       *  \param rescale_array The array/vector of rescale values.  Can be of any type that implements the [] operator.
       */
      //template <typename InputType>
      //void setRescaleValues (const InputType &rescale_array)
      void 
        setRescaleValues (const float * rescale_array)
      {
        alpha_.resize (nr_dimensions_);
        for (int i = 0; i < nr_dimensions_; ++i)
          alpha_[i] = rescale_array[i];
      }
      
      /** \brief Return the number of dimensions in the point's vector representation. */
      inline int getNumberOfDimensions () const { return (nr_dimensions_); }
  };

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief @b DefaultPointRepresentation extends PointRepresentation to define default behavior for common point types.
   */
  template <typename PointDefault>
  class DefaultPointRepresentation : public PointRepresentation <PointDefault>
  {
    using PointRepresentation <PointDefault>::nr_dimensions_;

    public:
      // Boost shared pointers
      typedef boost::shared_ptr<DefaultPointRepresentation<PointDefault> > Ptr;
      typedef boost::shared_ptr<const DefaultPointRepresentation<PointDefault> > ConstPtr;

      DefaultPointRepresentation ()
      {
        // If point type is unknown, assume it's a struct/array of floats, and compute the number of dimensions
        nr_dimensions_ = sizeof (PointDefault) / sizeof (float);
        // Limit the default representation to the first 3 elements
        if (nr_dimensions_ > 3) nr_dimensions_ = 3;
      }

      inline Ptr makeShared () const { return Ptr (new DefaultPointRepresentation<PointDefault> (*this)); } 

      virtual void 
        copyToFloatArray (const PointDefault &p, float * out) const
      {
        // If point type is unknown, treat it as a struct/array of floats
        const float * ptr = (float *)&p;
        for (int i = 0; i < nr_dimensions_; ++i)
        {
          out[i] = ptr[i];
        }
      }
  };

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief @b DefaulFeatureRepresentation extends PointRepresentation and is intended to be used when defining the
    * default behavior for feature descriptor types (i.e., copy each element of each field into a float array).
    */
  template <typename PointDefault>
  class DefaultFeatureRepresentation : public PointRepresentation <PointDefault>
  {
    using PointRepresentation <PointDefault>::nr_dimensions_;

    private:
      struct IncrementFunctor
      {
        IncrementFunctor (int &n) : n_ (n)
        {
          n_ = 0;
        }
        
        template<typename Key> inline void operator () ()
        {
          n_ += pcl::traits::datatype<PointDefault, Key>::size;
        }
        
      private:
        int &n_;
      };

    struct NdCopyPointFunctor
    {
      typedef typename traits::POD<PointDefault>::type Pod;
      
      NdCopyPointFunctor (const PointDefault &p1, float * p2)
        : p1_ (reinterpret_cast<const Pod&>(p1)), p2_ (p2), f_idx_ (0) { }
      
      template<typename Key> inline void operator() ()
      {  
        typedef typename pcl::traits::datatype<PointDefault, Key>::type FieldT;
        const int NrDims = pcl::traits::datatype<PointDefault, Key>::size;
        Helper<Key, FieldT, NrDims>::copyPoint (p1_, p2_, f_idx_);
      }
      
      // Copy helper for scalar fields
      template <typename Key, typename FieldT, int NrDims>
      struct Helper
      {
        static void copyPoint (const Pod &p1, float * p2, int &f_idx) 
        {
          const uint8_t * data_ptr = reinterpret_cast<const uint8_t *> (&p1) + 
            pcl::traits::offset<PointDefault, Key>::value;
          p2[f_idx++] = *reinterpret_cast<const FieldT*> (data_ptr);
        }
      };
      // Copy helper for array fields
      template <typename Key, typename FieldT, int NrDims>
      struct Helper<Key, FieldT[NrDims], NrDims>
      {
        static void copyPoint (const Pod &p1, float * p2, int &f_idx)
        {
          const uint8_t * data_ptr = reinterpret_cast<const uint8_t *> (&p1) + 
            pcl::traits::offset<PointDefault, Key>::value;
          int nr_dims = NrDims;
          const FieldT * array = reinterpret_cast<const FieldT *> (data_ptr);
          for (int i = 0; i < nr_dims; ++i)
          {
            p2[f_idx++] = array[i];
          }
        }
      };
  
    private:
      const Pod &p1_;
      float * p2_;
      int f_idx_;
    };

    public:
      // Boost shared pointers
    typedef int Foo;
      typedef typename boost::shared_ptr<DefaultFeatureRepresentation<PointDefault> > Ptr;
      typedef typename boost::shared_ptr<const DefaultFeatureRepresentation<PointDefault> > ConstPtr;
      typedef typename pcl::traits::fieldList<PointDefault>::type FieldList;

      DefaultFeatureRepresentation ()
      {      
        nr_dimensions_ = 0; // zero-out the nr_dimensions_ before it gets incremented
        pcl::for_each_type <FieldList> (IncrementFunctor (nr_dimensions_));
      }

      inline Ptr makeShared () const { return Ptr (new DefaultFeatureRepresentation<PointDefault> (*this)); } 

      virtual void 
        copyToFloatArray (const PointDefault &p, float * out) const
      {
        pcl::for_each_type <FieldList> (NdCopyPointFunctor (p, out));
      }
  };

  template <>
  class DefaultPointRepresentation <PointXYZ> : public  PointRepresentation <PointXYZ>
  {
    public:
      DefaultPointRepresentation ()
      {
        nr_dimensions_ = 3;
      }

      virtual void 
        copyToFloatArray (const PointXYZ &p, float * out) const
      {
        out[0] = p.x;
        out[1] = p.y;
        out[2] = p.z;
      }
  };

  template <>
  class DefaultPointRepresentation <PointXYZI> : public  PointRepresentation <PointXYZI>
  {
    public:
      DefaultPointRepresentation ()
      {
        nr_dimensions_ = 3;
      }

      virtual void 
        copyToFloatArray (const PointXYZI &p, float * out) const
      {
        out[0] = p.x;
        out[1] = p.y;
        out[2] = p.z;
        // By default, p.intensity is not part of the PointXYZI vectorization
      }
  };

  template <>
  class DefaultPointRepresentation <PointNormal> : public  PointRepresentation <PointNormal>
  {
    public:
      DefaultPointRepresentation ()
      {
        nr_dimensions_ = 3;
      }

      virtual void 
        copyToFloatArray (const PointNormal &p, float * out) const
      {
        out[0] = p.x;
        out[1] = p.y;
        out[2] = p.z;
      }
  };

  template <>
  class DefaultPointRepresentation <PFHSignature125> : public DefaultFeatureRepresentation <PFHSignature125>
  {};

  template <>
  class DefaultPointRepresentation <PPFSignature> : public DefaultFeatureRepresentation <PPFSignature>
  {};

  template <>
  class DefaultPointRepresentation <FPFHSignature33> : public DefaultFeatureRepresentation <FPFHSignature33>
  {};

  template <>
  class DefaultPointRepresentation <VFHSignature308> : public DefaultFeatureRepresentation <VFHSignature308>
  {};

  template <>
  class DefaultPointRepresentation <NormalBasedSignature12> : 
    public DefaultFeatureRepresentation <NormalBasedSignature12>
  {};


  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief @b CustomPointRepresentation extends PointRepresentation to allow for sub-part selection on the point.
   */
  template <typename PointDefault>
  class CustomPointRepresentation : public PointRepresentation <PointDefault>
  {
    using PointRepresentation <PointDefault>::nr_dimensions_;

    /** \brief Use at most this many dimensions (i.e. the "k" in "k-D" is at most max_dim_) -- \note float fields are assumed */
    int max_dim_;
    /** \brief Use dimensions only starting with this one (i.e. the "k" in "k-D" is = dim - start_dim_) -- \note float fields are assumed */
    int start_dim_;

    public:
      // Boost shared pointers
      typedef boost::shared_ptr<CustomPointRepresentation<PointDefault> > Ptr;
      typedef boost::shared_ptr<const CustomPointRepresentation<PointDefault> > ConstPtr;

      CustomPointRepresentation (int max_dim = 3, int start_dim = 0) : max_dim_(max_dim), start_dim_(start_dim)
      {
        // If point type is unknown, assume it's a struct/array of floats, and compute the number of dimensions
        nr_dimensions_ = sizeof (PointDefault) / sizeof (float) - start_dim_;
        // Limit the default representation to the first 3 elements
        if (nr_dimensions_ > max_dim_) nr_dimensions_ = max_dim_;
      }

      inline Ptr makeShared () const { return Ptr (new CustomPointRepresentation<PointDefault> (*this)); }

      virtual void
        copyToFloatArray (const PointDefault &p, float * out) const
      {
        // If point type is unknown, treat it as a struct/array of floats
        const float * ptr = ((float *)&p) + start_dim_;
        for (int i = 0; i < nr_dimensions_; ++i)
        {
          out[i] = ptr[i];
        }
      }
  };
}

#endif // #ifndef PCL_POINT_REPRESENTATION_H_
