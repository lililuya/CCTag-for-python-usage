/*
 * Copyright 2016, Simula Research Laboratory
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <cctag/EllipseGrowing.hpp>
#include <cctag/CCTag.hpp>
#include <cctag/EdgePoint.hpp>
#include <cctag/Fitting.hpp>
#include <cctag/utils/VisualDebug.hpp>
#include <cctag/utils/FileDebug.hpp>
#include <cctag/Fitting.hpp>
#include <cctag/geometry/Circle.hpp>
#include <cctag/geometry/Point.hpp>
#include <cctag/utils/Defines.hpp>
#include <cctag/utils/Talk.hpp> // for DO_TALK macro
// #include <cctag/algebra/Invert.hpp>
#include <cctag/geometry/Ellipse.hpp>

#include <opencv2/core/core_c.h>
#include <opencv2/core/types_c.h>

#include <boost/math/special_functions/pow.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/assert.hpp>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace cctag
{

bool initMarkerCenter(cctag::Point2d<Eigen::Vector3f> & markerCenter,
        const std::vector< std::vector< Point2d<Eigen::Vector3f> > > & markerPoints,
        int realPixelPerimeter)
{
  cctag::numerical::geometry::Ellipse innerEllipse;

  try
  {
    if (realPixelPerimeter > 200)
    {
      if (markerPoints[0].size() > 20)
      {
        numerical::ellipseFitting(innerEllipse, markerPoints[0]);

        for(const Point2d<Eigen::Vector3f>& pt : markerPoints[0])
        {
          CCTagVisualDebug::instance().drawPoint(pt, cctag::color_red);
        }
      }
      else
      {
        return false;
      }
    }
    else
    {
      return false;
    }
  }
  catch (...)
  {
    return false;
  }

  markerCenter = innerEllipse.center();

  return true;
}

bool addCandidateFlowtoCCTag(EdgePointCollection& edgeCollection,
        const std::vector< EdgePoint* > & filteredChildren,
        const std::vector< EdgePoint* > & outerEllipsePoints,
        const cctag::numerical::geometry::Ellipse& outerEllipse,
        std::vector< std::vector< DirectedPoint2d<Eigen::Vector3f> > >& cctagPoints,
        std::size_t numCircles)
{
  //cctag::numerical::geometry::Ellipse innerBoundEllipse(outerEllipse.center(), outerEllipse.a()/8.0, outerEllipse.b()/8.0, outerEllipse.angle());
  cctagPoints.resize(numCircles);

  std::vector< std::vector< DirectedPoint2d<Eigen::Vector3f> > >::reverse_iterator itp = cctagPoints.rbegin();
  itp->reserve(outerEllipsePoints.size());

  for(EdgePoint * e : outerEllipsePoints)
  {
    itp->push_back(DirectedPoint2d<Eigen::Vector3f>(e->x(), e->y(), e->dX(), e->dY()));
  }
  ++itp;
  for (; itp != cctagPoints.rend(); ++itp)
  {
    itp->reserve(filteredChildren.size());
  }

  std::list<EdgePoint*> vProcessedEdgePoint;

  DO_TALK( CCTAG_COUT_VAR_DEBUG(outerEllipse); )

  Eigen::Vector2f gradE(2);
  Eigen::Vector2f toto(2);

  std::size_t nGradientOut = 0;
  std::size_t nAddedPoint = 0;

  for (std::vector<EdgePoint*>::const_iterator it = filteredChildren.begin(); it != filteredChildren.end(); ++it)
  {
    int dir = -1;
    EdgePoint* p = *it;
    const Point2d<Eigen::Vector3f> outerPoint(p->x(), p->y());

    Eigen::Vector3f lineThroughCenter;
    float a = outerPoint.x() - outerEllipse.center().x();
    float b = outerPoint.y() - outerEllipse.center().y();
    lineThroughCenter(0) = a;
    lineThroughCenter(1) = b;
    lineThroughCenter(2) = -a * outerEllipse.center().x() - b * outerEllipse.center().y();

    for (std::size_t j = 1; j < numCircles; ++j)
    {
      if (dir == -1)
      {
        p = edgeCollection.before(p);
      }
      else
      {
        p = edgeCollection.after(p);
      }


      if (!edgeCollection.test_processed_aux(p))
      {
        //CCTAG_COUT(*p);

        edgeCollection.set_processed_aux(p, true);
        vProcessedEdgePoint.push_back(p);

        float normGrad = sqrt(p->dX() * p->dX() + p->dY() * p->dY());

        gradE(0) = p->dX() / normGrad;
        gradE(1) = p->dY() / normGrad;

        toto(0) = outerEllipse.center().x() - p->x();
        toto(1) = outerEllipse.center().y() - p->y();

        float distancePointToCenter = sqrt(toto(0) * toto(0) + toto(1) * toto(1));
        toto(0) /= distancePointToCenter;
        toto(1) /= distancePointToCenter;

        DirectedPoint2d<Eigen::Vector3f> pointToAdd(p->x(), p->y(), p->dX(), p->dY());

        if (isInEllipse(outerEllipse, pointToAdd) && isOnTheSameSide(outerPoint, pointToAdd, lineThroughCenter))
          // isInHull( innerBoundEllipse, outerEllipse, pMid ) && isInHull( innerBoundEllipse, outerEllipse, pointToAdd ) &&
        {
          if ((float(-dir) * gradE.dot(toto) < -0.5f) && (j >= numCircles - 2))
          {
            ++nGradientOut;
          }
          cctagPoints[numCircles - j - 1].push_back(pointToAdd);

          if (j >= numCircles - 2)
          {
            ++nAddedPoint;
          }
        }
        else
        {
          CCTagFileDebug::instance().outputFlowComponentAssemblingInfos(PTS_OUT_WHILE_ASSEMBLING);
          cctagPoints.clear();

          for (EdgePoint* point : vProcessedEdgePoint)
            edgeCollection.set_processed_aux(point, false);

          return false;
        }
      }
      dir = -dir;
    }
  }

  for (EdgePoint* point : vProcessedEdgePoint)
    edgeCollection.set_processed_aux(point, false);

  //std::cin.ignore().get();

  if (float(nGradientOut) / float(nAddedPoint) > 0.5f)
  {
    cctagPoints.clear();
    CCTagFileDebug::instance().outputFlowComponentAssemblingInfos(BAD_GRAD_WHILE_ASSEMBLING);
    return false;
  }
  else
  {
    return true;
  }
}

/**
 * @brief Check if points are good for ellipse growing.
 *
 * The goal is to avoid particular cases, with a bad initialization. The
 * condition is that the ellipse is too flattened.
 *
 * @pre children size >=5
 *
 * @param[in] children
 * @param[out] iMin1
 * @param[out] iMin2
 */
