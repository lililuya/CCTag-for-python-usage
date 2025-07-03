/*
 * Copyright 2016, Simula Research Laboratory
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _CCTAG_DATASERIALIZATION_HPP
#define	_CCTAG_DATASERIALIZATION_HPP

#include <cctag/CCTagFlowComponent.hpp>
#include <cctag/CCTag.hpp>
#include <cctag/geometry/Ellipse.hpp>

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/archive/text_oarchive.hpp>

namespace cctag {

  void serializeRadiusRatios(boost::archive::text_oarchive & ar, const std::vector<float> & radiusRatios);

  void serializeIdSet(boost::archive::text_oarchive & ar, const IdSet & idSet);

  void serializePoint(boost::archive::text_oarchive & ar, const Point2d<Eigen::Vector3f> & point);

  void serializePoint(boost::archive::text_oarchive & ar, const DirectedPoint2d<Eigen::Vector3f> & point);

  void serializeEdgePoint(boost::archive::text_oarchive & ar, const EdgePoint & e);

  void serializeVecPoint(boost::archive::text_oarchive & ar, const std::vector< DirectedPoint2d<Eigen::Vector3f> > & points);

  void serializePoints(boost::archive::text_oarchive & ar, const std::vector< std::vector< DirectedPoint2d<Eigen::Vector3f> > > & points);

  void serializeEllipse(boost::archive::text_oarchive & ar, const cctag::numerical::geometry::Ellipse & ellipse);

  void serializeEllipses(boost::archive::text_oarchive & ar, const std::vector<cctag::numerical::geometry::Ellipse> & ellipses);

  void serializeMatrix3f(boost::archive::text_oarchive & ar, const Eigen::Matrix3f& matrix);

  void serializeFlowComponent(boost::archive::text_oarchive & ar, const CCTagFlowComponent & flowComponent);

  void serializeFlowComponents(boost::archive::text_oarchive & ar, const std::vector<CCTagFlowComponent> & flowComponents);

} // namespace cctag

#endif	/* SERIALIZATION_HPP */

