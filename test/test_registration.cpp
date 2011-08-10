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

/* \author Radu Bogdan Rusu */

#include <gtest/gtest.h>

#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>
#include "pcl/features/normal_3d.h"
#include "pcl/features/fpfh.h"
#include "pcl/registration/registration.h"
#include "pcl/registration/icp.h"
#include "pcl/registration/icp_nl.h"
#include "pcl/registration/ia_ransac.h"
#include "pcl/registration/pyramid_feature_matching.h"
#include "pcl/features/ppf.h"
#include "pcl/registration/ppf_registration.h"

// We need Histogram<2> to function, so we'll explicitely add kdtree_flann.hpp here
#include <pcl/kdtree/impl/kdtree_flann.hpp>
//(pcl::Histogram<2>)

using namespace pcl;
using namespace pcl::io;
using namespace std;

PointCloud<PointXYZ> cloud_source, cloud_target, cloud_reg;  

template <typename PointSource, typename PointTarget>
class RegistrationWrapper : public Registration<PointSource, PointTarget>
{
public:
  void computeTransformation (pcl::PointCloud<PointSource> &foo) { }

  bool hasValidFeaturesTest ()
  {
    return (this->hasValidFeatures ());
  }
  void findFeatureCorrespondencesTest (int index, std::vector<int> &correspondence_indices)
  {
    this->findFeatureCorrespondences (index, correspondence_indices);
  }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, findFeatureCorrespondences)
{
  typedef Histogram<2> FeatureT;
  typedef PointCloud<FeatureT> FeatureCloud;
  typedef FeatureCloud::ConstPtr FeatureCloudConstPtr;

  RegistrationWrapper <PointXYZ, PointXYZ> reg;

  FeatureCloud feature0, feature1, feature2, feature3;
  feature0.height = feature1.height = feature2.height = feature3.height = 1;
  feature0.is_dense = feature1.is_dense = feature2.is_dense = feature3.is_dense = true;

  for (float x = -5.0; x <= 5.0; x += 0.2)
  {
    for (float y = -5.0; y <= 5.0; y += 0.2)
    {
      FeatureT f;
      f.histogram[0] = x;
      f.histogram[1] = y;
      feature0.points.push_back (f);

      f.histogram[0] = x;
      f.histogram[1] = y - 2.5;
      feature1.points.push_back (f);

      f.histogram[0] = x - 2.0;
      f.histogram[1] = y + 1.5;
      feature2.points.push_back (f);
      
      f.histogram[0] = x + 2.0;
      f.histogram[1] = y + 1.5;
      feature3.points.push_back (f);
    }
  }
  feature0.width = feature0.points.size ();
  feature1.width = feature1.points.size ();
  feature2.width = feature2.points.size ();
  feature3.width = feature3.points.size ();

  KdTreeFLANN<FeatureT> tree;

/*  int k = 600;

  reg.setSourceFeature<FeatureT> (feature0.makeShared (), "feature1");
  reg.setTargetFeature<FeatureT> (feature1.makeShared (), "feature1");
  reg.setKSearch<FeatureT> (tree.makeShared (), k, "feature1");

  reg.setSourceFeature<FeatureT> (feature0.makeShared (), "feature2");
  reg.setTargetFeature<FeatureT> (feature2.makeShared (), "feature2");
  reg.setKSearch<FeatureT> (tree.makeShared (), k, "feature2");

  reg.setSourceFeature<FeatureT> (feature0.makeShared (), "feature3");
  reg.setTargetFeature<FeatureT> (feature3.makeShared (), "feature3");
  reg.setKSearch<FeatureT> (tree.makeShared (), k, "feature3");

  ASSERT_TRUE (reg.hasValidFeaturesTest ());

  std::vector<int> indices;
  reg.findFeatureCorrespondencesTest (1300, indices);

  ASSERT_EQ ((int)indices.size (), 10);
  const int correct_values[] = {1197, 1248, 1249, 1299, 1300, 1301, 1302, 1350, 1351, 1401};
  for (size_t i = 0; i < indices.size (); ++i)
  {
    EXPECT_EQ (indices[i], correct_values[i]);
  }
*/
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, IterativeClosestPoint)
{
  IterativeClosestPoint<PointXYZ, PointXYZ> reg;
  reg.setInputCloud (cloud_source.makeShared ());
  reg.setInputTarget (cloud_target.makeShared ());
  reg.setMaximumIterations (50);
  reg.setTransformationEpsilon (1e-8);
  reg.setMaxCorrespondenceDistance (0.05);

  // Register 
  reg.align (cloud_reg);
  EXPECT_EQ ((int)cloud_reg.points.size (), (int)cloud_source.points.size ());

  Eigen::Matrix4f transformation = reg.getFinalTransformation ();

/*  EXPECT_NEAR (transformation (0, 0), 0.8806,  1e-4);
  EXPECT_NEAR (transformation (0, 1), 0.03648, 1e-4);
  EXPECT_NEAR (transformation (0, 2), -0.4724, 1e-4);
  EXPECT_NEAR (transformation (0, 3), 0.03453, 1e-4);

  EXPECT_NEAR (transformation (1, 0), -0.02354,  1e-4);
  EXPECT_NEAR (transformation (1, 1),  0.9992,   1e-4);
  EXPECT_NEAR (transformation (1, 2),  0.03326,  1e-4);
  EXPECT_NEAR (transformation (1, 3), -0.001519, 1e-4);

  EXPECT_NEAR (transformation (2, 0),  0.4732,  1e-4);
  EXPECT_NEAR (transformation (2, 1), -0.01817, 1e-4);
  EXPECT_NEAR (transformation (2, 2),  0.8808,  1e-4); 
  EXPECT_NEAR (transformation (2, 3),  0.04116, 1e-4);
*/
  EXPECT_EQ (transformation (3, 0), 0);
  EXPECT_EQ (transformation (3, 1), 0);
  EXPECT_EQ (transformation (3, 2), 0);
  EXPECT_EQ (transformation (3, 3), 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, IterativeClosestPointNonLinear)
{
  IterativeClosestPointNonLinear<PointXYZ, PointXYZ> reg;
  reg.setInputCloud (cloud_source.makeShared ());
  reg.setInputTarget (cloud_target.makeShared ());
  reg.setMaximumIterations (50);
  reg.setTransformationEpsilon (1e-8);

  // Register 
  reg.align (cloud_reg);
  EXPECT_EQ ((int)cloud_reg.points.size (), (int)cloud_source.points.size ());

  Eigen::Matrix4f transformation = reg.getFinalTransformation ();

/*  EXPECT_NEAR (transformation (0, 0),  0.951816,  1e-4);
  EXPECT_NEAR (transformation (0, 1),  0.100689,  1e-4);
  EXPECT_NEAR (transformation (0, 2), -0.289668,  1e-4);
  EXPECT_NEAR (transformation (0, 3),  0.0304748, 1e-4);

  EXPECT_NEAR (transformation (1, 0), -0.0741127,  1e-4);
  EXPECT_NEAR (transformation (1, 1),  0.992089,   1e-4);
  EXPECT_NEAR (transformation (1, 2),  0.101327,   1e-4);
  EXPECT_NEAR (transformation (1, 3), -0.00429342, 1e-4);

  EXPECT_NEAR (transformation (2, 0),  0.297579,  1e-4);
  EXPECT_NEAR (transformation (2, 1), -0.0749764, 1e-4);
  EXPECT_NEAR (transformation (2, 2),  0.951748,  1e-4); 
  EXPECT_NEAR (transformation (2, 3),  0.0406639, 1e-4);
*/
  EXPECT_EQ (transformation (3, 0), 0);
  EXPECT_EQ (transformation (3, 1), 0);
  EXPECT_EQ (transformation (3, 2), 0);
  EXPECT_EQ (transformation (3, 3), 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, SampleConsensusInitialAlignment)
{
  // Transform the source cloud by a large amount
  Eigen::Vector3f initial_offset (100, 0, 0);
  float angle = M_PI/2;
  Eigen::Quaternionf initial_rotation (cos (angle / 2), 0, 0, sin (angle / 2));
  PointCloud<PointXYZ> cloud_source_transformed;
  transformPointCloud (cloud_source, cloud_source_transformed, initial_offset, initial_rotation);

  // Create shared pointers
  PointCloud<PointXYZ>::Ptr cloud_source_ptr, cloud_target_ptr;
  cloud_source_ptr = cloud_source_transformed.makeShared ();
  cloud_target_ptr = cloud_target.makeShared ();

  // Initialize estimators for surface normals and FPFH features
  KdTreeFLANN<PointXYZ>::Ptr tree (new KdTreeFLANN<PointXYZ>);

  NormalEstimation<PointXYZ, Normal> norm_est;
  norm_est.setSearchMethod (tree);
  norm_est.setRadiusSearch (0.05);
  PointCloud<Normal> normals;

  FPFHEstimation<PointXYZ, Normal, FPFHSignature33> fpfh_est;
  fpfh_est.setSearchMethod (tree);
  fpfh_est.setRadiusSearch (0.05);
  PointCloud<FPFHSignature33> features_source, features_target;

  // Estimate the FPFH features for the source cloud
  norm_est.setInputCloud (cloud_source_ptr);
  norm_est.compute (normals);
  fpfh_est.setInputCloud (cloud_source_ptr);
  fpfh_est.setInputNormals (normals.makeShared ());
  fpfh_est.compute (features_source);

  // Estimate the FPFH features for the target cloud
  norm_est.setInputCloud (cloud_target_ptr);
  norm_est.compute (normals);
  fpfh_est.setInputCloud (cloud_target_ptr);
  fpfh_est.setInputNormals (normals.makeShared ());
  fpfh_est.compute (features_target);

  // Initialize Sample Consensus Initial Alignment (SAC-IA)
  SampleConsensusInitialAlignment<PointXYZ, PointXYZ, FPFHSignature33> reg;
  reg.setMinSampleDistance (0.05);
  reg.setMaxCorrespondenceDistance (0.2);
  reg.setMaximumIterations (1000);

  reg.setInputCloud (cloud_source_ptr);
  reg.setInputTarget (cloud_target_ptr);
  reg.setSourceFeatures (features_source.makeShared ());
  reg.setTargetFeatures (features_target.makeShared ());

  // Register 
  reg.align (cloud_reg);
  EXPECT_EQ ((int)cloud_reg.points.size (), (int)cloud_source.points.size ());
  EXPECT_EQ (reg.getFitnessScore () < 0.0005, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, PyramidFeatureHistogram)
{
  // Create shared pointers
   PointCloud<PointXYZ>::Ptr cloud_source_ptr = cloud_source.makeShared (),
       cloud_target_ptr = cloud_target.makeShared ();

  PointCloud<Normal>::Ptr cloud_source_normals (new PointCloud<Normal> ()),
      cloud_target_normals (new PointCloud<Normal> ());
  KdTreeFLANN<PointXYZ>::Ptr tree (new KdTreeFLANN<PointXYZ>);
  NormalEstimation<PointXYZ, Normal> normal_estimator;
  normal_estimator.setSearchMethod (tree);
  normal_estimator.setRadiusSearch (0.05);
  normal_estimator.setInputCloud (cloud_source_ptr);
  normal_estimator.compute (*cloud_source_normals);

  normal_estimator.setInputCloud (cloud_target_ptr);
  normal_estimator.compute (*cloud_target_normals);


  PointCloud<PPFSignature>::Ptr ppf_signature_source (new PointCloud<PPFSignature> ()),
      ppf_signature_target (new PointCloud<PPFSignature> ());
  PPFEstimation<PointXYZ, Normal, PPFSignature> ppf_estimator;
  ppf_estimator.setInputCloud (cloud_source_ptr);
  ppf_estimator.setInputNormals (cloud_source_normals);
  ppf_estimator.compute (*ppf_signature_source);

  ppf_estimator.setInputCloud (cloud_target_ptr);
  ppf_estimator.setInputNormals (cloud_target_normals);
  ppf_estimator.compute (*ppf_signature_target);


  vector<pair<float, float> > dim_range_input, dim_range_target;
  for (size_t i = 0; i < 3; ++i) dim_range_input.push_back (pair<float, float> (-M_PI, M_PI));
  dim_range_input.push_back (pair<float, float> (0.0f, 1.0f));
  for (size_t i = 0; i < 3; ++i) dim_range_target.push_back (pair<float, float> (-M_PI * 10.0f, M_PI * 10.0f));
  dim_range_target.push_back (pair<float, float> (0.0f, 50.0f));


  PyramidFeatureHistogram<PPFSignature>::Ptr pyramid_source (new PyramidFeatureHistogram<PPFSignature> ()),
      pyramid_target (new PyramidFeatureHistogram<PPFSignature> ());
  pyramid_source->setInputCloud (ppf_signature_source);
  pyramid_source->setInputDimensionRange (dim_range_input);
  pyramid_source->setTargetDimensionRange (dim_range_target);
  pyramid_source->compute ();

  pyramid_target->setInputCloud (ppf_signature_target);
  pyramid_target->setInputDimensionRange (dim_range_input);
  pyramid_target->setTargetDimensionRange (dim_range_target);
  pyramid_target->compute ();

  float similarity_value = PyramidFeatureHistogram<PPFSignature>::comparePyramidFeatureHistograms (pyramid_source, pyramid_target);
  EXPECT_NEAR (similarity_value, 0.739672, 1e-4);


  vector<pair<float, float> > dim_range_target2;
  for (size_t i = 0; i < 3; ++i) dim_range_target2.push_back (pair<float, float> (-M_PI * 5.0f, M_PI * 5.0f));
    dim_range_target2.push_back (pair<float, float> (0.0f, 20.0f));

  pyramid_source->setTargetDimensionRange (dim_range_target2);
  pyramid_source->compute ();

  pyramid_target->setTargetDimensionRange (dim_range_target2);
  pyramid_target->compute ();

  float similarity_value2 = PyramidFeatureHistogram<PPFSignature>::comparePyramidFeatureHistograms (pyramid_source, pyramid_target);
  EXPECT_NEAR (similarity_value2, 0.801435, 1e-4);


  vector<pair<float, float> > dim_range_target3;
  for (size_t i = 0; i < 3; ++i) dim_range_target3.push_back (pair<float, float> (-M_PI * 2.0f, M_PI * 2.0f));
  dim_range_target3.push_back (pair<float, float> (0.0f, 10.0f));

  pyramid_source->setTargetDimensionRange (dim_range_target3);
  pyramid_source->compute ();

  pyramid_target->setTargetDimensionRange (dim_range_target3);
  pyramid_target->compute ();

  float similarity_value3 = PyramidFeatureHistogram<PPFSignature>::comparePyramidFeatureHistograms (pyramid_source, pyramid_target);
  EXPECT_NEAR (similarity_value3, 0.881507, 1e-4);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TEST (PCL, PPFRegistration)
{
  // Transform the source cloud by a large amount
  Eigen::Vector3f initial_offset (100, 0, 0);
  float angle = M_PI/6;
  Eigen::Quaternionf initial_rotation (cos (angle / 2), 0, 0, sin (angle / 2));
  PointCloud<PointXYZ> cloud_source_transformed;
  transformPointCloud (cloud_source, cloud_source_transformed, initial_offset, initial_rotation);


  // Create shared pointers
  PointCloud<PointXYZ>::Ptr cloud_source_ptr, cloud_target_ptr, cloud_source_transformed_ptr;
  cloud_source_transformed_ptr = cloud_source_transformed.makeShared ();
  cloud_target_ptr = cloud_target.makeShared ();

  // Estimate normals for both clouds
  NormalEstimation<PointXYZ, Normal> normal_estimation;
  KdTreeFLANN<PointXYZ>::Ptr search_tree (new KdTreeFLANN<PointXYZ> ());
  normal_estimation.setSearchMethod (search_tree);
  normal_estimation.setRadiusSearch (0.05);
  PointCloud<Normal>::Ptr normals_target (new PointCloud<Normal> ()),
      normals_source_transformed (new PointCloud<Normal> ());
  normal_estimation.setInputCloud (cloud_target_ptr);
  normal_estimation.compute (*normals_target);
  normal_estimation.setInputCloud (cloud_source_transformed_ptr);
  normal_estimation.compute (*normals_source_transformed);

  PointCloud<PointNormal>::Ptr cloud_target_with_normals (new PointCloud<PointNormal> ()),
      cloud_source_transformed_with_normals (new PointCloud<PointNormal> ());
  concatenateFields (*cloud_target_ptr, *normals_target, *cloud_target_with_normals);
  concatenateFields (*cloud_source_transformed_ptr, *normals_source_transformed, *cloud_source_transformed_with_normals);

  // Compute PPFSignature feature clouds for source cloud
  PPFEstimation<PointXYZ, Normal, PPFSignature> ppf_estimator;
  PointCloud<PPFSignature>::Ptr features_source_transformed (new PointCloud<PPFSignature> ());
  ppf_estimator.setInputCloud (cloud_source_transformed_ptr);
  ppf_estimator.setInputNormals (normals_source_transformed);
  ppf_estimator.compute (*features_source_transformed);


  // Train the source cloud - create the hash map search structure
  PPFHashMapSearch::Ptr hash_map_search (new PPFHashMapSearch (15.0 / 180 * M_PI,
                                                               0.05));
  hash_map_search->setInputFeatureCloud (features_source_transformed);

  // Finally, do the registration
  PPFRegistration<PointNormal, PointNormal> ppf_registration;
  ppf_registration.setSceneReferencePointSamplingRate (20);
  ppf_registration.setPositionClusteringThreshold (0.15);
  ppf_registration.setRotationClusteringThreshold (45.0 / 180 * M_PI);
  ppf_registration.setSearchMethod (hash_map_search);
  ppf_registration.setInputCloud (cloud_source_transformed_with_normals);
  ppf_registration.setInputTarget (cloud_target_with_normals);

  PointCloud<PointNormal> cloud_output;
  ppf_registration.align (cloud_output);
  Eigen::Matrix4f transformation = ppf_registration.getFinalTransformation ();

  EXPECT_NEAR (transformation(0, 0), -0.105976, 1e-4);
  EXPECT_NEAR (transformation(0, 1), -0.987014, 1e-4);
  EXPECT_NEAR (transformation(0, 2), 0.120714, 1e-4);
  EXPECT_NEAR (transformation(0, 3), 10.701012, 1e-4);
  EXPECT_NEAR (transformation(1, 0), 0.914111, 1e-4);
  EXPECT_NEAR (transformation(1, 1), -0.144482, 1e-4);
  EXPECT_NEAR (transformation(1, 2), -0.378848, 1e-4);
  EXPECT_NEAR (transformation(1, 3), -91.315384, 1e-4);
  EXPECT_NEAR (transformation(2, 0), 0.391370, 1e-4);
  EXPECT_NEAR (transformation(2, 1), 0.070197, 1e-4);
  EXPECT_NEAR (transformation(2, 2), 0.917552, 1e-4);
  EXPECT_NEAR (transformation(2, 3), -39.084114, 1e-4);
  EXPECT_NEAR (transformation(3, 0), 0.000000, 1e-4);
  EXPECT_NEAR (transformation(3, 1), 0.000000, 1e-4);
  EXPECT_NEAR (transformation(3, 2), 0.000000, 1e-4);
  EXPECT_NEAR (transformation(3, 3), 1.000000, 1e-4);
}

/* ---[ */
int
  main (int argc, char** argv)
{
  if (argc < 3)
  {
    std::cerr << "No test files given. Please download `bun0.pcd` and `bun4.pcd` and pass their path to the test." << std::endl;
    return (-1);
  }

  // Input
  if (loadPCDFile (argv[1], cloud_source) < 0)
  {
    std::cerr << "Failed to read test file. Please download `bun0.pcd` and pass its path to the test." << std::endl;
    return (-1);
  }
  if (loadPCDFile (argv[2], cloud_target) < 0)
  {
    std::cerr << "Failed to read test file. Please download `bun4.pcd` and pass its path to the test." << std::endl;
    return (-1);
  }

  testing::InitGoogleTest (&argc, argv);
  return (RUN_ALL_TESTS ());

  // Tranpose the cloud_model
  /*for (size_t i = 0; i < cloud_model.points.size (); ++i)
  {
  //  cloud_model.points[i].z += 1;
  }*/
}
/* ]--- */