bool isGoodEGPoints(const std::vector<EdgePoint*>& filteredChildren, Point2d<Vector3s> & p1, Point2d<Vector3s> & p2)
{
  BOOST_ASSERT(filteredChildren.size() >= 5);

  static const float thrCosDiffMax = 0.25;

  const float min = numerical::innerProdMin(filteredChildren, thrCosDiffMax, p1, p2);

  return min <= thrCosDiffMax;
}

/**
 * @brief Create a circle that matches outer ellipse points.
 *
 * @param children
 * @param iMin1
 * @param iMin2
 * @return the circle
 */
numerical::geometry::Circle computeCircleFromOuterEllipsePoints(const std::vector<EdgePoint*>& filteredChildren, const Point2d<Eigen::Vector3i> & p1, const Point2d<Eigen::Vector3i> & p2)
{
  // Compute the line passing through filteredChildren[iMin1] and filteredChildren[iMin2] and
  // find i such as d(filteredChildren[i], l) is maximum.
  Eigen::Matrix2f mL;

  mL(0, 0) = static_cast<float>(p1.x());
  mL(0, 1) = static_cast<float>(p1.y());
  mL(1, 0) = static_cast<float>(p2.x());
  mL(1, 1) = static_cast<float>(p2.y());

  Eigen::Vector2f minones(2);
  minones(0) = -1;
  minones(1) = -1;

  Eigen::Matrix2f mLInv = mL.inverse();

  auto aux = mLInv * minones;
  Eigen::Vector3f l;
  l(0) = aux(0);
  l(1) = aux(1);
  l(2) = 1;

  const float normL = std::sqrt(boost::math::pow<2>(l(0)) + boost::math::pow<2>(l(1)));

  const EdgePoint * pMax = filteredChildren.front();
  float distMax = std::min(
                    cctag::numerical::distancePoints2D((Point2d<Vector3s>)(*pMax), p1),
                    cctag::numerical::distancePoints2D((Point2d<Vector3s>)(*pMax), p2));


  for(const EdgePoint * const e : filteredChildren)
  {
    const float dist = std::min(
            cctag::numerical::distancePoints2D((Point2d<Vector3s>)(*e), p1),
            cctag::numerical::distancePoints2D((Point2d<Vector3s>)(*e), p2));

    if (dist > distMax)
    {
      distMax = dist;
      pMax = e;
    }
  }

  Point2d<Eigen::Vector3f> equiPoint;
  float distanceToAdd = cctag::numerical::distancePoints2D(p1, p2) / 50;

  if (std::abs(pMax->cast<float>().dot(l)) / normL < 1e-6)
  {
    float normGrad = std::sqrt(pMax->dX() * pMax->dX() + pMax->dY() * pMax->dY());
    float gx = pMax->dX() / normGrad;
    float gy = pMax->dY() / normGrad;
    equiPoint.x() = (pMax->x() + distanceToAdd * gx);
    equiPoint.y() = (pMax->y() + distanceToAdd * gy);
  }
  else
  {
    equiPoint.x() = (pMax->x());
    equiPoint.y() = (pMax->y());
  }

  numerical::geometry::Circle resCircle(Point2d<Eigen::Vector3f>(p1.x(), p1.y()), Point2d<Eigen::Vector3f>(p2.x(), p2.y()), equiPoint);
  
  return resCircle;
}

bool ellipseGrowingInit(const std::vector<EdgePoint*>& filteredChildren, numerical::geometry::Ellipse& ellipse)
{

  Point2d<Vector3s> p1;
  Point2d<Vector3s> p2;

  bool goodInit = true;

  if (isGoodEGPoints(filteredChildren, p1, p2))
  {
    // Ellipse fitting based on the filtered children
    // todo@Lilian: create the construction Ellipse(pts) which calls the following.
    numerical::ellipseFitting(ellipse, filteredChildren);
  }
  else
  {
    // Initialize ellipse to a circle if the arc coverage is not covered enough. 
    numerical::circleFitting(ellipse, filteredChildren);

    goodInit = false;
  }

  return goodInit;
}

void connectedPoint(std::vector<EdgePoint*>& pts, int runId,
        const EdgePointCollection& img, numerical::geometry::Ellipse& qIn,
        numerical::geometry::Ellipse& qOut, int x, int y)
{
  BOOST_ASSERT(img(x,y));
  const size_t threadMask = (size_t)1 << runId;
  img(x,y)->_processed |= threadMask;  // Set as processed

  static int xoff[] = {1, 1, 0, -1, -1, -1, 0, 1};
  static int yoff[] = {0, -1, -1, -1, 0, 1, 1, 1};

  for (int i = 0; i < 8; ++i)
  {
    int sx = x + xoff[i];
    int sy = y + yoff[i];
    if (sx >= 0 && sx < int( img.shape()[0]) &&
        sy >= 0 && sy < int( img.shape()[1]))
    {
      EdgePoint* e = img(sx,sy);

      if (e && // If unprocessed
          isInHull(qIn, qOut, e) &&
          !(e->_processed & threadMask))
      {
        Eigen::Vector2f gradE;
        gradE(0) = e->dX();
        gradE(1) = e->dY();
        
        Eigen::Vector2f eO;
        eO(0) = qIn.center().x() - e->x();
        eO(1) = qIn.center().y() - e->y();

        if (gradE.dot(eO) < 0)
        {
          pts.push_back(e);
          e->_processed |= threadMask;
          connectedPoint(pts, runId, img, qIn, qOut, sx, sy);
        }
      }
    }
  }
}

void computeHull(const numerical::geometry::Ellipse& ellipse, float delta,
        numerical::geometry::Ellipse& qIn, numerical::geometry::Ellipse& qOut)
{
  qIn = numerical::geometry::Ellipse(cctag::Point2d<Eigen::Vector3f>(
          ellipse.center().x(),
          ellipse.center().y()),
          std::max(ellipse.a() - delta, 0.001f),
          std::max(ellipse.b() - delta, 0.001f),
          ellipse.angle());
  qOut = numerical::geometry::Ellipse(cctag::Point2d<Eigen::Vector3f>(ellipse.center().x(),
          ellipse.center().y()),
          ellipse.a() + delta,
          ellipse.b() + delta,
          ellipse.angle());
}

void ellipseHull(const EdgePointCollection& img,
        std::vector<EdgePoint*>& pts,
        numerical::geometry::Ellipse& ellipse,
        float delta,
        std::size_t runId)
{
  numerical::geometry::Ellipse qIn, qOut;
  computeHull(ellipse, delta, qIn, qOut);

  std::size_t initSize = pts.size();

  for (std::size_t i = 0; i < initSize; ++i)
  {
    EdgePoint *e = pts[i];
    connectedPoint(pts, runId, img, qIn, qOut, e->x(), e->y());
  }
}

void ellipseGrowing2(
        const EdgePointCollection& img,
        const std::vector<EdgePoint*>& filteredChildren,
        std::vector<EdgePoint*>& outerEllipsePoints,
        numerical::geometry::Ellipse& ellipse,
        float ellipseGrowingEllipticHullWidth,
        std::size_t runId,
        bool goodInit)
{
  const size_t threadMask = (size_t)1 << runId;
  outerEllipsePoints.reserve(filteredChildren.size()*3);

  for(EdgePoint * children : filteredChildren)
  {
    outerEllipsePoints.push_back(children);
    children->_processed |= threadMask;
  }

  int lastSizePoints = 0;
  int nIter = 0;

  if (!goodInit)
  {
    int newSizePoints = outerEllipsePoints.size();
    int maxNbPoints = newSizePoints;
    int nIterMax = 0;
    std::vector<std::vector<EdgePoint*> > edgePointsSets;
    edgePointsSets.reserve(6); // maximum of expected iterations
    edgePointsSets.push_back(outerEllipsePoints);
    std::vector<numerical::geometry::Ellipse> ellipsesSets;
    ellipsesSets.reserve(6); // maximum of expected iterations
    ellipsesSets.push_back(ellipse);
    ++nIter;
    while (newSizePoints - lastSizePoints > 0)
    {
      numerical::geometry::Ellipse qIn, qOut;
      computeHull(ellipse, ellipseGrowingEllipticHullWidth, qIn, qOut);
      lastSizePoints = 0;
      for(const EdgePoint * point : outerEllipsePoints)
      {
        if (isInHull(qIn, qOut, point))
        {
          ++lastSizePoints;
        }
      }

      ellipseHull(img, outerEllipsePoints, ellipse, ellipseGrowingEllipticHullWidth, runId);
      edgePointsSets.push_back(outerEllipsePoints);
      ellipsesSets.push_back(ellipse);


      // Compute the new circle which fits oulierEllipsePoints
      numerical::circleFitting(ellipse, outerEllipsePoints);

      computeHull(ellipse, ellipseGrowingEllipticHullWidth, qIn, qOut);
      newSizePoints = 0;
      for(const EdgePoint * point : outerEllipsePoints)
      {
        if (isInHull(qIn, qOut, point))
        {
          ++newSizePoints;
        }
      }
      
      if (newSizePoints > maxNbPoints)
      {
        maxNbPoints = newSizePoints;
        nIterMax = nIter;
      }

      ++nIter;
    }
    outerEllipsePoints = edgePointsSets[nIterMax];
    ellipse = ellipsesSets[nIterMax];
    
    // Set all the processed edge points as not processed as only a subset of them
    // correspond to outerEllipsePoints which must be finally set to runId.
    for(auto & vedgePoint: edgePointsSets)
    {
      for(auto & point: vedgePoint)
        point->_processed &= ~threadMask; // Could be any value different of runId
    }
    // Set as processed all the outerEllipsePoints
    for(auto & point: outerEllipsePoints)
    {
      point->_processed |= threadMask;
    }
    
  }
  lastSizePoints = 0;
  nIter = 0;

  // Once the circle is computed, compute the ellipse that fits the same set of points
  numerical::ellipseFitting(ellipse, outerEllipsePoints);

  while (outerEllipsePoints.size() - lastSizePoints > 0)
  {
    lastSizePoints = outerEllipsePoints.size();

    ellipseHull(img, outerEllipsePoints, ellipse, ellipseGrowingEllipticHullWidth, runId);
    // Compute the new ellipse which fits oulierEllipsePoints
    numerical::ellipseFitting(ellipse, outerEllipsePoints);

    ++nIter;
  }

}

} // namespace cctag


